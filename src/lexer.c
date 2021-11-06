#include "lexer.h"

#include <stdbool.h>
#include <string.h>

#include "helper.h"
#include "keyword_hashes.h"

struct FileContext {
	int line;
	int column;
};

static bool consumeLexableChar(struct LexerState* state);
static bool skipBackslashNewline(struct LexerState* state);

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
                                     uint64_t number, bool is_unsigned)
{
	token->line = ctx->line;
	token->column = ctx->column;
	if (is_unsigned) {
		token->type = LITERAL_UNSIGNED_INT;
	} else {
		token->type = LITERAL_INT;
	}
	token->value.int_literal = number;
	return 0;
}

static int createFloatingpointLiteralToken(struct LexerToken* token,
                                           const struct FileContext* ctx,
                                           double number, bool is_float)
{
	token->line = ctx->line;
	token->column = ctx->column;
	if (is_float) {
		token->type = LITERAL_FLOAT;
		token->value.float_literal = (float)number;
	} else {
		token->type = LITERAL_DOUBLE;
		token->value.double_literal = number;
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

static int createStringLiteralToken(struct LexerToken* token,
                                    const struct FileContext* ctx,
                                    uint16_t index)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = LITERAL_STRING;
	token->value.string_index = index;
	return 0;
}

static bool matchKeyword(const char* buffer, uint32_t buffer_hash,
                         const struct FileContext* ctx,
                         struct LexerToken* token)
{
	switch (buffer_hash) {
		case KEYWORD_HASH_AUTO:
			if (strcmp("auto", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_AUTO);
				return true;
			}
			break;
		case KEYWORD_HASH_BREAK:
			if (strcmp("break", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_BREAK);
				return true;
			}
			break;
		case KEYWORD_HASH_CASE:
			if (strcmp("case", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_CASE);
				return true;
			}
			break;
		case KEYWORD_HASH_CHAR:
			if (strcmp("char", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_CHAR);
				return true;
			}
			break;
		case KEYWORD_HASH_CONST:
			if (strcmp("const", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_CONST);
				return true;
			}
			break;
		case KEYWORD_HASH_CONTINUE:
			if (strcmp("continue", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_CONTINUE);
				return true;
			}
			break;
		case KEYWORD_HASH_DEFAULT:
			if (strcmp("default", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_DEFAULT);
				return true;
			}
			break;
		case KEYWORD_HASH_DO:
			if (strcmp("do", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_DO);
				return true;
			}
			break;
		case KEYWORD_HASH_DOUBLE:
			if (strcmp("double", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_DOUBLE);
				return true;
			}
		case KEYWORD_HASH_ELSE:
			if (strcmp("else", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_ELSE);
				return true;
			}
			break;
		case KEYWORD_HASH_ENUM:
			if (strcmp("enum", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_ENUM);
				return true;
			}
			break;
		case KEYWORD_HASH_EXTERN:
			if (strcmp("extern", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_EXTERN);
				return true;
			}
			break;
		case KEYWORD_HASH_FLOAT:
			if (strcmp("float", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_FLOAT);
				return true;
			}
			break;
		case KEYWORD_HASH_FOR:
			if (strcmp("for", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_FOR);
				return true;
			}
			break;
		case KEYWORD_HASH_GOTO:
			if (strcmp("goto", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_GOTO);
				return true;
			}
			break;
		case KEYWORD_HASH_IF:
			if (strcmp("if", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_IF);
				return true;
			}
			break;
		case KEYWORD_HASH_INLINE:
			if (strcmp("inline", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_INLINE);
				return true;
			}
			break;
		case KEYWORD_HASH_INT:
			if (strcmp("int", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_INT);
				return true;
			}
			break;
		case KEYWORD_HASH_LONG:
			if (strcmp("long", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_LONG);
				return true;
			}
			break;
		case KEYWORD_HASH_REGISTER:
			if (strcmp("register", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_REGISTER);
				return true;
			}
			break;
		case KEYWORD_HASH_RESTRICT:
			if (strcmp("restrict", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_RESTRICT);
				return true;
			}
			break;
		case KEYWORD_HASH_RETURN:
			if (strcmp("return", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_RETURN);
				return true;
			}
			break;
		case KEYWORD_HASH_SHORT:
			if (strcmp("short", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_SHORT);
				return true;
			}
			break;
		case KEYWORD_HASH_SIGNED:
			if (strcmp("signed", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_SIGNED);
				return true;
			}
			break;
		case KEYWORD_HASH_SIZEOF:
			if (strcmp("sizeof", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_SIZEOF);
				return true;
			}
			break;
		case KEYWORD_HASH_STATIC:
			if (strcmp("static", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_STATIC);
				return true;
			}
			break;
		case KEYWORD_HASH_STRUCT:
			if (strcmp("struct", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_STRUCT);
				return true;
			}
			break;
		case KEYWORD_HASH_SWITCH:
			if (strcmp("switch", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_SWITCH);
				return true;
			}
			break;
		case KEYWORD_HASH_TYPEDEF:
			if (strcmp("typedef", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_TYPEDEF);
				return true;
			}
			break;
		case KEYWORD_HASH_UNION:
			if (strcmp("union", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_UNION);
				return true;
			}
			break;
		case KEYWORD_HASH_UNSIGNED:
			if (strcmp("unsigned", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_UNSIGNED);
				return true;
			}
			break;
		case KEYWORD_HASH_VOID:
			if (strcmp("void", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_VOID);
				return true;
			}
			break;
		case KEYWORD_HASH_VOLATILE:
			if (strcmp("volatile", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_VOLATILE);
				return true;
			}
			break;
		case KEYWORD_HASH_WHILE:
			if (strcmp("while", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_WHILE);
				return true;
			}
			break;
		case KEYWORD_HASH_ALIGNAS:
			if (strcmp("_Alignas", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_ALIGNAS);
				return true;
			}
			break;
		case KEYWORD_HASH_ALIGNOF:
			if (strcmp("_Alignof", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_ALIGNOF);
				return true;
			}
			break;
		case KEYWORD_HASH_BOOL:
			if (strcmp("_Bool", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_BOOL);
				return true;
			}
			break;
		case KEYWORD_HASH_COMPLEX:
			if (strcmp("_Complex", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_COMPLEX);
				return true;
			}
			break;
		case KEYWORD_HASH_GENERIC:
			if (strcmp("_Generic", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_GENERIC);
				return true;
			}
			break;
		case KEYWORD_HASH_IMAGINARY:
			if (strcmp("_Imaginary", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_IMAGINARY);
				return true;
			}
			break;
		case KEYWORD_HASH_NORETURN:
			if (strcmp("_Noreturn", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_NORETURN);
				return true;
			}
			break;
		case KEYWORD_HASH_STATIC_ASSERT:
			if (strcmp("_Static_assert", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_STATIC_ASSERT);
				return true;
			}
			break;
		case KEYWORD_HASH_CONSTEXPR:
			if (strcmp("__constexpr", buffer) == 0) {
				createSimpleToken(token, ctx, KEYWORD_CONSTEXPR);
				return true;
			}
			break;
		default:
			break;
	}
	return false;
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
		fprintf(stderr, "Could not open file\n");
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
			consumeInput(state);
			skipBackslashNewline(state);
			break;
		}
		consumeInput(state);
	}
}
static bool lexEscapeSequence(int* c, struct LexerState* state,
                              struct FileContext* ctx)
{
	bool success = true;
	if (isOctalDigit(state->c)) {
		*c = 0;
		int i = 0;
		while (isOctalDigit(state->c)) {
			if (i >= 3) {
				success = false;
				break;
			}
			*c <<= 3;
			*c += state->c - '0';
			i++;
			state->column++;
			consumeInput(state);
		}
	} else {
		switch (state->c) {
			case '\'':
				*c = '\'';
				break;
			case '\"':
				*c = '\"';
				break;
			case '?':
				*c = 0x3f;
				break;
			case '\\':
				*c = '\\';
				break;
			case 'a':
				*c = 0x7;
				break;
			case 'b':
				*c = 0x8;
				break;
			case 'f':
				*c = 0xc;
				break;
			case 'n':
				*c = 0xa;
				break;
			case 'r':
				*c = 0xd;
				break;
			case 't':
				*c = 0x9;
				break;
			case 'v':
				*c = 0xb;
				break;
			case 'x':
				state->column++;
				consumeInput(state);
				*c = 0;
				int i = 0;
				while (isHexDigit(state->c)) {
					*c <<= 4;
					if (isDecimalDigit(state->c)) {
						*c += state->c - '0';
					} else {
						*c += (state->c & 0xdf) - 'A' + 10;
					}
					i++;
					state->column++;
					consumeInput(state);
				}
				break;
		}
		if (*c > 255) {
			success = false;
		}
	}
	return success;
}

