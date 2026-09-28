#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>

#include <libhardware2/nemc.h>

#define CMD_NEMC_SET_TIME                _IOWR('S', 120, struct nemc_timing *)
#define CMD_NEMC_GET_TIME                 _IOWR('S', 122, struct nemc_timing *)

#define NEMC_MAX_addr_width  13
#define NEMC_MAX_buswidth  16

#define NEMC_MAX_SIZE   (1 << NEMC_MAX_addr_width) * NEMC_MAX_buswidth / 8


static inline void nemc_err(const char *err_msg)
{
    fprintf(stderr, "nemc: failed to %s, %s\n", err_msg, strerror(errno));
}

int nemc_open(const char *dev_path, void **mem)
{
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        nemc_err("open");
        return -1;
    }

    void *base = mmap(NULL, NEMC_MAX_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!base) {
        nemc_err("mmap");
        close(fd);
        return -1;
    }

    *mem = base;

    return fd;
}

int nemc_set_timing(int fd, struct nemc_timing *timing)
{
    int ret;
    ret = ioctl(fd, CMD_NEMC_SET_TIME, timing);
    if (ret)
        nemc_err("set_timing");

    return ret;
}

int nemc_get_timeing(int fd, struct nemc_timing *timing)
{
    int ret;
    ret = ioctl(fd, CMD_NEMC_GET_TIME, timing);
    if (ret)
        nemc_err("get_timing");

    return ret;
}

void nemc_close(int fd, void *mem)
{

    int ret = munmap(mem, NEMC_MAX_SIZE);
    if (ret)
        nemc_err("munmap");

    close(fd);
}


