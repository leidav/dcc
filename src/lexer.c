#include "lexer.h"

#include <stdbool.h>

struct FileContext {
	int line;
	int column;
};

static void getFileContext(struct LexerState* state, struct FileContext* ctx)
{
	ctx->line = state->line;
	ctx->column = state->column;
}
static void setFileContext(struct LexerState* state,
                           const struct FileContext* ctx)
{
	state->line = ctx->line;
	state->column = ctx->column;
}

static int createSimpleToken(struct LexerToken* token,
                             const struct FileContext* ctx, enum TokenType type)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = type;
}

static int createIntegerLiteralToken(struct LexerToken* token,
                                     const struct FileContext* ctx,
                                     uint64_t number, bool isSigned)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = LITERAL_INT;
	if (isSigned) {
		token->value.int_literal = (int64_t)number;
	} else {
		token->value.uint_literal = number;
	}
}

static const char* fileName(const char* path)
{
	int last_seperator = 0;
	int i = 0;
	const char* ptr = path;
	while (*ptr) {
		if (*ptr == '/') {
			last_seperator = i;
		}
		ptr++;
		i++;
	}
	return &path[last_seperator + 1];
}

static void consumeInput(struct LexerState* state)
{
	state->c = readChar(&state->current_file);
	state->pos++;
}

int initLexer(struct LexerState* state, const char* file_path)
{
	state->line = 0;
	state->column = 0;
	state->pos = 0;
	if (!openInputFile(&state->current_file, file_path, fileName(file_path))) {
		return -1;
	}
	if (!createStringSet(&state->identifiers, LEXER_IDENTIFIER_STRINGSET_SIZE,
	                     LEXER_MAX_IDENTIFIER_COUNT)) {
		return -1;
	}
	if (!createStringSet(&state->string_literals, LEXER_LITERAL_STRINGSET_SIZE,
	                     LEXER_MAX_STRING_LITERAL_COUNT)) {
		return -1;
	}
	consumeInput(state);
	return 0;
}

static bool skipIfWhiteSpace(struct LexerState* state)
{
	bool isWhitespace;
	if (state->c == '\n') {
		state->column = 0;
		state->line++;
		isWhitespace = true;
		consumeInput(state);
	} else if (state->c == ' ' || state->c == '\t') {
		state->column++;
		isWhitespace = true;
		consumeInput(state);
	} else if (state->c == '\r') {
		state->column = 0;
		isWhitespace = true;
		consumeInput(state);
	} else {
		isWhitespace = false;
	}
	return isWhitespace;
}
static void skipWhiteSpaces(struct LexerState* state)
{
	while (skipIfWhiteSpace(state)) {
	}
}

static void skipMultiLineComment(struct LexerState* state)
{
	while (state->c != INPUT_EOF) {
		skipIfWhiteSpace(state);
		if (state->c == '*') {
			state->column++;
			consumeInput(state);
			if (state->c == '/') {
				state->column++;
				break;
			}
		} else {
			state->column++;
			consumeInput(state);
		}
	}
}

static void skipSingleLineComment(struct LexerState* state)
{
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
			state->column = 0;
			state->line++;
			consumeInput(state);
			break;
		} else if (state->c == '\r') {
			state->line = 0;
		}
		consumeInput(state);
	}
}
static bool lexWord(struct LexerState* state, struct LexerToken* token,
                    const struct FileContext* ctx)
{
	char buffer[256];
	int pos = 0;
	while (state->c != INPUT_EOF) {
		if (!((state->c >= 'A' && state->c <= 'Z') ||
		      (state->c >= 'a' && state->c <= 'z') ||
		      (state->c >= '0' && state->c <= '9') || (state->c == '_'))) {
			break;
		}
		buffer[pos] = state->c;
		state->column++;
		consumeInput(state);
		pos++;
	}
	return true;
}
static bool lexNumber(struct LexerState* state, struct LexerToken* token,
                      const struct FileContext* ctx)
{
	state->column++;
	consumeInput(state);
	return true;
}
static bool lexFractionalNumber(struct LexerState* state,
                                struct LexerToken* token,
                                const struct FileContext* ctx)
{
	state->column++;
	consumeInput(state);
	return true;
}

static bool lexHexNumber(struct LexerState* state, struct LexerToken* token,
                         const struct FileContext* ctx)
{
	state->column++;
	consumeInput(state);
	return true;
}

