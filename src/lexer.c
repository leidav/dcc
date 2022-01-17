#include "lexer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "helper.h"
#include "keyword_hashes.h"

#define READ_BUFFER_SIZE 2048
static char read_buffer[READ_BUFFER_SIZE];
struct FileContext {
	int line;
	int column;
	int line_pos;
};
struct StringIterator {
	const char* start;
	const char* cur;
};

static void initStringIterator(struct StringIterator* it, const char* string)
{
	it->start = string;
	it->cur = string;
}
static char next(struct StringIterator* it)
{
	return *(++it->cur);
}

static bool consumeLexableChar(struct LexerState* state);
static bool skipBackslashNewline(struct LexerState* state);

static void getFileContext(struct LexerState* state, struct FileContext* ctx)
{
	ctx->line = state->current_pos.line;
	ctx->column = state->current_pos.column;
	ctx->line_pos = state->current_pos.line_pos;
}
static void setFileContext(struct LexerState* state,
                           const struct FileContext* ctx)
{
	state->current_pos.line = ctx->line;
	state->current_pos.column = ctx->column;
	state->current_pos.line_pos = ctx->line_pos;
}

static int createSimpleToken(struct LexerToken* token,
                             const struct FileContext* ctx, enum TokenType type)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = type;
	token->line_pos = ctx->line_pos;
	return 0;
}

static int createIntegerConstantToken(struct LexerToken* token,
                                      const struct FileContext* ctx,
                                      uint64_t number, bool is_unsigned)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	if (is_unsigned) {
		token->type = CONSTANT_UNSIGNED_INT;
	} else {
		token->type = CONSTANT_INT;
	}
	token->value.int_literal = number;
	return 0;
}

static int createFloatingpointConstantToken(struct LexerToken* token,
                                            const struct FileContext* ctx,
                                            double number, bool is_float)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	if (is_float) {
		token->type = CONSTANT_FLOAT;
		token->value.float_literal = (float)number;
	} else {
		token->type = CONSTANT_DOUBLE;
		token->value.double_literal = number;
	}
	return 0;
}

static int createIdentifierToken(struct LexerToken* token,
                                 const struct FileContext* ctx, uint16_t index)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	token->type = IDENTIFIER;
	token->value.string_index = index;
	return 0;
}

static int createStringConstantToken(struct LexerToken* token,
                                     const struct FileContext* ctx,
                                     uint16_t index)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	token->type = LITERAL_STRING;
	token->value.string_index = index;
	return 0;
}
static int createCharacterConstantToken(struct LexerToken* token,
                                        const struct FileContext* ctx,
                                        int character)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	token->type = CONSTANT_CHAR;
	token->value.character_literal = character;
	return 0;
}

static int createPPNumberToken(struct LexerToken* token,
                               const struct FileContext* ctx, uint16_t index)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	token->type = PP_NUMBER;
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

static void readInputAndHandleLineEndings(struct LexerState* state)
{
	// support unix, dos and legacy mac text files
	char next = readChar(&state->current_file);
	state->lookahead_pos.file_pos++;
	switch (next) {
		case '\r':
			state->lookahead_pos.line_pos = state->lookahead_pos.file_pos;
			state->carriage_return = true;
			next = '\n';
			break;
		case '\n':
			state->lookahead_pos.line_pos = state->lookahead_pos.file_pos;
			if (state->carriage_return) {
				next = readChar(&state->current_file);
				state->lookahead_pos.file_pos++;
				if (next == '\r') {
					next = '\n';
				} else {
					state->carriage_return = false;
				}
			}
			break;
		default:
			state->carriage_return = false;
	}
	state->lookahead = next;
}

