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

#include "error.h"
#include "helper.h"
#include "lexer.h"
#include "scratchpad.h"

int main(int argc, const char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "No input file specified!\n");
		return 1;
	}
	if (scratchpadInit() != 0) {
		fprintf(stderr, "Coud not initialize scrtchpad memory");
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
			unexpectedCharacterError(&lexer_state);
		} else {
			printToken(&lexer_state, &token);
		}

		if (token.type == TOKEN_EOF) {
			break;
		}
	}
	return 0;
}
