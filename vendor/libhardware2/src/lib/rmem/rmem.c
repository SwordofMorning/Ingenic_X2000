#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>

#include <libhardware2/rmem.h>

#define rmem_extra_data rmem_alloc_data

#define CMD_RMEM_ALLOC                _IOWR('R', 120, struct rmem_alloc_data *)
#define CMD_RMEM_FREE                 _IOWR('R', 121, struct rmem_alloc_data *)
#define CMD_RMEM_CACHE                _IOWR('W', 122, void *)
#define CMD_RMEM_RECT_CACHE           _IOWR('W', 123, void *)
#define CMD_RMEM_ADD_EXTRA_MEM        _IOWR('W', 124, struct rmem_extra_data *)

static inline void rmem_err(const char *err_msg)
{
    fprintf(stderr, "RMEM: failed to %s, %s\n", err_msg, strerror(errno));
}

int rmem_open(void)
{
    int fd = open("/dev/rmem_manager", O_RDWR);
    if (fd < 0) {
        rmem_err("open device");
        return -1;
    }

    return fd;
}

void rmem_close(int fd)
{
    close(fd);
    return;
}

void *rmem_alloc(int fd, unsigned long *phy_addr, int size)
{
    int ret;
    struct rmem_alloc_data data;

    data.size = size;

    ret = ioctl(fd, CMD_RMEM_ALLOC, &data);

    if (ret < 0) {
        rmem_err("alloc rmem");
        return NULL;
    }

    *phy_addr = (unsigned int)data.mem;

    void *mmaped_rmem = mmap(0, size, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_LOCKED, fd,
                                     *phy_addr);

    return mmaped_rmem;
}

void rmem_free(int fd, void *mmaped_rmem, unsigned long phy_addr, int size)
{
    int ret;
    struct rmem_alloc_data data;

    ret = munmap(mmaped_rmem, size);
    if(ret < 0) {
        rmem_err("unmmap err");
        return;
    }

    data.mem = (void *)phy_addr;
    data.size = size;

    ret = ioctl(fd, CMD_RMEM_FREE, &data);
    if (ret < 0) {
        rmem_err("free rmem");
        return;
    }

    return;
}

int rmem_cache_sync(int fd, void *mmaped_mem, int size, enum rmem_cache_type type)
{
    int ret;
    unsigned long array[3] = {(unsigned long)mmaped_mem, size, type};

    ret = ioctl(fd, CMD_RMEM_CACHE, &array);

    if (ret < 0)
        rmem_err("cache sync");

    return ret;
}

int rmem_rect_cache_sync(int fd, void *mmaped_mem,
            int rect_w, int rect_h, int stride, enum rmem_cache_type type)
{
    int ret;
    unsigned long array[5] = {(unsigned long)mmaped_mem, rect_w, rect_h, stride, type};

    ret = ioctl(fd, CMD_RMEM_RECT_CACHE, &array);

    if (ret < 0)
        rmem_err("rect cache sync");

    return ret;
}

int rmem_add_extra_mem(int fd, unsigned long phymem, int size)
{
    int ret;
    struct rmem_extra_data extra_data = {
        .mem = (void *)phymem,
        .size = size,
    };

    ret = ioctl(fd, CMD_RMEM_ADD_EXTRA_MEM, &extra_data);
    if (ret < 0)
        rmem_err("add extra mem err\n");

    return ret;
}