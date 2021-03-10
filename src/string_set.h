#ifndef STRING_SET_H
#define STRING_SET_H

#include <stdint.h>

#include "allocator.h"
#define SHORT_STRING_LEN 9

struct String {
	union {
		char* ptr;
		char chars[SHORT_STRING_LEN + 1];
	} string;
	uint16_t len;
};

struct StringSet {
	struct LinearAllocator string_allocator;
	struct String* strings;
	uint32_t* hashes;
	int num;
	int max_num;
};

int createStringSet(struct StringSet* stringset, size_t allocator_size,
                    int max_strings);

int destroyStringSet(struct StringSet* stringset, size_t allocator_size,
                     int max_strings);

int addString(struct StringSet* stringset, const char* string, int length);

const char* getString(const struct String* string);

#endif