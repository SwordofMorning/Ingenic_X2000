#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <libhardware2/rotator.h>

#define CMD_rotator_complete_conversion       _IOWR('R', 122, struct rotator_config_data *)

int rotator_open(void)
{
    int handle;

    handle = open("/dev/jz_rotator", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "rotator open /dev/jz_rotator failed: %s\n", strerror(errno));
        return -1;
    }

    return handle;
}

int rotator_complete_conversion(int handle, struct rotator_config_data *data)
{
    int ret = 0;

    if (data->src_fmt == ROTATOR_NV12 && data->dst_fmt == ROTATOR_NV12) {
        struct rotator_config_data uv_rotator_config;
        int y_pixel_byte = 1;
        int uv_pixel_byte = 2;
        int src_height = data->frame_height;
        int dst_height = data->frame_width;

        if (data->rotate_angle == ROTATOR_ANGLE_0 || data->rotate_angle == ROTATOR_ANGLE_180)
            dst_height = src_height;

        memcpy(&uv_rotator_config, data, sizeof(uv_rotator_config));
        uv_rotator_config.src_fmt              = ROTATOR_YUV422;
        uv_rotator_config.dst_fmt              = ROTATOR_YUV422;
        uv_rotator_config.src_buf              = data->src_buf + data->src_stride * src_height * y_pixel_byte;
        uv_rotator_config.dst_buf              = data->dst_buf + data->dst_stride * dst_height * y_pixel_byte;
        uv_rotator_config.frame_width          = data->frame_width / 2;
        uv_rotator_config.frame_height         = data->frame_height / 2;
        uv_rotator_config.src_stride           = data->src_stride / uv_pixel_byte;
        uv_rotator_config.dst_stride           = data->dst_stride / uv_pixel_byte;

        ret = ioctl(handle, CMD_rotator_complete_conversion, &uv_rotator_config);
        if (ret < 0) {
            fprintf(stderr, "rotator converrion failed: %s\n", strerror(errno));
            return ret;
        }

        data->src_fmt = ROTATOR_Y8;
        data->dst_fmt = ROTATOR_Y8;

        ret = ioctl(handle, CMD_rotator_complete_conversion, data);
        if (ret < 0)
            fprintf(stderr, "rotator converrion failed: %s\n", strerror(errno));

        data->src_fmt = ROTATOR_NV12;
        data->dst_fmt = ROTATOR_NV12;

        return ret;
    }

    ret = ioctl(handle, CMD_rotator_complete_conversion, data);
    if (ret < 0)
        fprintf(stderr, "rotator converrion failed: %s\n", strerror(errno));

    return ret;
}

void rotator_close(int handle)
{
    close(handle);
}