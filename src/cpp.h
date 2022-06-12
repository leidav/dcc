#ifndef CPP_H
#define CPP_H

#include <stdint.h>

#include "string_set.h"

#define PREPROCESSOR_MAX_EXPANSION_DEPTH 1024

enum PPDefinitionFlags { FUNCTION_LIKE = 0x1 };

struct LexerState;
struct LexerToken;

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
	uint16_t cur;
	uint16_t end;
};

struct StackEntry {
	struct TokenIterator iterator;
	struct StackEntry* prev;
};

struct PreprocessorExpansionState {
	struct StackEntry* current_state;
	int pos;
	size_t memory_marker;
	bool function_like;
	bool begin_expansion;
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

int addPreprocessorToken(struct LexerState* state,
                         struct PreprocessorTokenSet* tokens,
                         const struct LexerToken* token);

int createPreprocessorDefinition(struct LexerState* state, int token_start,
                                 int num_tokens, int num_params,
                                 const char* name, int name_length,
                                 bool function_like);

struct PreprocessorDefinition* findDefinition(struct LexerState* state,
                                              const char* name, int length,
                                              uint32_t hash);
int createPreprocessorExpansionState(struct PreprocessorExpansionState* state);

void beginExpansion(struct LexerState* state,
                    struct PreprocessorDefinition* definition);

struct PreprocessorToken* getExpandedToken(struct LexerState* state);

void stopExpansion(struct LexerState* state);

#endif
