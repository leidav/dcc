#include "cpp.h"

#include <stdlib.h>

#include "string_set.h"

int createPreprocessorTokenSet(struct PreprocessorTokenSet* set,
                               size_t max_tokens)
{
	set->max_tokens = max_tokens;
	set->num = 0;
	set->tokens = malloc(sizeof(*set->tokens) * max_tokens);
	if (set->tokens == NULL) {
		return -1;
	}
	return 0;
}
int createPreprocessorDefinitionSet(struct PreprocessorDefinitionSet* set,
                                    size_t max_definitions,
                                    int pp_definition_stringset_size)
{
	set->max_definitions = max_definitions;
	set->num = 0;
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