static void consumeInput(struct LexerState* state)
{
	state->c = state->lookahead;
	state->current_pos = state->lookahead_pos;
	if (state->lookahead == '\n') {
		state->lookahead_pos.column = 0;
		state->lookahead_pos.line++;
	} else {
		state->lookahead_pos.column++;
	}
	readInputAndHandleLineEndings(state);
}
int initLexer(struct LexerState* state, const char* file_path)
{
	memset(&state->current_pos, 0, sizeof(state->current_pos));
	memset(&state->lookahead_pos, 0, sizeof(state->lookahead_pos));
	state->line_beginning = true;
	state->pp_num_definitions = 0;
	state->pp_num_tokens = 0;
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
	if (createStringSet(&state->pp_numbers, LEXER_PP_NUMBER_STRINGSET_SIZE,
	                    LEXER_MAX_PP_NUMBER_COUNT) != 0) {
		return -1;
	}
	state->pp_tokens =
	    malloc(sizeof(*state->pp_tokens) * LEXER_MAX_PP_NUMBER_COUNT);
	state->pp_definitions =
	    malloc(sizeof(*state->pp_definitions) * LEXER_MAX_DEFINITION_COUNT);
	readInputAndHandleLineEndings(state);
	consumeInput(state);
	state->carriage_return = false;
	return 0;
}
static bool skipIfWhiteSpace(struct LexerState* state)
{
	bool isWhitespace;
	if (state->c == '\n') {
		state->line_beginning = true;
		consumeInput(state);
		isWhitespace = true;
	} else if (state->c == ' ' || state->c == '\t') {
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
		consumeInput(state);
		while ((state->c == ' ') || (state->c == '\t')) {
			consumeInput(state);
		}
		if (state->c == '\n') {
			consumeInput(state);
		} else {
			success = false;
			break;
		}
	}
	return success;
}
static bool skipBackslashNewlineLookahead(struct LexerState* state)
{
	bool success = true;
	while (state->lookahead == '\\') {
		state->lookahead_pos.column++;
		readInputAndHandleLineEndings(state);
		while ((state->lookahead == ' ') || (state->lookahead == '\t')) {
			state->lookahead_pos.column++;
			readInputAndHandleLineEndings(state);
		}
		if (state->lookahead == '\n') {
			state->lookahead_pos.column = 0;
			state->lookahead_pos.line++;
			readInputAndHandleLineEndings(state);
		} else {
			success = false;
			break;
		}
	}
	return success;
}

static bool consumeLexableChar(struct LexerState* state)
{
	if (!skipBackslashNewlineLookahead(state)) {
		return false;
	}
	consumeInput(state);
	return true;
}

