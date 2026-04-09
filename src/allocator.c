#include "allocator.h"
#include <stdlib.h>
#include <assert.h>

Allocator *allocator_create(size_t capacity) {
    Allocator *allocator = malloc(sizeof(Allocator));
    assert(allocator != nullptr && "failed to allocate allocator");
    allocator->consumed = 0;
    allocator->capacity = capacity;
    return allocator;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator == nullptr) return;
    free(allocator);
}

void *allocator_func(void *ud, void *ptr, size_t osize, size_t size) {
    assert(ud != nullptr && "allocator_func called with null ud");
    Allocator *allocator = ud;

    if (ptr != nullptr && osize > 0)
        allocator->consumed -= osize <= allocator->consumed ? osize : allocator->consumed;

    if (size == 0) { free(ptr); return nullptr; }

    if (size > allocator->capacity - allocator->consumed) return nullptr;

    allocator->consumed += size;
    return realloc(ptr, size);
}