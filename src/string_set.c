#include "string_set.h"

#include <stdbool.h>
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
                        struct LinearAllocator* allocator)
{
	string->len = length;
	if (length <= SHORT_STRING_LEN) {
		memcpy(string->string.chars, string, length);
		string->string.chars[length] = 0;
	} else {
		void* ptr = allocate(allocator, length + 1);
		if (!ptr) {
			return -1;
		}
		string->string.ptr = ptr;
		memcpy(string->string.ptr, string, length);
		string->string.ptr[length] = 0;
	}
	return 0;
}

static bool compareStrings(const struct String* string, const char* string2,
                           int length2)
{
	int length = string->len;
	if (length != length2) {
		return false;
	}
	const char* string1 = getString(string);
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
	createAllocator(&stringset->string_allocator, allocator_size);
	stringset->num = 0;
	stringset->max_num = max_strings;
	stringset->strings = malloc(sizeof(*stringset->strings) * max_strings);
	stringset->hashes = malloc(sizeof(*stringset->hashes) * max_strings);
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
}

int addString(struct StringSet* stringset, const char* string, int length)
{
	uint32_t hash = fnv1a(string, length);
	for (int i = 0; i < stringset->num; i++) {
		if (stringset->hashes[i] == hash) {
			if (compareStrings(&stringset->strings[i], string, length)) {
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
}

const char* getString(const struct String* string)
{
	if (string->len <= SHORT_STRING_LEN) {
		return string->string.chars;
	}
	return string->string.ptr;
}