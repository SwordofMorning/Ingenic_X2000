#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <libhardware2/sslv.h>

#define MCU_MAGIC_NUMBER    'S'

#define CMD_enable             _IOW(MCU_MAGIC_NUMBER, 110, void *)
#define CMD_disable            _IOW(MCU_MAGIC_NUMBER, 111, void *)
#define CMD_receive            _IOW(MCU_MAGIC_NUMBER, 112, void *)
#define CMD_send               _IOW(MCU_MAGIC_NUMBER, 113, void *)
#define CMD_get_info           _IOW(MCU_MAGIC_NUMBER, 114, void *)
#define CMD_set_mode           _IOW(MCU_MAGIC_NUMBER, 115, void *)
#define CMD_set_bits           _IOW(MCU_MAGIC_NUMBER, 116, void *)

static inline void sslv_err(const char *err_msg)
{
    fprintf(stderr, "SSLV: failed to %s, %s\n", err_msg, strerror(errno));
}

int sslv_open(char *sslv_dev_path)
{
    int fd = open(sslv_dev_path, O_RDWR);
    if (fd < 0) {
        sslv_err("open dev");
        return -1;
    }

    return fd;
}

void sslv_close(int fd)
{
    close(fd);
}

int sslv_enable(int fd)
{
    int ret;
    ret = ioctl(fd, CMD_enable);
    if (ret < 0)
        sslv_err("enable");

    return ret;
}

int sslv_disable(int fd)
{
    int ret;
    ret = ioctl(fd, CMD_disable);
    if (ret < 0)
        sslv_err("disable");

    return ret;
}

int sslv_receive(int fd, void *buf, int size)
{
    int ret;
    unsigned long array[2] = {
        (unsigned long)buf, size
    };

    ret = ioctl(fd, CMD_receive, array);
    if (ret < 0)
        sslv_err("receive");

    return ret;
}

int sslv_send(int fd, void *buf, int size, char add_zero)
{
    int ret;
    unsigned long array[3] = {
        (unsigned long)buf, size, !add_zero,
    };

    ret = ioctl(fd, CMD_send, array);
    if (ret < 0)
        sslv_err("send");

    return ret;
}

void sslv_get_info(int fd, struct sslv_config_data *info)
{
    assert(info);

    ioctl(fd, CMD_get_info, info);
}

int sslv_set_mode(int fd, int mode)
{
    int ret = 0;
    ret = ioctl(fd, CMD_set_mode, &mode);
    if (ret < 0)
        sslv_err("set mode");

    return ret;
}

int sslv_set_bits(int fd, int bits)
{
    int ret = 0;
    ret = ioctl(fd, CMD_set_bits, &bits);
    if (ret < 0)
        sslv_err("set bits");

    return ret;
}
