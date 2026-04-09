#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "bridge.h"

#define MAX_COROUTINES 256

typedef struct {
    lua_State *thread;
    int        waiting;
} Coroutine;

typedef struct {
    Coroutine  coroutines[MAX_COROUTINES];
    int        count;
    Sandbox   *sandbox;
    Bridge    *bridge;
} Scheduler;

Scheduler *scheduler_create(Sandbox *sandbox, Bridge *bridge);
void       scheduler_destroy(Scheduler *scheduler);
void       scheduler_spawn(Scheduler *scheduler, const char *path);
void       scheduler_tick(Scheduler *scheduler, double delta);

#endif