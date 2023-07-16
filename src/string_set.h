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

#ifndef STRING_SET_H
#define STRING_SET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Allocator;
struct StringSetString;

struct StringSet {
	struct Allocator* parent_allocator;
	char* string_buffer;
	struct StringSetString* strings;
	uint32_t* hashes;
	int offset;
	int buffer_size;
	int num;
	int max_num;
};

uint32_t hashString(const char* string);

uint32_t hashSubstring(const char* string, int length);

int initStringSet(struct StringSet* stringset, size_t string_buffer_size,
                  int max_strings, struct Allocator* allocator);

int cleanupStringSet(struct StringSet* stringset);

int addStringAndHash(struct StringSet* stringset, const char* string,
                     int length, uint32_t hash, bool* exists);

int addString(struct StringSet* stringset, const char* string, int length);

const char* getStringAt(struct StringSet* stringset, int index);

int getLengthAt(struct StringSet* stringset, int index);

uint32_t getHashAt(struct StringSet* stringset, int index);

int findIndex(struct StringSet* stringset, const char* string, int length,
              uint32_t hash);

#endif
