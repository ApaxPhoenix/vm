#ifndef NETWORK_H
#define NETWORK_H

#include <libwebsockets.h>
#include <enet/enet.h>

typedef enum {
    STATUS_OK,
    STATUS_ERROR,
    STATUS_CLOSED,
    STATUS_TIMEOUT
} Status;

typedef void (*on_connect)(struct lws *socket);
typedef void (*on_receive)(struct lws *socket, const char *data, size_t size);
typedef void (*on_disconnect)(struct lws *socket, Status status);

typedef void (*on_peer_connect)(ENetPeer *peer);
typedef void (*on_peer_receive)(ENetPeer *peer, const char *data, size_t size);
typedef void (*on_peer_disconnect)(ENetPeer *peer, Status status);

typedef struct {
    struct lws_context *context;
    ENetHost *host;
    on_connect connect;
    on_receive receive;
    on_disconnect disconnect;
    on_peer_connect peer_connect;
    on_peer_receive peer_receive;
    on_peer_disconnect peer_disconnect;
} Network;

Network *network_create(void);
void network_destroy(Network *network);
void network_poll(Network *network);

int network_socket_listen(Network *network, int port, on_connect connect, on_receive receive, on_disconnect disconnect);
int network_socket_connect(Network *network, const char *host, int port, const char *path, on_connect connect, on_receive receive, on_disconnect disconnect);
int network_socket_send(struct lws *socket, const char *data, size_t size);

int network_peer_listen(Network *network, int port, int max, on_peer_connect connect, on_peer_receive receive, on_peer_disconnect disconnect);
int network_peer_connect(Network *network, const char *host, int port, on_peer_connect connect, on_peer_receive receive, on_peer_disconnect disconnect);
int network_peer_send(ENetPeer *peer, const char *data, size_t size, int reliable);
void network_peer_disconnect(ENetPeer *peer);

#endif