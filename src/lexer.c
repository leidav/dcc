#include "lexer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "error.h"
#include "helper.h"
#include "keyword_hashes.h"
#include "scratchpad.h"

#define MAX_STRING_LENGTH 2048
#define MAX_PP_NUMBER_LENGTH 1024
#define MAX_IDENTIFIER_LENGTH 256

#define NEXT(lexer_state, out_label)        \
	if (!consumeLexableChar(lexer_state)) { \
		goto out_label;                     \
	}

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
struct LexerToken createLexerTokenFromPPToken(
    struct LexerState* state, struct PreprocessorToken* pp_token)
{
	struct LexerToken token;
	token.type = pp_token->type;
	token.line = pp_token->line;
	token.column = pp_token->column;
	token.line_pos = pp_token->line_pos;
	if (pp_token->type == LITERAL_STRING) {
		token.value.string_index = pp_token->value_handle;
	} else if (pp_token->type >= CONSTANT_CHAR &&
	           pp_token->type <= CONSTANT_DOUBLE) {
		token.value = state->constants.constants[pp_token->value_handle];
	} else if (pp_token->type == PP_PARAM) {
		token.value.param_index = pp_token->value_handle;
	}
	return token;
}

static int createPPParamRefToken(struct LexerToken* token,
                                 const struct FileContext* ctx, int param)
{
	token->line = ctx->line;
	token->type = PP_PARAM;
	token->column = ctx->column;
	token->line_pos = ctx->line_pos;
	token->value.param_index = param;
	token->literal = false;
	return 0;
}

static int createSimpleToken(struct LexerToken* token,
                             const struct FileContext* ctx, enum TokenType type)
{
	token->line = ctx->line;
	token->column = ctx->column;
	token->type = type;
	token->line_pos = ctx->line_pos;
	token->literal = false;
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
	token->literal = true;
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
	token->literal = true;
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
	token->literal = false;
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
	token->literal = true;
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
	token->literal = true;
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
	token->literal = true;
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
	state->scratchpad = getScratchpadAllocator();
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
	if (createPreprocessorTokenSet(&state->pp_tokens,
	                               LEXER_MAX_DEFINITION_TOKEN_COUNT) != 0) {
		return -1;
	}
	if (createPreprocessorDefinitionSet(&state->pp_definitions,
	                                    LEXER_MAX_DEFINITION_COUNT,
	                                    LEXER_PP_NUMBER_STRINGSET_SIZE) != 0) {
		return -1;
	}
	state->constants.num = 0;
	state->constants.max_count = LEXER_MAX_PP_CONSTANT_COUNT;
	state->constants.constants = malloc(sizeof(*state->constants.constants) *
	                                    LEXER_MAX_PP_CONSTANT_COUNT);
	if (state->constants.constants == NULL) {
		return -1;
	}
	readInputAndHandleLineEndings(state);
	consumeInput(state);
	state->carriage_return = false;
	state->macro_body = false;
	state->error_handled = false;
	return 0;
}
static bool skipIfWhiteSpace(struct LexerState* state)
{
	bool isWhitespace;
	if (state->c == '\n') {
		state->line_beginning = true;
		if (state->macro_body) {
			state->c = INPUT_EOF;
		} else {
			consumeInput(state);
		}
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
	while (true) {
		while (state->c == ' ' || state->c == '\t') {
			consumeInput(state);
		}
		if (state->c == '\n') {
			state->line_beginning = true;
			if (state->macro_body) {
				state->c = INPUT_EOF;
				break;
			} else {
				consumeInput(state);
			}
		} else if (state->c == '\\') {
			if (!skipBackslashNewline(state)) {
				break;
			}
		} else {
			break;
		}
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
			lexerError(state, "Newline character expected");
			return false;
		}
	}
	return true;
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
	bool status = false;
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
			if (state->macro_body) {
				state->c = INPUT_EOF;
				break;
			} else {
				consumeInput(state);
			}
		} else if (state->c == '*') {
			NEXT(state, out);
			if (state->c == '/') {
				NEXT(state, out);
				break;
			}
		} else {
			consumeInput(state);
		}
	}
	if (state->c == INPUT_EOF) {
		lexerError(state, "New line expected but end of file reached instead");
		status = false;
	}
out:
	return status;
}

