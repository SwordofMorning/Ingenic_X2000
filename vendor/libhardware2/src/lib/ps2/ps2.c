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

#define MCU_MAGIC_NUMBER    'P'

#define CMD_read_raw_byte           _IOW(MCU_MAGIC_NUMBER, 110, void *)
#define CMD_read_str            _IOW(MCU_MAGIC_NUMBER, 111, void *)
#define CMD_write_raw_byte          _IOW(MCU_MAGIC_NUMBER, 112, void *)
#define CMD_write_str           _IOW(MCU_MAGIC_NUMBER, 113, void *)

int ps2_read_raw_byte_timeout(int fd, unsigned char *buf, int timeout_ms)
{
    unsigned long array[2] = {
        (unsigned long)buf, timeout_ms
    };

    return ioctl(fd, CMD_read_raw_byte, array);
}

int ps2_read_raw_byte(int fd, unsigned char *data)
{
    unsigned char ch;

    int ret = ps2_read_raw_byte_timeout(fd, &ch, 0);
    if (ret < 0)
        return -1;

    *data = ch;

    return 0;
}

int ps2_write_raw_byte(int fd, unsigned char byte)
{
    return ioctl(fd, CMD_write_raw_byte, &byte);
}

int ps2_write_str(int fd, void *buf, int size)
{
    unsigned long array[2] = {
        (unsigned long)buf, size
    };

    return ioctl(fd, CMD_write_str, array);
}

int ps2_open(const char *dev_name)
{
    int fd = open(dev_name, O_RDWR);

    if (fd < 0) {
        fprintf(stderr, "gpio_ps2:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    return fd;
}

void ps2_close(int fd)
{
    close(fd);
}