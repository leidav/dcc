/*	Copyright (C) 2021 David Leiter
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpp.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "lexer.h"
#include "memory/linear_allocator.h"
#include "string_set.h"

#define SCRATCHPAD_SIZE (4096 << 0)

enum ExpansionResult {
	EXPANSION_RESULT_TOKEN = 0,
	EXPANSION_RESULT_ERROR = 1,
	EXPANSION_RESULT_CONTINUE = 2
};

static struct TokenIterator* allocateIterators(struct PreprocessorState* state,
                                               int num_iterators)
{
	return ALLOCATE_TYPE(&state->allocator, num_iterators,
	                     struct TokenIterator);
}

static int expand(struct PreprocessorState* state,
                  struct StringSet* identifiers,
                  struct PreprocessorToken* token);

int initPreprocessorState(struct PreprocessorState* state)
{
	struct Allocator* global_allocator = getGlobalAllocator();
	struct MemoryArena* arena =
	    allocateArena(global_allocator, SCRATCHPAD_SIZE);
	if (arena == NULL) {
		return -1;
	}
	initLinearAllocator(&state->allocator, arena);

	if (createPreprocessorTokenSet(&state->tokens,
	                               PREPROCESSOR_MAX_DEFINITION_TOKEN_COUNT,
	                               global_allocator) != 0) {
		return -1;
	}
	if (createPreprocessorDefinitionSet(
	        &state->definitions, PREPROCESSOR_MAX_DEFINITION_COUNT,
	        LEXER_PP_NUMBER_STRINGSET_SIZE, global_allocator) != 0) {
		return -1;
	}

	state->expansion_state.expansion_stack = allocate(
	    global_allocator, sizeof(*state->expansion_state.expansion_stack) *
	                          PREPROCESSOR_MAX_EXPANSION_DEPTH);
	state->expansion_state.expansion_depth = 0;
	return 0;
}

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
	set->definitions =
	    allocate(allocator, sizeof(*set->definitions) * max_definitions);
	if (set->definitions == NULL) {
		return -1;
	}
	if (initStringSet(&set->pp_definition_names, pp_definition_stringset_size,
	                  max_definitions, allocator) != 0) {
		deallocate(allocator, set->definitions);
		return -1;
	}
	return 0;
}

int addPreprocessorToken(struct PreprocessorState* state,
                         struct LexerConstantSet* constants,
                         const struct LexerToken* token)
{
	struct PreprocessorTokenSet* tokens = &state->tokens;
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
			int index = constants->num;
			if (index == constants->max_count) {
				generalError("not enough memory to store numeric constant");
				return -1;
			}
			constants->constants[index] = token->value;
			pp_token->value_handle = index;
			constants->num++;
		}
	}
	if (token->type == IDENTIFIER) {
		pp_token->value_handle = token->value.string_index;
	} else if (token->type == PP_PARAM) {
		pp_token->value_handle = token->value.param_index;
	}

	return ++tokens->num;
}

struct PreprocessorTokenSet* getPreprocessorTokenSet(
    struct PreprocessorState* state)
{
	return &state->tokens;
}

int getPreprocessorTokenPos(struct PreprocessorState* state)
{
	return state->tokens.num;
}

int createPreprocessorDefinition(struct PreprocessorState* state,
                                 int start_index, int num_tokens,
                                 int num_params, const char* name,
                                 int name_length, bool function_like)
{
	struct PreprocessorDefinitionSet* definitions = &state->definitions;

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
	def->token_start = start_index;
	def->num_tokens = num_tokens;
	def->num_params = num_params;
	def->flags = 0;
	if (function_like) {
		def->flags |= FUNCTION_LIKE;
	}
	return index;
}

struct PreprocessorDefinition* findDefinition(struct PreprocessorState* state,
                                              const char* name, int length,
                                              uint32_t hash)
{
	int index =
	    findIndex(&state->definitions.pp_definition_names, name, length, hash);
	if (index < 0) {
		return NULL;
	}
	return &state->definitions.definitions[index];
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

static struct PreprocessorToken* getTokenAt(struct PreprocessorState* state,
                                            int index)
{
	return &state->tokens.tokens[index];
}

static int pushContext(struct PreprocessorState* state,
                       const struct TokenIterator* it,
                       const struct ParamContext* params)
{
	if (++state->expansion_state.expansion_depth ==
	    PREPROCESSOR_MAX_EXPANSION_DEPTH) {
		generalError("expansion stack full");
		return -1;
	}

	int expansion_depth = state->expansion_state.expansion_depth;
	struct ExpansionContext* context =
	    &state->expansion_state.expansion_stack[expansion_depth];

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
	return 0;
}

static void popContext(struct PreprocessorState* state)
{
	if (--state->expansion_state.expansion_depth == -1) {
		generalError("invalid context pop");
		exit(1);
	}
	int expansion_depth = state->expansion_state.expansion_depth;
}

void beginExpansion(struct PreprocessorState* state,
                    struct PreprocessorDefinition* definition)
{
	struct PreprocessorExpansionState* expansion_state =
	    &state->expansion_state;

	expansion_state->token_marker = state->tokens.num;

	expansion_state->function_like = isFunctionLike(definition);
	expansion_state->begin_expansion = true;

	expansion_state->expansion_depth = 0;

	struct ExpansionContext* current_context =
	    &expansion_state->expansion_stack[0];

	current_context->param.iterators = NULL;
	current_context->param.parent = NULL;
	current_context->param.num_params = definition->num_params;

	initTokenIterator(&current_context->iterator, definition);
}

void stopExpansion(struct PreprocessorState* state)
{
	state->expansion_state.begin_expansion = false;
	state->expansion_state.function_like = false;
	resetLinearAllocator(&state->allocator);
	state->tokens.num = state->expansion_state.token_marker;
}

bool prepareMacroParamTokens(struct PreprocessorState* state,
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
			generalError("macro parantheses not closed");
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
						generalError("to many macro parameters");
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
		generalError("more macro parameters expected");
		goto out;
	}
	status = true;
out:
	return status;
}

int expand(struct PreprocessorState* state, struct StringSet* identifiers,
           struct PreprocessorToken* token)
{
	int expansion_depth = state->expansion_state.expansion_depth;

	struct ExpansionContext* current_context =
	    &state->expansion_state.expansion_stack[expansion_depth];
	struct TokenIterator* it = &current_context->iterator;

	if (it->cur > it->end) {
		if (state->expansion_state.expansion_depth == 0) {
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
		const char* identifier = getStringAt(identifiers, index);
		uint32_t hash = getHashAt(identifiers, index);
		int length = getLengthAt(identifiers, index);
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
					generalError(
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
						generalError("macro parantheses not closed");
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

bool getExpandedToken(struct PreprocessorState* state,
                      struct StringSet* identifier,
                      struct PreprocessorToken* token)
{
	int result;
	int count = 0;
	do {
		result = expand(state, identifier, token);
		count++;
	} while (result == EXPANSION_RESULT_CONTINUE);

	return result == EXPANSION_RESULT_ERROR ? false : true;
}

void printPPToken(struct LexerState* state,
                  const struct PreprocessorToken* pp_token)
{
	struct LexerToken token;
	createLexerTokenFromPPToken(state, pp_token, &token);
	printToken(state, &token);
}
