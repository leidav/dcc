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
	struct ExpansionContext* prev;
	int depth;
};

struct PreprocessorExpansionState {
	struct ExpansionContext* current_context;
	int pos;
	uint16_t token_marker;
	bool function_like;
	bool begin_expansion;
};

struct TokenIterator* allocateIterators(struct LexerState* state,
                                        int num_iterators);

struct ExpansionContext* allocateExpansionContext(struct LexerState* state);

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

bool getExpandedToken(struct LexerState* state,
                      struct PreprocessorToken* token);

void stopExpansion(struct LexerState* state);

void printPPToken(struct LexerState* state,
                  const struct PreprocessorToken* token);

#endif
