#ifndef SANDBOX_H
#define SANDBOX_H

#include <lua.h>
#include "allocator.h"

#define SANDBOX_MEMORY       (64 * 1024 * 1024)
#define SANDBOX_INSTRUCTIONS 1000000
#define MAX_COROUTINES       16
#define MAX_STRING           (1024 * 1024)

#ifndef VM_PATH
#error  "VM_PATH must be defined"
#endif
#define SANDBOX_PATH(x) VM_PATH "/" x

typedef enum {
    SANDBOX_SERVER,
    SANDBOX_CLIENT
} Context;

typedef struct {
    Context context;
    Allocator *allocator;
    lua_State *state;
} Sandbox;

Sandbox *sandbox_create(Context context);
void sandbox_destroy(Sandbox *sandbox);
int sandbox_run(Sandbox *sandbox, const char *path);

#endif