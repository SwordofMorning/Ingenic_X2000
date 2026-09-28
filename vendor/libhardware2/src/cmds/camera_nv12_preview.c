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
#include <libhardware2/camera.h>
#include <libhardware2/fb.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

static struct camera_info info;
static struct fb_device_info fb_info;
static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help              : show help info\n");
    fprintf(stderr, "    device_path            : fb show camera data\n");
    fprintf(stderr, "    Example: %s  /dev/camera\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

void bayer16_to_nv12(void *_dst, void *_src, int xres, int yres, int line_length)
{
    int len = line_length * yres;
    unsigned char *src = _src + 1;
    unsigned char *dst = _dst;

    int i = len / 32;
    while (i--) {
        unsigned char s0 = src[0*2];
        unsigned char s1 = src[1*2];
        unsigned char s2 = src[2*2];
        unsigned char s3 = src[3*2];
        unsigned char s4 = src[4*2];
        unsigned char s5 = src[5*2];
        unsigned char s6 = src[6*2];
        unsigned char s7 = src[7*2];
        unsigned char s8 = src[8*2];
        unsigned char s9 = src[9*2];
        unsigned char s10 = src[10*2];
        unsigned char s11 = src[11*2];
        unsigned char s12 = src[12*2];
        unsigned char s13 = src[13*2];
        unsigned char s14 = src[14*2];
        unsigned char s15 = src[15*2];

        dst[0] = s0;
        dst[1] = s1;
        dst[2] = s2;
        dst[3] = s3;
        dst[4] = s4;
        dst[5] = s5;
        dst[6] = s6;
        dst[7] = s7;
        dst[8] = s8;
        dst[9] = s9;
        dst[10] = s10;
        dst[11] = s11;
        dst[12] = s12;
        dst[13] = s13;
        dst[14] = s14;
        dst[15] = s15;

        src += 32;
        dst += 16;
    }

    memset(dst, 0x80, len/4);
}

int main(int argc, char *argv[])
{
    int ret, fd;
    int fb_fd = -1;

    prg_name = argv[0];

    if (argc < 2)
        usage(-1);

    fd = camera_open(&info, argv[1]);
    if (fd == -1)
        return -1;


    if ( !(camera_fmt_is_16BIT(info.data_fmt) || camera_fmt_is_NV12(info.data_fmt)) ) {
        fprintf(stderr, "this fmt not support now: %x\n", info.data_fmt);
        goto close_fd;
    }

    ret = camera_power_on(fd);
    if (ret)
        goto close_fd;

    ret = camera_stream_on(fd);
    if (ret) {
        camera_power_off(fd);
        goto close_fd;
    }

    fb_fd = fb_open("/dev/fb1", &fb_info);
    if (fb_fd == -1)
        goto close_fd;

    if (fb_enable(fb_fd))
        goto close_fd;

    ret = fb_pan_display_enable_user_cfg(fb_fd);
    if (ret)
        goto close_fd;

    camera_drop_frames(fd, 10);

    while (1) {
        void *mem = camera_wait_frame(fd);
        if (!mem) {
            camera_power_off(fd);
            goto close_fd;
        }

        void *y_mem = mem;
        void *uv_mem = NULL;
        int line_length = 0;

        if (camera_fmt_is_16BIT(info.data_fmt)) {
            bayer16_to_nv12(mem, mem, info.width, info.height, info.line_length);
            line_length = info.line_length / 2;
            uv_mem = mem + line_length * info.height;
        } else if (camera_fmt_is_NV12(info.data_fmt)) {
            uv_mem = mem + info.line_length * info.height;
            line_length = info.line_length;
        }

        int width = info.width;
        int height = info.height;

        if (info.width > fb_info.xres) {
            width = fb_info.xres;
            y_mem += (info.width - fb_info.xres) / 2;
        }

        if (info.height > fb_info.yres) {
            height = fb_info.yres;
            uv_mem += (info.height - fb_info.yres) / 4;
        }

        struct lcdc_layer cfg = {
            .fb_fmt = fb_fmt_NV12,
            .xres = width,
            .yres = height,
            .xpos = 0,
            .ypos = 0,
            .layer_order = lcdc_layer_bottom,
            .layer_enable = 1,
            .y = {
                .stride = line_length,
                .mem = (void *)info.phys_mem + (y_mem - info.mapped_mem),
            },
            .uv = {
                .stride = line_length / 2,
                .mem = (void *)info.phys_mem + (uv_mem - info.mapped_mem),
            },
            .alpha = {
                .enable = 0,
                .value = 0xff,
            },
        };

        ret = fb_pan_display_set_user_cfg(fb_fd, &cfg);
        if (ret) {
            fb_disable(fb_fd);
            goto close_fd;
        }

        fb_pan_display(fb_fd, &fb_info, 0);

        ret = camera_put_frame(fd, mem);
        if (ret) {
            camera_power_off(fd);
            goto close_fd;
        }
    }

    return 0;
close_fd:
    camera_close(fd, &info);
    if (fb_fd != -1)
        fb_close(fb_fd, &fb_info);
    return -1;
}
