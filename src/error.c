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

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "helper.h"
#include "lexer.h"

inline static void setRedColor(bool is_tty)
{
	if (is_tty) {
		fprintf(stderr, "\e[31m");
	}
}

inline static void setYellowColor(bool is_tty)
{
	if (is_tty) {
		fprintf(stderr, "\e[33m");
	}
}

inline static void resetColor(bool is_tty)
{
	if (is_tty) {
		fprintf(stderr, "\e[39m");
	}
}

inline static void setBold(bool is_tty)
{
	if (is_tty) {
		fprintf(stderr, "\e[1m");
	}
}
inline static void resetBold(bool is_tty)
{
	if (is_tty) {
		fprintf(stderr, "\e[22m");
	}
}

static void printErrorLine(int line_pos, int column, const char* file_path,
                           bool is_tty)
{
	char buff[128];
	FILE* file = fopen(file_path, "r");
	fseek(file, line_pos, SEEK_SET);
	int length = fread(buff, 1, 128, file);
	fclose(file);

	char* line = buff;
	int pos = 0;
	char* ptr = line;
	int error_pos = 0;
	while ((pos < length - 1) && (*ptr != '\n') && (*ptr != '\r')) {
		if (pos < column) {
			if (*ptr == '\t') {
				error_pos += 8;
			} else {
				error_pos++;
			}
		}
		ptr++;
		pos++;
	}
	line[pos] = 0;
	fprintf(stderr, "%s\n", line);
	for (int i = 0; i < error_pos; i++) {
		putc(' ', stderr);
	}
	setRedColor(is_tty);
	fprintf(stderr, "^\n");
	resetColor(is_tty);
}

void lexerError(struct LexerState* state, const char* reason)
{
	if (state->error_handled) {
		return;
	}
	state->error_handled = true;
	bool is_tty = isatty(STDERR_FILENO) != 0;

	setBold(is_tty);
	fprintf(stderr, "%s:%d:%d ", state->current_file.name,
	        state->current_pos.line + 1, state->current_pos.column + 1);
	setRedColor(is_tty);
	fprintf(stderr, "error: ");
	resetColor(is_tty);

	fprintf(stderr, "%s\n", reason);

	int max_length = 120;
	if (isatty(STDOUT_FILENO)) {
		struct winsize terminal_size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
		max_length = MIN(terminal_size.ws_col, 120);
	}
	resetBold(is_tty);
	if (state->current_pos.column < max_length) {
		printErrorLine(state->current_pos.line_pos, state->current_pos.column,
		               state->current_file.full_path, is_tty);
	}
}

void generalError(const char* reason)
{
	bool is_tty = isatty(STDERR_FILENO) != 0;
	setBold(is_tty);
	setRedColor(is_tty);
	fprintf(stderr, "error: ");
	resetColor(is_tty);
	fprintf(stderr, "%s\n", reason);
	resetBold(is_tty);
}

void generalWarning(const char* reason)
{
	bool is_tty = isatty(STDERR_FILENO) != 0;
	setBold(is_tty);
	setYellowColor(is_tty);
	fprintf(stderr, "warning: ");
	resetColor(is_tty);
	fprintf(stderr, "%s\n", reason);
	resetBold(is_tty);
}
