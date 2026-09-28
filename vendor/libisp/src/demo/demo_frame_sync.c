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
#include <libhardware2/fb.h>

#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define CHANEL_NUM       2

static struct frame_image_format ms_ch = {
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


typedef struct {
    int index;

    char ms_ch_name[32];
    int ms_ch_fd;
    struct camera_info cam_info;

    int flag; //0, 帧无数据； 1，帧有数据
    struct frame_info frame;
} camera_data_t;

static camera_data_t camera[CHANEL_NUM];


static int init_isp(camera_data_t *dat)
{
    int ret;
    int fd;
    struct camera_info *info;

    if (dat == NULL)
        return -1;

    fd = isp_open(dat->ms_ch_name);
    if (fd == -ENODEV)
        return -ENODEV;
    dat->ms_ch_fd = fd;

    ret = isp_set_format(fd, &ms_ch);
    if (ret < 0) {
        fprintf(stderr, "isp set format failed\n");
        close(fd);
        return ret;
    }

    ret = isp_requset_buffer(fd, &ms_ch);
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

    return 0;

close_fd:
    isp_free_buffer(fd);
    isp_close(fd, info);

    return -1;
}

static int streamon_isp(camera_data_t *dat)
{
    return isp_stream_on(dat->ms_ch_fd);
}

static void exit_isp(camera_data_t *dat)
{
    isp_stream_off(dat->ms_ch_fd);
    isp_power_off(dat->ms_ch_fd);
    isp_free_buffer(dat->ms_ch_fd);
    isp_close(dat->ms_ch_fd, &dat->cam_info);
}


static void usage(const char *prg_name, int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                    : show help info\n");
    fprintf(stderr, "    -t timestamp_thresh_us       : set frame sync timestamp thresh,start frame sync\n");
    fprintf(stderr, "    Example: %s -t 15000 \n", prg_name);
    fprintf(stderr, "\n");
    exit(status);
}

int main(int argc, char *argv[])
{
    int ret;
    int index;
    int timestamp_thresh_us = 15*1000;

    while((ret = getopt(argc, argv, "t:h")) != -1 ) {
        switch(ret) {
                break;
            case 'h':
                usage(argv[0], 0);
                break;
            case 't':
                timestamp_thresh_us = atoi(optarg);
                break;

            case '?':
                printf("Unknown option: %c\n",(char)optopt);
                usage(argv[0], -1);
                break;
        }
    }

    memset(camera, 0, sizeof(camera));
    for (index = 0; index < CHANEL_NUM; index++) {
        camera[index].index = 0;

        sprintf(camera[index].ms_ch_name, "/dev/mscaler%d-ch0", index);
        ret = init_isp(&camera[index]);
        if (ret < 0) {
            fprintf(stderr, "init isp failed, index:%d\n", index);
            goto exit;
        }
    }

    for (index = 0; index < CHANEL_NUM; index++) {
        ret = streamon_isp(&camera[index]);
        if (ret < 0) {
            fprintf(stderr, "stream on isp failed, index:%d\n", index);
            goto exit;
        }
    }

    while (1) {
        for (index = 0; index < CHANEL_NUM; index++) {
            if (!camera[index].flag) {
                ret = isp_dqbuf(camera[index].ms_ch_fd, &camera[index].frame);
                if (0 == ret) {
                    camera[index].flag = 1;
                }
            }
        }

        if (camera[0].flag && camera[1].flag) {
            unsigned long long time_diff = (unsigned long long)llabs(camera[0].frame.timestamp - camera[1].frame.timestamp);
            printf("time_diff: %llu,\n", time_diff);
            if (time_diff <= timestamp_thresh_us) {
                printf("camera[0].timestamp: %llu, camera[1].timestamp: %llu,\n", camera[0].frame.timestamp, camera[1].frame.timestamp);
                for (index = 0; index < CHANEL_NUM; index++) {
                    //frame handle

                    isp_qbuf(camera[index].ms_ch_fd, &camera[index].frame);
                    camera[index].flag = 0;
                }
            } else {
                if (camera[0].frame.timestamp < camera[1].frame.timestamp) {
                    index = 0;
                } else {
                    index = 1;
                }
                isp_qbuf(camera[index].ms_ch_fd, &camera[index].frame);
                camera[index].flag = 0;
            }
        }

        usleep(3*1000);
    }

exit:
    for (index = 0; index < CHANEL_NUM; index++) {
        if (camera[index].ms_ch_fd > 0) {
            exit_isp(&camera[index]);
        }
    }

    return ret;
}
