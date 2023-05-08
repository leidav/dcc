#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
	/*operators*/
	OPERATOR_PLUS,
	OPERATOR_MINUS,
	OPERATOR_DIV,
	OPERATOR_MODULO,
	OPERATOR_PLUSPLUS,
	OPERATOR_MINUSMINUS,
	OPERATOR_AND,
	OPERATOR_OR,
	OPERATOR_XOR,
	OPERATOR_SHIFT_LEFT,
	OPERATOR_SHIFT_RIGHT,
	OPERATOR_NEGATE,
	OPERATOR_LOGICAL_AND,
	OPERATOR_LOGICAL_OR,
	OPERATOR_LOGICAL_NOT,
	OPERATOR_EQUAL,
	OPERATOR_NOT_EQUAL,
	OPERATOR_LESS,
	OPERATOR_GREATER,
	OPERATOR_LESS_OR_EQUAL,
	OPERATOR_GREATER_OR_EQUAL,
	OPERATOR_ASSIGNMENT,
	OPERATOR_PLUS_ASSIGNMENT,
	OPERATOR_MINUS_ASSIGNMENT,
	OPERATOR_MUL_ASSIGNMENT,
	OPERATOR_DIV_ASSIGNMENT,
	OPERATOR_MODULO_ASSIGNMENT,
	OPERATOR_AND_ASSIGNMENT,
	OPERATOR_OR_ASSIGNMENT,
	OPERATOR_XOR_ASSIGNMENT,
	OPERATOR_SHIFT_LEFT_ASSIGNMENT,
	OPERATOR_SHIFT_RIGHT_ASSIGNMENT,
	OPERATOR_POINT,
	OPERATOR_DEREFERENCE,
	OPERATOR_CONDITIONAL,
	/*other symbols*/
	PARENTHESE_LEFT,
	PARENTHESE_RIGHT,
	BRACKET_LEFT,
	BRACKET_RIGHT,
	BRACE_LEFT,
	BRACE_RIGHT,
	ASTERISC,
	COMMA,
	COLON,
	SEMICOLON,
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

bool getNextToken(struct LexerState* state, struct LexerToken* token);

void printToken(struct LexerState* state, const struct LexerToken* token);

bool createLexerTokenFromPPToken(struct LexerState* state,
                                 const struct PreprocessorToken* pp_token,
                                 struct LexerToken* token);
#endif
