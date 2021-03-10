#include "allocator.h"

#include <stdlib.h>

static size_t alignmentOffset(void* address, size_t alignment)
{
	size_t misalignment = ((size_t)(address - 1) & (alignment - 1));
	return alignment - misalignment - 1;
}

int createAllocator(struct LinearAllocator* allocator, size_t size)
{
	void* memory = malloc(size);
	if (!memory) {
		return -1;
	}
	allocator->end = memory + size;
	allocator->free = memory;
	allocator->start = memory;
	return 0;
}

void destroyAllocator(struct LinearAllocator* allocator)
{
	if (allocator->start) {
		free(allocator->start);
	}
	allocator->start = NULL;
	allocator->free = NULL;
	allocator->end = NULL;
}

size_t markAllocatorState(struct LinearAllocator* allocator)
{
	return allocator->free - allocator->start;
}

void resetAllocatorState(struct LinearAllocator* allocator, size_t pos)
{
	allocator->free = allocator->start + pos;
}

void* allocate(struct LinearAllocator* allocator, size_t size)
{
	return allocate_aligned(allocator, size, 1);
}

void* allocate_aligned(struct LinearAllocator* allocator, size_t size,
                       size_t power_of_two_alignment)
{
	size_t offset = alignmentOffset(allocator->free, power_of_two_alignment);
	void* next_free = allocator->free + offset + size;
	if (next_free > allocator->end) {
		return NULL;
	}
	void* address = allocator->free + offset;
	allocator->free = next_free;
	return address;
}