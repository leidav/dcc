#include "cpp.h"

#include <stdlib.h>

#include "allocator.h"
#include "error.h"
#include "lexer.h"
#include "string_set.h"

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
				return -1;
			}
			state->constants.constants[index] = token->value;
			pp_token->value_handle = index;
			state->constants.num++;
		}
	}
	if (token->type == PP_PARAM) {
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
	state->pp_expansion_state.current_state =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1,
	                  typeof(*state->pp_expansion_state.current_state));
	state->pp_expansion_state.current_state->prev = NULL;
	state->pp_expansion_state.current_state->num_params =
	    definition->num_params;
	state->pp_expansion_state.current_state->param_iterators = NULL;
	initTokenIterator(&state->pp_expansion_state.current_state->iterator,
	                  definition);
}

static int pushStack(struct LexerState* state, struct TokenIterator* it,
                     struct TokenIterator* param_iterators, int num_params)
{
	struct StackEntry* entry =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1,
	                  typeof(*state->pp_expansion_state.current_state));
	if (entry == NULL) {
		return -1;
	}
	entry->prev = state->pp_expansion_state.current_state;
	entry->iterator = *it;
	entry->param_iterators = param_iterators;
	entry->num_params = num_params;
	state->pp_expansion_state.current_state = entry;
	return 0;
}
static void popStack(struct LexerState* state)
{
	struct StackEntry* entry = state->pp_expansion_state.current_state;
	if (entry->prev != NULL) {
		state->pp_expansion_state.current_state = entry->prev;
	}
}

bool expand(struct LexerState* state, struct PreprocessorToken* token)
{
	struct StackEntry* current_state = state->pp_expansion_state.current_state;
	struct TokenIterator* it = &current_state->iterator;

	bool status = true;
	if (current_state->prev == NULL && it->cur > it->end) {
		token->type = TOKEN_EOF;
	} else if (it->cur <= it->end) {
		*token = *getTokenAt(state, it->cur);
		it->cur++;
		if (token->type == PP_PARAM) {
			struct TokenIterator* param_iterators =
			    current_state->param_iterators;
			int num_params = current_state->num_params;
			struct TokenIterator* p = &param_iterators[token->value_handle];
			if (pushStack(state, p, NULL, 0) != 0) {
				generalError("expansion stack full");
				return false;
			}
			status = expand(state, token);
		}
	} else {
		popStack(state);
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
