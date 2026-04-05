#include "allocator.h"
#include <stdlib.h>
#include <stdio.h>

Allocator *allocator_create(const size_t capacity) {
    fprintf(stdout, "%s: creating allocator with capacity %zu bytes\n", __FILE__, capacity);
    Allocator *allocator = malloc(sizeof(Allocator));
    if (allocator == NULL) {
        fprintf(stderr, "%s: failed to allocate allocator\n", __FILE__);
        return nullptr;
    }
    allocator->consumed = 0;
    allocator->capacity = capacity;
    fprintf(stdout, "%s: allocator ready\n", __FILE__);
    return allocator;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator == NULL) return;
    fprintf(stdout, "%s: destroying allocator (consumed: %zu, capacity: %zu)\n", __FILE__, allocator->consumed,
            allocator->capacity);
    free(allocator);
    fprintf(stdout, "%s: allocator destroyed\n", __FILE__);
}

void *allocator_func(void *ud, void *ptr, const size_t osize, const size_t size) {
    Allocator *allocator = ud;

    if (ptr != NULL && osize > 0) {
        if (osize <= allocator->consumed)
            allocator->consumed -= osize;
        else
            allocator->consumed = 0;
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (size > allocator->capacity - allocator->consumed) {
        fprintf(stderr, "%s: out of memory (consumed: %zu, capacity: %zu, requested: %zu)\n",
                __FILE__, allocator->consumed, allocator->capacity, size);
        return NULL;
    }

    allocator->consumed += size;
    return realloc(ptr, size);
}
