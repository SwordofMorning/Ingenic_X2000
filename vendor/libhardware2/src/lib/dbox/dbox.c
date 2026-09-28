#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <libhardware2/dbox.h>

#define JZDBOX_IOC_MAGIC                'X'
#define IOCTL_DBOX_START                _IO(JZDBOX_IOC_MAGIC, 106)
#define IOCTL_DBOX_RES_PBUFF            _IO(JZDBOX_IOC_MAGIC, 114)
#define IOCTL_DBOX_GET_PBUFF            _IO(JZDBOX_IOC_MAGIC, 115)
#define IOCTL_DBOX_BUF_LOCK             _IO(JZDBOX_IOC_MAGIC, 116)
#define IOCTL_DBOX_BUF_UNLOCK           _IO(JZDBOX_IOC_MAGIC, 117)
#define IOCTL_DBOX_BUF_FLUSH_CACHE      _IO(JZDBOX_IOC_MAGIC, 118)

#define DBOX_DEV_NAME                   "/dev/dbox"

static inline void dbox_err(const char *err_msg)
{
    fprintf(stderr, "dbox: failed to %s, %s\n", err_msg, strerror(errno));
}

int dbox_open(void)
{
    int fd = 0;

    fd = open(DBOX_DEV_NAME, O_RDWR);
    if (fd < 0) {
        dbox_err("open device");
        return -1;
    }
    return fd;
}

int dbox_close(int fd)
{
    return close(fd);
}

static int dbox_lock_buf(int fd, const dbox_param_t* dbox_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_DBOX_BUF_LOCK, (void *)dbox_param);
    if (ret) {
        dbox_err("DBOX_BUF_LOCK\n");
    }
    return ret;
}

static int dbox_unlock_buf(int fd, const dbox_param_t* dbox_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_DBOX_BUF_UNLOCK, (void *)dbox_param);
    if (ret < 0) {
        dbox_err("DBOX_BUF_UNLOCK\n");
    }
    return ret;
}

static int dbox_start(int fd, const dbox_param_t* dbox_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_DBOX_START, (void *)dbox_param);
    if (ret < 0) {
        dbox_err("DBOX_START\n");
    }
    return ret;
}

int dbox_start_draw(const int fd, const dbox_param_t *dbox_param)
{
    int ret = 0;

    ret = dbox_lock_buf(fd, dbox_param);
    if (ret) {
        dbox_err("dbox_lock_buf fail!\n");
        goto err;
    }

    ret = dbox_start(fd, dbox_param);
    if (ret) {
        dbox_err("dbox_start fail!\n");
        goto err;
    }

    ret = dbox_unlock_buf(fd, dbox_param);
    if (ret) {
        dbox_err("dbox_unlock_buf fail!\n");
        goto err;
    }

err:
    return ret;
}
