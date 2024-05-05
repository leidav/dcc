#include <assert.h>
#include <memory/allocator.h>
#include <memory/block_allocator.h>
#include <stdio.h>

#include "test.h"

#define MEMORY_SIZE 256
#define BLOCK_SIZE 32
#define MAX_ALLOCATIONS 4

unsigned char* memory[MEMORY_SIZE];

int main()
{
	/*int x = 0;
	EXPECT_EQ(x, 10);
	EXPECT_EQ(x, 10);
	EXPECT_TRUE(x);
	*/
	struct MemoryArena* arena = createNonOwningArena(memory, MEMORY_SIZE);
	struct BlockAllocator allocator;
	initBlockAllocator(&allocator, arena, BLOCK_SIZE, 16);
	char* allocations[MAX_ALLOCATIONS];
	for (int i = 0; i < MAX_ALLOCATIONS; i++) {
		allocations[i] = allocateBlock(&allocator);
	}
	char* prev = allocations[0];
	for (int i = 1; i < MAX_ALLOCATIONS; i++) {
		assert(allocations[i] > prev);
		prev = allocations[i];
	}
	char* old_mem1 = allocations[1];
	char* old_mem3 = allocations[3];
	deallocateBlock(&allocator, allocations[1]);
	allocations[1] = NULL;
	deallocateBlock(&allocator, allocations[3]);
	allocations[3] = NULL;

	allocations[1] = allocateBlock(&allocator);
	allocations[3] = allocateBlock(&allocator);
	assert(allocations[1] == old_mem3);
	assert(allocations[3] == old_mem1);
	return 0;
}
