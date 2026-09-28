#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <libhardware2/hw_timer.h>
#include <errno.h>
#include <string.h>

#define HW_TIMER_MAGIC_NUMBER           'T'
#define HW_TIMER_START                  _IOW(HW_TIMER_MAGIC_NUMBER, 131, unsigned long)
#define HW_TIMER_STOP                   _IO(HW_TIMER_MAGIC_NUMBER, 132)
#define HW_TIMER_WAIT                   _IOWR(HW_TIMER_MAGIC_NUMBER, 133, unsigned int)

int hw_timer_open(const char *dev_name)
{
    int fd;
    unsigned char path_name[64];

    snprintf(path_name, 63, "/dev/timer_%s", dev_name);

    fd = open(path_name, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "hw_timer: open dev failed: %s!\n", strerror(errno));
        return -1;
    }

    return fd;
}

int hw_timer_close(int dev_fd)
{
    int ret = close(dev_fd);
    if (ret < 0) {
        fprintf(stderr, "hw_timer: close dev failed: %s!\n", strerror(errno));
        return -1;
    }

    return 0;
}

int hw_timer_start(int dev_fd, unsigned long usecs)
{
    int ret = 0;

    ret = ioctl(dev_fd, HW_TIMER_START, &usecs);
    if (ret < 0)
        fprintf(stderr, "hw_timer: start failed: %s!\n", strerror(errno));

    return ret;
}

int hw_timer_stop(int dev_fd)
{
    int ret = 0;

    ret = ioctl(dev_fd, HW_TIMER_STOP);
    if (ret < 0)
        fprintf(stderr, "hw_timer: stop failed: %s!\n", strerror(errno));

    return ret;
}

int hw_timer_wait(int dev_fd)
{
    int ret = 0;

    ret = ioctl(dev_fd, HW_TIMER_WAIT);
    if (ret < 0) {
        fprintf(stderr, "hw_timer: wait failed: %s!\n", strerror(errno));
        return ret;
    }

    return ret;
}
