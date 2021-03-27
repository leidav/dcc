#include "lexer.h"

#include <stdbool.h>
#include <string.h>

struct FileContext {
	int line;
	int column;
};

static bool consumeLexableChar(struct LexerState* state);
static bool skipBackslashNewline(struct LexerState* state);

static const char* tokenNames[] = {
    "IDENTIFIER",
    /*keywords*/
    "KEYWORD_IF",
    "KEYWORD_ELSE",
    "KEYWORD_WHILE",
    "KEYOWRD_FOR",
    "KEYWORD_DO",
    "KEYWORD_SWITCH",
    "KEYWORD_CASE",
    "KEYWORD_BREAK",
    "KEYWORD_STRUCT",
    "KEYWORD_ENUM",
    "KEYWORD_UNION",
    "KEYWORD_TYPEDEF",
    "KEYWORD_VOID",
    "KEYWORD_CHAR",
    "KEYWORD_SHORT",
    "KEYWORD_INT",
    "KEYWORD_FLOAT",
    "KEYWORD_DOUBLE",
    "KEYWORD_SIGNED",
    "KEYWORD_UNSIGNED",
    "KEYWORD_STATIC",
    "KEYWORD_EXTERN",
    "KEYWORD_CONST",
    "KEYWORD_INLINE",
    "KEYWORD_RETURN",
    "KEYWORD_GOTO",
    /*operators*/
    "OPERATOR_PLUS",
    "OPERATOR_MINUS",
    "OPERATOR_DIV",
    "OPERATOR_MODULO",
    "OPERATOR_PLUSPLUS",
    "OPERATOR_MINUSMINUS",
    "OPERATOR_AND",
    "OPERATOR_OR",
    "OPERATOR_XOR",
    "OPERATOR_SHIFT_LEFT",
    "OPERATOR_SHIFT_RIGHT",
    "OPERATOR_NEGATE",
    "OPERATOR_LOGICAL_AND",
    "OPERATOR_LOGICAL_OR",
    "OPERATOR_LOGICAL_NOT",
    "OPERATOR_EQUAL",
    "OPERATOR_NOT_EQUAL",
    "OPERATOR_LESS",
    "OPERATOR_GREATER",
    "OPERATOR_LESS_OR_EQUAL",
    "OPERATOR_GREATER_OR_EQUAL",
    "OPERATOR_ASSIGNMENT",
    "OPERATOR_PLUS_ASSIGNMENT",
    "OPERATOR_MINUS_ASSIGNMENT",
    "OPERATOR_MUL_ASSIGNMENT",
    "OPERATOR_DIV_ASSIGNMENT",
    "OPERATOR_MODULO_ASSIGNMENT",
    "OPERATOR_AND_ASSIGNMENT",
    "OPERATOR_OR_ASSIGNMENT",
    "OPERATOR_XOR_ASSIGNMENT",
    "OPERATOR_SHIFT_LEFT_ASSIGNMENT",
    "OPERATOR_SHIFT_RIGHT_ASSIGNMENT",
    "OPERATOR_POINT",
    "OPERATOR_DEREFERENCE",
    "OPERATOR_CONDITIONAL",
    /*other symbols*/
    "PARENTHESE_LEFT",
    "PARENTHESE_RIGHT",
    "BRACKET_LEFT",
    "BRACKET_RIGHT",
    "BRACE_LEFT",
    "BRACE_RIGHT",
    "ASTERISC",
    "COMMA",
    "COLON",
    "SEMICOLON",
    /*literals*/
    "LITERAL_STRING",
    "LITERAL_CHAR",
    "LITERAL_UNSIGNED_CHAR",
    "LITERAL_INT",
    "LITERAL_UNSIGNED_INT",
    "LITERAL_FLOAT",
    "LITERAL_DOUBLE",
    // end of file
    "TOKEN_EOF",
    "TOKEN_UNKNOWN",
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
	return 0;
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
	return 0;
}
static int createIdentifierToken(struct LexerToken* token,
                                 const struct FileContext* ctx, uint16_t index)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = IDENTIFIER;
	token->value.string_index = index;
	return 0;
}

