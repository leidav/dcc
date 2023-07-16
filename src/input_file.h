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

#ifndef INPUT_FILE
#define INPUT_FILE

#include <stdio.h>

#define INPUT_CHUNK_SIZE (4096 << 2)
#define INPUT_EOF 0x4

struct InputFile {
	const char* name;
	char* buffer;
	size_t read_pos;
	size_t file_size;
	FILE* file;
};

int openInputFile(struct InputFile* file, const char* path, const char* name);
char readChar(struct InputFile* file);
#endif