static void skipSingleLineComment(struct LexerState* state)
{
	while (state->c != INPUT_EOF) {
		if (state->c == '\n') {
			if (state->macro_body) {
				state->c = INPUT_EOF;
				break;
			} else {
				consumeInput(state);
				skipBackslashNewline(state);
			}
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
static int lexStringLiteralPiece(int offset, struct LexerState* state,
                                 char* read_buffer)
{
	int length = offset;
	while (state->c != '"') {
		if (length == MAX_STRING_LENGTH - 1 || state->c == INPUT_EOF ||
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
	size_t marker = markAllocatorState(state->scratchpad);
	char* read_buffer = allocate(state->scratchpad, MAX_STRING_LENGTH);
	if (!read_buffer) {
		return false;
	}
	bool status = false;
	while (true) {
		length = lexStringLiteralPiece(length, state, read_buffer);
		if (length < 0) {
			goto out;
		}
		if (!skipWhiteSpaceOrComments(state)) {
			goto out;
		}
		if (state->c != '"') {
			break;
		}
		NEXT(state, out);
	}
	read_buffer[length] = 0;
	int index = addString(&state->string_literals, read_buffer, length);
	if (index == -1) {
		generalError("Could not allocate string");
		goto out;
	}
	createStringConstantToken(token, ctx, index);
	status = true;
out:
	resetAllocatorState(state->scratchpad, marker);
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

static int readWord(struct LexerState* state, const struct FileContext* ctx,
                    char* read_buffer)
{
	int length = 0;
	while (state->c != INPUT_EOF) {
		if (!isAlphaNumeric(state->c)) {
			break;
		}
		read_buffer[length] = state->c;
		if (!consumeLexableChar(state)) {
			return -1;
		}
		length++;

		// identifier to long
		if (length == MAX_IDENTIFIER_LENGTH - 1) {
			return -1;
		}
	}
	read_buffer[length] = 0;
	return length;
}

static bool createKeywordOrIdentifierToken(struct LexerState* state,
                                           struct LexerToken* token,
                                           const struct FileContext* ctx,
                                           const char* string, int length)
{
	uint32_t hash = hashString(string);
	if (!matchKeyword(string, hash, ctx, token)) {
		int index =
		    addStringAndHash(&state->identifiers, string, length, hash, NULL);
		if (index < 0) {
			return false;
		}
		createIdentifierToken(token, ctx, index);
	}
	return true;
}

static bool lexWord(struct LexerState* state, struct LexerToken* token,
                    const struct FileContext* ctx)
{
	bool status = false;
	size_t marker = markAllocatorState(state->scratchpad);
	char* read_buffer = allocate(state->scratchpad, MAX_IDENTIFIER_LENGTH);
	if (!read_buffer) {
		goto out;
	}
	int length = readWord(state, ctx, read_buffer);
	if (length < 0) {
		goto out;
	}
	status =
	    createKeywordOrIdentifierToken(state, token, ctx, read_buffer, length);
out:
	resetAllocatorState(state->scratchpad, marker);
	return status;
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
	size_t marker = markAllocatorState(state->scratchpad);
	bool status = false;
	char* read_buffer = allocate(state->scratchpad, MAX_PP_NUMBER_LENGTH);
	if (!read_buffer) {
		generalError("buffer allocation failed");
		goto out;
	}
	int length = 0;
	if (state->c == '.') {
		read_buffer[length] = state->c;
		length++;
		NEXT(state, out);
	}
	while (isDecimalDigit(state->c)) {
		read_buffer[length] = state->c;
		length++;
		NEXT(state, out);
	}
	while (isAlphaNumeric(state->c) || state->c == '.') {
		read_buffer[length] = state->c;
		length++;
		if ((state->c == 'e') || (state->c == 'E')) {
			if ((state->lookahead == '-') || (state->lookahead == '+')) {
				NEXT(state, out);
				read_buffer[length] = state->c;
				length++;
			}
		}
		NEXT(state, out);
	}
	read_buffer[length] = 0;
	if (create_number_constant) {
		struct StringIterator it;
		initStringIterator(&it, read_buffer);
		if (!parseNumber(&it, token, ctx)) {
			lexerError(state, "Invalid number");
			goto out;
		}
	} else {
		int index = addString(&state->pp_numbers, read_buffer, length);
		if (index == -1) {
			generalError("Could not allocate the pp-number string");
			goto out;
		}
		createPPNumberToken(token, ctx, index);
	}
	status = true;
out:
	resetAllocatorState(state->scratchpad, marker);
	return status;
}

bool lexTokens(struct LexerState* state, struct LexerToken* token,
               const struct FileContext* ctx)
{
	bool status = false;
	switch (state->c) {
		case '/':
			NEXT(state, out);
			if (state->c == '=') {
				// Divison Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_DIV_ASSIGNMENT);
			} else {
				//  Division Operator
				createSimpleToken(token, ctx, OPERATOR_DIV);
			}
			break;
		case '*':
			NEXT(state, out);
			if (state->c == '=') {
				// Multiplication Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_MUL_ASSIGNMENT);
			} else {
				// ASTERISC
				createSimpleToken(token, ctx, ASTERISC);
			}
			break;

		case '%':
			NEXT(state, out);
			if (state->c == '=') {
				// Modulo Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_MODULO_ASSIGNMENT);
			} else {
				// Modulo Operator
				createSimpleToken(token, ctx, OPERATOR_MODULO);
			}
			break;
		case '+':
			NEXT(state, out);
			if (state->c == '=') {
				// Plus Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_PLUS_ASSIGNMENT);
			} else if (state->c == '+') {
				// PlusPlus Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_PLUSPLUS);
			} else {
				// Plus Operator
				createSimpleToken(token, ctx, OPERATOR_PLUS);
			}
			break;
		case '-':
			NEXT(state, out);
			if (state->c == '=') {
				// Minus Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_MINUS_ASSIGNMENT);
			} else if (state->c == '-') {
				// MinusMinus Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_MINUSMINUS);
			} else if (state->c == '>') {
				// Dereference Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_DEREFERENCE);
			} else {
				// Minus Operator
				createSimpleToken(token, ctx, OPERATOR_MINUS);
			}
			break;
		case '&':
			NEXT(state, out);
			if (state->c == '=') {
				// And Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_AND_ASSIGNMENT);
			} else if (state->c == '&') {
				// Logical And Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_AND);
			} else {
				// And Operator
				createSimpleToken(token, ctx, OPERATOR_AND);
			}
			break;
		case '|':
			NEXT(state, out);
			if (state->c == '=') {
				// Or Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_OR_ASSIGNMENT);
			} else if (state->c == '|') {
				// Logical Or Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_OR);
			} else {
				// Or Operator
				createSimpleToken(token, ctx, OPERATOR_OR);
			}
			break;
		case '^':
			NEXT(state, out);
			if (state->c == '=') {
				// Xor Assignment Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_XOR_ASSIGNMENT);
			} else {
				// Xor Operator
				createSimpleToken(token, ctx, OPERATOR_XOR);
			}
			break;

		case '~':
			NEXT(state, out);
			// Negate Operator
			createSimpleToken(token, ctx, OPERATOR_NEGATE);
			break;
		case '!':
			NEXT(state, out);
			if (state->c == '=') {
				// Not Equal Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_NOT_EQUAL);
			} else {
				// Logical Not Operator
				createSimpleToken(token, ctx, OPERATOR_LOGICAL_NOT);
			}
			break;
		case '<':
			NEXT(state, out);
			if (state->c == '<') {
				NEXT(state, out);
				if (state->c == '=') {
					// Shift left Assignment Operator
					NEXT(state, out);
					createSimpleToken(token, ctx,
					                  OPERATOR_SHIFT_LEFT_ASSIGNMENT);
				} else {
					// Shift left Operator
					createSimpleToken(token, ctx, OPERATOR_SHIFT_LEFT);
				}
			} else if (state->c == '=') {
				// Less or equal Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_LESS_OR_EQUAL);
			} else {
				// Less than Operator
				createSimpleToken(token, ctx, OPERATOR_LESS);
			}
			break;
		case '>':
			NEXT(state, out);
			if (state->c == '>') {
				NEXT(state, out);
				if (state->c == '=') {
					// Shift right Assignment Operator
					NEXT(state, out);
					createSimpleToken(token, ctx,
					                  OPERATOR_SHIFT_RIGHT_ASSIGNMENT);
				} else {
					// Shift right Operator
					createSimpleToken(token, ctx, OPERATOR_SHIFT_RIGHT);
				}
			} else if (state->c == '=') {
				// Greater or equal Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_GREATER_OR_EQUAL);
			} else {
				// Greater than Operator
				createSimpleToken(token, ctx, OPERATOR_GREATER);
			}
			break;
		case '=':
			NEXT(state, out);
			if (state->c == '=') {
				// Equal Comparsion Operator
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_EQUAL);
			} else {
				// Assignment Operator
				createSimpleToken(token, ctx, OPERATOR_ASSIGNMENT);
			}
			break;
		case '?':
			NEXT(state, out);
			createSimpleToken(token, ctx, OPERATOR_CONDITIONAL);
			break;
		case ':':
			NEXT(state, out);
			createSimpleToken(token, ctx, COLON);
			break;
		case ';':
			NEXT(state, out);
			createSimpleToken(token, ctx, SEMICOLON);
			break;
		case ',':
			NEXT(state, out);
			createSimpleToken(token, ctx, COMMA);
		case '(':
			NEXT(state, out);
			createSimpleToken(token, ctx, PARENTHESE_LEFT);
			break;
		case ')':
			NEXT(state, out);
			createSimpleToken(token, ctx, PARENTHESE_RIGHT);
			break;
		case '[':
			NEXT(state, out);
			createSimpleToken(token, ctx, BRACKET_LEFT);
			break;
		case ']':
			NEXT(state, out);
			createSimpleToken(token, ctx, BRACKET_RIGHT);
			break;
		case '{':
			NEXT(state, out);
			createSimpleToken(token, ctx, BRACE_LEFT);
			break;
		case '}':
			NEXT(state, out);
			createSimpleToken(token, ctx, BRACE_RIGHT);
			break;
		case '"':
			consumeInput(state);
			if (!lexStringLiteral(state, token, ctx)) {
				lexerError(state, "Invalid string literal");
				goto out;
			}
			break;
		case '\'':
			consumeInput(state);
			if (!lexCharacterLiteral(state, token, ctx)) {
				lexerError(state, "Invalid character literal");
				goto out;
			}
			break;
		case '.':
			if (isDecimalDigit(state->lookahead)) {
				if (!lexPPNumber(state, token, ctx, true)) {
					goto out;
				}
			} else {
				NEXT(state, out);
				createSimpleToken(token, ctx, OPERATOR_POINT);
			}
			break;
		default:
			if (isAlphabetic(state->c)) {
				// Keyword or Identifier
				if (!lexWord(state, token, ctx)) {
					goto out;
				}
			} else if (isDecimalDigit(state->c)) {
				// number
				if (!lexPPNumber(state, token, ctx, true)) {
					goto out;
				}
			} else {
				createSimpleToken(token, ctx, TOKEN_UNKNOWN);
				unexpectedCharacterError(state);
				goto out;
			}
			break;
	}
	status = true;
out:
	return status;
}

static bool lexMacroBody(struct LexerState* state, struct FileContext* ctx,
                         const char* macro_name, int macro_name_length,
                         bool function_like, struct StringSet* params)
{
	bool status = false;
	state->macro_body = true;
	skipWhiteSpaceOrComments(state);

	int start_index = state->pp_tokens.num;
	int num = 0;
	while (state->c != INPUT_EOF) {
		struct LexerToken token;
		struct FileContext macro_context;
		getFileContext(state, &macro_context);
		//  skip preprocessor lines
		if (state->c == '#') {
			// stringify or concatenation operator
			NEXT(state, out);
		} else if (function_like && isAlphabetic(state->c)) {
			size_t marker = markAllocatorState(state->scratchpad);
			char* read_buffer =
			    allocate(state->scratchpad, MAX_IDENTIFIER_LENGTH);
			if (read_buffer == NULL) {
				goto out;
			}
			int length = readWord(state, &macro_context, read_buffer);
			if (length == 0) {
				resetAllocatorState(state->scratchpad, marker);
				goto out;
			}
			uint32_t hash = hashSubstring(read_buffer, length);
			int index = findIndex(params, read_buffer, length, hash);
			if (index >= 0) {
				createPPParamRefToken(&token, &macro_context, index);
			} else {
				createKeywordOrIdentifierToken(state, &token, &macro_context,
				                               read_buffer, length);
			}
			//    store token
			addPreprocessorToken(state, &token);
			num++;
			resetAllocatorState(state->scratchpad, marker);
		} else {
			if (!lexTokens(state, &token, &macro_context)) {
				goto out;
			}
			//  store token
			addPreprocessorToken(state, &token);
			num++;
		}
		skipWhiteSpaceOrComments(state);
	}
	consumeInput(state);
	createPreprocessorDefinition(state, start_index, num, params->num,
	                             macro_name, macro_name_length, function_like);
	status = true;
out:
	state->macro_body = false;
	return status;
}

static bool handleDefineDirective(struct LexerState* state,
                                  struct FileContext* ctx, char* read_buffer)
{
	char macro_name[256];
	if (!skipWhiteSpaceOrComments(state)) {
		return false;
	}
	if (!isAlphabetic(state->c)) {
		lexerError(state, "Valid macro name expected");
		return false;
	}
	int macro_name_length = readWord(state, ctx, read_buffer);
	if (macro_name_length <= 0 || macro_name_length > 255) {
		lexerError(state, "macro name is to long");
		return false;
	}
	memcpy(macro_name, read_buffer, macro_name_length + 1);

	struct StringSet params;

	bool exists = false;
	size_t marker = markAllocatorState(state->scratchpad);
	char* buffer = allocate(state->scratchpad, 1024);
	if (buffer == NULL) {
		return false;
	}

	bool function_like = false;
	bool status = false;
	if (createStringSetInBuffer(&params, 256, 64, buffer, 1024) != 0) {
		goto out;
	}
	if (state->c == '(') {
		// Function like macro
		function_like = true;

		NEXT(state, out);
		if (!skipWhiteSpaceOrComments(state)) {
			goto out;
		}
		if (isAlphabetic(state->c)) {
			int len = readWord(state, ctx, read_buffer);
			if (len < 0) {
				lexerError(state, "identifier is to long");
				goto out;
			}
			uint32_t hash = hashSubstring(read_buffer, len);
			addStringAndHash(&params, read_buffer, len, hash, &exists);
			if (exists == true) {
				goto out;
			}
			if (!skipWhiteSpaceOrComments(state)) {
				goto out;
			}
			while (state->c != ')') {
				if (state->c == ',') {
					NEXT(state, out);
					if (!skipWhiteSpaceOrComments(state)) {
						goto out;
					}
					if (!isAlphabetic(state->c)) {
						goto out;
					}
					int len = readWord(state, ctx, read_buffer);
					if (len < 0) {
						goto out;
					}
					uint32_t hash = hashSubstring(read_buffer, len);
					addStringAndHash(&params, read_buffer, len, hash, &exists);
					if (exists) {
						goto out;
					}
					if (!skipWhiteSpaceOrComments(state)) {
						goto out;
					}
				} else {
					goto out;
				}
			}
		} else {
			if (state->c != ')') {
				goto out;
			}
		}

		NEXT(state, out);
	} else if (!isWhitespace(state->c)) {
		goto out;
	}
	if (!lexMacroBody(state, ctx, macro_name, macro_name_length, function_like,
	                  &params)) {
		goto out;
	}
	status = true;
out:
	resetAllocatorState(state->scratchpad, marker);
	return status;
}

static bool skipLine(struct LexerState* state)
{
	while (state->c != '\n') {
		// skip preprocessor lines
		if (!consumeLexableChar(state)) {
			return false;
		}
	}
	return true;
}

static bool handlePreprocessorDirective(struct LexerState* state,
                                        struct FileContext* ctx)

{
	bool status = false;
	size_t marker = markAllocatorState(state->scratchpad);
	char* read_buffer = allocate(state->scratchpad, MAX_IDENTIFIER_LENGTH);
	if (!read_buffer) {
		generalError("Memory allocation failed");
		goto out;
	}

	if (!skipWhiteSpaceOrComments(state)) {
		goto out;
	}
	if (!isAlphabetic(state->c)) {
		lexerError(state, "Preprocessor directive expected");
		goto out;
	}
	int len = readWord(state, ctx, read_buffer);
	if (len < 0) {
		lexerError(state, "Identifier is to long");
		goto out;
	}
	if (strcmp("include", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("define", read_buffer) == 0) {
		if (!handleDefineDirective(state, ctx, read_buffer)) {
			goto out;
		}
	} else if (strcmp("undef", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("if", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("ifdef", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("ifndef", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}

	} else if (strcmp("elsif", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("else", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("endif", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else if (strcmp("error", read_buffer) == 0) {
		if (!skipLine(state)) {
			goto out;
		}
	} else {
		lexerError(state, "Unknown preprocessor directive");
		goto out;
	}
	status = true;
out:
	resetAllocatorState(state->scratchpad, marker);
	return status;
}
bool getNextToken(struct LexerState* state, struct LexerToken* token)
{
	bool status = false;
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
				status = false;
				lexerError(state,
				           "Preprocessor definitions must start at the "
				           "beginning of a line");
				goto out;
			}
			state->line_beginning = false;
			NEXT(state, out);
			if (!handlePreprocessorDirective(state, &ctx)) {
				status = false;
				break;
			}
			again = true;
		} else {
			state->line_beginning = false;
			status = lexTokens(state, token, &ctx);
		}
	}
	status = true;
out:
	return status;
}
