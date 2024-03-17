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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#define DEFAULT_ALIGNMENT 16

#define ALLOCATOR_CAST(allocator) ((struct Allocator*)(allocator))

#define ALLOCATE_TYPE(allocator, num_elements, type)   \
	((type*)allocateAligned(ALLOCATOR_CAST(allocator), \
	                        sizeof(type) * num_elements, alignof(type)))

typedef void* (*AllocateFn)(void* allocator, size_t size,
                            size_t power_of_two_alignment);
typedef void* (*ReallocateFn)(void* allocator, void* ptr, size_t new_size,
                              size_t power_of_two_alignment);
typedef void (*DeallocateFn)(void* allocator, void* ptr);

struct Allocator {
	AllocateFn allocate;
	ReallocateFn reallocate;
	DeallocateFn deallocate;
};

struct MemoryArena {
	struct Allocator* owner;
	size_t size;
	unsigned char memory[];
};

static inline size_t alignmentOffset(void* address, size_t alignment)
{
	size_t misalignment = ((size_t)address - 1) & (alignment - 1);
	return alignment - misalignment - 1;
}

struct Allocator* getGlobalAllocator();

struct MemoryArena* allocateArena(struct Allocator* allocator, size_t size);

struct MemoryArena* globallyAllocateArena(size_t size);

struct MemoryArena* createNonOwningArena(void* buffer, size_t buffer_size);

void deallocateArena(struct MemoryArena* arena);

void initAllocator(struct Allocator* allocator, AllocateFn allocate,
                   ReallocateFn reallocate, DeallocateFn deallocate);

void* allocateAligned(struct Allocator* allocator, size_t size,
                      size_t power_of_two_alignment);
void* allocate(struct Allocator* allocator, size_t size);

void* reallocate(struct Allocator* allocator, void* ptr, size_t new_size);

void deallocate(struct Allocator* allocator, void* ptr);

#endif
