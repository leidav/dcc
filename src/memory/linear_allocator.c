#include "linear_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* allocateLinearWrapper(void* allocator, size_t size,
                                   size_t power_of_two_alignment)
{
	return allocateLinear(allocator, size, power_of_two_alignment);
}

static void* reallocateLinearWrapper(void* allocator, void* ptr,
                                     size_t new_size,
                                     size_t power_of_two_alignment)
{
	return reallocateLinear(allocator, ptr, new_size, power_of_two_alignment);
}

static void deallocateNothing(void* allocator, void* ptr)
{
	return;
}

void initLinearAllocator(struct LinearAllocator* allocator,
                         const struct MemoryArena* arena)
{
	initAllocator(&allocator->base, allocateLinearWrapper,
	              reallocateLinearWrapper, deallocateNothing);
	allocator->arena = *arena;
	void* memory = arena->memory;
	allocator->end = memory + arena->size;
	allocator->start = memory;
	allocator->free = memory;
	allocator->last = memory;
}
void cleanupLinearAllocator(struct LinearAllocator* allocator)
{
	cleanupArena(&allocator->arena);
	allocator->start = NULL;
	allocator->free = NULL;
	allocator->last = NULL;
	allocator->end = NULL;
}

size_t markLinearAllocatorState(struct LinearAllocator* allocator)
{
	return allocator->free - allocator->start;
}

void resetLinearAllocatorState(struct LinearAllocator* allocator, size_t pos)
{
	allocator->free = allocator->start + pos;
}

void* allocateLinear(struct LinearAllocator* allocator, size_t size,
                     size_t power_of_two_alignment)
{
	size_t offset = alignmentOffset(allocator->free, power_of_two_alignment);
	void* aligned = allocator->free + offset;
	void* next_free = aligned + size;
	if (next_free >= allocator->end) {
		return NULL;
	}
	allocator->free = next_free;
	allocator->last = aligned;
	return aligned;
}

void* reallocateLinear(struct LinearAllocator* allocator, void* ptr,
                       size_t new_size, size_t power_of_two_alignment)
{
	if (ptr == allocator->last) {
		void* next_free = allocator->last + new_size;
		if (next_free >= allocator->end) {
			return NULL;
		}
		return next_free;
	}
	void* new_ptr = allocateLinear(allocator, new_size, power_of_two_alignment);
	if (ptr != NULL) {
		memcpy(new_ptr, ptr, allocator->free - allocator->last);
	}
	return new_ptr;
}
