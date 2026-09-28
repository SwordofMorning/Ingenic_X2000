#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <libhardware2/pwm_battery.h>


int pwm_battery_open(void)
{
    int fd = open("/dev/pwm_battery", O_RDWR);

    if (fd < 0) {
        fprintf(stderr, "pwm_battery:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    return fd;
}

int pwm_battery_read(int fd)
{
    int voltage;
    int ret;

    ret = read(fd, &voltage, sizeof(int));
    if (ret < 0) {
        fprintf(stderr, "pwm_battery:read voltage failed: %s\n", strerror(errno));
        return -1;
    }

    return voltage;
}

void pwm_battery_close(int fd)
{
    close(fd);
}