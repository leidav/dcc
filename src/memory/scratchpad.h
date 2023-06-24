#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#define SCRATCHPAD_SIZE (4096 << 4)
struct Allocator;

int scratchpadInit();
struct Allocator* getScratchpadAllocator();

#endif
