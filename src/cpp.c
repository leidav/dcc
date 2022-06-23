#include "cpp.h"

#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "error.h"
#include "lexer.h"
#include "string_set.h"

static bool expand(struct LexerState* state, struct PreprocessorToken* token);

int createPreprocessorTokenSet(struct PreprocessorTokenSet* set,
                               size_t max_tokens, struct Allocator* allocator)
{
	struct PreprocessorToken* tokens =
	    ALLOCATE_TYPE(allocator, max_tokens, typeof(*set->tokens));
	if (tokens == NULL) {
		return -1;
	}
	createPreprocessorTokenSetFromBuffer(set, max_tokens, tokens);
	return 0;
}

void createPreprocessorTokenSetFromBuffer(struct PreprocessorTokenSet* set,
                                          size_t max_tokens, void* buffer)
{
	set->max_tokens = max_tokens;
	set->num = 0;
	set->tokens = (struct PreprocessorToken*)buffer;
}
int createPreprocessorDefinitionSet(struct PreprocessorDefinitionSet* set,
                                    size_t max_definitions,
                                    int pp_definition_stringset_size,
                                    struct Allocator* allocator)
{
	set->definitions = malloc(sizeof(*set->definitions) * max_definitions);
	if (set->definitions == NULL) {
		return -1;
	}
	if (createStringSet(&set->pp_definition_names, pp_definition_stringset_size,
	                    max_definitions, allocator) != 0) {
		free(set->definitions);
		return -1;
	}
	return 0;
}
int addPreprocessorToken(struct LexerState* state,
                         struct PreprocessorTokenSet* tokens,
                         const struct LexerToken* token)
{
	if (tokens->num == tokens->max_tokens) {
		generalError("not enough memory to store token");
		return -1;
	}
	struct PreprocessorToken* pp_token = &tokens->tokens[tokens->num];
	pp_token->line = token->line;
	pp_token->column = token->column;
	pp_token->line_pos = token->line_pos;
	pp_token->type = token->type;
	if (token->literal) {
		if (token->type == LITERAL_STRING || token->type == PP_NUMBER) {
			pp_token->value_handle = token->value.string_index;
		} else {
			int index = state->constants.num;
			if (index == state->constants.max_count) {
				generalError("not enough memory to store numeric constant");
				return -1;
			}
			state->constants.constants[index] = token->value;
			pp_token->value_handle = index;
			state->constants.num++;
		}
	}
	if (token->type == IDENTIFIER) {
		pp_token->value_handle = token->value.string_index;
	} else if (token->type == PP_PARAM) {
		pp_token->value_handle = token->value.param_index;
	}

	return ++tokens->num;
}

int createPreprocessorDefinition(struct LexerState* state, int token_start,
                                 int num_tokens, int num_params,
                                 const char* name, int name_length,
                                 bool function_like)
{
	struct PreprocessorDefinitionSet* definitions = &state->pp_definitions;

	int hash = hashSubstring(name, name_length);

	bool exists;
	int index = addStringAndHash(&definitions->pp_definition_names, name,
	                             name_length, hash, &exists);
	if (index < 0) {
		return -1;
	}
	if (exists) {
		generalWarning("Macro redefined!");
	}

	struct PreprocessorDefinition* def = &definitions->definitions[index];
	def->token_start = token_start;
	def->num_tokens = num_tokens;
	def->num_params = num_params;
	def->flags = 0;
	if (function_like) {
		def->flags |= FUNCTION_LIKE;
	}
	return index;
}

struct PreprocessorDefinition* findDefinition(struct LexerState* state,
                                              const char* name, int length,
                                              uint32_t hash)
{
	int index = findIndex(&state->pp_definitions.pp_definition_names, name,
	                      length, hash);
	if (index < 0) {
		return NULL;
	}
	return &state->pp_definitions.definitions[index];
}

static void initTokenIterator(struct TokenIterator* it,
                              const struct PreprocessorDefinition* definition)
{
	it->cur = definition->token_start;
	it->end = definition->token_start + definition->num_tokens - 1;
}

static struct PreprocessorToken* getTokenAt(struct LexerState* state, int index)
{
	return &state->pp_tokens.tokens[index];
}

