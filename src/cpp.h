#ifndef CPP_H
#define CPP_H

#include <stdint.h>

#include "string_set.h"

struct LexerState;

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
	uint16_t replacement_list_start;
	uint16_t num_tokens;
	uint16_t name_index;
	uint8_t num_params;
	uint8_t flags;
};

struct PreprocessorDefinitionSet {
	int num;
	int max_definitions;
	struct PreprocessorDefinition* definitions;
	struct StringSet pp_definition_names;
};

int createPreprocessorTokenSet(struct PreprocessorTokenSet* set,
                               size_t max_tokens);
int createPreprocessorDefinitionSet(struct PreprocessorDefinitionSet* set,
                                    size_t max_definition,
                                    int pp_definition_stringset_size);
int addPreprocessorToken(struct LexerState* state,
                         const struct PreprocessorToken tokens);
int addPreprocessorDefinition(struct LexerState* state,
                              const struct PreprocessorDefinition* definition);

#endif
