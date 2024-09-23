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

#ifndef CPP_H
#define CPP_H

#include <stdint.h>

#include "memory/linear_allocator.h"
#include "string_set.h"

#define PREPROCESSOR_MAX_EXPANSION_DEPTH 1024

#define PREPROCESSOR_MAX_DEFINITION_COUNT 1024
#define PREPROCESSOR_MAX_DEFINITION_TOKEN_COUNT (4096 << 2)

enum PPDefinitionFlags { FUNCTION_LIKE = 0x1 };

struct LexerState;
struct LexerToken;
struct LexerConstantSet;

struct PreprocessorToken {
	uint16_t line;
	uint16_t column;
	uint16_t value_handle;
	uint16_t line_pos;
	uint8_t type;
};

struct PreprocessorTokenSet {
	int num;
	int max_tokens;
	struct PreprocessorToken* tokens;
};

struct PreprocessorDefinition {
	uint16_t token_start;
	uint16_t num_tokens;
	uint8_t num_params;
	uint8_t flags;
};

struct PreprocessorDefinitionSet {
	struct PreprocessorDefinition* definitions;
	struct StringSet pp_definition_names;
};

struct TokenIterator {
	int16_t start;
	int16_t cur;
	int16_t end;
};

struct ParamContext {
	const struct ParamContext* parent;
	const struct TokenIterator* iterators;
	uint8_t num_params;
};

struct ExpansionContext {
	struct TokenIterator iterator;
	struct ParamContext param;
};

struct PreprocessorExpansionState {
	struct ExpansionContext* expansion_stack;
	int expansion_depth;
	int token_marker;
	bool function_like;
	bool begin_expansion;
};

struct PreprocessorState {
	struct PreprocessorTokenSet tokens;
	struct PreprocessorDefinitionSet definitions;
	struct PreprocessorExpansionState expansion_state;
	struct LinearAllocator allocator;
};

static inline bool isFunctionLike(struct PreprocessorDefinition* definition)
{
	return definition->flags & FUNCTION_LIKE;
}

void createPreprocessorTokenSetFromBuffer(struct PreprocessorTokenSet* set,
                                          size_t max_tokens, void* buffer);

int createPreprocessorTokenSet(struct PreprocessorTokenSet* set,
                               size_t max_tokens, struct Allocator* allocator);

int createPreprocessorDefinitionSet(struct PreprocessorDefinitionSet* set,
                                    size_t max_definition,
                                    int pp_definition_stringset_size,
                                    struct Allocator* allocator);

int addPreprocessorToken(struct PreprocessorState* state,
                         struct LexerConstantSet* constants,
                         const struct LexerToken* token);

int getPreprocessorTokenPos(struct PreprocessorState* state);

struct PreprocessorTokenSet* getPreprocessorTokenSet(
    struct PreprocessorState* state);

int createPreprocessorDefinition(struct PreprocessorState* state,
                                 int start_index, int num_tokens,
                                 int num_params, const char* name,
                                 int name_length, bool function_like);

struct PreprocessorDefinition* findDefinition(struct PreprocessorState* state,
                                              const char* name, int length,
                                              uint32_t hash);

int initPreprocessorState(struct PreprocessorState* state);

void cleanupPreprocessorState(struct PreprocessorState* state);

void beginExpansion(struct PreprocessorState* state,
                    struct PreprocessorDefinition* definition);

bool getExpandedToken(struct PreprocessorState* state,
                      struct StringSet* identifier,
                      struct PreprocessorToken* token);

void stopExpansion(struct PreprocessorState* state);

void printPPToken(struct LexerState* state,
                  const struct PreprocessorToken* token);

bool prepareMacroParamTokens(struct PreprocessorState* state,
                             struct TokenIterator* params,
                             struct TokenIterator* iterator,
                             int expected_param_count);

#endif
