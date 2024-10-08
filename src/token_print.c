/*	Copyright (C) 2021 David Leiter
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>

#include "lexer.h"

#define RETURN_AS_STRING_IF_MATCH(name) \
	case name:                          \
		return #name;                   \
		break;

// use switch to let the compiler create a string table for us instead of
// manually indexing a static array
static const char* getTokenName(enum TokenType type)
{
	switch (type) {
		RETURN_AS_STRING_IF_MATCH(IDENTIFIER)
		/*keywords*/
		RETURN_AS_STRING_IF_MATCH(KEYWORD_IF)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_ELSE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_WHILE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_FOR)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_DO)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_SWITCH)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CASE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_BREAK)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CONTINUE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_STRUCT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_ENUM)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_UNION)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_TYPEDEF)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_VOID)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CHAR)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_SHORT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_INT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_LONG)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_FLOAT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_DOUBLE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_SIGNED)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_UNSIGNED)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_STATIC)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_EXTERN)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CONST)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_INLINE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_RETURN)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_GOTO)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_REGISTER)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_RESTRICT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_VOLATILE)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_DEFAULT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_BOOL)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_COMPLEX)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_IMAGINARY)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CONSTEXPR)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_AUTO)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_SIZEOF)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_ALIGNAS)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_ALIGNOF)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_GENERIC)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_NORETURN)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_STATIC_ASSERT)
		RETURN_AS_STRING_IF_MATCH(KEYWORD_CONSTEVAL)

		/*operator punctuators*/
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_PLUS)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MINUS)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_DIV)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MODULO)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_PLUSPLUS)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MINUSMINUS)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_AND)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_OR)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_XOR)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_SHIFT_LEFT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_SHIFT_RIGHT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_NEGATE)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_LOGICAL_AND)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_LOGICAL_OR)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_LOGICAL_NOT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_EQUAL)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_NOT_EQUAL)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_LESS)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_GREATER)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_LESS_OR_EQUAL)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_GREATER_OR_EQUAL)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_PLUS_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MINUS_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MUL_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_DIV_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_MODULO_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_AND_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_OR_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_XOR_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_SHIFT_LEFT_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_SHIFT_RIGHT_ASSIGNMENT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_POINT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_DEREFERENCE)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_CONDITIONAL)
		/*other punctuators*/
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_PARENTHESE_LEFT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_PARENTHESE_RIGHT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_BRACKET_LEFT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_BRACKET_RIGHT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_BRACE_LEFT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_BRACE_RIGHT)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_ASTERISC)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_COMMA)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_COLON)
		RETURN_AS_STRING_IF_MATCH(PUNCTUATOR_SEMICOLON)
		/*literals*/
		RETURN_AS_STRING_IF_MATCH(LITERAL_STRING)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_CHAR)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_UNSIGNED_CHAR)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_INT)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_UNSIGNED_INT)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_FLOAT)
		RETURN_AS_STRING_IF_MATCH(CONSTANT_DOUBLE)
		/*preprocessor*/
		RETURN_AS_STRING_IF_MATCH(PP_NUMBER)
		RETURN_AS_STRING_IF_MATCH(PP_PARAM)
		RETURN_AS_STRING_IF_MATCH(PP_CONCAT)
		RETURN_AS_STRING_IF_MATCH(PP_STRINGIFY)
		// end of file
		RETURN_AS_STRING_IF_MATCH(TOKEN_EOF)
		RETURN_AS_STRING_IF_MATCH(TOKEN_UNKNOWN)
		RETURN_AS_STRING_IF_MATCH(TOKEN_EMPTY)
		default:
			return "UNKNOWN";
			break;
	}
};

