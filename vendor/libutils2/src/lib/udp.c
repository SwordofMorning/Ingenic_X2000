

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>

#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/un.h>

#include <poll.h>

#include <semaphore.h>
#include <time.h>

#include "libutils2/udp.h"

int udp_open_socket(struct udp *udp, int address_family, int protocol)
{
    memset(udp, 0, sizeof(*udp));

    udp->address_family = address_family;
    udp->socket_fd = socket(address_family, SOCK_DGRAM, protocol);
    if (udp->socket_fd < 0) {
        fprintf(stderr, "udp: failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    int reuse_socket = 1;
    setsockopt(udp->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(reuse_socket));
    int reuse_port = 1;
    setsockopt(udp->socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));

    return 0;
}

void udp_close_socket(struct udp *udp)
{
    close(udp->socket_fd);
    udp->socket_fd = -1;
}

int udp_bind(struct udp *udp, struct udp_addr *c)
{
    struct udp_sockaddr addr = {0};

    int addrlen = 0;
    if (udp->address_family == AF_INET) {
        addr.inet.sin_family = udp->address_family;
        addr.inet.sin_port = htons(c->inet.port);
        addr.inet.sin_addr.s_addr = c->inet.ip ? inet_addr(c->inet.ip) : htonl(INADDR_ANY);
        addrlen = sizeof(addr.inet);
    }
    if (udp->address_family == AF_LOCAL ||
        udp->address_family == AF_UNIX) {
        addr.local.sun_family = udp->address_family;
        strncpy(addr.local.sun_path, c->local.socket_path, sizeof(addr.local.sun_path));
        addrlen = sizeof(addr.local);
        unlink(addr.local.sun_path);
    }

    if (!addrlen) {
        fprintf(stderr, "udp: not support this socket type: %d\n", udp->address_family);
        return -1;
    }

    int ret = bind(udp->socket_fd, &addr.addr, addrlen);
    if (ret < 0) {
        fprintf(stderr, "udp: failed to bind socket: %s\n", strerror(errno));
        return -1;
    }

    udp->addr = addr;
    udp->c = *c;

    return 0;
}

int udp_set_send_buf_size(struct udp *udp, int buf_size)
{
    if (setsockopt(udp->socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) < 0) {
        fprintf(stderr, "udp: failed to set send buf size: %s\n", strerror(errno));
        return -1;
    }
    unsigned int len = sizeof(buf_size);
    if (getsockopt(udp->socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, &len) < 0) {
        fprintf(stderr, "udp: failed to get recv buf size\n");
        return -1;
    }
    return buf_size;
}

int udp_set_recv_buf_size(struct udp *udp, int buf_size)
{
    if (setsockopt(udp->socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) < 0) {
        fprintf(stderr, "udp: failed to set recv buf size: %s\n", strerror(errno));
        return -1;
    }
    unsigned int len = sizeof(buf_size);
    if (getsockopt(udp->socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, &len) < 0) {
        fprintf(stderr, "udp: failed to get recv buf size\n");
        return -1;
    }
    return buf_size;
}

int udp_send(struct udp *udp, struct udp_sockaddr *addr, uint8_t *data, int size)
{
    return sendto(udp->socket_fd, data, size, 0, &addr->addr, addr->len);
}

int udp_recv(struct udp *udp, struct udp_sockaddr *addr, uint8_t *buf, int size, int timeout_msecs)
{
    struct pollfd fds[1] = {{0}};
    fds[0].fd = udp->socket_fd;
    fds[0].events = POLLIN;

    int ret = poll(fds, 1, timeout_msecs);
    if (!(fds[0].revents & POLLIN)) {
        if (errno != EAGAIN && ret) {
            fprintf(stderr, "udp: poll error: %s", strerror(errno));
            return -1;
        }
        return 0;
    }

    addr->len = udp->addr.len;

    return recvfrom(udp->socket_fd, buf, size, 0, &addr->addr, &addr->len);
}

int udp_sockaddr_init(struct udp_sockaddr *addr, int address_family, struct udp_addr *c)
{
    int ret = -1;
    bzero(addr, sizeof(*addr));

    if (address_family == AF_INET) {
        addr->len = sizeof(addr->inet);
        addr->inet.sin_family = address_family;
        addr->inet.sin_port = htons(c->inet.port);
        addr->inet.sin_addr.s_addr = inet_addr(c->inet.ip);
        ret = addr->inet.sin_addr.s_addr == INADDR_NONE ? -1 : 0;
    }

    if (address_family == AF_LOCAL || address_family == AF_UNIX) {
        addr->len = sizeof(addr->local);
        addr->local.sun_family = address_family;
        strncpy(addr->local.sun_path, c->local.socket_path, sizeof(addr->local.sun_path));
        ret = 0;
    }

    return ret;
}

int udp_sockaddr_equal(struct udp_sockaddr *s0, struct udp_sockaddr *s1)
{
    return !memcmp(&s0->addr, &s1->addr, s0->len);
}
