#include "cpp.h"

#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "error.h"
#include "lexer.h"
#include "string_set.h"

enum ExpansionResult {
	EXPANSION_RESULT_TOKEN = 0,
	EXPANSION_RESULT_ERROR = 1,
	EXPANSION_RESULT_CONTINUE = 2
};

static struct TokenIterator* allocateIterators(struct LexerState* state,
                                               int num_iterators)
{
	return ALLOCATE_TYPE(&state->expansion_allocator, num_iterators,
	                     struct TokenIterator);
}

struct ExpansionContext* allocateExpansionContext(struct LexerState* state)
{
	return ALLOCATE_TYPE(&state->expansion_allocator, 1,
	                     struct ExpansionContext);
}

static int expand(struct LexerState* state, struct PreprocessorToken* token);

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

int addPreprocessorPPToken(struct LexerState* state,
                           struct PreprocessorTokenSet* tokens,
                           const struct PreprocessorToken* token)
{
	if (tokens->num == tokens->max_tokens) {
		generalError("not enough memory to store token");
		return -1;
	}
	tokens->tokens[tokens->num] = *token;
	return ++tokens->num;
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
	it->start = definition->token_start;
	it->cur = definition->token_start;
	it->end = definition->token_start + definition->num_tokens - 1;
}

static void resetTokenIterator(struct TokenIterator* it)
{
	it->cur = it->start;
}

static struct PreprocessorToken* getTokenAt(struct LexerState* state, int index)
{
	return &state->pp_tokens.tokens[index];
}

static int pushContext(struct LexerState* state, const struct TokenIterator* it,
                       const struct ParamContext* params)
{
	struct ExpansionContext* context = allocateExpansionContext(state);

	if (context == NULL) {
		generalError("expansion stack full");
		return -1;
	}
	context->prev = state->pp_expansion_state.current_context;
	context->iterator.start = it->start;
	context->iterator.cur = it->start;
	context->iterator.end = it->end;
	if (params != NULL) {
		int num = params->num_params;
		context->param.num_params = num;
		context->param.iterators = params->iterators;
		context->param.parent = params->parent;
	} else {
		context->param.iterators = NULL;
		context->param.num_params = 0;
		context->param.parent = NULL;
	}
	state->pp_expansion_state.current_context = context;
	return 0;
}

static void popContext(struct LexerState* state)
{
	struct ExpansionContext* context =
	    state->pp_expansion_state.current_context;

	if (context->prev == NULL) {
		generalError("invalid context pop");
		exit(1);
	}
	state->pp_expansion_state.current_context = context->prev;
}

void beginExpansion(struct LexerState* state,
                    struct PreprocessorDefinition* definition)
{
	struct PreprocessorExpansionState* pp_state = &state->pp_expansion_state;

	state->expand_macro = true;
	pp_state->token_marker = state->pp_tokens.num;

	pp_state->function_like = isFunctionLike(definition);
	pp_state->begin_expansion = true;

	pp_state->current_context = allocateExpansionContext(state);
	pp_state->current_context->prev = NULL;

	pp_state->current_context->param.iterators = NULL;
	pp_state->current_context->param.parent = NULL;
	pp_state->current_context->param.num_params = definition->num_params;

	initTokenIterator(&pp_state->current_context->iterator, definition);
}

bool prepareMacroParamTokens(struct LexerState* state,
                             struct TokenIterator* params,
                             struct TokenIterator* iterator,
                             int expected_param_count)
{
	bool status = false;

	int token_offset = iterator->cur;
	int param_index = 0;
	int counter = 1;
	int16_t token_count = 0;
	int16_t token_start = 0;
	params[0].start = token_offset;
	params[0].cur = token_offset;
	while (true) {
		if (iterator->cur > iterator->end) {
			lexerError(state, "macro parantheses not closed");
			goto out;
		}
		struct PreprocessorToken* token = getTokenAt(state, iterator->cur);
		int type = token->type;

		switch (type) {
			case PARENTHESE_LEFT:
				counter++;
				break;
			case PARENTHESE_RIGHT:
				counter--;
				break;
			case COMMA:
				if (counter == 1) {
					if (param_index == expected_param_count - 1) {
						lexerError(state, "to many macro parameters");
						goto out;
					}
					params[param_index].cur = token_start + token_offset;
					params[param_index].start = params[param_index].cur;
					params[param_index].end = token_count - 1 + token_offset;
					token_start = token_count + 1;
					params[param_index + 1].cur = token_start + token_offset;
					params[param_index + 1].start = params[param_index + 1].cur;
					param_index++;
				}
				break;
		}
		iterator->cur++;
		if (counter == 0) {
			params[param_index].end = token_count - 1 + token_offset;
			break;
		}

		token_count++;
	}
	if (param_index < expected_param_count - 1) {
		lexerError(state, "more macro parameters expected");
		goto out;
	}
	status = true;
out:
	return status;
}

