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

#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"

struct LinearAllocator {
	struct Allocator base;
	struct MemoryArena* arena;
	void* free;
	void* last;
	void* end;
};

struct LinearAllocatorMarker {
	ptrdiff_t pos;
	ptrdiff_t last;
};

void initLinearAllocator(struct LinearAllocator* allocator,
                         struct MemoryArena* arena);

void cleanupLinearAllocator(struct LinearAllocator* allocator);

struct LinearAllocatorMarker markLinearAllocatorState(
    struct LinearAllocator* allocator);

void resetLinearAllocatorState(struct LinearAllocator* allocator,
                               struct LinearAllocatorMarker* state);

void resetLinearAllocator(struct LinearAllocator* allocator);

void* allocateLinear(struct LinearAllocator* allocator, size_t size,
                     size_t power_of_two_alignment);

void* reallocateLinear(struct LinearAllocator* allocator, void* ptr,
                       size_t new_size, size_t power_of_two_alignment);

#endif
