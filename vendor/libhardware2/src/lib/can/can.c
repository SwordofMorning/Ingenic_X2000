#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libhardware2/can.h>

#define CMD_can_set_rate                _IOWR('c', 11, int *)
#define CMD_can_set_filter              _IOWR('c', 12, void *)
#define CMD_can_put_filter              _IOWR('c', 13, int *)
#define CMD_can_get_filter              _IO('c', 14)
#define CMD_can_enable                  _IO('c', 15)
#define CMD_can_write                   _IOWR('c', 16, struct can_frame_cfg *)
#define CMD_can_read                    _IOWR('c', 17, struct can_frame_cfg *)
#define CMD_can_disable                 _IO('c', 18)

static inline void can_err(const char *err_msg)
{
    fprintf(stderr, "CAN: failed to %s, %s\n", err_msg, strerror(errno));
}

int can_set_rate(int fd, int rate)
{
    int ret = ioctl(fd, CMD_can_set_rate, &rate);
    if (ret < 0)
        can_err("set_rate");

    return ret;
}

int can_set_filter(int fd, int filter_num, int filter_id)
{
    unsigned long array[2] = {
        (unsigned long)filter_num,
        (unsigned long)filter_id
    };

    int ret = ioctl(fd, CMD_can_set_filter, array);
    if (ret < 0)
        can_err("set_filter");

    return ret;
}

int can_put_filter(int fd, int filter_num)
{
    int ret = ioctl(fd, CMD_can_put_filter, &filter_num);
    if (ret < 0)
        can_err("put_filter");

    return ret;
}

int can_get_filter(int fd)
{
    int ret = ioctl(fd, CMD_can_get_filter);
    if (ret < 0)
        can_err("get_filter");

    return ret;
}

int can_enable(int fd)
{
    int ret = ioctl(fd, CMD_can_enable);
    if (ret < 0)
        can_err("enable");

    return ret;
}

int can_write(int fd, struct can_frame_cfg *cfg)
{
    int ret = ioctl(fd, CMD_can_write, cfg);
    if (ret < 0)
        can_err("write");

    return ret;
}

int can_read(int fd, struct can_frame_cfg *cfg)
{
    int ret = ioctl(fd, CMD_can_read, cfg);
    if (ret < 0)
        can_err("read");

    return ret;
}

static void can_dump_frame(struct can_frame_cfg *cfg)
{
    int i;
    printf("%s - %s - ", (cfg->mode == CAN_EXTENDED) ? "extended" : "standard",
        (cfg->type == CAN_REMOTE_REQUEST) ? "request" : "data");

    if (cfg->mode == CAN_EXTENDED)
        printf("id: 0x%08x, ", cfg->frm_id);
    else
        printf("id: 0x%03x, ", cfg->frm_id);

    printf("len: %d, data: ", cfg->len);

    for (i = 0; i < cfg->len; i++)
        printf("0x%02x ", cfg->data[i]);
    printf("\n");
}

int can_dump(int fd)
{
    struct can_frame_cfg cfg;
    int ret, size = sizeof(struct can_frame_cfg);

    while (1) {
        memset(&cfg, 0, size);
        ret = ioctl(fd, CMD_can_read, &cfg);
        if (ret >= 0) {
            can_dump_frame(&cfg);
            continue;
        }

        if (errno != ETIMEDOUT)
            break;
    }

    return ret;
}

int can_disable(int fd)
{
    int ret = ioctl(fd, CMD_can_disable);
    if (ret < 0)
        can_err("disable");

    return ret;
}

int can_open(char *dev_path)
{
    int fd = open(dev_path, O_RDWR);
    if (fd < 0)
        can_err("open dev");

    return fd;
}

void can_close(int fd)
{
    close(fd);
}