static bool skipMultiLineComment(struct LexerState* state)
{
	bool success = true;
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
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
			consumeInput(state);
			skipBackslashNewline(state);
			break;
		}
		consumeInput(state);
	}
}
static bool skipWhiteSpaceOrComments(struct LexerState* state)
{
	skipWhiteSpaces(state);
	while (state->c == '/') {
		skipBackslashNewlineLookahead(state);
		if (state->lookahead == '/') {
			consumeInput(state);
			skipSingleLineComment(state);
		} else if (state->lookahead == '*') {
			consumeInput(state);
			consumeInput(state);
			if (!skipMultiLineComment(state)) {
				return false;
			}
		} else {
			return true;
		}
		skipWhiteSpaces(state);
	}
	return true;
}
static bool lexEscapeSequence(int* c, struct LexerState* state)
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
				// non standard
			case 'e':
				*c = 0x1b;
				break;
			case 'x':
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
					consumeInput(state);
				}
				break;
		}
	}
	return success;
}
static int lexStringLiteralPiece(int offset, struct LexerState* state)
{
	int length = offset;
	while (state->c != '"') {
		if (length == READ_BUFFER_SIZE - 1 || state->c == INPUT_EOF ||
		    state->c == '\n') {
			return -1;
		} else if (state->c == '\\') {
			if (!skipBackslashNewline(state)) {
				int c;
				if (!lexEscapeSequence(&c, state)) {
					return -1;
				}
				if (c > 255) {
					return -1;
				}
				read_buffer[length] = (char)c;
				length++;
				consumeInput(state);
			}
		} else {
			read_buffer[length] = state->c;
			consumeInput(state);
			length++;
		}
	}
	if (!consumeLexableChar(state)) {
		return -1;
	}
	return length;
}
static bool lexStringLiteral(struct LexerState* state, struct LexerToken* token,
                             const struct FileContext* ctx)
{
	int length = 0;
	while (true) {
		length = lexStringLiteralPiece(length, state);
		if (length < 0) {
			return false;
		}
		if (!skipWhiteSpaceOrComments(state)) {
			return false;
		}
		if (state->c != '"') {
			break;
		}
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	read_buffer[length] = 0;
	int index = addString(&state->string_literals, read_buffer, length);
	if (index == -1) {
		return false;
	}
	createStringConstantToken(token, ctx, index);
	return true;
}

static bool lexCharacterLiteral(struct LexerState* state,
                                struct LexerToken* token,
                                const struct FileContext* ctx)
{
	int character = 0;
	while (state->c != '\'') {
		if (state->c == INPUT_EOF || state->c == '\n') {
			return false;
		} else if (state->c == '\\') {
			if (!skipBackslashNewline(state)) {
				int c;
				if (!lexEscapeSequence(&c, state)) {
					return -1;
				}
				character <<= 8;
				character |= c;
				consumeInput(state);
			}
		} else {
			character <<= 8;
			character |= state->c;
			consumeInput(state);
		}
	}
	if (!consumeLexableChar(state)) {
		return -1;
	}
	createCharacterConstantToken(token, ctx, character);
	return true;
}

static bool lexWord(struct LexerState* state, struct LexerToken* token,
                    const struct FileContext* ctx)
{
	int length = 0;
	bool success = true;
	while (state->c != INPUT_EOF) {
		if (!isAlphaNumeric(state->c)) {
			break;
		}
		read_buffer[length] = state->c;
		if (!consumeLexableChar(state)) {
			return false;
		}
		length++;

		// identifier to long
		if (length == READ_BUFFER_SIZE - 1) {
			success = false;
			break;
		}
	}
	if (!success) {
		return false;
	}
	read_buffer[length] = 0;
	uint32_t hash = hashString(read_buffer);
	if (!matchKeyword(read_buffer, hash, ctx, token)) {
		int string =
		    addStringAndHash(&state->identifiers, read_buffer, length, hash);
		createIdentifierToken(token, ctx, string);
	}
	return true;
}
static const double exponent_lookup[] = {1e1,  1e2,  1e4, 1e8,
                                         1e16, 1e32, 1e64

};
static double exponential(double value, int exponent)
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
	return negative ? value / factor : value * factor;
}

static bool parseExponent(struct StringIterator* it, int* exponent)
{
	int sign = 1;
	char c = *it->cur;
	if (c == '-') {
		c = next(it);
		sign = -1;
	} else if (c == '+') {
		c = next(it);
	}
	if (!isDecimalDigit(c)) {
		return false;
	}
	int integer = (int)(c - '0');
	c = next(it);
	while (isDecimalDigit(c)) {
		integer *= 10;
		integer += (int)(c - '0');
		c = next(it);
	}
	*exponent = integer * sign;
	return true;
}

static bool parseFractionalNumber(struct StringIterator* it,
                                  struct LexerToken* token,
                                  const struct FileContext* ctx,
                                  bool has_integer_part, uint64_t start)
{
	uint64_t num = start;
	int length = 0;
	char c = *it->cur;
	while (isDecimalDigit(c)) {
		num *= 10;
		num += c - '0';
		c = next(it);
		length++;
	}
	if ((!has_integer_part) && (length == 0)) {
		return false;
	}

	int exponent = 0;
	if ((c == 'e') || (c == 'E')) {
		if (length == 0) {
			return false;
		}
		// exponent
		it->cur++;
		if (!parseExponent(it, &exponent)) {
			return false;
		}
	}
	exponent -= length;
	bool is_float = false;
	c = *it->cur;
	if (c == 'f') {
		// float
		it->cur++;
		is_float = true;
	}
	double floatingpoint = exponential(num, exponent);
	createFloatingpointConstantToken(token, ctx, floatingpoint, is_float);
	return true;
}