static const char* fileName(const char* path)
{
	int last_seperator = -1;
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
	// support unix, dos and legacy mac text files
	state->c = readChar(&state->current_file);
	state->pos++;
	if (state->c == '\r') {
		state->carriage_return = true;
		state->c = '\n';
	} else if ((state->c == '\n') && state->carriage_return) {
		state->carriage_return = false;
		consumeInput(state);
	} else {
		state->carriage_return = false;
	}
}
int initLexer(struct LexerState* state, const char* file_path)
{
	state->line = 0;
	state->column = 0;
	state->pos = 0;
	if (openInputFile(&state->current_file, file_path, fileName(file_path)) !=
	    0) {
		fprintf(stderr, "could not open file\n");
		return -1;
	}
	if (createStringSet(&state->identifiers, LEXER_IDENTIFIER_STRINGSET_SIZE,
	                    LEXER_MAX_IDENTIFIER_COUNT) != 0) {
		return -1;
	}
	if (createStringSet(&state->string_literals, LEXER_LITERAL_STRINGSET_SIZE,
	                    LEXER_MAX_STRING_LITERAL_COUNT) != 0) {
		return -1;
	}
	consumeInput(state);
	state->carriage_return = false;
	return 0;
}
static bool skipIfWhiteSpace(struct LexerState* state)
{
	bool isWhitespace;
	if (state->c == '\n') {
		state->column = 0;
		state->line++;
		consumeInput(state);
		isWhitespace = true;
	} else if (state->c == ' ' || state->c == '\t') {
		state->column++;
		consumeInput(state);
		isWhitespace = true;
	} else if (state->c == '\\') {
		isWhitespace = skipBackslashNewline(state);
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

static bool skipBackslashNewline(struct LexerState* state)
{
	bool success = true;
	while (state->c == '\\') {
		state->column++;
		consumeInput(state);
		while ((state->c == ' ') || (state->c == '\t')) {
			state->column++;
			consumeInput(state);
		}
		if (state->c == '\n') {
			state->column = 0;
			state->line++;
			consumeInput(state);
		} else {
			success = false;
			break;
		}
	}
	return success;
}

static bool consumeLexableChar(struct LexerState* state)
{
	state->column++;
	consumeInput(state);
	return skipBackslashNewline(state);
}

static bool skipMultiLineComment(struct LexerState* state)
{
	bool success = true;
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
			state->column = 0;
			state->line++;
			consumeInput(state);
		} else if (state->c == '*') {
			if (!consumeLexableChar(state)) {
				return false;
			}
			if (state->c == '/') {
				if (!consumeLexableChar(state)) {
					return false;
				}
				break;
			}
		} else {
			state->column++;
			consumeInput(state);
		}
	}
	if (state->c == INPUT_EOF) {
		success = false;
	}
	return success;
}

static void skipSingleLineComment(struct LexerState* state)
{
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
			state->column = 0;
			state->line++;
			skipBackslashNewline(state);
			break;
		}
		consumeInput(state);
	}
}
static bool lexWord(struct LexerState* state, struct LexerToken* token,
                    const struct FileContext* ctx)
{
	char buffer[256];
	int length = 0;
	bool success = true;
	while (state->c != INPUT_EOF) {
		if (!((state->c >= 'A' && state->c <= 'Z') ||
		      (state->c >= 'a' && state->c <= 'z') ||
		      (state->c >= '0' && state->c <= '9') || (state->c == '_') ||
		      (state->c == '$'))) {
			break;
		}
		buffer[length] = state->c;
		if (!consumeLexableChar(state)) {
			return false;
		}
		length++;

		// identifier to long
		if (length == 256) {
			success = false;
			break;
		}
	}
	if (!success) {
		return false;
	}
	buffer[length] = 0;
	if (strcmp("if", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_IF);
	} else if (strcmp("else", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_ELSE);
	} else if (strcmp("while", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_WHILE);
	} else if (strcmp("for", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_FOR);
	} else if (strcmp("do", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_DO);
	} else if (strcmp("switch", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_SWITCH);
	} else if (strcmp("case", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_CASE);
	} else if (strcmp("break", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_BREAK);
	} else if (strcmp("struct", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_STRUCT);
	} else if (strcmp("enum", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_ENUM);
	} else if (strcmp("union", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_UNION);
	} else if (strcmp("typedef", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_TYPEDEF);
	} else if (strcmp("void", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_VOID);
	} else if (strcmp("char", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_CHAR);
	} else if (strcmp("short", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_SHORT);
	} else if (strcmp("int", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_INT);
	} else if (strcmp("float", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_FLOAT);
	} else if (strcmp("double", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_DOUBLE);
	} else if (strcmp("double", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_DOUBLE);
	} else if (strcmp("signed", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_SIGNED);
	} else if (strcmp("unsigned", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_UNSIGNED);
	} else if (strcmp("unsigned", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_UNSIGNED);
	} else if (strcmp("static", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_STATIC);
	} else if (strcmp("extern", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_EXTERN);
	} else if (strcmp("const", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_CONST);
	} else if (strcmp("inline", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_INLINE);
	} else if (strcmp("return", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_RETURN);
	} else if (strcmp("goto", buffer) == 0) {
		createSimpleToken(token, ctx, KEYWORD_GOTO);
	} else {
		int string = addString(&state->identifiers, buffer, length);
		createIdentifierToken(token, ctx, string);
	}
	return true;
}

