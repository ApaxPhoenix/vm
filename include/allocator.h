#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct {
    size_t consumed;
    size_t capacity;
} Allocator;

Allocator *allocator_create(size_t capacity);

void allocator_destroy(Allocator *pointer);

void *allocator_func(void *ud, void *ptr, size_t osize, size_t size);

#endif
