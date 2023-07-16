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

#include "string_set.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory/allocator.h"

struct StringSetString {
	uint16_t offset;
	uint16_t length;
};

static uint32_t fnv1a(const char* string, int length)
{
	uint32_t hash = 2166136261;
	for (int i = 0; i < length; i++) {
		hash ^= *string;
		hash *= 16777619;
		string++;
	}
	return hash;
}

static uint32_t fnv1a_nullterminated(const char* string)
{
	uint32_t hash = 2166136261;
	while (*string) {
		hash ^= (int)*string;
		hash *= 16777619;
		string++;
	}
	return hash;
}

static uint32_t djb2(const char* string, int length)
{
	uint32_t hash = 5381;
	for (int i = 0; i < length; i++) {
		hash = ((hash << 5) + hash) ^ (int)*string;
		string++;
	}
	return hash;
}

static uint32_t djb2_nullterminated(const char* string)
{
	uint32_t hash = 5381;
	while (*string) {
		hash = ((hash << 5) + hash) ^ (int)*string;
		string++;
	}
	return hash;
}

uint32_t hashString(const char* string)
{
	return djb2_nullterminated(string);
}

uint32_t hashSubstring(const char* string, int length)
{
	return djb2(string, length);
}

static int createString(struct StringSetString* string, const char* str,
                        int length, char* string_buffer, int buffer_size,
                        int offset)
{
	int new_offset = offset + length + 1;
	if (new_offset >= buffer_size) {
		return -1;
	}
	string->length = length;
	char* ptr = string_buffer + offset;

	string->offset = offset;
	memcpy(ptr, str, length);
	ptr[length] = 0;
	return new_offset;
}

static inline const char* getString(const struct StringSetString* string,
                                    char* string_buffer)
{
	return string_buffer + string->offset;
}

static bool compareStrings(const struct StringSetString* string,
                           const char* string2, int length2,
                           struct StringSet* stringset)
{
	int length = string->length;
	if (length != length2) {
		return false;
	}
	const char* string1 = getString(string, stringset->string_buffer);
	for (int i = 0; i < length; i++) {
		if (*string1 != *string2) {
			return false;
		}
		string1++;
		string2++;
	}
	return true;
}

int initStringSet(struct StringSet* stringset, size_t string_buffer_size,
                  int max_strings, struct Allocator* allocator)
{
	stringset->parent_allocator = allocator;
	char* buffer = allocateAligned(allocator, string_buffer_size, 1);
	if (buffer == NULL) {
		return -1;
	}
	stringset->string_buffer = buffer;
	stringset->buffer_size = string_buffer_size;
	stringset->offset = 0;
	stringset->num = 0;
	stringset->max_num = max_strings;
	stringset->strings =
	    ALLOCATE_TYPE(allocator, max_strings, typeof(*stringset->strings));
	if (!stringset->strings) {
		goto err1;
	}
	stringset->hashes =
	    ALLOCATE_TYPE(allocator, max_strings, typeof(*stringset->hashes));
	if (!stringset->hashes) {
		goto err2;
	}
	return 0;

err2:
	deallocate(allocator, stringset->strings);
err1:
	deallocate(allocator, stringset->string_buffer);
	return -1;
}

int cleanupStringSet(struct StringSet* stringset)
{
	deallocate(stringset->parent_allocator, stringset->string_buffer);
	deallocate(stringset->parent_allocator, stringset->strings);
	deallocate(stringset->parent_allocator, stringset->hashes);
	stringset->string_buffer = NULL;
	stringset->strings = NULL;
	stringset->hashes = NULL;
	stringset->offset = 0;
	stringset->buffer_size = 0;
	stringset->num = 0;
	stringset->max_num = 0;
	return 0;
}

int addStringAndHash(struct StringSet* stringset, const char* string,
                     int length, uint32_t hash, bool* exists)
{
	for (int i = 0; i < stringset->num; i++) {
		if (stringset->hashes[i] == hash) {
			if (compareStrings(&stringset->strings[i], string, length,
			                   stringset)) {
				if (exists != NULL) {
					*exists = true;
				}
				return i;
			}
		}
	}
	if (exists != NULL) {
		*exists = false;
	}

	if (stringset->num + 1 > stringset->max_num) {
		return -1;
	}
	int index = stringset->num++;
	stringset->hashes[index] = hash;
	int new_offset = createString(&stringset->strings[index], string, length,
	                              stringset->string_buffer,
	                              stringset->buffer_size, stringset->offset);
	if (new_offset < 0) {
		return -1;
	}
	stringset->offset = new_offset;

	return index;
}

int addString(struct StringSet* stringset, const char* string, int length)
{
	uint32_t hash = fnv1a(string, length);
	return addStringAndHash(stringset, string, length, hash, NULL);
}

const char* getStringAt(struct StringSet* stringset, int index)
{
	return getString(&stringset->strings[index], stringset->string_buffer);
}

uint32_t getHashAt(struct StringSet* stringset, int index)
{
	return stringset->hashes[index];
}

int getLengthAt(struct StringSet* stringset, int index)
{
	return stringset->strings[index].length;
}

int findIndex(struct StringSet* stringset, const char* string, int length,
              uint32_t hash)
{
	for (int i = 0; i < stringset->num; i++) {
		if (stringset->hashes[i] == hash) {
			if (compareStrings(&stringset->strings[i], string, length,
			                   stringset)) {
				return i;
			}
		}
	}
	return -1;
}
