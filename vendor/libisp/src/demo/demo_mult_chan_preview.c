/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Ingenic Media Development Kit(IMDK)
 *
 */

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
#include <signal.h>
#include <isp.h>
#include <isp_tuning.h>
#include <libhardware2/fb.h>

#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define CHANEL_NUM       2

static struct frame_image_format output_fmt = {
    .width              = 600,
    .height             = 1024,
    .pixel_format       = CAMERA_PIX_FMT_NV12,
    .frame_nums         = 2,

    .scaler.enable      = 0,
    .scaler.width       = 1600,
    .scaler.height      = 1200,

    .crop.enable        = 1,
    .crop.top           = 28,
    .crop.left          = 660,
    .crop.width         = 600,
    .crop.height        = 1024,
};

typedef struct preview_data {
    pthread_t pid;
    int chan_fd;
    int fb_fd;
    struct camera_info cam_info;
    struct fb_device_info fb_info;
    struct frame_image_format fmt; /* 获取frame_size(非对齐大小) */
    int *isp_tuning_fd;
} preview_data_t;

static preview_data_t pre_dat[CHANEL_NUM];

static int fb_disply(int index, preview_data_t *dat, void *mem)
{
    int ret = 0;
    void *y_mem = mem;
    void *uv_mem = NULL;
    int line_length = 0;
    int fb_ypos = 0;
    int lcd_layer = lcdc_layer_top;

    uv_mem = mem + pre_dat[index].cam_info.line_length * pre_dat[index].cam_info.height;
    line_length = pre_dat[index].cam_info.line_length;

    if (index > 0) {
        fb_ypos = 512;
        lcd_layer = lcdc_layer_bottom;
    }

    struct lcdc_layer cfg = {
        .fb_fmt = fb_fmt_NV12,
        .xres = pre_dat[index].fb_info.xres,
        .yres = pre_dat[index].fb_info.yres,
        .xpos = 0,
        .ypos = fb_ypos,
        .layer_order = lcd_layer,
        .layer_enable = 1,
        .scaling = {
            .enable = 1,
            .xres = 600,
            .yres = 512,
        },
        .y = {
            .stride = line_length,
            .mem = (void *)pre_dat[index].cam_info.phys_mem + (y_mem - pre_dat[index].cam_info.mapped_mem),
        },
        .uv = {
            .stride = line_length,
            .mem = (void *)pre_dat[index].cam_info.phys_mem + (uv_mem - pre_dat[index].cam_info.mapped_mem),
        },
        .alpha = {
            .enable = 0,
            .value = 0xff,
        },

    };

    ret = fb_pan_display_set_user_cfg(dat->fb_fd, &cfg);
    fb_pan_display(dat->fb_fd, &pre_dat[index].fb_info, 0);

    return ret;
}

static void *frame_proc(void *argc)
{
    int index = -1;
    index = *(int *)argc;

    while (1) {
        void *mem = isp_wait_frame(pre_dat[index].chan_fd);
        if (mem) {
            fb_disply(index, &pre_dat[index], mem);
            isp_put_frame(pre_dat[index].chan_fd, mem);
        }
    }

    return NULL;
}

static void usage(const char *prg_name, int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                        : show help info\n");
    fprintf(stderr, "    device_path                      : start frame\n");
    fprintf(stderr, "    Example: %s  /dev/mscaler0-ch0 (signle channel)\n", prg_name);
    fprintf(stderr, "         or: %s  /dev/mscaler0-ch0 /dev/mscaler1-ch0 (double channels)\n", prg_name);
    fprintf(stderr, "\n");
    exit(status);
}

