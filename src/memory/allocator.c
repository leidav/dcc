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

int allocateArena(struct MemoryArena* arena, struct Allocator* allocator,
                  size_t size)
{
	void* memory = allocate(allocator, size);
	if (!memory) {
		return -1;
	}
	arena->size = size;
	arena->memory = memory;
	arena->owner = allocator;
	arena->resizable = false;
	return 0;
}

int globallyAllocateArena(struct MemoryArena* arena, size_t size)
{
	struct Allocator* allocator = getGlobalAllocator();
	void* memory = allocate(allocator, size);
	if (!memory) {
		return -1;
	}
	arena->size = size;
	arena->memory = memory;
	arena->owner = allocator;
	arena->resizable = true;
	return 0;
}

void initNonOwningArena(struct MemoryArena* arena, void* buffer, size_t size)
{
	arena->memory = buffer;
	arena->size = size;
	arena->owner = NULL;
	arena->resizable = false;
}

void cleanupArena(struct MemoryArena* arena)
{
	if (arena->owner != NULL) {
		deallocate(arena->owner, arena->memory);
	}
	arena->memory = NULL;
	arena->size = 0;
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
