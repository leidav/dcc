#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#define ALLOCATE_TYPE(allocator, num_elements, type)                 \
	((type*)allocate_aligned(allocator, sizeof(type) * num_elements, \
	                         alignof(type)))

/*struct Allocator {
    void* (*allocate)(void* state, size_t size);
    void (*deallocate)(void* state, void* ptr);
};*/

struct LinearAllocator {
	void* start;
	void* free;
	void* end;
	bool memory_owned;
};

/*static inline void* allocate(struct Allocator* allocator, size_t size)
{
    return allocator->allocate(allocator->state, size);
}
static inline void deallocate(struct Allocator* allocator, void* ptr)
{
    allocator->deallocate(allocator->state, ptr);
}*/

int createAllocator(struct LinearAllocator* allocator, size_t size);

int createAllocatorFromBuffer(struct LinearAllocator* allocator, void* buffer,
                              size_t size);

void destroyAllocator(struct LinearAllocator* allocator);

size_t markAllocatorState(struct LinearAllocator* allocator);

void resetAllocatorState(struct LinearAllocator* allocator, size_t pos);

void* allocate(struct LinearAllocator* allocator, size_t size);

void* allocate_aligned(struct LinearAllocator* allocator, size_t size,
                       size_t power_of_two_alignment);

#endif
