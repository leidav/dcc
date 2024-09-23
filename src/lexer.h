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

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdint.h>

#include "cpp.h"
#include "input_file.h"
#include "string_set.h"

#define LEXER_IDENTIFIER_STRINGSET_SIZE (4096 << 2)
#define LEXER_LITERAL_STRINGSET_SIZE (4096 << 3)
#define LEXER_PP_NUMBER_STRINGSET_SIZE (4096 << 3)
#define LEXER_MAX_DEFINITION_STRINGSET_SIZE (4096 << 3)

#define LEXER_MAX_IDENTIFIER_COUNT 1024
#define LEXER_MAX_STRING_LITERAL_COUNT 1024
#define LEXER_MAX_PP_NUMBER_COUNT 1024
#define LEXER_MAX_PP_CONSTANT_COUNT 1024

#define LEXER_IS_PREPROCESSOR_MACRO 0x1

enum LexerResult {
	LEXER_RESULT_SUCCESS = 0,
	LEXER_RESULT_NO_MATCH = 1,
	LEXER_RESULT_FAIL = -1,
};
struct LexerErrorState {
	uint16_t line;
	uint16_t column;
	int line_pos;
	const char* error_message;
	const struct InputFile* file;
};

enum TokenType {
	IDENTIFIER = 0,
	/*keywords*/
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_WHILE,
	KEYWORD_FOR,
	KEYWORD_DO,
	KEYWORD_SWITCH,
	KEYWORD_CASE,
	KEYWORD_BREAK,
	KEYWORD_CONTINUE,
	KEYWORD_STRUCT,
	KEYWORD_ENUM,
	KEYWORD_UNION,
	KEYWORD_TYPEDEF,
	KEYWORD_VOID,
	KEYWORD_CHAR,
	KEYWORD_SHORT,
	KEYWORD_INT,
	KEYWORD_LONG,
	KEYWORD_FLOAT,
	KEYWORD_DOUBLE,
	KEYWORD_SIGNED,
	KEYWORD_UNSIGNED,
	KEYWORD_STATIC,
	KEYWORD_EXTERN,
	KEYWORD_CONST,
	KEYWORD_INLINE,
	KEYWORD_RETURN,
	KEYWORD_GOTO,
	KEYWORD_REGISTER,
	KEYWORD_RESTRICT,
	KEYWORD_VOLATILE,
	KEYWORD_DEFAULT,
	KEYWORD_BOOL,
	KEYWORD_COMPLEX,
	KEYWORD_IMAGINARY,
	KEYWORD_CONSTEXPR,
	KEYWORD_AUTO,
	KEYWORD_SIZEOF,
	KEYWORD_ALIGNAS,
	KEYWORD_ALIGNOF,
	KEYWORD_GENERIC,
	KEYWORD_NORETURN,
	KEYWORD_STATIC_ASSERT,
	KEYWORD_CONSTEVAL,
	/*operator punctuators*/
	PUNCTUATOR_PLUS,
	PUNCTUATOR_MINUS,
	PUNCTUATOR_DIV,
	PUNCTUATOR_MODULO,
	PUNCTUATOR_PLUSPLUS,
	PUNCTUATOR_MINUSMINUS,
	PUNCTUATOR_AND,
	PUNCTUATOR_OR,
	PUNCTUATOR_XOR,
	PUNCTUATOR_SHIFT_LEFT,
	PUNCTUATOR_SHIFT_RIGHT,
	PUNCTUATOR_NEGATE,
	PUNCTUATOR_LOGICAL_AND,
	PUNCTUATOR_LOGICAL_OR,
	PUNCTUATOR_LOGICAL_NOT,
	PUNCTUATOR_EQUAL,
	PUNCTUATOR_NOT_EQUAL,
	PUNCTUATOR_LESS,
	PUNCTUATOR_GREATER,
	PUNCTUATOR_LESS_OR_EQUAL,
	PUNCTUATOR_GREATER_OR_EQUAL,
	PUNCTUATOR_ASSIGNMENT,
	PUNCTUATOR_PLUS_ASSIGNMENT,
	PUNCTUATOR_MINUS_ASSIGNMENT,
	PUNCTUATOR_MUL_ASSIGNMENT,
	PUNCTUATOR_DIV_ASSIGNMENT,
	PUNCTUATOR_MODULO_ASSIGNMENT,
	PUNCTUATOR_AND_ASSIGNMENT,
	PUNCTUATOR_OR_ASSIGNMENT,
	PUNCTUATOR_XOR_ASSIGNMENT,
	PUNCTUATOR_SHIFT_LEFT_ASSIGNMENT,
	PUNCTUATOR_SHIFT_RIGHT_ASSIGNMENT,
	PUNCTUATOR_POINT,
	PUNCTUATOR_DEREFERENCE,
	PUNCTUATOR_CONDITIONAL,
	/*other punctuators*/
	PUNCTUATOR_PARENTHESE_LEFT,
	PUNCTUATOR_PARENTHESE_RIGHT,
	PUNCTUATOR_BRACKET_LEFT,
	PUNCTUATOR_BRACKET_RIGHT,
	PUNCTUATOR_BRACE_LEFT,
	PUNCTUATOR_BRACE_RIGHT,
	PUNCTUATOR_ASTERISC,
	PUNCTUATOR_COMMA,
	PUNCTUATOR_COLON,
	PUNCTUATOR_SEMICOLON,
	/*literals*/
	LITERAL_STRING,
	CONSTANT_CHAR,
	CONSTANT_UNSIGNED_CHAR,
	CONSTANT_INT,
	CONSTANT_UNSIGNED_INT,
	CONSTANT_FLOAT,
	CONSTANT_DOUBLE,
	// Preprocessor
	PP_NUMBER,
	PP_PARAM,
	PP_CONCAT,
	PP_STRINGIFY,
	// other
	TOKEN_EOF,
	TOKEN_UNKNOWN,
	TOKEN_EMPTY,
};

struct LexerConstant {
	union {
		uint8_t param_index;
		uint16_t string_index;
		int character_literal;
		uint64_t int_literal;
		float float_literal;
		double double_literal;
	};
};

struct LexerConstantSet {
	int num;
	int max_count;
	struct LexerConstant* constants;
};

struct LexerToken {
	struct LexerConstant value;
	uint16_t line;
	uint16_t column;
	uint16_t line_pos;
	uint8_t type;
	bool literal;
};

struct LexerSourcePos {
	int line;
	int column;
	int file_pos;
	int line_pos;
};

struct LexerState {
	struct LexerSourcePos current_pos;
	struct LexerSourcePos lookahead_pos;
	bool carriage_return;
	bool line_beginning;
	bool macro_body;
	bool expand_macro;
	bool error_handled;
	char c;
	char lookahead;
	struct InputFile current_file;
	struct StringSet identifiers;
	struct StringSet string_literals;
	struct StringSet pp_numbers;
	struct LexerConstantSet constants;
	struct PreprocessorState pp_state;
	struct LinearAllocator* scratchpad;
};

int initLexer(struct LexerState* state, const char* file_path);

void cleanupLexer(struct LexerState* state);

bool getNextToken(struct LexerState* state, struct LexerToken* token);

void printToken(struct LexerState* state, const struct LexerToken* token);

void printTokenAsCStruct(struct LexerState* state,
                         const struct LexerToken* token);

bool createLexerTokenFromPPToken(struct LexerState* state,
                                 const struct PreprocessorToken* pp_token,
                                 struct LexerToken* token);
#endif
