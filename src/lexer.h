#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdio.h>

#include "input_file.h"
#include "string_set.h"

enum TokenType {
	IDENTIFIER = 0,
	/*keywords*/
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_WHILE,
	KEYOWRD_FOR,
	KEYWORD_DO,
	KEYWORD_SWITCH,
	KEYWORD_CASE,
	KEYWORD_BREAK,
	KEYWORD_STRUCT,
	KEYWORD_ENUM,
	KEYWORD_UNION,
	KEYWORD_TYPEDEF,
	KEYWORD_CHAR,
	KEYWORD_SHORT,
	KEYWORD_INT,
	KEYWORD_FLOAT,
	KEYWORD_DOUBLE,
	KEYWORD_SIGNED,
	KEYWORD_UNSIGNED,
	KEYWORD_STATIC,
	KEYWORD_EXTERN,
	/*operators*/
	OPERATOR_PLUS,
	OPERATOR_MINUS,
	OPERATOR_DIV,
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
	OPERATOR_EQUAL,
	OPERATOR_LESS,
	OPERATOR_BIGGER,
	OPERATOR_LESS_OR_EQUAL,
	OPERATOR_BIGGER_OR_EQUL,
	OPERATOR_ASSIGNMENT,
	OPERATOR_PLUS_ASSIGNMENT,
	OPERATOR_MINUS_ASSIGNMENT,
	OPERATOR_MUL_ASSIGNMENT,
	OPERATOR_DIV_ASSIGNMENT,
	OPERATOR_AND_ASSIGNMENT,
	OPERATOR_OR_ASSIGNMENT,
	OPERATOR_XOR_ASSIGNMENT,
	OPERATOR_POINT,
	OPERATOR_DEREFERENCE,
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
	LITERAL_CHAR,
	LITERAL_UNSIGNED_CHAR,
	LITERAL_INT,
	LITERAL_UNSIGNED_INT,
	LITERAL_FLOAT,
	LITERAL_DOUBLE,
	// end of file
	TOKEN_EOF,
};

struct LexerToken {
	uint16_t line;
	uint16_t column;
	enum TokenType type;
	union {
		struct LexerString* string;
		char character_literal;
		int64_t int_literal;
		uint64_t uint_literal;
		float float_literal;
		double double_literal;
	} value;
};

struct LexerState {
	uint16_t current_line;
	uint16_t current_column;
	int current_pos;
	struct InputFile current_file;
	struct StringSet identifiers;
	struct StringSet string_literals;
};

void initLexer(struct LexerState* state);

int getNextToken(struct LexerState* state, struct LexerToken* token);

#endif