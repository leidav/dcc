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

static struct ParamContext* createParamContext(
    struct LexerState* state, struct ParamContext* prev,
    const struct TokenIterator* param_iterators, int num_params)

{
	struct ParamContext* context =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1, typeof(*context));
	if (context == NULL) {
		return NULL;
	}
	context->prev = prev;
	context->iterators = param_iterators;
	context->num_params = num_params;
	return context;
}

static int pushParamContext(struct LexerState* state,
                            const struct TokenIterator* iterators,
                            int num_params)

{
	struct ParamContext* context = createParamContext(
	    state, state->pp_expansion_state.current_context->param, iterators,
	    num_params);
	if (context == NULL) {
		return -1;
	}
	state->pp_expansion_state.current_context->param = context;
	return 0;
}

/*static void popParamContext(struct LexerState* state)
{
    struct ParamContext* context = state->pp_expansion_state.param_context;
    if (context->prev != NULL) {
        state->pp_expansion_state.param_context = context->prev;
    }
}*/

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
	state->pp_expansion_state.current_context->param =
	    createParamContext(state, NULL, NULL, definition->num_params);

	state->pp_expansion_state.current_context->depth = 0;
	initTokenIterator(&state->pp_expansion_state.current_context->iterator,
	                  definition);
}

static int pushContext(struct LexerState* state, const struct TokenIterator* it)
{
	struct ExpansionContext* context =
	    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad), 1,
	                  typeof(*state->pp_expansion_state.current_context));

	if (context == NULL) {
		generalError("expansion stack full");
		return -1;
	}
	printf("push %p -> %p\n", state->pp_expansion_state.current_context,
	       context);

	context->prev = state->pp_expansion_state.current_context;
	context->iterator.start = it->start;
	context->iterator.cur = it->start;
	context->iterator.end = it->end;
	context->param = state->pp_expansion_state.current_context->param;
	context->depth = context->prev->depth + 1;
	state->pp_expansion_state.current_context = context;
	return 0;
}
static void popContext(struct LexerState* state)
{
	struct ExpansionContext* context =
	    state->pp_expansion_state.current_context;

	printf("pop %p -> %p\n", context, context->prev);
	if (context->prev != NULL) {
		state->pp_expansion_state.current_context = context->prev;
	}
}

static bool prepareMacroParamTokens(struct LexerState* state,
                                    struct TokenIterator* params,
                                    int expected_param_count)
{
	struct ExpansionContext* current_context =
	    state->pp_expansion_state.current_context;
	struct TokenIterator* it = &current_context->iterator;

	bool status = false;

	int token_offset = it->cur;
	int param_index = 0;
	int counter = 1;
	int16_t token_count = 0;
	int16_t token_start = 0;
	params[0].start = token_offset;
	params[0].cur = token_offset;
	while (true) {
		if (it->cur > it->end) {
			lexerError(state, "macro parantheses not closed");
			goto out;
		}
		struct PreprocessorToken* token = getTokenAt(state, it->cur);

		switch (token->type) {
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
		it->cur++;
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

bool expand(struct LexerState* state, struct PreprocessorToken* token)
{
	struct ExpansionContext* current_context =
	    state->pp_expansion_state.current_context;
	struct TokenIterator* it = &current_context->iterator;
	bool status = true;

	printf("expand %d\n", current_context->depth);
	if (current_context->prev == NULL && it->cur > it->end) {
		token->type = TOKEN_EOF;
	} else if (it->cur <= it->end) {
		struct PreprocessorToken* tok = getTokenAt(state, it->cur);
		it->cur++;
		if (tok->type == PP_PARAM) {
			const struct ParamContext* param_context = current_context->param;
			if (param_context == NULL) {
				generalError("param context is NULL");
				return false;
			}
			struct ParamContext* param_context_prev = param_context->prev;
			/*if (param_context_prev == NULL) {
			    generalError("previous param context is NULL");
			    return false;
			}*/
			/*printf("pos:%d,value:%d, num:%d\n", it->cur - 1,
			   tok->value_handle, current_context->num_params);
			       */
			const struct TokenIterator* param_iterators =
			    param_context->iterators;
			if (param_iterators == NULL) {
				generalError("Invalid param iterator");
				return false;
			}
			printf("expand param %d inside level %d at pos %d\n",
			       tok->value_handle, current_context->depth, it->cur - 1);

			if (param_context->num_params > 2) {
				generalError("param context corrupt");
				return false;
			}
			const struct TokenIterator* p = &param_iterators[tok->value_handle];

			printf("\tnum params: %d, %p\n", param_context->num_params,
			       param_context);
			printf("\t prev %p\n", param_context_prev);
			for (int i = 0; i < param_context->num_params; i++) {
				printf("\t\tparam %d: %d - %d\n", i, param_iterators[i].start,
				       param_iterators[i].end);
			}

			if (pushContext(state, p) != 0) {
				return false;
			}
			state->pp_expansion_state.current_context->param =
			    param_context_prev;

			status = expand(state, token);

			//*token = *tok;
		} else if (tok->type == IDENTIFIER) {
			/*printf("pos: %d,%d\n", it->cur - it->start - 1,
			       it->end - it->start);
			       */
			int index = tok->value_handle;
			const char* identifier = getStringAt(&state->identifiers, index);
			printf("identifier: %s\n", identifier);
			uint32_t hash = getHashAt(&state->identifiers, index);
			int length = getLengthAt(&state->identifiers, index);
			struct PreprocessorDefinition* def =
			    findDefinition(state, identifier, length, hash);
			if (def != NULL) {
				struct TokenIterator* param_iterators = NULL;
				int num_params = 0;
				if (isFunctionLike(def)) {
					if ((it->cur > it->end) ||
					    (getTokenAt(state, it->cur)->type != PARENTHESE_LEFT)) {
						lexerError(
						    state,
						    "asdf function like macro must be called like a "
						    "function");
						return false;
					}
					it->cur++;
					num_params = def->num_params;
					// struct ParamContext* parent_params =
					// current_context->param;
					param_iterators =
					    ALLOCATE_TYPE(ALLOCATOR_CAST(state->scratchpad),
					                  num_params, typeof(*param_iterators));
					if (param_iterators == NULL) {
						return false;
					}

					printf("prepare params: %d\n", num_params);
					prepareMacroParamTokens(state, param_iterators, num_params);
					/*for (int i = 0; i < num_params; i++) {
					    printf("%d, pos:%d,type:%d\n", i,
					           param_iterators[i].cur,
					           getTokenAt(state, param_iterators[i].cur)->type);
					}*/
				}
				struct TokenIterator iter;
				initTokenIterator(&iter, def);
				if (pushContext(state, &iter) != 0) {
					return false;
				}
				if (isFunctionLike(def)) {
					if (pushParamContext(state, param_iterators, num_params) !=
					    0) {
						return false;
					}
				}
				status = expand(state, token);

			} else {
				*token = *tok;
				printf("return token\n");
			}
		} else {
			*token = *tok;
			printf("return token\n");
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

void printPPToken(struct LexerState* state,
                  const struct PreprocessorToken* pp_token)
{
	struct LexerToken token;
	createLexerTokenFromPPToken(state, pp_token, &token);
	printToken(state, &token);
}
