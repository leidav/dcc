#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"

struct LinearAllocator {
	struct Allocator base;
	struct MemoryArena* arena;
	void* start;
	void* free;
	void* last;
	void* end;
};

void initLinearAllocator(struct LinearAllocator* allocator,
                         struct MemoryArena* arena);

void cleanupLinearAllocator(struct LinearAllocator* allocator);

size_t markLinearAllocatorState(struct LinearAllocator* allocator);

void resetLinearAllocatorState(struct LinearAllocator* allocator, size_t pos);

void* allocateLinear(struct LinearAllocator* allocator, size_t size,
                     size_t power_of_two_alignment);

void* reallocateLinear(struct LinearAllocator* allocator, void* ptr,
                       size_t new_size, size_t power_of_two_alignment);

#endif
