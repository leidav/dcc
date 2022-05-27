#ifndef ERROR_H
#define ERROR_H

struct LexerState;
struct FileContext;
struct LexerToken;

void lexerError(struct LexerState* state, const char* reason);
void generalError(const char* reason);

static inline void unexpectedCharacterError(struct LexerState* state)
{
	lexerError(state, "unexpected character");
}
#endif
