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

#include "scratchpad.h"

#include "allocator.h"
#include "linear_allocator.h"
#include "stdio.h"

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

void scratchpadCleanup()
{
	cleanupLinearAllocator(&allocator);
}

struct Allocator* getScratchpadAllocator()
{
	return (struct Allocator*)&allocator;
}