int expand(struct LexerState* state, struct PreprocessorToken* token)
{
	struct ExpansionContext* current_context =
	    state->pp_expansion_state.current_context;
	struct TokenIterator* it = &current_context->iterator;

	if (it->cur > it->end) {
		if (current_context->prev == NULL) {
			token->type = TOKEN_EOF;
			return EXPANSION_RESULT_TOKEN;
		}
		popContext(state);
		return EXPANSION_RESULT_CONTINUE;
	}

	struct PreprocessorToken* tok = getTokenAt(state, it->cur);
	it->cur++;

	if (tok->type == PP_PARAM) {
		const struct ParamContext* param_context = &current_context->param;
		if (param_context == NULL) {
			generalError("param context is NULL");
			return EXPANSION_RESULT_ERROR;
		}

		const struct TokenIterator* param_iterators = param_context->iterators;
		if (param_iterators == NULL) {
			generalError("Invalid param iterator");
			return EXPANSION_RESULT_ERROR;
		}

		const struct TokenIterator* p = &param_iterators[tok->value_handle];

		const struct ParamContext* param_context_parent = param_context->parent;
		const struct ParamContext* parent_parent = NULL;
		if (param_context_parent != NULL) {
			parent_parent = param_context_parent->parent;
		}

		if (pushContext(state, p, param_context_parent) != 0) {
			return EXPANSION_RESULT_ERROR;
		}
		return EXPANSION_RESULT_CONTINUE;
	}

	if (tok->type == IDENTIFIER) {
		int index = tok->value_handle;
		const char* identifier = getStringAt(&state->identifiers, index);
		uint32_t hash = getHashAt(&state->identifiers, index);
		int length = getLengthAt(&state->identifiers, index);
		struct PreprocessorDefinition* def =
		    findDefinition(state, identifier, length, hash);

		if (def != NULL) {
			struct TokenIterator iter;
			initTokenIterator(&iter, def);

			struct TokenIterator* param_iterators = NULL;
			int num_params = 0;
			struct ParamContext param_context = {NULL, NULL, 0};
			if (isFunctionLike(def)) {
				if ((it->cur > it->end) ||
				    (getTokenAt(state, it->cur)->type != PARENTHESE_LEFT)) {
					lexerError(state,
					           "function like macro must be called like a "
					           "function");
					return EXPANSION_RESULT_ERROR;
				}
				it->cur++;
				num_params = def->num_params;

				if (num_params > 0) {
					param_iterators = allocateIterators(state, num_params);

					if (param_iterators == NULL) {
						return EXPANSION_RESULT_ERROR;
					}

					prepareMacroParamTokens(state, param_iterators, it,
					                        num_params);

					param_context.parent = &current_context->param;
					param_context.iterators = param_iterators;
					param_context.num_params = num_params;

				} else {
					if (getTokenAt(state, it->cur)->type != PARENTHESE_RIGHT) {
						lexerError(state, "macro parantheses not closed");
						return EXPANSION_RESULT_ERROR;
					}
				}
			}
			if (pushContext(state, &iter, &param_context) != 0) {
				return EXPANSION_RESULT_ERROR;
			}

			return EXPANSION_RESULT_CONTINUE;
		}
	}
	*token = *tok;
	return EXPANSION_RESULT_TOKEN;
}

bool getExpandedToken(struct LexerState* state, struct PreprocessorToken* token)
{
	int result;
	int count = 0;
	do {
		result = expand(state, token);
		count++;
	} while (result == EXPANSION_RESULT_CONTINUE);

	return result == EXPANSION_RESULT_ERROR ? false : true;
}

void stopExpansion(struct LexerState* state)
{
	state->pp_expansion_state.begin_expansion = false;
	state->pp_expansion_state.function_like = false;
	state->expand_macro = false;
	resetLinearAllocatorState(&state->expansion_allocator, 0);
	state->pp_tokens.num = state->pp_expansion_state.token_marker;
}

void printPPToken(struct LexerState* state,
                  const struct PreprocessorToken* pp_token)
{
	struct LexerToken token;
	createLexerTokenFromPPToken(state, pp_token, &token);
	printToken(state, &token);
}
