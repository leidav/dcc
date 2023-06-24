#include "scratchpad.h"

#include "memory/allocator.h"
#include "memory/linear_allocator.h"

static struct LinearAllocator allocator;

int scratchpadInit()
{
	struct Allocator* global_allocator = getGlobalAllocator();
	struct MemoryArena arena;
	if (allocateArena(&arena, global_allocator, SCRATCHPAD_SIZE) != 0) {
		return -1;
	}
	initLinearAllocator(&allocator, &arena);
	return 0;
}

struct Allocator* getScratchpadAllocator()
{
	return (struct Allocator*)&allocator;
}
