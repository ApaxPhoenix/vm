#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct {
    size_t consumed;
    size_t capacity;
} Allocator;

Allocator *allocator_create(size_t capacity);
void allocator_destroy(Allocator *allocator);
void *allocator_func(void *context, void *pointer, size_t previous, size_t next);

#endif