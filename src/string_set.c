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

static int createString(struct String* string, const char* str, int length,
                        struct LinearAllocator* string_allocator)
{
	string->length = length;
	char* ptr = allocate(string_allocator, length + 1);
	if (!ptr) {
		return -1;
	}
	string->offset = ptr - (char*)string_allocator->start;
	memcpy(ptr, string, length);
	ptr[length] = 0;
	return 0;
}

static const char* getString(const struct String* string,
                             struct LinearAllocator* string_allocator)
{
	return string_allocator->start + string->offset;
}

static bool compareStrings(const struct String* string, const char* string2,
                           int length2, struct StringSet* stringset)
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

int addString(struct StringSet* stringset, const char* string, int length)
{
	uint32_t hash = fnv1a(string, length);
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
	int num = ++stringset->num;
	stringset->hashes[num] = hash;
	createString(&stringset->strings[num], string, length,
	             &stringset->string_allocator);
	return num;
}

const char* getStringAt(struct StringSet* stringset, int index)
{
	return getString(&stringset->strings[index], &stringset->string_allocator);
}