#include "error.h"

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

inline static void setYellowColor()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[33m");
	}
}

inline static void resetColor()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[39m");
	}
}

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

void lexerError(struct LexerState* state, const char* reason)
{
	if (state->error_handled) {
		return;
	}
	state->error_handled = true;
	setRedColor();
	fprintf(stderr, "Lexer error ");
	resetColor();
	fprintf(stderr, "at %s:%d:%d %s\n", state->current_file.name,
	        state->current_pos.line + 1, state->current_pos.column + 1, reason);

	int max_length = 120;
	if (isatty(STDOUT_FILENO)) {
		struct winsize terminal_size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
		max_length = MIN(terminal_size.ws_col, 120);
	}
	if (state->current_pos.column < max_length) {
		printErrorLine(state->current_pos.line_pos, state->current_pos.column,
		               state->current_file.name);
	}
}
void generalError(const char* reason)
{
	fprintf(stderr, "Error: %s\n", reason);
}

void generalWarning(const char* reason)
{
	setYellowColor();
	fprintf(stderr, "Warning: %s\n", reason);
	resetColor();
}
