#include "scratchpad.h"

#include "allocator.h"

static struct LinearAllocator allocator;

int scratchpadInit()
{
	if (createLinearAllocator(&allocator, SCRATCHPAD_SIZE, NULL) != 0) {
		return -1;
	}
	return 0;
}

struct Allocator* getScratchpadAllocator()
{
	return (struct Allocator*)&allocator;
}