static bool parseIntegerSuffix(struct StringIterator* it,
                               struct LexerToken* token,
                               const struct FileContext* ctx, uint64_t integer)
{
	// integer
	bool is_unsigned = false;
	char c = *it->cur;
	if ((c == 'u') || (c == 'U')) {
		// unsigned
		is_unsigned = true;
		c = next(it);
		if (c == 'l') {
			// long
			c = next(it);
			if (c == 'l') {
				// long long
				c = next(it);
			}
		} else if (c == 'L') {
			// long
			c = next(it);
			if (c == 'L') {
				// long long
				c = next(it);
			}
		}
	}
	if (c == 'l') {
		// long
		c = next(it);
		if (c == 'l') {
			// long long
			c = next(it);
		}
		if ((c == 'u') || (c == 'U')) {
			// unsigned
			it->cur++;
			is_unsigned = true;
		}
	} else if (c == 'L') {
		// long
		c = next(it);
		if (c == 'L') {
			// long long
			c = next(it);
		}
		if ((c == 'u') || (c == 'U')) {
			// unsigned
			it->cur++;
			is_unsigned = true;
		}
	}
	createIntegerConstantToken(token, ctx, integer, is_unsigned);
	return true;
}

static bool parseDecimalNumber(struct StringIterator* it,
                               struct LexerToken* token,
                               const struct FileContext* ctx)
{
	uint64_t integer = 0;
	char c = *it->cur;
	while (isDecimalDigit(c)) {
		integer *= 10;
		integer += (uint64_t)(c - '0');
		c = next(it);
	}
	if (c == '.') {
		// floating point
		it->cur++;
		if (!parseFractionalNumber(it, token, ctx, true, integer)) {
			return false;
		}
		c = *it->cur;
	} else if ((c == 'e') || (c == 'E')) {
		// floating point exponent
		it->cur++;
		int exponent;
		if (!parseExponent(it, &exponent)) {
			return false;
		}
		bool is_float = false;
		c = *it->cur;
		if (c == 'f') {
			// float
			it->cur++;
			is_float = true;
		}
		double floatingpoint = exponential(integer, exponent);
		createFloatingpointConstantToken(token, ctx, floatingpoint, is_float);
	} else {
		if (!parseIntegerSuffix(it, token, ctx, integer)) {
			return false;
		}
	}
	return true;
}
static bool parseHexNumber(struct StringIterator* it, struct LexerToken* token,
                           const struct FileContext* ctx)
{
	uint64_t number = 0;
	char c = *it->cur;
	while (isHexDigit(c)) {
		number <<= 4;
		if (isDecimalDigit(c)) {
			number += c - '0';
		} else {
			number += (c & 0xdf) - 'A' + 10;
		}
		c = next(it);
	}
	if (!parseIntegerSuffix(it, token, ctx, number)) {
		return false;
	}
	return true;
}

static bool parseOctalNumber(struct StringIterator* it,
                             struct LexerToken* token,
                             const struct FileContext* ctx)
{
	uint64_t number = 0;
	char c = *it->cur;
	while (isOctalDigit(c)) {
		number <<= 3;
		number += c - '0';
		c = next(it);
	}
	if (!parseIntegerSuffix(it, token, ctx, number)) {
		return false;
	}
	return true;
}
static bool parseBinaryNumber(struct StringIterator* it,
                              struct LexerToken* token,
                              const struct FileContext* ctx)
{
	uint64_t number = 0;
	char c = *it->cur;
	while (isBinaryDigit(c)) {
		number <<= 1;
		number += c - '0';
		c = next(it);
	}
	if (!parseIntegerSuffix(it, token, ctx, number)) {
		return false;
	}
	return true;
}
static bool parseNumber(struct StringIterator* it, struct LexerToken* token,
                        const struct FileContext* ctx)
{
	char c = *it->cur;
	switch (c) {
		case '.':
			c = next(it);
			if (isDecimalDigit(c)) {
				if (!parseFractionalNumber(it, token, ctx, false, 0)) {
					return false;
				}
			} else {
				return false;
			}
			break;
		case '0':
			c = next(it);
			if (c == 'x' || c == 'X') {
				c = next(it);
				if (isHexDigit(c)) {
					if (!parseHexNumber(it, token, ctx)) {
						return false;
					}
				} else {
					// not a valid hex number
					return false;
				}
			} else if (c == 'b') {
				c = next(it);
				if (isBinaryDigit(c)) {
					if (!parseBinaryNumber(it, token, ctx)) {
						return false;
					}
				} else {
					return false;
				}
			} else if (c >= '1' && c <= '7') {
				if (!parseOctalNumber(it, token, ctx)) {
					return LEXER_RESULT_FAIL;
				}
			} else {
				createIntegerConstantToken(token, ctx, 0, false);
			}
			break;
		default:
			if (!parseDecimalNumber(it, token, ctx)) {
				return false;
			};
			break;
	}
	if (*it->cur != 0) {
		return false;
	}
	return true;
}

