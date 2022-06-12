#ifndef STRING_SET_H
#define STRING_SET_H

#include <stdint.h>

#include "allocator.h"

struct StringSetString {
	uint16_t offset;
	uint16_t length;
};

struct StringSet {
	struct LinearAllocator string_allocator;
	struct Allocator* parent_allocator;
	struct StringSetString* strings;
	uint32_t* hashes;
	int num;
	int max_num;
};

uint32_t hashString(const char* string);

uint32_t hashSubstring(const char* string, int length);

int createStringSet(struct StringSet* stringset, size_t string_buffer_size,
                    int max_strings, struct Allocator* allocator);

int createStringSetInBuffer(struct StringSet* stringset,
                            size_t string_buffer_size, int max_strings,
                            void* buffer, size_t buffer_size);

int destroyStringSet(struct StringSet* stringset, size_t allocator_size,
                     int max_strings);

int addStringAndHash(struct StringSet* stringset, const char* string,
                     int length, uint32_t hash, bool* exists);

int addString(struct StringSet* stringset, const char* string, int length);

const char* getStringAt(struct StringSet* stringset, int index);

int findIndex(struct StringSet* stringset, const char* string, int length,
              uint32_t hash);

#endif
