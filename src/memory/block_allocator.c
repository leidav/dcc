#include "block_allocator.h"

#include <assert.h>

#include "helper.h"

struct Block {
	struct Block* next;
};

void initBlockAllocator(struct BlockAllocator* allocator,
                        struct MemoryArena* arena, size_t block_size,
                        size_t block_alignment)
{
	size_t alignment = MAX(block_alignment, alignof(struct Block));
	size_t offset = alignmentOffset(arena->memory, alignment);
	size_t size = MAX(block_size, sizeof(struct Block));
	// int num = (arena->size - offset) / size;
	allocator->arena = arena;
	allocator->end = arena->memory + arena->size;
	allocator->block_size = size;
	struct Block* next_free = (struct Block*)arena->memory + offset;
	allocator->brk = (unsigned char*)(next_free + 1);
	next_free->next = NULL;
}

void* allocateBlock(struct BlockAllocator* allocator)
{
	struct Block* free = allocator->next_free;
	if (free == NULL) {
		return NULL;
	}
	size_t block_size = allocator->block_size;
	struct Block* next = free->next;

	if (next == NULL) {
		unsigned char* next_brk = allocator->brk + block_size;
		if (next_brk <= allocator->end) {
			next = (struct Block*)allocator->brk;
			next->next = NULL;
			allocator->brk = next_brk;
		}
	} else {
		next = free->next;
	}
	allocator->next_free = next;
	return free;
}

void deallocateBlock(struct BlockAllocator* allocator, void* ptr)
{
	assert(((unsigned char*)ptr >= allocator->arena->memory) &&
	       ((unsigned char*)ptr <= allocator->end));
	struct Block* free = (struct Block*)ptr;
	free->next = allocator->next_free;
	allocator->next_free = free;
}
