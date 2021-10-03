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

#include <stdio.h>
#include <unistd.h>

#include "lexer.h"

inline static void setRedColor()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[31m");
	}
}

inline static void resetColor()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[39m");
	}
}

/*static void printErrorMessage(int line, int column, FILE* file) {

}*/

int main(int argc, const char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "No input file specified!\n");
		return 1;
	}
	struct LexerState lexer_state;
	if (initLexer(&lexer_state, argv[1]) != 0) {
		fprintf(stderr, "Could not initialize lexer\n");
		return -1;
	}
	bool validInput = true;
	while (validInput) {
		struct LexerToken token;
		validInput = getNextToken(&lexer_state, &token);
		if (!validInput) {
			setRedColor();
			fprintf(stderr, "Lexer error ");
			resetColor();
			int character = (int)lexer_state.c;
			fprintf(stderr, "At %s:%d:%d unexpected character:%c (0x%X)\n",
			        lexer_state.current_file.name, lexer_state.line + 1,
			        lexer_state.column + 1, lexer_state.c, character);
		} else {
			printToken(&lexer_state, &token);
		}

		if (token.type == TOKEN_EOF) {
			break;
		}
	}
	// showPosition("A short text", 8);
	return 0;
}