void printToken(struct LexerState* state, const struct LexerToken* token)
{
	switch (token->type) {
		case IDENTIFIER: {
			int index = token->value.string_index;
			printf(
			    "line:%d, column: %d, type: IDENTIFIER, id:%d, name: "
			    "\"%s\"\n",
			    token->line + 1, token->column + 1, index,
			    getStringAt(&state->identifiers, index));
			break;
		}
		case PP_PARAM: {
			int index = token->value.param_index;
			printf("line:%d, column: %d, type: PP_PARAM, param:%d\n",
			       token->line + 1, token->column + 1, index);

			break;
		}
		case LITERAL_STRING: {
			int index = token->value.string_index;
			printf(
			    "line:%d, column: %d, type: LITERAL_STRING, id:%d, "
			    "value: "
			    "\"%s\"\n",
			    token->line + 1, token->column + 1, index,
			    getStringAt(&state->string_literals, index));
			break;
		}
		case PP_NUMBER: {
			int index = token->value.string_index;
			printf(
			    "line:%d, column: %d, type: PP_NUMBER, id:%d, value: "
			    "\"%s\"\n",
			    token->line + 1, token->column + 1, index,
			    getStringAt(&state->pp_numbers, index));
			break;
		}
		case CONSTANT_INT:
		case CONSTANT_UNSIGNED_INT: {
			uint64_t value = token->value.int_literal;
			printf(
			    "line:%d, column: %d, type: LITERAL_INT, value: "
			    "%" PRIu64 "\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_DOUBLE: {
			double value = token->value.double_literal;
			printf(
			    "line:%d, column: %d, type: LITERAL_DOUBLE, value: "
			    "%f\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_FLOAT: {
			float value = token->value.float_literal;
			printf("line:%d, column: %d, type: LITERAL_FLOAT, value: %f\n",
			       token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_CHAR: {
			int value = token->value.character_literal;
			printf(
			    "line:%d, column: %d, type: LITERAL_CHAR, value: "
			    "\'%c\' (%d)\n",
			    token->line + 1, token->column + 1, value, value);
			break;
		}
		default:
			printf("line:%d, column: %d, type: %s\n", token->line + 1,
			       token->column + 1, getTokenName(token->type));
	}
}

void printTokenAsCStruct(struct LexerState* state,
                         const struct LexerToken* token)
{
	switch (token->type) {
		case IDENTIFIER: {
			int index = token->value.string_index;
			printf(
			    "{.line = %d, .column = %d, .type = IDENTIFIER, "
			    ".string_value= \"%s\"\n},",
			    token->line + 1, token->column + 1,
			    getStringAt(&state->identifiers, index));
			break;
		}
		case PP_PARAM: {
			int index = token->value.param_index;
			printf(
			    "{.line = %d, .column = %d, .type = PP_PARAM, "
			    ".int_value= %d\n},",
			    token->line + 1, token->column + 1, index);

			break;
		}
		case LITERAL_STRING: {
			int index = token->value.string_index;
			printf(
			    "{.line = %d, .column = %d, .type = LITERAL_STRING, "
			    ".string_value= \"%s\"\n},",
			    token->line + 1, token->column + 1,
			    getStringAt(&state->string_literals, index));
			break;
		}
		case PP_NUMBER: {
			int index = token->value.string_index;
			printf(
			    "{.line = %d, .column = %d, .type = PP_NUMBER, "
			    ".string_value= \"%s\"\n},",
			    token->line + 1, token->column + 1,
			    getStringAt(&state->pp_numbers, index));
			break;
		}
		case CONSTANT_INT:
		case CONSTANT_UNSIGNED_INT: {
			uint64_t value = token->value.int_literal;
			printf(
			    "{.line = %d, .column = %d, .type = CONSTANT_UNSIGNED_INT, "
			    ".int_value = %" PRIu64 "},\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_DOUBLE: {
			double value = token->value.double_literal;
			printf(
			    "{.line = %d, .column = %d, .type = CONSTANT_DOUBLE, "
			    ".float_value = %f},\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_FLOAT: {
			float value = token->value.float_literal;
			printf(
			    "{.line = %d, .column = %d, .type = CONSTANT_FLOAT, "
			    ".float_value = %f},\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		case CONSTANT_CHAR: {
			int value = token->value.character_literal;
			printf(
			    "{.line = %d, .column = %d, .type = CONSTANT_CHAR, "
			    ".int_value = %d},\n",
			    token->line + 1, token->column + 1, value);
			break;
		}
		default:
			printf("{.line = %d, .column = %d, .type = %s },\n", token->line + 1,
			       token->column + 1, getTokenName(token->type));
	}
}
