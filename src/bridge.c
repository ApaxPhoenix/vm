#include "bridge.h"
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

Bridge *bridge_create(Sandbox *server, Sandbox *client) {
    assert(server != nullptr && "server sandbox is null");
    assert(client != nullptr && "client sandbox is null");

    Bridge *bridge = malloc(sizeof(Bridge));
    assert(bridge != nullptr && "failed to allocate bridge");

    bridge->server                  = server;
    bridge->client                  = client;
    bridge->server_to_client.head   = 0;
    bridge->server_to_client.tail   = 0;
    bridge->server_to_client.count  = 0;
    bridge->client_to_server.head   = 0;
    bridge->client_to_server.tail   = 0;
    bridge->client_to_server.count  = 0;

    return bridge;
}

void bridge_destroy(Bridge *bridge) {
    assert(bridge != nullptr && "bridge is null");
    free(bridge);
}

static void queue_push(Queue *queue, const char *name, const char *payload) {
    assert(queue != nullptr && "queue is null");
    assert(queue->count < QUEUE_MAX && "message queue is full");

    Message *message = &queue->messages[queue->tail];
    strncpy(message->name,    name,    sizeof(message->name)    - 1);
    strncpy(message->payload, payload, sizeof(message->payload) - 1);
    message->name[sizeof(message->name)       - 1] = '\0';
    message->payload[sizeof(message->payload) - 1] = '\0';

    queue->tail  = (queue->tail + 1) % QUEUE_MAX;
    queue->count++;
}

static Message *queue_pop(Queue *queue) {
    assert(queue != nullptr && "queue is null");
    if (queue->count == 0) return nullptr;

    Message *message = &queue->messages[queue->head];
    queue->head  = (queue->head + 1) % QUEUE_MAX;
    queue->count--;
    return message;
}

static void deliver_to(Sandbox *sandbox, const char *name, const char *payload) {
    assert(sandbox != nullptr && "sandbox is null");

    lua_State *state = sandbox->state;

    lua_getglobal(state, "Signal");
    if (!lua_istable(state, -1)) { lua_pop(state, 1); return; }

    lua_getfield(state, -1, "Fire");
    if (!lua_isfunction(state, -1)) { lua_pop(state, 2); return; }

    lua_remove(state, -2);
    lua_pushstring(state, name);
    lua_pushstring(state, payload);

    if (lua_pcall(state, 2, 0, 0) != LUA_OK) {
        fprintf(stderr, "bridge: deliver error [%s]: %s\n", name, lua_tostring(state, -1));
        lua_pop(state, 1);
    }
}

static int lua_bridge_fire(lua_State *state) {
    assert(lua_gettop(state) >= 3 && "bridge.fire expects bridge, name, payload");

    Bridge     *bridge  = lua_touserdata(state, lua_upvalueindex(1));
    Sandbox    *sandbox = lua_touserdata(state, lua_upvalueindex(2));
    const char *name    = luaL_checkstring(state, 1);
    const char *payload = luaL_checkstring(state, 2);

    assert(bridge  != nullptr && "bridge upvalue is null");
    assert(sandbox != nullptr && "sandbox upvalue is null");

    Queue *queue = sandbox == bridge->server
        ? &bridge->server_to_client
        : &bridge->client_to_server;

    queue_push(queue, name, payload);
    return 0;
}

void bridge_register(Bridge *bridge, Sandbox *sandbox) {
    assert(bridge  != nullptr && "bridge is null");
    assert(sandbox != nullptr && "sandbox is null");

    lua_State *state = sandbox->state;

    lua_pushlightuserdata(state, bridge);
    lua_pushlightuserdata(state, sandbox);
    lua_pushcclosure(state, lua_bridge_fire, 2);
    lua_setglobal(state, "_bridge_fire");
}

void bridge_deliver(Bridge *bridge) {
    assert(bridge != nullptr && "bridge is null");

    Queue *to_client = &bridge->server_to_client;
    Queue *to_server = &bridge->client_to_server;

    Message *message;
    while ((message = queue_pop(to_client)) != nullptr)
        deliver_to(bridge->client, message->name, message->payload);

    while ((message = queue_pop(to_server)) != nullptr)
        deliver_to(bridge->server, message->name, message->payload);
}