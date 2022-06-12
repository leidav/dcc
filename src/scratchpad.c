#include "scratchpad.h"

#include "allocator.h"

static char scratchpad_memory[SCRATCHPAD_SIZE];

static struct LinearAllocator allocator;

int scratchpadInit()
{
	if (createLinearAllocatorFromBuffer(&allocator, scratchpad_memory,
	                                    SCRATCHPAD_SIZE) != 0) {
		return -1;
	}
	return 0;
}

struct LinearAllocator* getScratchpadAllocator()
{
	return &allocator;
}