static bool lexStringLiteral(struct LexerState* state, struct LexerToken* token,
                             struct FileContext* ctx)
{
	char buffer[1024];
	int length = 0;
	while (state->c != '"') {
		if (state->c == '\n') {
			return false;
		}
		if (state->c == '\\') {
			state->column++;
			consumeInput(state);
			int c;
			if (!lexEscapeSequence(&c, state, ctx)) {
				return false;
			}
			buffer[length] = (char)c;
		} else {
			buffer[length] = state->c;
			state->column++;
			consumeInput(state);
		}
		length++;
	}
	if (!consumeLexableChar(state)) {
		return false;
	}
	int index = addString(&state->string_literals, buffer, length);
	if (index == -1) {
		return false;
	}
	createStringLiteralToken(token, ctx, index);
	return true;
}

/*static bool lexCharacterLiteral(struct LexerState* state,
                                struct LexerToken* token,
                                struct FileContext* ctx)
{
    char buffer[1024];
    int length = 0;
    while (state->c != '\'') {
        if (state->c == '\n') {
            return false;
        }
        if (state->c == '\\') {
            int c;
            state->column++;
            consumeInput(state);
            if (!lexEscapeSequence(&c, state, ctx)) {
                return false;
            }
            buffer[length] = (char)c;
        } else {
            buffer[length] = state->c;
            consumeInput(state);
        }
        length++;
    }
    if (!consumeLexableChar(state)) {
        return false;
    }
    int index = addString(&state->string_literals, buffer, length);
    if (index == -1) {
        return false;
    }
    createStringLiteralToken(token, ctx, index);
    return true;
}*/

