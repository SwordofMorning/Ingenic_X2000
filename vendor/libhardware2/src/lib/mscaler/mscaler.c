#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <libhardware2/mscaler.h>

struct mscaler_info
{
    void                        *phys_addr;
    unsigned long               alloc_size;
    unsigned long               alloc_alignsize;
};

struct mscaler_param
{
    struct mscaler_frame        *src;
    struct mscaler_frame        *dst;
};

#define JZMSCALER_IOC_MAGIC  'M'
#define IOCTL_MSCALER_CONVERT            _IOW(JZMSCALER_IOC_MAGIC, 100, struct mscaler_param)
#define IOCTL_MSCALER_ALIGN_SIZE         _IOR(JZMSCALER_IOC_MAGIC, 101, int)
#define IOCTL_L1CACHE_ALIGN_SIZE         _IOR(JZMSCALER_IOC_MAGIC, 102, int)
#define IOCTL_MSCALER_ALLOC_ALIGN_SIZE   _IOWR(JZMSCALER_IOC_MAGIC, 103, struct mscaler_info)
#define IOCTL_MSCALER_FREE_ALIGN_SIZE   _IO(JZMSCALER_IOC_MAGIC, 104)

static inline void mscaler_err(const char *err_msg)
{
    fprintf(stderr, "mscaler: failed to %s, %s\n", err_msg, strerror(errno));
}

int mscaler_open(struct mscaler_device_info *info)
{
    int fd = open("/dev/jz_mscaler", O_RDWR);
    if (fd < 0) {
        mscaler_err("open device");
        return -1;
    }

    int ret = ioctl(fd, IOCTL_MSCALER_ALIGN_SIZE, &info->stride_align);
    if (ret < 0) {
        mscaler_err("mscaler_line_alignsize");
        close(fd);
        return -1;
    }

    ret = ioctl(fd, IOCTL_L1CACHE_ALIGN_SIZE, &info->frame_align);
    if (ret < 0) {
        mscaler_err("mscaler_frame_alignsize");
        close(fd);
        return -1;
    }

    return fd;
}

int mscaler_close(int fd)
{
    return close(fd);
}

int mscaler_convert(int fd, struct mscaler_frame *src, struct mscaler_frame *dst)
{
    struct mscaler_param ms_param;
    ms_param.src = src;
    ms_param.dst = dst;

    int ret = ioctl(fd, IOCTL_MSCALER_CONVERT, &ms_param);
    if (ret < 0) {
        mscaler_err("convert");
        return -1;
    }

    return 0;
}

int mscaler_alloc_mem(int fd, struct mscaler_device_info *info, int mem_size)
{
    struct mscaler_info ms_info;
    ms_info.alloc_size = mem_size;

    int ret = ioctl(fd, IOCTL_MSCALER_ALLOC_ALIGN_SIZE, &ms_info);
    if (ret < 0) {
        mscaler_err("alloc");
        return -1;
    }

    info->mapped_mem = mmap(NULL, ms_info.alloc_alignsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((int) info->mapped_mem == -1) {
        mscaler_err("mmap");
        return -1;
    }

    info->phys_addr = (unsigned long)ms_info.phys_addr;
    info->mem_align_size = ms_info.alloc_alignsize;
    return 0;
}

int mscaler_free_mem(int fd, struct mscaler_device_info *info)
{
    int ret = munmap(info->mapped_mem, info->mem_align_size);
    if (ret == -1) {
        mscaler_err("munmap");
        return ret;
    }

    ret = ioctl(fd, IOCTL_MSCALER_FREE_ALIGN_SIZE, NULL);
    if (ret < 0) {
        mscaler_err("alloc");
        return -1;
    }

    info->phys_addr = 0;
    info->mapped_mem = NULL;
    info->mem_align_size = 0;
    return 0;
}
