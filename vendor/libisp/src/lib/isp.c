/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Ingenic Media Development Kit(IMDK)
 *
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

#include <isp.h>

#define ISP_CMD_MAGIC                   'C'
#define ISP_CMD_MAGCIC_NR               120

#define CMD_get_info                    _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 0, struct camera_info)
#define CMD_power_on                    _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 1)
#define CMD_power_off                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 2)
#define CMD_stream_on                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 3)
#define CMD_stream_off                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 4)
#define CMD_set_format                  _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 5, struct frame_image_format)
#define CMD_get_format                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 6)
#define CMD_request_buffer              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 7, struct frame_image_format)
#define CMD_free_buffer                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 8)
#define CMD_get_max_scaler_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 9, unsigned int)
#define CMD_get_line_align_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 10, unsigned int)
#define CMD_set_scaler_algo             _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 11, struct mscaler_algo_attr)

#define CMD_wait_frame                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 20)
#define CMD_get_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 21)
#define CMD_put_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 22)
#define CMD_dqbuf                       _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 23)
#define CMD_dqbuf_wait                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 24)
#define CMD_qbuf                        _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 25)
#define CMD_skip_frames                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 26)
#define CMD_get_frame_count             _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 27)

#define CMD_get_sensor_info             _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 30, struct camera_info)
#define CMD_get_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 31, struct sensor_dbg_register)
#define CMD_set_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 32, struct sensor_dbg_register)


static inline void isp_err(const char *err_msg)
{
    fprintf(stderr, "isp: failed to %s, %d\n", err_msg, errno);
}

int isp_open(const char* device_path)
{
    int fd;

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        isp_err("open device");
        return -ENODEV;
    }

    return fd;
}

int isp_mmap(int fd, struct camera_info *info)
{
    assert(info);

    int ret = ioctl(fd, CMD_get_info, info);
    if (ret) {
        isp_err("get info");
        return ret;
    }

    info->mapped_mem = NULL;

    unsigned int size = info->frame_nums * info->frame_align_size;
    if (size == 0) {
        fprintf(stderr, "isp: error size: %d %d\n", info->frame_nums, info->frame_size);
        return -EINVAL;
    }

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((int) mem == -1) {
        isp_err("mmap");
        return -ENOMEM;
    }

    info->mapped_mem = mem;

    return 0;
}

int isp_close(int fd, struct camera_info *info)
{
    unsigned int size = info->frame_nums * info->frame_align_size;

    if (info->mapped_mem) {
        int ret = munmap(info->mapped_mem, size);
        if (ret)
            isp_err("munmap");
    }

    return close(fd);
}

int isp_power_on(int fd)
{
    int ret = ioctl(fd, CMD_power_on);
    if (ret)
        isp_err("power on");

    return ret;
}

int isp_power_off(int fd)
{
    int ret = ioctl(fd, CMD_power_off);
    if (ret)
        isp_err("power off");

    return ret;
}

int isp_stream_on(int fd)
{
    int ret = ioctl(fd, CMD_stream_on);
    if (ret)
        isp_err("stream on");

    return ret;
}

int isp_stream_off(int fd)
{
    int ret = ioctl(fd, CMD_stream_off);
    if (ret)
        isp_err("stream off");

    return ret;
}

void *isp_wait_frame(int fd)
{
    void *mem = NULL;

    int ret = ioctl(fd, CMD_wait_frame, &mem);
    if (ret)
        isp_err("wait frame");

    return mem;
}

void *isp_get_frame(int fd)
{
    void *mem = NULL;

    ioctl(fd, CMD_get_frame, &mem);

    return mem;
}

int isp_put_frame(int fd, void *mem)
{
    assert(mem);

    int ret = ioctl(fd, CMD_put_frame, mem);
    if (ret)
        isp_err("put frame");

    return ret;
}

int isp_dqbuf(int fd, struct frame_info *frame)
{
    assert(frame);

    return ioctl(fd, CMD_dqbuf, frame);
}

int isp_dqbuf_wait(int fd, struct frame_info *frame)
{
    assert(frame);

    int ret = ioctl(fd, CMD_dqbuf_wait, frame);
    if (ret)
        isp_err("dqbuf wait");

    return ret;
}

int isp_qbuf(int fd, struct frame_info *frame)
{
    assert(frame);

    int ret = ioctl(fd, CMD_qbuf, frame);
    if (ret)
        isp_err("qbuf");

    return ret;
}

int isp_get_avaliable_frame_count(int fd)
{
    int count = 0;
    int ret = ioctl(fd, CMD_get_frame_count, &count);
    if (ret)
        isp_err("get frame count");

    return count;
}

int isp_drop_frames(int fd, unsigned int frames)
{
    int ret = ioctl(fd, CMD_skip_frames, frames);
    if (ret)
        isp_err("skip frames");

    return ret;
}

int isp_get_info(int fd, struct camera_info *info)
{
    assert(info);

    int ret = ioctl(fd, CMD_get_info, info);
    if (ret) {
        isp_err("get info");
        return ret;
    }

    return 0;
}

int isp_get_sensor_info(int fd, struct camera_info *info)
{
    assert(info);

    int ret = ioctl(fd, CMD_get_sensor_info, info);
    if (ret) {
        isp_err("get sensor info");
        return ret;
    }

    return 0;
}

int isp_get_max_scaler_size(int fd, int *width, int *height)
{

    unsigned int size[2];
    int ret = ioctl(fd, CMD_get_max_scaler_size, (unsigned int)size);
    if (ret) {
        isp_err("get max scaler size");
        return ret;
    }

    *width = size[0];
    *height = size[1];

    return 0;
}

int isp_get_line_align_size(int fd, int *align_size)
{
    int ret = ioctl(fd, CMD_get_line_align_size, (unsigned int)align_size);
    if (ret)
        isp_err("get line align size");

    return ret;
}

int isp_set_format(int fd, struct frame_image_format *fmt)
{
    int ret = ioctl(fd, CMD_set_format, fmt);
    if (ret)
        isp_err("set frame format");

    return ret;
}

int isp_get_format(int fd, struct frame_image_format *fmt)
{
    int ret = ioctl(fd, CMD_get_format, fmt);
    if (ret)
        isp_err("get frame format");

    return ret;
}

int isp_requset_buffer(int fd, struct frame_image_format *fmt)
{
    int ret = ioctl(fd, CMD_request_buffer, fmt);
    if (ret)
        isp_err("request frame buffer");

    return ret;
}

int isp_free_buffer(int fd)
{
    int ret = ioctl(fd, CMD_free_buffer);
    if (ret)
        isp_err("free frame buffer");

    return ret;
}

int isp_set_scale_algo(int fd, struct mscaler_algo_attr *attr)
{
    assert(attr);

    int ret = ioctl(fd, CMD_set_scaler_algo, attr);
    if (ret)
        isp_err("set scale algo");

    return ret;
}

int isp_get_sensor_reg(int fd, struct sensor_dbg_register *reg)
{
    assert(reg);

    int ret = ioctl(fd, CMD_get_sensor_reg, reg);
    if (ret)
        isp_err("get sensor reg");

    return ret;
}

int isp_set_sensor_reg(int fd, struct sensor_dbg_register *reg)
{
    assert(reg);

    int ret = ioctl(fd, CMD_set_sensor_reg, reg);
    if (ret)
        isp_err("set sensor reg");

    return ret;
}
