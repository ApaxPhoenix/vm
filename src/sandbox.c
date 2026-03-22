#include "sandbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luajit.h>

static void governor(lua_State *state, lua_Debug *ar) {
    (void)ar;
    fprintf(stderr, "%s: script exceeded CPU budget, terminating\n", __FILE__);
    luaL_error(state, "vm: script exceeded CPU budget");
}

Sandbox *sandbox_create(void) {
    fprintf(stdout, "%s: creating sandbox\n", __FILE__);

    Sandbox *sandbox = malloc(sizeof(Sandbox));
    if (sandbox == NULL) {
        fprintf(stderr, "%s: failed to allocate sandbox\n", __FILE__);
        return NULL;
    }

    sandbox->allocator = allocator_create(SANDBOX_MEMORY);
    if (sandbox->allocator == NULL) {
        fprintf(stderr, "%s: failed to create allocator\n", __FILE__);
        free(sandbox);
        return NULL;
    }
    fprintf(stdout, "%s: allocator created with %d MB cap\n", __FILE__, SANDBOX_MEMORY / 1024 / 1024);

    sandbox->state = lua_newstate(allocator_func, sandbox->allocator);
    if (sandbox->state == NULL) {
        fprintf(stderr, "%s: failed to create lua state\n", __FILE__);
        allocator_destroy(sandbox->allocator);
        free(sandbox);
        return NULL;
    }
    fprintf(stdout, "%s: lua state created\n", __FILE__);

    luaL_openlibs(sandbox->state);
    fprintf(stdout, "%s: standard libs loaded\n", __FILE__);

    if (luaL_dofile(sandbox->state, "scripts/init.lua") != LUA_OK) {
        fprintf(stderr, "%s: init failed: %s\n", __FILE__, lua_tostring(sandbox->state, -1));
        sandbox_destroy(sandbox);
        return NULL;
    }
    fprintf(stdout, "%s: environment locked down\n", __FILE__);

    fprintf(stdout, "%s: sandbox ready\n", __FILE__);
    return sandbox;
}

void sandbox_destroy(Sandbox *sandbox) {
    if (sandbox == NULL) return;
    fprintf(stdout, "%s: destroying sandbox\n", __FILE__);
    if (sandbox->state != NULL) lua_close(sandbox->state);
    allocator_destroy(sandbox->allocator);
    free(sandbox);
    fprintf(stdout, "%s: sandbox destroyed\n", __FILE__);
}

int sandbox_run(Sandbox *sandbox, const char *script) {
    fprintf(stdout, "%s: running script\n", __FILE__);

    lua_State *state = sandbox->state;

    lua_State *thread = lua_newthread(state);

    luaJIT_setmode(thread, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    lua_sethook(thread, governor, LUA_MASKCOUNT, SANDBOX_INSTRUCTIONS);

    if (luaL_loadstring(thread, script) != LUA_OK) {
        fprintf(stderr, "%s: compile error: %s\n", __FILE__, lua_tostring(thread, -1));
        lua_pop(state, 1);
        return 0;
    }

    int result = lua_resume(thread, 0);

    if (result != LUA_OK && result != LUA_YIELD) {
        fprintf(stderr, "%s: runtime error: %s\n", __FILE__, lua_tostring(thread, -1));
        lua_pop(state, 1);
        return 0;
    }

    lua_pop(state, 1);
    fprintf(stdout, "%s: script completed successfully\n", __FILE__);
    return 1;
}