
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libutils2/udp.h"
#include "libutils2/boot_time.h"

static void print_speed(const char *tag, uint32_t period_us, uint64_t *old, uint64_t *total, int sz)
{
    if (sz > 0) {
        *total += sz;
        if (*old == 0)
            *old = boot_time_usecs();
        uint64_t now = boot_time_usecs();
        if (now - *old >= period_us) {
            double s = (*total/((now-*old)/(1000*1000.0)))/(1024.0*1024);
            printf("%s: %0.03f MB/s\n", tag, s);
            *old = now;
            *total = 0;
        }
    } else {
        *old = 0;
        *total = 0;
    }
}

static int parse_uint(const char *str, const char *prefix, unsigned int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid int: %s\n", str);
        return 0;
    }

    *value = v;
    return 1;
}

static int parse_uint16(const char *str, const char *prefix, unsigned short *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid int: %s\n", str);
        return -1;
    }

    *value = v;
    return 1;
}

static const char *parse_str(const char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}

int main(int argc, char *argv[])
{
    int address_family = 0;
    if (!argv[1] || !argv[2]) {
        fprintf(stderr, "udp: too few args\n");
        return -1;
    }

    if (!strcmp(argv[1], "unix"))
        address_family = AF_UNIX;
    if (!strcmp(argv[1], "local"))
        address_family = AF_LOCAL;
    if (!strcmp(argv[1], "inet"))
        address_family = AF_INET;
    if (address_family == 0) {
        fprintf(stderr, "udp: not support this socket type: %s\n", argv[1]);
        return -1;
    }

    int is_write = 0;
    int is_read = 0;
    if (!strcmp(argv[2], "write"))
        is_write = 1;
    if (!strcmp(argv[2], "read"))
        is_read = 1;
    if (!is_write && !is_read) {
        fprintf(stderr, "udp: not support this trans type: %s\n", argv[2]);
        return -1;
    }

    struct udp_addr m_addr;
    struct udp_addr target_addr;
    uint32_t mtu_size = 1400;
    uint32_t period_us = 1000*1000;

    const char *str;
    int i;
    for (i = 3; i < argc; i++) {
        if (parse_uint(argv[i], "mtu=", &mtu_size, 10))
            continue;
        if (parse_uint(argv[i], "report_usecs=", &period_us, 10))
            continue;
    }

    if (address_family == AF_UNIX || address_family == AF_LOCAL) {
        m_addr.local.socket_path = is_write ? "/tmp/unix_socket.from" : "/tmp/unix_socket.to";
        target_addr.local.socket_path  = is_write ? "/tmp/unix_socket.to" : "/tmp/unix_socket.from";

        for (i = 3; i < argc; i++) {
            if ((str = parse_str(argv[i], "path="))) {
                m_addr.local.socket_path = str;
                continue;
            }
            if ((str = parse_str(argv[i], "to_path="))) {
                target_addr.local.socket_path = str;
                continue;
            }
        }
    }
    if (address_family == AF_INET) {
        m_addr.inet.port = is_write ? 9018 : 9019;
        target_addr.inet.port = is_write ? 9019 : 9018;
        target_addr.inet.ip = "127.0.0.1";

        for (i = 3; i < argc; i++) {
            if (parse_uint16(argv[i], "port=", &m_addr.inet.port, 10))
                continue;
            if ((str = parse_str(argv[i], "ip="))) {
                m_addr.inet.ip = str;
                continue;
            }
            if (parse_uint16(argv[i], "to_port=", &target_addr.inet.port, 10))
                continue;
            if ((str = parse_str(argv[i], "to_ip="))) {
                target_addr.inet.ip = str;
                continue;
            }
        }
    }

    struct udp udp;
    if (udp_open_socket(&udp, address_family, 0))
        return -1;

    if (udp_bind(&udp, &m_addr)) {
        udp_close_socket(&udp);
        return -1;
    }

    struct udp_sockaddr addr;
    if (udp_sockaddr_init(&addr, address_family, &target_addr)) {
        fprintf(stderr, "udp: failed to init addr\n");
        return -1;
    }

    uint64_t total = 0;
    uint64_t old = 0;

    printf("------------------\n");

    int ret;
    while (1) {
        uint8_t data[mtu_size];
        if (is_write) {
            ret = udp_send(&udp, &addr, data, sizeof(data));
            if (ret <= 0)
                perror("send");
            print_speed("send", period_us, &old, &total, ret);
        }

        if (is_read) {
            ret = udp_recv(&udp, &addr, data, sizeof(data), 1000);
            print_speed("recv", period_us, &old, &total, ret);
        }
    }

    return 0;
}
