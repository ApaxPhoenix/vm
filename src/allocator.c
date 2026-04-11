#include "allocator.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <assert.h>

Allocator *allocator_create(size_t capacity) {
    Allocator *allocator = malloc(sizeof(Allocator));
    assert(allocator != NULL);
    if (allocator == NULL) return NULL;
    allocator->consumed = 0;
    allocator->capacity = capacity;
    return allocator;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator == NULL) return;
    free(allocator);
}

void *allocator_func(void *context, void *pointer, size_t previous, size_t next) {
    assert(context != NULL);
    if (context == NULL) return NULL;

    Allocator *allocator = context;

    if (pointer != NULL && previous > 0) {
        size_t sub = previous <= allocator->consumed ? previous : allocator->consumed;
        allocator->consumed -= sub;
    }

    if (next == 0) {
        free(pointer);
        return NULL;
    }

    if (next > allocator->capacity - allocator->consumed) {
        fprintf(stderr, "allocator: out of memory next=%zu consumed=%zu capacity=%zu\n", next, allocator->consumed, allocator->capacity);
        return NULL;
    }

    void *block = realloc(pointer, next);
    if (block == NULL) {
        fprintf(stderr, "allocator: realloc failed next=%zu\n", next);
        return NULL;
    }

    allocator->consumed += next;
    return block;
}