#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <libhardware2/watchdog.h>
#include <errno.h>
#include <string.h>

#define WATCHDOG_MAGIC_NUMBER   'W'
#define WATCHDOG_START              _IOW(WATCHDOG_MAGIC_NUMBER, 13, unsigned long)
#define WATCHDOG_STOP               _IO(WATCHDOG_MAGIC_NUMBER, 14)
#define WATCHDOG_FEED               _IO(WATCHDOG_MAGIC_NUMBER, 15)
#define WATCHDOG_RESET              _IO(WATCHDOG_MAGIC_NUMBER, 16)

int watchdog_start(unsigned long ms)
{
    int fd, ret = 0;

    fd = open("/dev/jz_watchdog", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "WATCHDOG: open dev failed: %s!\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, WATCHDOG_START, &ms);
    if (ret < 0)
        fprintf(stderr, "WATCHDOG: start failed: %s!\n", strerror(errno));

    close(fd);
    return ret;
}

int watchdog_stop(void)
{
    int fd, ret = 0;

    fd = open("/dev/jz_watchdog", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "WATCHDOG: open dev failed: %s!\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, WATCHDOG_STOP);
    if (ret < 0)
        fprintf(stderr, "WATCHDOG: stop failed: %s!\n", strerror(errno));

    close(fd);
    return ret;
}

int watchdog_feed(void)
{
    int fd, ret = 0;

    fd = open("/dev/jz_watchdog", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "WATCHDOG: open dev failed: %s!\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, WATCHDOG_FEED);
    if (ret < 0)
        fprintf(stderr, "WATCHDOG: feed failed: %s!\n", strerror(errno));

    close(fd);
    return ret;
}

int watchdog_reset(void)
{
    int fd, ret = 0;

    fd = open("/dev/jz_watchdog", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "WATCHDOG: open dev failed: %s!\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, WATCHDOG_RESET);
    if (ret < 0)
        fprintf(stderr, "WATCHDOG: reset failed: %s!\n", strerror(errno));

    close(fd);
    return ret;
}
