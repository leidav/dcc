#include <memory/allocator.h>
#include <memory/block_allocator.h>
#include <stdio.h>

#define MEMORY_SIZE 256
#define BLOCK_SIZE 32
unsigned char* memory[MEMORY_SIZE];

int main()
{
	struct MemoryArena* arena = createNonOwningArena(memory, MEMORY_SIZE);
	struct BlockAllocator allocator;
	initBlockAllocator(&allocator, arena, BLOCK_SIZE, 16);
	for (int i = 0; i < 4; i++) {
		char* memory = allocateBlock(&allocator);
		printf("addr:%p\n", memory);
	}
	return 0;
}
