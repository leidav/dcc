#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdalign.h>
#include <stddef.h>

#define ALLOCATE_TYPE(allocator, num_elements, type)                 \
	((type*)allocate_aligned(allocator, sizeof(type) * num_elements, \
	                         alignof(type)))

struct LinearAllocator {
	void* start;
	void* free;
	void* end;
};

int createAllocator(struct LinearAllocator* allocator, size_t size);

void destroyAllocator(struct LinearAllocator* allocator);

size_t markllocatorState(struct LinearAllocator* allocator);

void resetAllocatorState(struct LinearAllocator* allocator, size_t pos);

void* allocate(struct LinearAllocator* allocator, size_t size);

void* allocate_aligned(struct LinearAllocator* allocator, size_t size,
                       size_t power_of_two_alignment);

#endif