bool getNextToken(struct LexerState* state, struct LexerToken* token)
{
	bool success = true;
	struct FileContext ctx;
	if (state->c == INPUT_EOF) {
		getFileContext(state, &ctx);
		createSimpleToken(token, &ctx, TOKEN_EOF);
	} else {
		skipIfWhiteSpace(state);
		getFileContext(state, &ctx);
		if (state->c == '/') {
			state->column++;
			consumeInput(state);
			if (state->c == '/') {
				// Single Line Comment
				state->column++;
				consumeInput(state);
				skipSingleLineComment(state);
			} else if (state->c == '*') {
				// Multi Line Comment
				state->column++;
				consumeInput(state);
				skipMultiLineComment(state);
			} else if (state->c == '=') {
				// Divison Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_DIV_ASSIGNMENT);
			} else {
				//  Division Operator
				createSimpleToken(token, &ctx, OPERATOR_DIV);
			}
		} else if (state->c == '*') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Multiplication Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_MUL_ASSIGNMENT);
			} else {
				// ASTERISC
				createSimpleToken(token, &ctx, ASTERISC);
			}
		} else if (state->c == '%') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Modulo Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_MODULO_ASSIGNMENT);
			} else {
				// Modulo Operator
				createSimpleToken(token, &ctx, OPERATOR_MODULO);
			}
		} else if (state->c == '+') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Plus Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_PLUS_ASSIGNMENT);
			} else if (state->c == '+') {
				// PlusPlus Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_PLUSPLUS);
			} else {
				// Plus Operator
				createSimpleToken(token, &ctx, OPERATOR_PLUS);
			}
		} else if (state->c == '-') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Minus Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_MINUS_ASSIGNMENT);
			} else if (state->c == '-') {
				// MinusMinus Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_MINUSMINUS);
			} else if (state->c == '>') {
				// Dereference Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_DEREFERENCE);
			} else {
				// Plus Operator
				createSimpleToken(token, &ctx, OPERATOR_MINUS);
			}
		} else if (state->c == '&') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// And Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_AND_ASSIGNMENT);
			} else if (state->c == '&') {
				// Logical And Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_LOGICAL_AND);
			} else {
				// And Operator
				createSimpleToken(token, &ctx, OPERATOR_AND);
			}
		} else if (state->c == '|') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Or Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_OR_ASSIGNMENT);
			} else if (state->c == '&') {
				// Logical And Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_LOGICAL_OR);
			} else {
				// Plus Operator
				createSimpleToken(token, &ctx, OPERATOR_OR);
			}
		} else if (state->c == '^') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Xor Assignment Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_XOR_ASSIGNMENT);
			} else {
				// Plus Operator
				createSimpleToken(token, &ctx, OPERATOR_XOR);
			}
		} else if (state->c == '~') {
			state->column++;
			consumeInput(state);
			// Negate Operator
			createSimpleToken(token, &ctx, OPERATOR_NEGATE);
		} else if (state->c == '!') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Not Equal Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_NOT_EQUAL);
			} else {
				// Logical Not Operator
				createSimpleToken(token, &ctx, OPERATOR_LOGICAL_NOT);
			}
		} else if (state->c == '<') {
			state->column++;
			consumeInput(state);
			if (state->c == '<') {
				state->column++;
				consumeInput(state);
				if (state->c == '=') {
					// Shift left Assignment Operator
					state->column++;
					consumeInput(state);
					createSimpleToken(token, &ctx,
					                  OPERATOR_SHIFT_LEFT_ASSIGNMENT);
				} else {
					// Shift left Operator
					createSimpleToken(token, &ctx, OPERATOR_SHIFT_LEFT);
				}
			} else if (state->c == '=') {
				// Less or equal Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_LESS_OR_EQUAL);
			} else {
				// Less than Operator
				createSimpleToken(token, &ctx, OPERATOR_LESS);
			}
		} else if (state->c == '>') {
			state->column++;
			consumeInput(state);
			if (state->c == '>') {
				state->column++;
				consumeInput(state);
				if (state->c == '=') {
					// Shift right Assignment Operator
					state->column++;
					consumeInput(state);
					createSimpleToken(token, &ctx,
					                  OPERATOR_SHIFT_RIGHT_ASSIGNMENT);
				} else {
					// Shift right Operator
					createSimpleToken(token, &ctx, OPERATOR_SHIFT_RIGHT);
				}
			} else if (state->c == '=') {
				// Greater or equal Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_GREATER_OR_EQUAL);
			} else {
				// Greater than Operator
				createSimpleToken(token, &ctx, OPERATOR_GREATER);
			}
		} else if (state->c == '=') {
			state->column++;
			consumeInput(state);
			if (state->c == '=') {
				// Equal Comparsion Operator
				state->column++;
				consumeInput(state);
				createSimpleToken(token, &ctx, OPERATOR_EQUAL);
			} else {
				// Assignment Operator
				createSimpleToken(token, &ctx, OPERATOR_ASSIGNMENT);
			}
		} else if (state->c == '?') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, OPERATOR_CONDITIONAL);
		} else if (state->c == ':') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, COLON);
		} else if (state->c == ';') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, SEMICOLON);
		} else if (state->c == ',') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, COMMA);
		} else if (state->c == '(') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, PARENTHESE_LEFT);
		} else if (state->c == ')') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, PARENTHESE_RIGHT);
		} else if (state->c == '[') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, BRACKET_LEFT);
		} else if (state->c == ']') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, BRACKET_RIGHT);
		} else if (state->c == '{') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, BRACE_LEFT);
		} else if (state->c == '}') {
			state->column++;
			consumeInput(state);
			createSimpleToken(token, &ctx, BRACE_RIGHT);
		} else if (state->c == '.') {
			state->column++;
			consumeInput(state);
			if (state->c >= '0' && state->c <= '9') {
				lexFractionalNumber(state, token, &ctx);
			} else {
				createSimpleToken(token, &ctx, OPERATOR_POINT);
			}
		} else if ((state->c >= 'A' && state->c <= 'Z') ||
		           (state->c >= 'a' && state->c <= 'z') || (state->c == '_')) {
			// Keyword or Identifier
			success = lexWord(state, token, &ctx);
		} else if (state->c == '0') {
			state->column++;
			consumeInput(state);
			if (state->c == 'x') {
				state->column++;
				consumeInput(state);
				if ((state->c >= 'a' && state->c <= 'f') ||
				    (state->c >= 'A' && state->c <= 'F') ||
				    (state->c >= '0' && state->c <= '9')) {
					// hexadecimal number
					success = lexHexNumber(state, token, &ctx);
				} else {
					// not a valid hex number
					success = false;
				}
			} else {
				success = lexNumber(state, token, &ctx);
			}
		} else if (state->c >= '1' && state->c <= '9') {
			// number
			success = lexNumber(state, token, &ctx);

		} else {
			success = false;
		}
	}
	return success;
}