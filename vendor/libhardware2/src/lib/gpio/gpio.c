
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <assert.h>

#define CMD_gpio_set_func                _IO('G', 120)
#define CMD_gpio_get_func                _IO('G', 121)
#define CMD_gpio_get_value               _IO('G', 122)
#define CMD_gpio_get_help                _IO('G', 123)
#define CMD_gpio_get_func2               _IO('G', 124)
#define CMD_gpio_set_0                   _IO('G', 125)
#define CMD_gpio_set_1                   _IO('G', 126)
#define CMD_gpio_set_strength            _IO('G', 127)
#define CMD_gpio_get_strength            _IO('G', 128)
#define CMD_gpio_set_slew_rate           _IO('G', 129)
#define CMD_gpio_get_slew_rate           _IO('G', 130)
#define CMD_gpio_set_schmitt             _IO('G', 131)
#define CMD_gpio_get_schmitt             _IO('G', 132)

static inline void gpio_err(const char *err_msg)
{
    fprintf(stderr, "gpio: failed to %s, %s\n", err_msg, strerror(errno));
}

int gpio_open(void)
{
    int fd = open("/dev/gpio", O_RDWR);
    if (fd < 0)
        gpio_err("open device");

    return fd;
}

int gpio_close(int fd)
{
    return close(fd);
}

int gpio_set_func(int fd, const char *gpio, char *funcs[], unsigned int func_count)
{
    unsigned long data[func_count+2];
    data[0] = (unsigned long)gpio;
    data[1] = func_count;

    int i;
    for (i = 0; i < func_count; i++)
        data[2 + i] = (unsigned long)funcs[i];

    int ret = ioctl(fd, CMD_gpio_set_func, (unsigned long)data);
    if (ret < 0)
        gpio_err("set_func");

    return ret;
}

int gpio_get_func(int fd, const char *gpio, char *buf, int buf_size)
{
    unsigned long data[3];
    data[0] = (unsigned long)gpio;
    data[1] = (unsigned long)buf;
    data[2] = buf_size;

    if (buf_size < 1)
        return -EINVAL;

    buf[0] = 0;
    int ret = ioctl(fd, CMD_gpio_get_func, (unsigned long)data);
    if (ret < 0)
        gpio_err("get_func");

    return ret;
}

int gpio_get_func2(int fd, const char *gpio, char *buf[], int buf_count, int buf_size)
{
    unsigned long data[4];
    data[0] = (unsigned long)gpio;
    data[1] = (unsigned long)buf;
    data[2] = buf_count;
    data[3] = buf_size;

    if (buf_count < 1)
        return -EINVAL;

    if (buf_size < 1)
        return -EINVAL;

    int i;
    for (i = 0; i < buf_count; i++)
        buf[i][0] = 0;

    int ret = ioctl(fd, CMD_gpio_get_func2, (unsigned long)data);
    if (ret < 0)
        gpio_err("get_func2");

    return ret;
}

int gpio_set_strength(int fd, const char *gpio, int value)
{
    unsigned long data[2];
    data[0] = (unsigned long)gpio;
    data[1] = value;

    int ret = ioctl(fd, CMD_gpio_set_strength, (unsigned long)data);
    if (ret < 0)
        gpio_err("set_strength");

    return ret;
}

int gpio_get_strength(int fd, const char *gpio)
{
    int ret = ioctl(fd, CMD_gpio_get_strength, (unsigned long)gpio);
    if (ret < 0)
        gpio_err("get_strength");

    return ret;
}

int gpio_set_slew_rate(int fd, const char *gpio, int value)
{
    unsigned long data[2];
    data[0] = (unsigned long)gpio;
    data[1] = value;

    int ret = ioctl(fd, CMD_gpio_set_slew_rate, (unsigned long)data);
    if (ret < 0)
        gpio_err("set_slew_rate");

    return ret;
}

int gpio_get_slew_rate(int fd, const char *gpio)
{
    int ret = ioctl(fd, CMD_gpio_get_slew_rate, (unsigned long)gpio);
    if (ret < 0)
        gpio_err("get_slew_rate");

    return ret;
}

int gpio_set_schmitt(int fd, const char *gpio, int value)
{
    unsigned long data[2];
    data[0] = (unsigned long)gpio;
    data[1] = value;

    int ret = ioctl(fd, CMD_gpio_set_schmitt, (unsigned long)data);
    if (ret < 0)
        gpio_err("set_schmitt");

    return ret;
}

int gpio_get_schmitt(int fd, const char *gpio)
{
    int ret = ioctl(fd, CMD_gpio_get_schmitt, (unsigned long)gpio);
    if (ret < 0)
        gpio_err("get_schmitt");

    return ret;
}

int gpio_get_value(int fd, const char *gpio)
{
    int ret = ioctl(fd, CMD_gpio_get_value, (unsigned long)gpio);
    if (ret < 0)
        gpio_err("get_value");

    return ret;
}

int gpio_set_value(int fd, const char *gpio, int value)
{
    long cmd = value ? CMD_gpio_set_1 : CMD_gpio_set_0;
    int ret = ioctl(fd, cmd, (unsigned long)gpio);
    if (ret < 0)
        gpio_err("set_value");

    return ret;
}

int gpio_get_help(int fd, char *buf, unsigned int buf_size)
{
    unsigned long data[2];
    data[0] = (unsigned long)buf;
    data[1] = buf_size;

    buf[0] = 0;
    int ret = ioctl(fd, CMD_gpio_get_help, (unsigned long)data);
    if (ret < 0)
        gpio_err("get_help");

    return ret;
}
