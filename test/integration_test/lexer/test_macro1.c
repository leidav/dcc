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

#include "test_lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "error.h"
#include "lexer.h"
#include "memory/scratchpad.h"

static const struct ExpectedToken expected_output1 [] = {
    {.line = 1, .column = 1, .type = KEYWORD_STRUCT},

};
const char* input = "data/test.c";

int main(int argc, const char** argv)
{
	if (scratchpadInit() != 0) {
		fprintf(stderr, "Coud not initialize scrtchpad memory");
		return 1;
	}
	scratchpadCleanup();
	return 0;
}