static bool lexWord(struct LexerState* state, struct LexerToken* token,
                    const struct FileContext* ctx)
{
	char buffer[256];
	int length = 0;
	bool success = true;
	while (state->c != INPUT_EOF) {
		if (!isAlphaNumeric(state->c)) {
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
	uint32_t hash = hashString(buffer);
	if (!matchKeyword(buffer, hash, ctx, token)) {
		int string =
		    addStringAndHash(&state->identifiers, buffer, length, hash);
		createIdentifierToken(token, ctx, string);
	}
	return true;
}
static const double exponent_lookup[] = {1e1,  1e2,  1e4, 1e8,
                                         1e16, 1e32, 1e64

};
static double exponential(int exponent)
{
	double factor = 1.0;
	bool negative = exponent < 0 ? true : false;
	int exp = negative ? -exponent : exponent;

	for (int i = 0; i < 7; i++) {
		if (exp & 0x1) {
			factor *= exponent_lookup[i];
		}
		exp = exp >> 1;
	}
	if (negative) {
		factor = 1.0 / factor;
	}
	return factor;
}

static bool lexExponent(struct LexerState* state, int* exponent)
{
	int sign = 1;
	if (state->c == '-') {
		if (!consumeLexableChar(state)) {
			return false;
		}
		sign = -1;
	} else if (state->c == '+') {
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	if (!isDecimalDigit(state->c)) {
		return false;
	}
	int integer = (int)(state->c - '0');
	if (!consumeLexableChar(state)) {
		return false;
	}
	while (isDecimalDigit(state->c)) {
		integer *= 10;
		integer += (int)(state->c - '0');
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	*exponent = integer * sign;
	return true;
}

static bool lexFractionalNumber(struct LexerState* state,
                                struct LexerToken* token,
                                const struct FileContext* ctx,
                                bool has_integer_part, double start)
{
	double num = 0.0;
	double factor = 1.0;
	int length = 0;
	while (isDecimalDigit(state->c)) {
		num += (double)(state->c - '0');
		num /= 10.0;
		if (!consumeLexableChar(state)) {
			return false;
		}
		length++;
	}
	if ((!has_integer_part) && (length == 0)) {
		return false;
	}

	if ((state->c == 'e') || (state->c == 'E')) {
		if (length == 0) {
			return false;
		}
		// exponent
		if (!consumeLexableChar(state)) {
			return false;
		}
		int exponent;
		if (!lexExponent(state, &exponent)) {
			return -1;
		}
		factor = exponential(exponent);
	}
	bool is_float = false;
	if (state->c == 'f') {
		// float
		if (!consumeLexableChar(state)) {
			return false;
		}
		is_float = true;
	}
	double floatingpoint = (start + num) * factor;
	createFloatingpointLiteralToken(token, ctx, floatingpoint, is_float);
	return true;
}

static bool lexIntegerSuffix(struct LexerState* state, struct LexerToken* token,
                             const struct FileContext* ctx, uint64_t integer)
{
	// integer
	bool is_unsigned = false;
	if ((state->c == 'u') || (state->c == 'U')) {
		// unsigned
		if (!consumeLexableChar(state)) {
			return false;
		}
		is_unsigned = true;

		if (state->c == 'l') {
			// long
			if (!consumeLexableChar(state)) {
				return false;
			}
			if (state->c == 'l') {
				// long long
				if (!consumeLexableChar(state)) {
					return false;
				}
			}
		} else if (state->c == 'L') {
			// long
			if (!consumeLexableChar(state)) {
				return false;
			}
			if (state->c == 'L') {
				// long long
				if (!consumeLexableChar(state)) {
					return false;
				}
			}
		}
	}
	if (state->c == 'l') {
		// long
		if (!consumeLexableChar(state)) {
			return false;
		}
		if (state->c == 'l') {
			// long long
			if (!consumeLexableChar(state)) {
				return false;
			}
		}
		if ((state->c == 'u') || (state->c == 'U')) {
			// unsigned
			if (!consumeLexableChar(state)) {
				return false;
			}
			is_unsigned = true;
		}
	} else if (state->c == 'L') {
		// long
		if (!consumeLexableChar(state)) {
			return false;
		}
		if (state->c == 'L') {
			// long long
			if (!consumeLexableChar(state)) {
				return false;
			}
		}
		if ((state->c == 'u') || (state->c == 'U')) {
			// unsigned
			if (!consumeLexableChar(state)) {
				return false;
			}
			is_unsigned = true;
		}
	}
	createIntegerLiteralToken(token, ctx, integer, is_unsigned);
	return true;
}

static bool lexNumber(struct LexerState* state, struct LexerToken* token,
                      const struct FileContext* ctx)
{
	uint64_t integer = 0;
	while (isDecimalDigit(state->c)) {
		integer *= 10;
		integer += (uint64_t)(state->c - '0');
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	if (state->c == '.') {
		// floating point
		if (!consumeLexableChar(state)) {
			return false;
		}
		if (!lexFractionalNumber(state, token, ctx, true, (double)integer)) {
			return false;
		}
	} else if ((state->c == 'e') || (state->c == 'E')) {
		// floating point exponent
		if (!consumeLexableChar(state)) {
			return false;
		}
		int exponent;
		if (!lexExponent(state, &exponent)) {
			return false;
		}
		double factor = exponential(exponent);
		bool is_float = false;
		if (state->c == 'f') {
			// float
			if (!consumeLexableChar(state)) {
				return false;
			}
			is_float = true;
		}
		double floatingpoint = integer * factor;
		createFloatingpointLiteralToken(token, ctx, floatingpoint, is_float);
	} else {
		if (!lexIntegerSuffix(state, token, ctx, integer)) {
			return false;
		}
	}
	return true;
}
static bool lexHexNumber(struct LexerState* state, struct LexerToken* token,
                         const struct FileContext* ctx)
{
	uint64_t number = 0;
	while (isHexDigit(state->c)) {
		number <<= 4;
		if (isDecimalDigit(state->c)) {
			number += state->c - '0';
		} else {
			number += (state->c & 0xdf) - 'A' + 10;
		}
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	if (!lexIntegerSuffix(state, token, ctx, number)) {
		return false;
	}
	return true;
}

static bool lexOctalNumber(struct LexerState* state, struct LexerToken* token,
                           const struct FileContext* ctx)
{
	uint64_t number = 0;
	while (isOctalDigit(state->c)) {
		number <<= 3;
		number += state->c - '0';
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	if (!lexIntegerSuffix(state, token, ctx, number)) {
		return false;
	}
	return true;
}
static bool lexBinaryNumber(struct LexerState* state, struct LexerToken* token,
                            const struct FileContext* ctx)
{
	uint64_t number = 0;
	while (isBinaryDigit(state->c)) {
		number <<= 1;
		number += state->c - '0';
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	if (!lexIntegerSuffix(state, token, ctx, number)) {
		return false;
	}
	return true;
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
			} else if (state->c == '"') {
				consumeInput(state);
				success = lexStringLiteral(state, token, &ctx);
			} /* else if (state->c == '\'') {
			     consumeInput(state);
			     success = lexCharacterLiteral(state, token, &ctx);

			 }*/
			else if (state->c == '.') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (isDecimalDigit(state->c)) {
					if (!lexFractionalNumber(state, token, &ctx, false, 0)) {
						success = false;
						break;
					}
				} else {
					createSimpleToken(token, &ctx, OPERATOR_POINT);
				}
			} else if (isAlphabetic(state->c)) {
				// Keyword or Identifier
				success = lexWord(state, token, &ctx);
			} else if (state->c == '0') {
				if (!consumeLexableChar(state)) {
					success = false;
					break;
				}
				if (state->c == 'x' || state->c == 'X') {
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					if (isHexDigit(state->c)) {
						success = lexHexNumber(state, token, &ctx);
					} else {
						// not a valid hex number
						success = false;
					}
				} else if (state->c == 'b') {
					if (!consumeLexableChar(state)) {
						success = false;
						break;
					}
					if (isBinaryDigit(state->c)) {
						success = lexBinaryNumber(state, token, &ctx);
					} else {
						success = false;
					}

				} else if (state->c >= '1' && state->c <= '7') {
					success = lexOctalNumber(state, token, &ctx);
				} else {
					createIntegerLiteralToken(token, &ctx, 0, false);
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