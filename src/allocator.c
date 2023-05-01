#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t alignmentOffset(void* address, size_t alignment)
{
	size_t misalignment = ((size_t)(address - 1) & (alignment - 1));
	return alignment - misalignment - 1;
}

void* allocateAligned(struct Allocator* allocator, size_t size,
                      size_t power_of_two_alignment)
{
	if (allocator == NULL) {
		return malloc(size);
	}
	return allocator->allocate(allocator, size, power_of_two_alignment);
}

void* allocate(struct Allocator* allocator, size_t size)
{
	void* mem = allocateAligned(allocator, size, DEFAULT_ALIGNMENT);
	if (mem == NULL) {
		fprintf(stderr, "memory allocation failed\n");
	}
	return mem;
}

void* reallocate(struct Allocator* allocator, void* ptr, size_t new_size)
{
	if (allocator == NULL) {
		return realloc(ptr, new_size);
	}
	return allocator->reallocate(allocator, ptr, new_size, 16);
}

void deallocate(struct Allocator* allocator, void* ptr)
{
	if (allocator == NULL) {
		free(ptr);
	} else {
		allocator->deallocate(allocator, ptr);
	}
}

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

int createLinearAllocator(struct LinearAllocator* allocator, size_t size,
                          struct Allocator* parent_allocator)
{
	void* memory = allocate(parent_allocator, size);
	if (!memory) {
		return -1;
	}
	allocator->parent_allocator = parent_allocator;
	allocator->end = memory + size;
	allocator->free = memory;
	allocator->last = memory;
	allocator->start = memory;
	allocator->memory_owned = true;
	allocator->base.allocate = allocateLinearWrapper;
	allocator->base.reallocate = reallocateLinearWrapper;
	allocator->base.deallocate = deallocateNothing;
	return 0;
}

int createLinearAllocatorFromBuffer(struct LinearAllocator* allocator,
                                    void* buffer, size_t size)
{
	allocator->parent_allocator = NULL;
	allocator->end = buffer + size;
	allocator->free = buffer;
	allocator->last = buffer;
	allocator->start = buffer;
	allocator->memory_owned = false;
	allocator->base.allocate = allocateLinearWrapper;
	allocator->base.deallocate = deallocateNothing;
	return 0;
}

void destroyLinearAllocator(struct LinearAllocator* allocator)
{
	if (allocator->memory_owned && allocator->start) {
		deallocate(allocator->parent_allocator, allocator->start);
	}
	allocator->start = NULL;
	allocator->free = NULL;
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