static int init_fb(const char *dev_name, preview_data_t *dat)
{
    int fd;
    int ret;

    fd = fb_open(dev_name, &dat->fb_info);
    if (fd < 0) {
        printf("fb open failed.\n");
        return -1;
    }
    dat->fb_fd = fd;

    ret = fb_enable(fd);
    if (ret) {
        printf("fb enable failed.\n");
        close(fd);
        return -1;
    }

    ret = fb_pan_display_enable_user_cfg(fd);
    if (ret) {
        fprintf(stderr, "fb enable user cfg failed\n");
        fb_disable(fd);
        close(fd);
        return -1;
    }

    return 0;
}

static void exit_fb(preview_data_t *dat)
{
    fb_disable(dat->fb_fd);
    close(dat->fb_fd);
}

static int init_isp(const char *dev_name, preview_data_t *dat)
{
    int ret;
    int fd;
    struct camera_info *info;

    if (dev_name == NULL || dat == NULL)
        return -1;

    dat->chan_fd = isp_open(dev_name);
    if (dat->chan_fd == -ENODEV)
        return -ENODEV;

    fd = dat->chan_fd;
    ret = isp_set_format(fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set format failed\n");
        close(fd);
        return ret;
    }

    ret = isp_requset_buffer(fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set request buffer failed\n");
        close(fd);
        return ret;
    }

    info = &dat->cam_info;
    isp_get_info(fd, info);

    ret = isp_mmap(fd, info);
    if (ret < 0) {
        isp_free_buffer(fd);
        close(fd);
        return ret;
    }

    ret = isp_power_on(fd);
    if (ret)
        goto close_fd;

    ret = isp_stream_on(fd);
    if (ret)
        goto close_fd;

    fprintf(stderr, "name        : %s\n", info->name);
    fprintf(stderr, "width       : %d\n", info->width);
    fprintf(stderr, "height      : %d\n", info->height);
    fprintf(stderr, "fps         : %d\n", info->fps);
    fprintf(stderr, "data_fmt    : %d\n", info->data_fmt);
    fprintf(stderr, "line_length : %d\n", info->line_length);
    fprintf(stderr, "frame_size  : %d\n", info->frame_size);
    fprintf(stderr, "frame_nums  : %d\n", info->frame_nums);
    fprintf(stderr, "phys_mem    : %08lx\n", info->phys_mem);
    fprintf(stderr, "mapped_mem  : %p\n", info->mapped_mem);

    isp_get_format(fd, &dat->fmt);
    return 0;

close_fd:
    isp_free_buffer(fd);
    isp_close(fd, info);

    return -1;
}

static void exit_isp(preview_data_t *dat)
{
    int fd;

    fd = dat->chan_fd;
    isp_stream_off(fd);
    isp_power_off(fd);
    isp_free_buffer(fd);
    isp_close(fd, &dat->cam_info);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int index;

    /* 参数检查 */
    if (argc < 2 || argc > 3)
        usage(argv[0], -1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argv[0], argc == 2 ? 0 : -1);

    memset(pre_dat, -1, sizeof(pre_dat));
    for (index = 0; index < argc-1; index++) {
        ret = init_isp(argv[index+1], &pre_dat[index]);
        if (ret < 0) {
            fprintf(stderr, "open %s failed\n", argv[index+1]);
            return -1;
        }
        int tmp = index;
        ret = pthread_create(&pre_dat[index].pid, NULL, frame_proc, (void *)&tmp);
        if (ret < 0) {
            fprintf(stderr, "create pthread failed.\n");
            goto exit;
        }
    }
    init_fb("/dev/fb0", &pre_dat[0]);
    init_fb("/dev/fb1", &pre_dat[1]);

    while (1) {
        sleep(5);
    }

    for (index = 0; index < CHANEL_NUM; index++) {
        if (pre_dat[index].chan_fd > 0) {
            pthread_join(pre_dat[index].pid, NULL);
        }
    }

exit:
    for (index = 0; index < CHANEL_NUM; index++) {
        if (pre_dat[index].chan_fd > 0) {
            exit_isp(&pre_dat[index]);
        }

        if (pre_dat[index].fb_fd > 0) {
            exit_fb(&pre_dat[index]);
        }
    }

    return ret;
}