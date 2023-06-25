#include "scratchpad.h"

#include "memory/allocator.h"
#include "memory/linear_allocator.h"

static struct LinearAllocator allocator;

int scratchpadInit()
{
	struct MemoryArena* arena = globallyAllocateArena(SCRATCHPAD_SIZE);
	if (arena == NULL) {
		return -1;
	}
	initLinearAllocator(&allocator, arena);
	return 0;
}

struct Allocator* getScratchpadAllocator()
{
	return (struct Allocator*)&allocator;
}
