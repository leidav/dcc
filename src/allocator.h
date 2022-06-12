#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#define DEFAULT_ALIGNMENT 8

#define ALLOCATOR_CAST(allocator) ((struct Allocator*)(allocator))

#define ALLOCATE_TYPE(allocator, num_elements, type)                \
	((type*)allocateAligned(allocator, sizeof(type) * num_elements, \
	                        alignof(type)))

struct Allocator {
	void* (*allocate)(void* allocator, size_t size,
	                  size_t power_of_two_alignment);
	void (*deallocate)(void* allocator, void* ptr);
};

struct LinearAllocator {
	struct Allocator base;
	struct Allocator* parent_allocator;
	void* start;
	void* free;
	void* end;
	bool memory_owned;
};

void* allocateAligned(struct Allocator* allocator, size_t size,
                      size_t power_of_two_alignment);
void* allocate(struct Allocator* allocator, size_t size);

void deallocate(struct Allocator* allocator, void* ptr);

int createLinearAllocator(struct LinearAllocator* allocator, size_t size,
                          struct Allocator* parent_allocator);

int createLinearAllocatorFromBuffer(struct LinearAllocator* allocator,
                                    void* buffer, size_t size);

void destroyLinearAllocator(struct LinearAllocator* allocator);

size_t markAllocatorState(struct LinearAllocator* allocator);

void resetAllocatorState(struct LinearAllocator* allocator, size_t pos);

void* allocateLinear(struct LinearAllocator* allocator, size_t size,
                     size_t power_of_two_alignment);

#endif
