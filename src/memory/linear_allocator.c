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

#include "linear_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* allocateLinearWrapper(void* allocator, size_t size,
                                   size_t power_of_two_alignment)
{
	return allocateLinear(allocator, size, power_of_two_alignment);
}

static void* reallocateLinearWrapper(void* allocator, void* ptr,
                                     size_t new_size,
                                     size_t power_of_two_alignment)
{
	return reallocateLinear(allocator, ptr, new_size, power_of_two_alignment);
}

static void deallocateNothing(void* allocator, void* ptr)
{
	return;
}

void initLinearAllocator(struct LinearAllocator* allocator,
                         struct MemoryArena* arena)
{
	initAllocator(&allocator->base, allocateLinearWrapper,
	              reallocateLinearWrapper, deallocateNothing);
	allocator->arena = arena;
	void* start = arena->memory;
	allocator->end = start + arena->size;
	allocator->free = start;
	allocator->last = start;
}
void cleanupLinearAllocator(struct LinearAllocator* allocator)
{
	deallocateArena(allocator->arena);
	allocator->arena = NULL;
	allocator->free = NULL;
	allocator->last = NULL;
	allocator->end = NULL;
}

struct LinearAllocatorMarker markLinearAllocatorState(
    struct LinearAllocator* allocator)
{
	void* start = allocator->arena->memory;
	struct LinearAllocatorMarker state = {.pos = allocator->free - start,
	                                      .last = allocator->last - start};
	return state;
}

void resetLinearAllocatorState(struct LinearAllocator* allocator,
                               struct LinearAllocatorMarker* state)
{
	void* start = allocator->arena->memory;
	allocator->free = start + state->pos;
	allocator->last = start + state->last;
}
void resetLinearAllocator(struct LinearAllocator* allocator)
{
	void* start = allocator->arena->memory;
	allocator->free = start;
	allocator->last = start;
}

void* allocateLinear(struct LinearAllocator* allocator, size_t size,
                     size_t power_of_two_alignment)
{
	size_t offset = alignmentOffset(allocator->free, power_of_two_alignment);
	void* aligned = allocator->free + offset;
	void* next_free = aligned + size;
	if (next_free >= allocator->end) {
		return NULL;
	}
	allocator->free = next_free;
	allocator->last = aligned;
	return aligned;
}

void* reallocateLinear(struct LinearAllocator* allocator, void* ptr,
                       size_t new_size, size_t power_of_two_alignment)
{
	if (ptr != allocator->last) {
		// Not supported
		return NULL;
	}
	void* next_free = allocator->last + new_size;
	if (next_free >= allocator->end) {
		return NULL;
	}
	allocator->free = next_free;
	return ptr;
}
