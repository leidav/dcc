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
#include <unistd.h>

#include "error.h"
#include "lexer.h"

static void compareTokens(const struct LexerToken* token,
                          const struct ExpectedToken* expected)
{
}

int test_lexer(const char* file, const struct ExpectedToken* expected_tokens,
               int num_tokens)
{
	int result = -1;
	struct LexerState lexer_state;
	if (initLexer(&lexer_state, file) != 0) {
		fprintf(stderr, "Could not initialize lexer\n");
		return result;
	}

	for (int i = 0; i < num_tokens; i++) {
		struct LexerToken token;
		if (!getNextToken(&lexer_state, &token)) {
			lexerError(&lexer_state,
			           "An unexpected error occured during lexing");
			goto out;
		}
		if (token.type == TOKEN_EOF) {
			break;
		}
	}

/*for (int i = 0;
	 i < lexer_state.pp_state.definitions.pp_definition_names.num; i++) {
	printf("begin %s\n",
	       getStringAt(
	           &lexer_state.pp_state.definitions.pp_definition_names, i));
	int start = lexer_state.pp_state.definitions.definitions[i].token_start;
	int end =
	    lexer_state.pp_state.definitions.definitions[i].num_tokens + start;
	for (int j = start; j < end; j++) {
	    struct PreprocessorToken* pp_token =
	        &lexer_state.pp_state.tokens.tokens[j];
	    struct LexerToken t;
	    createLexerTokenFromPPToken(&lexer_state, pp_token, &t);
	    printToken(&lexer_state, &t);
	}
}
*/
out:
	cleanupLexer(&lexer_state);
	return 0;
}
