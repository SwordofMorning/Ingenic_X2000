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

#define CMD_get_info                _IOWR('C', 120, struct camera_info)
#define CMD_power_on                _IO('C', 121)
#define CMD_power_off               _IO('C', 122)
#define CMD_stream_on               _IO('C', 123)
#define CMD_stream_off              _IO('C', 124)
#define CMD_wait_frame              _IO('C', 125)
#define CMD_put_frame               _IO('C', 126)
#define CMD_get_frame_count         _IO('C', 127)
#define CMD_skip_frames             _IO('C', 128)
#define CMD_get_sensor_reg          _IO('C', 129)
#define CMD_set_sensor_reg          _IO('C', 130)
#define CMD_get_frame               _IO('C', 131)
#define CMD_dqbuf                   _IO('C', 132)
#define CMD_dqbuf_wait              _IO('C', 133)
#define CMD_qbuf                    _IO('C', 134)
#define CMD_get_fps                 _IO('C', 135)
#define CMD_set_fps                 _IO('C', 136)
#define CMD_get_hflip               _IO('C', 137)
#define CMD_set_hflip               _IO('C', 138)
#define CMD_get_vflip               _IO('C', 139)
#define CMD_set_vflip               _IO('C', 140)
#define CMD_reset_fmt               _IO('C', 141)

static inline void camera_err(const char *err_msg)
{
    fprintf(stderr, "camera: failed to %s, %s\n", err_msg, strerror(errno));
}

unsigned int cam_bytes_per_pixel(camera_pixel_fmt cam_fmt)
{
    if (camera_fmt_is_16BIT(cam_fmt))
        return 2;
    if (camera_fmt_is_YUV422(cam_fmt))
        return 2;
    if (camera_fmt_is_NV12(cam_fmt))
        return 1;
    if (camera_fmt_is_8BIT(cam_fmt))
        return 1;
    return 2;
}

int camera_open(struct camera_info *info, const char* device_path)
{
    int fd;

    assert(info);

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        camera_err("open device");
        return -1;
    }

    int ret = ioctl(fd, CMD_get_info, info);
    if (ret) {
        camera_err("get info");
        close(fd);
        return ret;
    }

    info->mapped_mem = NULL;

    unsigned int size = info->frame_nums * info->frame_align_size;
    if (size == 0) {
        fprintf(stderr, "camera: error size: %d %d\n", info->frame_nums, info->frame_size);
        return fd;
    }

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((int) mem == -1) {
        camera_err("mmap");
        return fd;
    }

    info->mapped_mem = mem;

    return fd;
}

int camera_close(int fd, struct camera_info *info)
{
    unsigned int size = info->frame_nums * info->frame_align_size;
    int ret = munmap(info->mapped_mem, size);
    if (ret)
        camera_err("munmap");

    return close(fd);
}

int camera_power_on(int fd)
{
    int ret = ioctl(fd, CMD_power_on);
    if (ret)
        camera_err("power on");

    return ret;
}

int camera_power_off(int fd)
{
    int ret = ioctl(fd, CMD_power_off);
    if (ret)
        camera_err("power off");

    return ret;
}

int camera_stream_on(int fd)
{
    int ret = ioctl(fd, CMD_stream_on);
    if (ret)
        camera_err("stream on");

    return ret;
}

int camera_stream_off(int fd)
{
    int ret = ioctl(fd, CMD_stream_off);
    if (ret)
        camera_err("stream off");

    return ret;
}

void *camera_wait_frame(int fd)
{
    void *mem = NULL;

    int ret = ioctl(fd, CMD_wait_frame, &mem);
    if (ret)
        camera_err("wait frame");

    return mem;
}

void *camera_get_frame(int fd)
{
    void *mem = NULL;

    int ret = ioctl(fd, CMD_get_frame, &mem);
    if (ret) {
        return NULL;
    } else {
        return mem;
    }
}

