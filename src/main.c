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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "helper.h"
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

static void printErrorLine(int line_pos, int column, const char* file_path)
{
	char buff[128];
	FILE* file = fopen(file_path, "r");
	fseek(file, line_pos, SEEK_SET);
	int length = fread(buff, 1, 128, file);
	fclose(file);

	char* line = buff;
	int pos = 0;
	char* ptr = line;
	while ((pos < length - 1) && (*ptr != '\n') && (*ptr != '\r')) {
		ptr++;
		pos++;
	}
	line[pos] = 0;
	fprintf(stderr, "%s\n", line);
	for (int i = 0; i < column; i++) {
		putc(' ', stderr);
	}
	setRedColor();
	fprintf(stderr, "^\n");
	resetColor();
}

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

			int max_length = 120;
			if (isatty(STDOUT_FILENO)) {
				struct winsize terminal_size;
				ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
				max_length = MIN(terminal_size.ws_col, 120);
			}
			if (lexer_state.column < max_length) {
				printErrorLine(lexer_state.line_pos, lexer_state.column,
				               argv[1]);
			}
		} else {
			printToken(&lexer_state, &token);
		}

		if (token.type == TOKEN_EOF) {
			break;
		}
	}
	/*for (int i = 0; i < lexer_state.identifiers.num; i++) {
	    printf("%d, %s\n", i, getStringAt(&lexer_state.identifiers, i));
	}*/
	// showPosition("A short text", 8);
	return 0;
}
