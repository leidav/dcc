#include "cpp.h"

#include <stdlib.h>

#include "allocator.h"
#include "error.h"
#include "lexer.h"
#include "string_set.h"

int createPreprocessorTokenSet(struct PreprocessorTokenSet* set,
                               size_t max_tokens)
{
	struct PreprocessorToken* tokens =
	    malloc(sizeof(*set->tokens) * max_tokens);
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
                                    int pp_definition_stringset_size)
{
	set->definitions = malloc(sizeof(*set->definitions) * max_definitions);
	if (set->definitions == NULL) {
		return -1;
	}
	if (createStringSet(&set->pp_definition_names, pp_definition_stringset_size,
	                    max_definitions) != 0) {
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
	state->pp_expansion_state.current_state = ALLOCATE_TYPE(
	    state->scratchpad, 1, typeof(*state->pp_expansion_state.current_state));
	state->pp_expansion_state.current_state->prev = NULL;
	initTokenIterator(&state->pp_expansion_state.current_state->iterator,
	                  definition);
}

struct PreprocessorToken* getExpandedToken(struct LexerState* state)
{
	int p = state->pp_expansion_state.pos;
	struct TokenIterator* it =
	    &state->pp_expansion_state.current_state->iterator;
	struct PreprocessorToken* token = NULL;
	if (it->cur <= it->end) {
		token = getTokenAt(state, it->cur);
		it->cur++;
	}

	return token;
}

void stopExpansion(struct LexerState* state)
{
	state->pp_expansion_state.pos = -1;
	state->pp_expansion_state.begin_expansion = false;
	state->pp_expansion_state.function_like = false;
	state->expand_macro = false;
	resetAllocatorState(state->scratchpad,
	                    state->pp_expansion_state.memory_marker);
}
