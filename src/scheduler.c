#include "scheduler.h"
#include <luajit.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

Scheduler *scheduler_create(Sandbox *sandbox, Bridge *bridge) {
    assert(sandbox != nullptr && "sandbox is null");
    assert(bridge  != nullptr && "bridge is null");

    Scheduler *scheduler = malloc(sizeof(Scheduler));
    assert(scheduler != nullptr && "failed to allocate scheduler");

    scheduler->sandbox = sandbox;
    scheduler->bridge  = bridge;
    scheduler->count   = 0;

    for (int i = 0; i < MAX_COROUTINES; i++) {
        scheduler->coroutines[i].thread  = nullptr;
        scheduler->coroutines[i].waiting = 0;
    }

    return scheduler;
}

void scheduler_destroy(Scheduler *scheduler) {
    assert(scheduler != nullptr && "scheduler is null");
    free(scheduler);
}

void scheduler_spawn(Scheduler *scheduler, const char *path) {
    assert(scheduler != nullptr && "scheduler is null");
    assert(path      != nullptr && "path is null");
    assert(scheduler->count < MAX_COROUTINES && "coroutine limit reached");

    lua_State *state  = scheduler->sandbox->state;
    lua_State *thread = lua_newthread(state);
    assert(thread != nullptr && "failed to create thread");

    if (scheduler->sandbox->context == CLIENT)
        luaJIT_setmode(thread, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);

    const char *init = scheduler->sandbox->context == SERVER
        ? "vendor/vm/server.lua"
        : "vendor/vm/client.lua";

    if (luaL_loadfile(thread, init) != LUA_OK) {
        fprintf(stderr, "scheduler: init load: %s\n", lua_tostring(thread, -1));
        lua_pop(state, 1);
        return;
    }

    if (lua_pcall(thread, 0, 1, 0) != LUA_OK) {
        fprintf(stderr, "scheduler: init call: %s\n", lua_tostring(thread, -1));
        lua_pop(state, 1);
        return;
    }

    if (lua_pcall(thread, 0, 1, 0) != LUA_OK) {
        fprintf(stderr, "scheduler: isolate: %s\n", lua_tostring(thread, -1));
        lua_pop(state, 1);
        return;
    }

    if (luaL_loadfile(thread, path) != LUA_OK) {
        fprintf(stderr, "scheduler: script load: %s\n", lua_tostring(thread, -1));
        lua_pop(state, 1);
        return;
    }

    lua_pushvalue(thread, -2);
    lua_setfenv(thread, -2);
    lua_remove(thread, -2);

    Coroutine *coroutine = &scheduler->coroutines[scheduler->count++];
    coroutine->thread    = thread;
    coroutine->waiting   = 0;
}

static void tick_coroutines(Scheduler *scheduler) {
    for (int i = 0; i < scheduler->count; i++) {
        Coroutine *coroutine = &scheduler->coroutines[i];
        if (coroutine->thread == nullptr || coroutine->waiting) continue;

        int result = lua_resume(coroutine->thread, 0);

        if (result == LUA_YIELD) {
            coroutine->waiting = 1;
            continue;
        }

        if (result != LUA_OK)
            fprintf(stderr, "scheduler: coroutine error: %s\n", lua_tostring(coroutine->thread, -1));

        scheduler->coroutines[i] = scheduler->coroutines[--scheduler->count];
        i--;
    }
}

void scheduler_tick(Scheduler *scheduler, double delta) {
    assert(scheduler != nullptr && "scheduler is null");
    (void) delta;
    bridge_deliver(scheduler->bridge);
    tick_coroutines(scheduler);
}