#ifndef SANDBOX_H
#define SANDBOX_H

#include "allocator.h"
#include <lua.h>

#define MEMORY       (64 * 1024 * 1024)
#define INSTRUCTIONS 10000

typedef enum {
    SERVER,
    CLIENT,
} Context;

typedef struct {
    lua_State    *state;
    Allocator    *allocator;
    Context context;
} Sandbox;

Sandbox *sandbox_create(Context context);
void     sandbox_destroy(Sandbox *sandbox);
int      sandbox_run(Sandbox *sandbox, const char *path);

#endif