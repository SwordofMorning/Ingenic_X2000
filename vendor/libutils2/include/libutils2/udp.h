#ifndef _LIBUTILS2_UDP_H_
#define _LIBUTILS2_UDP_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/un.h>
#include <netinet/in.h>

struct udp_addr {
    union {
        // address_family = AF_INET
        struct {
            uint16_t port;
            const char *ip;
        } inet;

        // address_family = AF_UNIX 或 AF_LOCAL
        struct {
            const char *socket_path;
        } local;
    };
};

struct udp_sockaddr {
    socklen_t len;
    union {
        struct sockaddr addr;

        struct sockaddr_in inet;
        struct sockaddr_un local;
    };
};

struct udp {
    int socket_fd;
    int address_family;
    struct udp_addr c;
    struct udp_sockaddr addr;
};

/**
 * 打开 udp 套接字
 * udp->socket_fd = socket(address_family, SOCK_DGRAM, protocol);
 * 默认设置 reuse_addr 和 reuse_port 为 1
 * @param udp udp 结构体指针
 * @param address_family 见 man socket 中 domain 参数的说明 (AF_INET AF_UNIX AF_LOCAL ...)
 * @param protocol 见 man socket 中 protocol 参数的说明
 * @return 0 表示成功  < 0 表示失败
 */
int udp_open_socket(struct udp *udp, int address_family, int protocol);

/**
 * 关闭 udp 套接字
 * @param udp udp 结构体指针
 */
void udp_close_socket(struct udp *udp);

/**
 * 设置发送缓冲区大小,可以将发送缓冲区设置的比较小,以减少延迟
 * @param udp udp 结构体指针
 * @return < 0 设置失败, >=0 设置之后发送buf的大小
 */
int udp_set_send_buf_size(struct udp *udp, int buf_size);

/**
 * 设置接收缓冲区大小,可以将发送缓冲区设置的比较小,以减少延迟
 * @param udp udp 结构体指针
 * @return < 0 设置失败, >=0 设置之后发送buf的大小
 */
int udp_set_recv_buf_size(struct udp *udp, int buf_size);

/**
 * bind udp 地址/端口
 * @param udp udp 结构体指针
 * @return 0 成功, < 0 表示失败
 */
int udp_bind(struct udp *udp, struct udp_addr *c);

/**
 * udp 发送数据
 * @param udp udp 结构体指针
 * @param addr udp 目标地址, 由 udp_sockaddr_init 初始化
 * @param data 发送的数据指针
 * @param size 发送的数据大小
 * @return <0 发送失败, >= 0 表示发送的数据大小
 */
int udp_send(struct udp *udp, struct udp_sockaddr *addr, uint8_t *data, int size);

/**
 * udp 接收数据
 * @param udp udp 结构体指针
 * @param addr udp 目标地址, 由 udp_sockaddr_init 初始化
 * @param data 接收的buf指针
 * @param size 接收的buf大小
 * @return <0 发送失败, >= 0 表示接收到的数据大小
 */
int udp_recv(struct udp *udp, struct udp_sockaddr *addr, uint8_t *buf, int size, int timeout_msecs);

/**
 * 初始化 udp sockaddr
 * @param addr 需要被初始化的 udp sockaddr
 * @param address_family 见 man socket 中 domain 参数的说明 (AF_INET AF_UNIX AF_LOCAL ...)
 * @param c udp 地址参数
 * @return <0 初始化失败,  0 表示成功
 */
int udp_sockaddr_init(struct udp_sockaddr *addr, int address_family, struct udp_addr *c);

/**
 * 判断 sockaddr 是否相等
 * @param s0 需要比较的 udp sockaddr 0
 * @param s1 需要比较的 udp sockaddr 1
 * @return 1 表示相等 0 不相等
 */
int udp_sockaddr_equal(struct udp_sockaddr *s0, struct udp_sockaddr *s1);

#endif /* _LIBUTILS2_UDP_H_ */
