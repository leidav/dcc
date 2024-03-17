#include "parser.h"

int initParser(struct ParserState *state, struct LexerState *lexer)
{
	state->lexer = lexer;
	return 0;
}
