#include "string_set.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                        int length, struct LinearAllocator* string_allocator)
{
	string->length = length;
	char* ptr = allocate(string_allocator, length + 1);
	if (!ptr) {
		return -1;
	}
	string->offset = ptr - (char*)string_allocator->start;
	memcpy(ptr, str, length);
	ptr[length] = 0;
	return 0;
}

static inline const char* getString(
    const struct StringSetString* string,
    const struct LinearAllocator* string_allocator)
{
	return string_allocator->start + string->offset;
}

static bool compareStrings(const struct StringSetString* string,
                           const char* string2, int length2,
                           struct StringSet* stringset)
{
	int length = string->length;
	if (length != length2) {
		return false;
	}
	const char* string1 = getString(string, &stringset->string_allocator);
	for (int i = 0; i < length; i++) {
		if (*string1 != *string2) {
			return false;
		}
		string1++;
		string2++;
	}
	return true;
}

int createStringSet(struct StringSet* stringset, size_t allocator_size,
                    int max_strings)
{
	if (createAllocator(&stringset->string_allocator, allocator_size) != 0) {
		return -1;
	}
	stringset->num = 0;
	stringset->max_num = max_strings;
	stringset->strings = malloc(sizeof(*stringset->strings) * max_strings);
	stringset->hashes = malloc(sizeof(*stringset->hashes) * max_strings);
	if (!stringset->strings) {
		return -1;
	}
	if (!stringset->hashes) {
		return -1;
	}
	return 0;
}

int destroyStringSet(struct StringSet* stringset, size_t allocator_size,
                     int max_strings)
{
	destroyAllocator(&stringset->string_allocator);
	free(stringset->strings);
	free(stringset->hashes);
	stringset->strings = NULL;
	stringset->hashes = NULL;
	stringset->num = 0;
	stringset->max_num = 0;
	return 0;
}

int addStringAndHash(struct StringSet* stringset, const char* string,
                     int length, uint32_t hash)
{
	for (int i = 0; i < stringset->num; i++) {
		if (stringset->hashes[i] == hash) {
			if (compareStrings(&stringset->strings[i], string, length,
			                   stringset)) {
				return i;
			}
		}
	}
	if (stringset->num + 1 > stringset->max_num) {
		return -1;
	}
	int index = stringset->num++;
	stringset->hashes[index] = hash;
	createString(&stringset->strings[index], string, length,
	             &stringset->string_allocator);

	return index;
}

int addString(struct StringSet* stringset, const char* string, int length)
{
	uint32_t hash = fnv1a(string, length);
	return addStringAndHash(stringset, string, length, hash);
}

const char* getStringAt(struct StringSet* stringset, int index)
{
	return getString(&stringset->strings[index], &stringset->string_allocator);
}