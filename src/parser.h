#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

#include "lexer.h"

enum BinaryOperatorType {
	BINOP_ADD,
	BINOP_SUB,
	BINOP_MUL,
	BINOP_DIV,
	BINOP_MOD,
	BINOP_SHIFT_LEFT,
	BINOP_SHIFT_RIGHT,
	BINOP_BITWISE_AND,
	BINOP_BITWISE_OR,
	BINOP_BITWISE_XOR,
	BINOP_LOGICAL_AND,
	BINOP_LOGICAL_OR,
	BINOP_CMP_EQUAL,
	BINOP_CMP_LESS,
	BINOP_CMP_GREATER,
	BINOP_CMP_LESS_EQUAL,
	BINOP_CMP_GREATER_EQUAL,
	BINOP_CMP_NOT_EQUAL
};

enum UnaryOperatorType {
	UNOP_PREFIX_INC,
	UNOP_PREFIX_DEC,
	UNOP_POSTFIX_INC,
	UNOP_POSTFIX_DEC
};

enum NodeType {
	AST_TRANSLATION_UNIT,

	AST_VARIABLE_DECLARATION,
	AST_FUNCTION_DECLARATION,
	AST_TYPEDEF_DECLARATION,
	AST_STRUCT_DECLARATION,

	AST_DECLARE_STATEMENT,
	AST_ASSIGNMENT_STATMENT,
	AST_COMPOUND_STATEMENT,
	AST_IF_STATEMENT,
	AST_WHILE_STATEMENT,
	AST_FOR_STATEMENT,

	AST_BINARY_OP,
	AST_UNARY_OP,
	AST_CAST_OP,

	AST_IDENTIFIER,
	AST_CONSTANT,
	AST_STRING_LITERAL,
};

struct ASTNode {
	uint8_t type;
	uint8_t sub_type;
	uint16_t child_nodes_start;
	uint16_t num_children;
};

struct AST {
	struct ASTNode *nodes;
	int num_nodes;
	int root_index;
};

struct ParserState {
	struct LexerState *lexer;
	struct LexerToken current_token;
	struct AST ast;
};

int initParser(struct ParserState *state, struct LexerState *lexer);

int parse(struct ParserState *state);

#endif