static bool lexNumber(struct LexerState* state, struct LexerToken* token,
                      const struct FileContext* ctx)
{
	// success=consumeLexableChar(state);
	return false;
}
static bool lexFractionalNumber(struct LexerState* state,
                                struct LexerToken* token,
                                const struct FileContext* ctx)
{
	// success=consumeLexableChar(state);
	return false;
}

static bool lexHexNumber(struct LexerState* state, struct LexerToken* token,
                         const struct FileContext* ctx)
{
	// success=consumeLexableChar(state);
	return false;
}

bool getNextToken(struct LexerState* state, struct LexerToken* token)
{
	bool success = true;
	bool again = true;
	while (again) {
		again = false;
		struct FileContext ctx;
		skipWhiteSpaces(state);
		getFileContext(state, &ctx);
		if (state->c == INPUT_EOF) {
			getFileContext(state, &ctx);
			createSimpleToken(token, &ctx, TOKEN_EOF);
		} else {
			if (state->c == '/') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '/') {
					// Single Line Comment
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					skipSingleLineComment(state);
					again = true;
				} else if (state->c == '*') {
					// Multi Line Comment
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					success = skipMultiLineComment(state);
					if (success) {
						again = true;
					}
				} else if (state->c == '=') {
					// Divison Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_DIV_ASSIGNMENT);
				} else {
					//  Division Operator
					createSimpleToken(token, &ctx, OPERATOR_DIV);
				}
			} else if (state->c == '*') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Multiplication Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_MUL_ASSIGNMENT);
				} else {
					// ASTERISC
					createSimpleToken(token, &ctx, ASTERISC);
				}
			} else if (state->c == '%') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Modulo Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_MODULO_ASSIGNMENT);
				} else {
					// Modulo Operator
					createSimpleToken(token, &ctx, OPERATOR_MODULO);
				}
			} else if (state->c == '+') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Plus Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_PLUS_ASSIGNMENT);
				} else if (state->c == '+') {
					// PlusPlus Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_PLUSPLUS);
				} else {
					// Plus Operator
					createSimpleToken(token, &ctx, OPERATOR_PLUS);
				}
			} else if (state->c == '-') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Minus Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_MINUS_ASSIGNMENT);
				} else if (state->c == '-') {
					// MinusMinus Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_MINUSMINUS);
				} else if (state->c == '>') {
					// Dereference Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_DEREFERENCE);
				} else {
					// Plus Operator
					createSimpleToken(token, &ctx, OPERATOR_MINUS);
				}
			} else if (state->c == '&') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// And Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_AND_ASSIGNMENT);
				} else if (state->c == '&') {
					// Logical And Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_LOGICAL_AND);
				} else {
					// And Operator
					createSimpleToken(token, &ctx, OPERATOR_AND);
				}
			} else if (state->c == '|') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Or Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_OR_ASSIGNMENT);
				} else if (state->c == '&') {
					// Logical And Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_LOGICAL_OR);
				} else {
					// Plus Operator
					createSimpleToken(token, &ctx, OPERATOR_OR);
				}
			} else if (state->c == '^') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Xor Assignment Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_XOR_ASSIGNMENT);
				} else {
					// Plus Operator
					createSimpleToken(token, &ctx, OPERATOR_XOR);
				}
			} else if (state->c == '~') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				// Negate Operator
				createSimpleToken(token, &ctx, OPERATOR_NEGATE);
			} else if (state->c == '!') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Not Equal Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_NOT_EQUAL);
				} else {
					// Logical Not Operator
					createSimpleToken(token, &ctx, OPERATOR_LOGICAL_NOT);
				}
			} else if (state->c == '<') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '<') {
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					if (state->c == '=') {
						// Shift left Assignment Operator
						if (!consumeLexableChar(state)) {
							success = false;
							break;
						}
						createSimpleToken(token, &ctx,
						                  OPERATOR_SHIFT_LEFT_ASSIGNMENT);
					} else {
						// Shift left Operator
						createSimpleToken(token, &ctx, OPERATOR_SHIFT_LEFT);
					}
				} else if (state->c == '=') {
					// Less or equal Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_LESS_OR_EQUAL);
				} else {
					// Less than Operator
					createSimpleToken(token, &ctx, OPERATOR_LESS);
				}
			} else if (state->c == '>') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '>') {
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					if (state->c == '=') {
						// Shift right Assignment Operator
						if (!consumeLexableChar(state)) {
							success = false;
							break;
						}
						createSimpleToken(token, &ctx,
						                  OPERATOR_SHIFT_RIGHT_ASSIGNMENT);
					} else {
						// Shift right Operator
						createSimpleToken(token, &ctx, OPERATOR_SHIFT_RIGHT);
					}
				} else if (state->c == '=') {
					// Greater or equal Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_GREATER_OR_EQUAL);
				} else {
					// Greater than Operator
					createSimpleToken(token, &ctx, OPERATOR_GREATER);
				}
			} else if (state->c == '=') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == '=') {
					// Equal Comparsion Operator
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					createSimpleToken(token, &ctx, OPERATOR_EQUAL);
				} else {
					// Assignment Operator
					createSimpleToken(token, &ctx, OPERATOR_ASSIGNMENT);
				}
			} else if (state->c == '?') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, OPERATOR_CONDITIONAL);
			} else if (state->c == ':') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, COLON);
			} else if (state->c == ';') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, SEMICOLON);
			} else if (state->c == ',') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, COMMA);
			} else if (state->c == '(') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, PARENTHESE_LEFT);
			} else if (state->c == ')') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, PARENTHESE_RIGHT);
			} else if (state->c == '[') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, BRACKET_LEFT);
			} else if (state->c == ']') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, BRACKET_RIGHT);
			} else if (state->c == '{') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, BRACE_LEFT);
			} else if (state->c == '}') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				createSimpleToken(token, &ctx, BRACE_RIGHT);
			} else if (state->c == '.') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c >= '0' && state->c <= '9') {
					lexFractionalNumber(state, token, &ctx);
				} else {
					createSimpleToken(token, &ctx, OPERATOR_POINT);
				}
			} else if ((state->c >= 'A' && state->c <= 'Z') ||
			           (state->c >= 'a' && state->c <= 'z') ||
			           (state->c == '_')) {
				// Keyword or Identifier
				success = lexWord(state, token, &ctx);
			} else if (state->c == '0') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == 'x') {
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
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
				createSimpleToken(token, &ctx, TOKEN_UNKNOWN);
			}
		}
	}
	return success;
}

void printToken(struct LexerState* state, struct LexerToken* token)
{
	if (token->type == IDENTIFIER) {
		int index = token->value.string_index;
		printf("line:%d, column: %d, type: <IDENTIFIER>, id:%d, name: %s\n",
		       token->line + 1, token->column + 1, index,
		       getStringAt(&state->identifiers, index));
	} else {
		printf("line:%d, column: %d, type: <%s>\n", token->line + 1,
		       token->column + 1, tokenNames[token->type]);
	}
}
