#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#define SCRATCHPAD_SIZE 4096 << 8
struct LinearAllocator;

int scratchpadInit();
struct LinearAllocator* getScratchpadAllocator();

#endif
