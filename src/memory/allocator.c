#include "allocator.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* _mallocWrapper(void* allocator, size_t size,
                            size_t power_of_two_alignment)
{
	return malloc(size);
}

static void* _reallocWrapper(void* allocator, void* ptr, size_t new_size,
                             size_t power_of_two_alignment)
{
	return realloc(ptr, new_size);
}

static void _freeWrapper(void* allocator, void* ptr)
{
	free(ptr);
}

static struct Allocator global_allocator = {.allocate = _mallocWrapper,
                                            .reallocate = _reallocWrapper,
                                            .deallocate = _freeWrapper};

void* allocateAligned(struct Allocator* allocator, size_t size,
                      size_t power_of_two_alignment)
{
	return allocator->allocate(allocator, size, power_of_two_alignment);
}

void* allocate(struct Allocator* allocator, size_t size)
{
	return allocateAligned(allocator, size, DEFAULT_ALIGNMENT);
}

void* reallocate(struct Allocator* allocator, void* ptr, size_t new_size)
{
	return allocator->reallocate(allocator, ptr, new_size, 16);
}

void deallocate(struct Allocator* allocator, void* ptr)
{
	allocator->deallocate(allocator, ptr);
}

struct MemoryArena* allocateArena(struct Allocator* allocator, size_t size)
{
	struct MemoryArena* arena =
	    allocateAligned(allocator, sizeof(struct MemoryArena) + size,
	                    alignof(struct MemoryArena));
	if (arena == NULL) {
		return NULL;
	}
	arena->size = size;
	arena->owner = allocator;
	return arena;
}

struct MemoryArena* globallyAllocateArena(size_t size)
{
	struct Allocator* allocator = getGlobalAllocator();
	return allocateArena(allocator, size);
}

struct MemoryArena* createNonOwningArena(void* buffer, size_t buffer_size)
{
	size_t offset = alignmentOffset(buffer, alignof(struct MemoryArena));
	size_t size = sizeof(struct MemoryArena) + offset;
	if (size > buffer_size) {
		return NULL;
	}
	size_t payload_size = buffer_size - size;
	struct MemoryArena* arena = buffer + offset;
	arena->size = payload_size;
	arena->owner = NULL;
	return arena;
}

void deallocateArena(struct MemoryArena* arena)
{
	if (arena == NULL) {
		return;
	}

	if (arena->owner != NULL) {
		deallocate(arena->owner, arena);
	}
}

void initAllocator(struct Allocator* allocator, AllocateFn allocate,
                   ReallocateFn reallocate, DeallocateFn deallocate)
{
	allocator->allocate = allocate;
	allocator->reallocate = reallocate;
	allocator->deallocate = deallocate;
}

struct Allocator* getGlobalAllocator()
{
	return &global_allocator;
}
