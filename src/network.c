#include "network.h"
#include <libwebsockets.h>
#include <enet/enet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    on_connect connect;
    on_receive receive;
    on_disconnect disconnect;
} SocketSession;

static int socket_callback(struct lws *socket, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len) {
    SocketSession *session = user;
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        if (session && session->connect) session->connect(socket);
        break;
    case LWS_CALLBACK_RECEIVE:
    case LWS_CALLBACK_CLIENT_RECEIVE:
        if (session && session->receive) session->receive(socket, in, len);
        break;
    case LWS_CALLBACK_CLOSED:
        if (session && session->disconnect) session->disconnect(socket, STATUS_CLOSED);
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "game", socket_callback, sizeof(SocketSession), 4096 },
    { NULL, NULL, 0, 0 }
};

Network *network_create(void) {
    if (enet_initialize() != 0) {
        fprintf(stderr, "network: enet init failed\n");
        return NULL;
    }
    Network *network = calloc(1, sizeof(Network));
    if (!network) return NULL;
    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    network->context = lws_create_context(&info);
    if (!network->context) { free(network); return NULL; }
    return network;
}

void network_destroy(Network *network) {
    if (!network) return;
    if (network->context) lws_context_destroy(network->context);
    if (network->host) enet_host_destroy(network->host);
    enet_deinitialize();
    free(network);
}

void network_poll(Network *network) {
    if (!network) return;
    if (network->context)
        lws_service(network->context, 0);
    if (network->host) {
        ENetEvent event;
        while (enet_host_service(network->host, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (network->peer_connect) network->peer_connect(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (network->peer_receive) network->peer_receive(event.peer,
                    (const char *)event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (network->peer_disconnect) network->peer_disconnect(event.peer, STATUS_CLOSED);
                break;
            default:
                break;
            }
        }
    }
}

int network_socket_listen(Network *network, int port,
                  on_connect connect, on_receive receive, on_disconnect disconnect) {
    if (!network) return 0;
    struct lws_context_creation_info info = {0};
    info.port = port;
    info.protocols = protocols;
    network->context = lws_create_context(&info);
    if (!network->context) return 0;
    network->connect = connect;
    network->receive = receive;
    network->disconnect = disconnect;
    return 1;
}

int network_socket_connect(Network *network, const char *host, int port, const char *path,
                   on_connect connect, on_receive receive, on_disconnect disconnect) {
    if (!network || !host) return 0;
    struct lws_client_connect_info info = {0};
    info.context = network->context;
    info.address = host;
    info.port = port;
    info.path = path ? path : "/";
    info.host = host;
    info.origin = host;
    info.protocol = protocols[0].name;
    struct lws *socket = lws_client_connect_via_info(&info);
    if (!socket) return 0;
    SocketSession *session = lws_wsi_user(socket);
    session->connect = connect;
    session->receive = receive;
    session->disconnect = disconnect;
    return 1;
}

int network_socket_send(struct lws *socket, const char *data, size_t size) {
    unsigned char buffer[LWS_PRE + size];
    memcpy(buffer + LWS_PRE, data, size);
    return lws_write(socket, buffer + LWS_PRE, size, LWS_WRITE_TEXT) >= 0;
}

int network_peer_listen(Network *network, int port, int max,
                on_peer_connect connect, on_peer_receive receive, on_peer_disconnect disconnect) {
    if (!network) return 0;
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    network->host = enet_host_create(&address, max, 2, 0, 0);
    if (!network->host) return 0;
    network->peer_connect = connect;
    network->peer_receive = receive;
    network->peer_disconnect = disconnect;
    return 1;
}

int network_peer_connect(Network *network, const char *host, int port,
                 on_peer_connect connect, on_peer_receive receive, on_peer_disconnect disconnect) {
    if (!network || !host) return 0;
    network->host = enet_host_create(NULL, 1, 2, 0, 0);
    if (!network->host) return 0;
    ENetAddress address;
    enet_address_set_host(&address, host);
    address.port = port;
    ENetPeer *peer = enet_host_connect(network->host, &address, 2, 0);
    if (!peer) return 0;
    network->peer_connect = connect;
    network->peer_receive = receive;
    network->peer_disconnect = disconnect;
    return 1;
}

int network_peer_send(ENetPeer *peer, const char *data, size_t size, int reliable) {
    ENetPacket *packet = enet_packet_create(data, size,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (!packet) return 0;
    return enet_peer_send(peer, 0, packet) == 0;
}

void network_peer_disconnect(ENetPeer *peer) {
    enet_peer_disconnect(peer, 0);
}