void beginExpansion(struct LexerState* state,
                    struct PreprocessorDefinition* definition)
{
	state->expand_macro = true;
	state->pp_expansion_state.pos = 0;
	state->pp_expansion_state.function_like = isFunctionLike(definition);
	state->pp_expansion_state.begin_expansion = true;
	state->pp_expansion_state.memory_marker =
	    markAllocatorState(state->scratchpad);
	state->pp_expansion_state.token_marker = state->pp_tokens.num;
	state->pp_expansion_state.current_context =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1,
	                  typeof(*state->pp_expansion_state.current_context));
	state->pp_expansion_state.current_context->prev = NULL;
	state->pp_expansion_state.current_context->num_params =
	    definition->num_params;
	state->pp_expansion_state.current_context->param_iterators = NULL;
	initTokenIterator(&state->pp_expansion_state.current_context->iterator,
	                  definition);
}

static int pushContext(struct LexerState* state, struct TokenIterator* it,
                       struct TokenIterator* param_iterators, int num_params)
{
	struct ExpansionContext* context =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1,
	                  typeof(*state->pp_expansion_state.current_context));
	if (context == NULL) {
		generalError("expansion stack full");
		return -1;
	}
	context->prev = state->pp_expansion_state.current_context;
	context->iterator = *it;
	context->param_iterators = param_iterators;
	context->num_params = num_params;
	state->pp_expansion_state.current_context = context;
	return 0;
}
static void popContext(struct LexerState* state)
{
	struct ExpansionContext* context =
	    state->pp_expansion_state.current_context;
	if (context->prev != NULL) {
		state->pp_expansion_state.current_context = context->prev;
	}
}

int preExpandParams(struct LexerState* state,
                    struct TokenIterator* param_tokens,
                    struct PreprocessorToken* output_buffer, int max_len)
{
	if (pushContext(state, param_tokens, NULL, 0) != 0) {
		return -1;
	}
	struct ExpansionContext* current_context =
	    state->pp_expansion_state.current_context;
	struct TokenIterator* it = &current_context->iterator;
	int i = 0;
	while (it->cur <= it->end) {
		struct PreprocessorToken* token = getTokenAt(state, it->cur);
		struct PreprocessorToken expanded_token;
		do {
			if (i == max_len) {
				generalError("macro parameter buffer full");
				return -1;
			}
			if (!expand(state, &expanded_token)) {
				return -1;
			}
			output_buffer[i] = expanded_token;
			i++;
		} while (expanded_token.type != TOKEN_EOF);
		it->cur++;
	}
	popContext(state);
	return i;
}

bool expand(struct LexerState* state, struct PreprocessorToken* token)
{
	struct ExpansionContext* current_context =
	    state->pp_expansion_state.current_context;
	struct TokenIterator* it = &current_context->iterator;

	bool status = true;
	if (current_context->prev == NULL && it->cur > it->end) {
		token->type = TOKEN_EOF;
	} else if (it->cur <= it->end) {
		struct PreprocessorToken* tok = getTokenAt(state, it->cur);
		it->cur++;
		if (tok->type == PP_PARAM) {
			struct TokenIterator* param_iterators =
			    current_context->param_iterators;
			int num_params = current_context->num_params;
			struct TokenIterator* p = &param_iterators[tok->value_handle];
			if (pushContext(state, p, NULL, 0) != 0) {
				return false;
			}
			status = expand(state, token);
		} else if (tok->type == IDENTIFIER) {
			int index = tok->value_handle;
			const char* identifier = getStringAt(&state->identifiers, index);
			uint32_t hash = getHashAt(&state->identifiers, index);
			int length = getLengthAt(&state->identifiers, index);
			struct PreprocessorDefinition* def =
			    findDefinition(state, identifier, length, hash);
			if (def != NULL) {
				struct TokenIterator iter;
				initTokenIterator(&iter, def);
				if (pushContext(state, &iter, NULL, 0) != 0) {
					return false;
				}
				status = expand(state, token);

			} else {
				*token = *tok;
			}
		} else {
			*token = *tok;
		}
	} else {
		popContext(state);
		status = expand(state, token);
	}
	return status;
}

bool getExpandedToken(struct LexerState* state, struct PreprocessorToken* token)
{
	return expand(state, token);
}

void stopExpansion(struct LexerState* state)
{
	state->pp_expansion_state.pos = -1;
	state->pp_expansion_state.begin_expansion = false;
	state->pp_expansion_state.function_like = false;
	state->expand_macro = false;
	resetAllocatorState(state->scratchpad,
	                    state->pp_expansion_state.memory_marker);
	state->pp_tokens.num = state->pp_expansion_state.token_marker;
}
