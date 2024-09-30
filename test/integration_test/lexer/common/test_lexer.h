#ifndef TEST_LEXER_H
#define TEST_LEXER_H

#include <stdint.h>

#include "lexer.h"

struct ExpectedToken {
	int line;
	int column;
	enum TokenType type;
	union {
		int64_t int_value;
		double float_value;
		const char* string_value;
	};
};

int test_lexer(const char* file, const struct ExpectedToken* expcted_tokens,
               int num_tokens);

#endif