int camera_put_frame(int fd, void *mem)
{
    assert(mem);

    int ret = ioctl(fd, CMD_put_frame, mem);
    if (ret)
        camera_err("put frame");

    return ret;
}

int camera_dqbuf(int fd, struct frame_info *frame)
{
    assert(frame);

    return ioctl(fd, CMD_dqbuf, frame);
}

int camera_dqbuf_wait(int fd, struct frame_info *frame)
{
    assert(frame);

    int ret = ioctl(fd, CMD_dqbuf_wait, frame);
    if (ret)
        camera_err("dqbuf wait");

    return ret;
}

int camera_qbuf(int fd, struct frame_info *frame)
{
    assert(frame);

    int ret = ioctl(fd, CMD_qbuf, frame);
    if (ret)
        camera_err("qbuf");

    return ret;
}

int camera_get_avaliable_frame_count(int fd)
{
    int count = 0;
    int ret = ioctl(fd, CMD_get_frame_count, &count);
    if (ret)
        camera_err("get frame count");

    return count;
}

int camera_drop_frames(int fd, unsigned int frames)
{
    int ret = ioctl(fd, CMD_skip_frames, frames);
    if (ret)
        camera_err("skip frames");

    return ret;
}

int camera_get_sensor_reg(int fd, struct sensor_dbg_register *reg)
{
    int ret = ioctl(fd, CMD_get_sensor_reg, reg);
    if (ret)
        camera_err("get sensor reg");

    return ret;
}

int camera_set_sensor_reg(int fd, struct sensor_dbg_register *reg)
{
    int ret = ioctl(fd, CMD_set_sensor_reg, reg);
    if (ret)
        camera_err("set sensor reg");

    return ret;
}

int camera_get_fps(int fd, unsigned int *fps_num, unsigned int *fps_den)
{
    unsigned int fps;
    int ret;

    ret = ioctl(fd, CMD_get_fps, &fps);
    if (!ret) {
        *fps_num = (fps >> 16) & 0xffff;
        *fps_den = fps & 0xffff;
    } else
        camera_err("get fps");

    return ret;
}

int camera_set_fps(int fd, unsigned int fps_num, unsigned int fps_den)
{
    unsigned int fps;
    int ret;

    fps = (fps_num << 16) | fps_den;
    ret = ioctl(fd, CMD_set_fps, fps);
    if (ret)
        camera_err("set fps");

    return ret;
}

int camera_get_hflip(int fd, camera_ops_mode *mode)
{
    int ret;

    if (NULL == mode) {
        camera_err("param is NULL");
        return -1;
    }

    ret = ioctl(fd, CMD_get_hflip, mode);
    if (ret)
        camera_err("get hflip");

    return ret;
}

int camera_set_hflip(int fd, camera_ops_mode mode)
{
    int ret = ioctl(fd, CMD_set_hflip, mode);
    if (ret)
        camera_err("set hflip");

    return ret;
}

int camera_get_vflip(int fd, camera_ops_mode *mode)
{
    int ret;

    if (NULL == mode) {
        camera_err("param is NULL");
        return -1;
    }

    ret = ioctl(fd, CMD_get_vflip, mode);
    if (ret)
        camera_err("get vflip");

    return ret;
}

int camera_set_vflip(int fd, camera_ops_mode mode)
{
    int ret = ioctl(fd, CMD_set_vflip, mode);
    if (ret)
        camera_err("set vflip");

    return ret;
}

int camera_reset_fmt(int fd, struct camera_info *info)
{
    assert(info);

    /*
     * 1.camera_stream_ off() if stream on
     * 2.camera_power_off() if power off
     * 3.camera_reset_fmt()
     * 4.camera_close()
     * 5.camera_open() remap
     * 6.camera_power_on()
     * 7.camera_stream_on()
     */
    int ret = ioctl(fd, CMD_reset_fmt, info);
    if (ret)
        camera_err("reset fmt");

    return ret;
}