static bool lexPPNumber(struct LexerState* state, struct LexerToken* token,
                        const struct FileContext* ctx,
                        bool create_number_constant)
{
	int length = 0;
	if (state->c == '.') {
		read_buffer[length] = state->c;
		length++;
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	while (isDecimalDigit(state->c)) {
		read_buffer[length] = state->c;
		length++;
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	while (isAlphaNumeric(state->c) || state->c == '.') {
		read_buffer[length] = state->c;
		length++;
		if ((state->c == 'e') || (state->c == 'E')) {
			if ((state->lookahead == '-') || (state->lookahead == '+')) {
				if (!consumeLexableChar(state)) {
					return false;
				}
				read_buffer[length] = state->c;
				length++;
			}
		}
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	read_buffer[length] = 0;
	if (create_number_constant) {
		struct StringIterator it;
		initStringIterator(&it, read_buffer);
		if (!parseNumber(&it, token, ctx)) {
			return false;
		}
	} else {
		int index = addString(&state->pp_numbers, read_buffer, length);
		if (index == -1) {
			return false;
		}
		createPPNumberToken(token, ctx, index);
	}
	return true;
}

int lexTokens(struct LexerState* state, struct LexerToken* token,
              const struct FileContext* ctx)
{
	switch (state->c) {
		case '/':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Divison Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_DIV_ASSIGNMENT);
			} else {
				//  Division Operator
				createSimpleToken(token, ctx, OPERATOR_DIV);
			}
			break;
		case '*':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Multiplication Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_MUL_ASSIGNMENT);
			} else {
				// ASTERISC
				createSimpleToken(token, ctx, ASTERISC);
			}
			break;

		case '%':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Modulo Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_MODULO_ASSIGNMENT);
			} else {
				// Modulo Operator
				createSimpleToken(token, ctx, OPERATOR_MODULO);
			}
			break;
		case '+':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Plus Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_PLUS_ASSIGNMENT);
			} else if (state->c == '+') {
				// PlusPlus Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_PLUSPLUS);
			} else {
				// Plus Operator
				createSimpleToken(token, ctx, OPERATOR_PLUS);
			}
			break;
		case '-':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Minus Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_MINUS_ASSIGNMENT);
			} else if (state->c == '-') {
				// MinusMinus Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_MINUSMINUS);
			} else if (state->c == '>') {
				// Dereference Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_DEREFERENCE);
			} else {
				// Minus Operator
				createSimpleToken(token, ctx, OPERATOR_MINUS);
			}
			break;
		case '&':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// And Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_AND_ASSIGNMENT);
			} else if (state->c == '&') {
				// Logical And Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_AND);
			} else {
				// And Operator
				createSimpleToken(token, ctx, OPERATOR_AND);
			}
			break;
		case '|':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Or Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_OR_ASSIGNMENT);
			} else if (state->c == '|') {
				// Logical Or Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_OR);
			} else {
				// Or Operator
				createSimpleToken(token, ctx, OPERATOR_OR);
			}
			break;
		case '^':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Xor Assignment Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_XOR_ASSIGNMENT);
			} else {
				// Xor Operator
				createSimpleToken(token, ctx, OPERATOR_XOR);
			}
			break;

		case '~':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			// Negate Operator
			createSimpleToken(token, ctx, OPERATOR_NEGATE);
			break;
		case '!':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Not Equal Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_NOT_EQUAL);
			} else {
				// Logical Not Operator
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_NOT);
			}
			break;
		case '<':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '<') {
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				if (state->c == '=') {
					// Shift left Assignment Operator
					if (!consumeLexableChar(state)) {
						return LEXER_RESULT_FAIL;
					}
					createSimpleToken(token, ctx,
					                  OPERATOR_SHIFT_LEFT_ASSIGNMENT);
				} else {
					// Shift left Operator
					createSimpleToken(token, ctx, OPERATOR_SHIFT_LEFT);
				}
			} else if (state->c == '=') {
				// Less or equal Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_LESS_OR_EQUAL);
			} else {
				// Less than Operator
				createSimpleToken(token, ctx, OPERATOR_LESS);
			}
			break;
		case '>':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '>') {
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				if (state->c == '=') {
					// Shift right Assignment Operator
					if (!consumeLexableChar(state)) {
						return LEXER_RESULT_FAIL;
					}
					createSimpleToken(token, ctx,
					                  OPERATOR_SHIFT_RIGHT_ASSIGNMENT);
				} else {
					// Shift right Operator
					createSimpleToken(token, ctx, OPERATOR_SHIFT_RIGHT);
				}
			} else if (state->c == '=') {
				// Greater or equal Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_GREATER_OR_EQUAL);
			} else {
				// Greater than Operator
				createSimpleToken(token, ctx, OPERATOR_GREATER);
			}
			break;
		case '=':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			if (state->c == '=') {
				// Equal Comparsion Operator
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_EQUAL);
			} else {
				// Assignment Operator
				createSimpleToken(token, ctx, OPERATOR_ASSIGNMENT);
			}
			break;
		case '?':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, OPERATOR_CONDITIONAL);
			break;
		case ':':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, COLON);
			break;
		case ';':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, SEMICOLON);
			break;
		case ',':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, COMMA);
		case '(':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, PARENTHESE_LEFT);
			break;
		case ')':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, PARENTHESE_RIGHT);
			break;
		case '[':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, BRACKET_LEFT);
			break;
		case ']':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, BRACKET_RIGHT);
			break;
		case '{':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, BRACE_LEFT);
			break;
		case '}':
			if (!consumeLexableChar(state)) {
				return LEXER_RESULT_FAIL;
			}
			createSimpleToken(token, ctx, BRACE_RIGHT);
			break;
		case '"':
			consumeInput(state);
			if (!lexStringLiteral(state, token, ctx)) {
				return LEXER_RESULT_FAIL;
			}
			break;
		case '\'':
			consumeInput(state);
			if (!lexCharacterLiteral(state, token, ctx)) {
				return LEXER_RESULT_FAIL;
			}
			break;
		case '.':
			if (isDecimalDigit(state->lookahead)) {
				if (!lexPPNumber(state, token, ctx, true)) {
					return LEXER_RESULT_FAIL;
				}
			} else {
				if (!consumeLexableChar(state)) {
					return LEXER_RESULT_FAIL;
				}
				createSimpleToken(token, ctx, OPERATOR_POINT);
			}
			break;
		default:
			if (isAlphabetic(state->c)) {
				// Keyword or Identifier
				if (!lexWord(state, token, ctx)) {
					return LEXER_RESULT_FAIL;
				}
			} else if (isDecimalDigit(state->c)) {
				// number
				if (!lexPPNumber(state, token, ctx, true)) {
					return LEXER_RESULT_FAIL;
				}
			} else {
				createSimpleToken(token, ctx, TOKEN_UNKNOWN);
				return LEXER_RESULT_FAIL;
			}
			break;
	}
	return LEXER_RESULT_SUCCESS;
}

bool handlePreprocessorDirective(struct LexerState* state)
{
	while (state->c != '\n') {
		// skip preprocessor lines
		if (!consumeLexableChar(state)) {
			return false;
		}
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
		skipWhiteSpaceOrComments(state);
		getFileContext(state, &ctx);
		if (state->c == INPUT_EOF) {
			getFileContext(state, &ctx);
			createSimpleToken(token, &ctx, TOKEN_EOF);
			break;
		} else if (state->c == '#') {
			// preprocessor
			if (!state->line_beginning) {
				success = false;
				break;
			}
			if (!consumeLexableChar(state)) {
				success = false;
				break;
			}
			if (!handlePreprocessorDirective(state)) {
				success = false;
				break;
			}
			again = true;
		} else {
			if (lexTokens(state, token, &ctx) != LEXER_RESULT_SUCCESS) {
				success = false;
				break;
			}
		}
	}
	state->line_beginning = false;
	return success;
}
