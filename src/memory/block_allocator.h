/*	Copyright (C) 2021 David Leiter
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "allocator.h"

struct Block;

struct BlockAllocator {
	struct Allocator base;
	struct MemoryArena* arena;
	struct Block* next_free;
	unsigned char* end;
	unsigned char* brk;
	size_t block_size;
};

void initBlockAllocator(struct BlockAllocator* allocator,
                        struct MemoryArena* arena, size_t block_size,
                        size_t block_alignment);

void* allocateBlock(struct BlockAllocator* allocator);

void deallocateBlock(struct BlockAllocator* allocator, void* ptr);

#endif
