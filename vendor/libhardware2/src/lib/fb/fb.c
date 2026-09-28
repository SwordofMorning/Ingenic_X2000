#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>

#include <libhardware2/fb.h>

#define CMD_set_cfg       _IOWR('i', 80, struct lcdc_layer)
#define CMD_enable_cfg    _IO('i', 81)
#define CMD_disable_cfg   _IO('i', 82)
#define CMD_read_reg      _IOWR('i', 83, unsigned long)
#define sum(r, g, b)      (r)+(g)+(b)

unsigned int fb_bytes_per_pixel(enum fb_fmt fb_fmt)
{
    switch (fb_fmt) {
    case fb_fmt_RGB888:
    case fb_fmt_ARGB8888:
        return 4;
    case fb_fmt_RGB555:
    case fb_fmt_RGB565:
        return 2;
    case fb_fmt_NV12:
    case fb_fmt_NV21:
        return 2;
    default:
        return 2;
    }
}

static inline enum fb_fmt get_fb_fmt(unsigned int r_len, unsigned int g_len, unsigned int b_len, unsigned int a_len)
{
    int bytes = sum(r_len, g_len, b_len);
    if ((a_len == 8) && (bytes == 24))
        return fb_fmt_ARGB8888;
    if (bytes == 24)
        return fb_fmt_RGB888;
    if (bytes == 16)
        return fb_fmt_RGB565;
    if (bytes == 15)
        return fb_fmt_RGB555;
    return fb_fmt_ARGB8888;
}

static inline void fb_err(const char *err_msg)
{
    fprintf(stderr, "fb: failed to %s, %s\n", err_msg, strerror(errno));
}

int fb_open(const char *dev_path, struct fb_device_info *info)
{
    assert(info);

    int fd = open(dev_path, O_RDWR);
    if(fd < 0) {
        fb_err("open device");
        return -1;
    }

    int ret = ioctl(fd, FBIOGET_FSCREENINFO, &info->fix);
    if (ret) {
        fb_err("get fixed information");
        goto close_fd;
    }

    ret = ioctl(fd, FBIOGET_VSCREENINFO, &info->var);
    if (ret) {
        fb_err("get variable information");
        goto close_fd;
    }

    info->xres = info->var.xres;
    info->yres = info->var.yres;
    info->line_length = info->fix.line_length;
    info->bits_per_pixel = info->var.bits_per_pixel;

    info->fb_fmt = get_fb_fmt(info->var.red.length, info->var.green.length, info->var.blue.length, info->var.transp.length);

    unsigned int frame_size = info->yres * info->line_length;
    unsigned int nums = info->var.yres_virtual / info->yres;
    info->frame_size = nums ? info->fix.smem_len / nums : frame_size;
    info->frame_nums = nums;
    info->mapped_mem = NULL;

    if (!info->frame_nums)
        return fd;

    unsigned int size = info->frame_size * info->frame_nums;
    void *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((int) base == -1) {
        fb_err("mmap");
        goto close_fd;
    }

    info->mapped_mem = base;

    return fd;
close_fd:
    close(fd);
    return -1;
}

int fb_close(int fd, struct fb_device_info *info)
{
    if (info->mapped_mem) {
        unsigned int size = info->frame_size * info->frame_nums;
        int ret = munmap(info->mapped_mem, size);
        if (ret)
            fb_err("munmap");
        info->mapped_mem = NULL;
    }

    return close(fd);
}

int fb_enable(int fd)
{
    int ret = ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK);
    if (ret)
        fb_err("unblank");

    return ret;
}

int fb_disable(int fd)
{
    int ret = ioctl(fd, FBIOBLANK, FB_BLANK_POWERDOWN);
    if (ret)
        fb_err("power down");

    return ret;
}

int fb_pan_display(int fd, struct fb_device_info *info, unsigned int frame_index)
{
    assert(info);

    int nums = info->frame_nums;

    if (frame_index >= nums && nums != 0) {
        fprintf(stderr, "fb: frame_index invalid: %d %d\n", frame_index, nums);
        return -EINVAL;
    }

    info->var.yoffset = frame_index * info->yres;
    int ret = ioctl(fd, FBIOPAN_DISPLAY, &info->var);
    if (ret)
        fb_err("pan display");

    return ret;
}

int fb_pan_display_enable_user_cfg(int fd)
{
    int ret = ioctl(fd, CMD_enable_cfg);
    if (ret)
        fb_err("enable cfg");

    return ret;
}

int fb_pan_display_disable_user_cfg(int fd)
{
    int ret = ioctl(fd, CMD_disable_cfg);
    if (ret)
        fb_err("disable cfg");

    return ret;
}

int fb_pan_display_set_user_cfg(int fd, struct lcdc_layer *cfg)
{
    int ret = ioctl(fd, CMD_set_cfg, cfg);
    if (ret)
        fb_err("set cfg");

    return ret;
}

int fb_slcd_read_reg_8(int fd, int reg, char *buffer, int count)
{
    unsigned long data[] = {
        reg, count, (unsigned long)buffer
    };

    int ret = ioctl(fd, CMD_read_reg, data);
    if (ret < 0)
        fb_err("failed to read data");

    return ret;
}