#ifndef SANDBOX_H
#define SANDBOX_H

#include "allocator.h"
#include <lua.h>

#define SANDBOX_MEMORY       (64 * 1024 * 1024)
#define SANDBOX_INSTRUCTIONS 10000

typedef struct {
    lua_State *state;
    Allocator *allocator;
} Sandbox;

Sandbox *sandbox_create(void);

void sandbox_destroy(Sandbox *sandbox);

int sandbox_run(Sandbox *sandbox, const char *script);

#endif
