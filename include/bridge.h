#ifndef BRIDGE_H
#define BRIDGE_H

#include "sandbox.h"

#define QUEUE_MAX 256

typedef struct {
    char   name[64];
    char   payload[1024];
    int    argc;
} Message;

typedef struct {
    Message messages[QUEUE_MAX];
    int     head;
    int     tail;
    int     count;
} Queue;

typedef struct {
    Sandbox      *server;
    Sandbox      *client;
    Queue  server_to_client;
    Queue  client_to_server;
} Bridge;

Bridge *bridge_create(Sandbox *server, Sandbox *client);
void    bridge_destroy(Bridge *bridge);
void    bridge_register(Bridge *bridge, Sandbox *sandbox);
void    bridge_fire(Bridge *bridge, Sandbox *from, const char *name, const char *payload);
void    bridge_deliver(Bridge *bridge);

#endif