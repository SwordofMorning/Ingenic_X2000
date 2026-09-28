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
#include <assert.h>
#include <signal.h>

#include <isp.h>
#include <libhardware2/fb.h>

#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define ALIGN_DOWN(x, n) ((x) - (x)%(n))

static struct camera_info cam;
static struct fb_device_info fb;

static struct frame_image_format output_fmt = {
    .pixel_format       = CAMERA_PIX_FMT_NV12,
    .frame_nums         = 2,

    .scaler.enable      = 0,
    .crop.enable        = 0,
};

static void usage(const char *prg_name, int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                        : show help info\n");
    fprintf(stderr, "    device_path                      : start frame\n");
    fprintf(stderr, "    [loops]                          : loop count\n");
    fprintf(stderr, "    Example: %s  /dev/mscaler0-ch0\n", prg_name);
    fprintf(stderr, "             %s  /dev/mscaler0-ch0 100\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

static void recalc_output_fmt_by_fb(int isp_fd)
{
    struct camera_info sensor_info;
    int scaler_enable = 1;
    int crop_enable = 1;
    int width = 0;
    int height = 0;
    int align_size;
    int fb_width;
    int fb_height;
    int scaler_width;
    int scaler_height;
    float sensor_ratio;
    float fb_ratio;

    isp_get_line_align_size(isp_fd, &align_size);
    isp_get_max_scaler_size(isp_fd, &scaler_width, &scaler_height);
    isp_get_sensor_info(isp_fd, &sensor_info);

    /* scaler所支持的宽高与fb的宽高比较，取二者相交的区域作为cam可显示的最大区域P  */
    fb_width = scaler_width;
    if (fb_width > fb.xres)
        fb_width = fb.xres;

    fb_height = scaler_height;
    if (fb_height > fb.yres)
        fb_height = fb.yres;

    /* 计算sensor的宽高比(宽：高)、fb的宽高比(宽：高) */
    sensor_ratio = (float)sensor_info.width / sensor_info.height;
    fb_ratio = (float)fb_width / fb_height;

    /* 当sensor的宽高比(宽：高)等于fb的宽高比(宽：高)，则sensor出图尺寸不变 */
    if (sensor_ratio == fb_ratio) {
        width = fb_width;
        height = fb_height;
    }

    /* 当sensor的宽高比(宽：高)大于fb的宽高比(宽：高)，
     * 先缩放sensor出图的高与fb的高相等，再根据高换算出对应比例的宽作为sensor出图的宽。
     * */
    if (sensor_ratio > fb_ratio) {
        height = fb_height;
        width = height * sensor_info.width / sensor_info.height;

        /* 当宽超过scaler最大宽度时，需要重新计算高 */
        if (width > scaler_width) {
            width = scaler_width;
            height = 0;
        }
    }

    /* 当sensor的宽高比(宽：高)小于fb的宽高比(宽：高)，
     * 先缩放sensor出图的宽与fb的宽相等，再根据宽换算出对应比例的高作为sensor出图的高。
     * */
    if (sensor_ratio < fb_ratio) {
        width = fb_width;
        height = width * sensor_info.height / sensor_info.width;

        /* 当高超过scaler最大高度时，需要重新计算宽 */
        if (height > scaler_height) {
            height = scaler_height;
            width = height * sensor_info.width / sensor_info.height;
            height = 0;
        }
    }

    /* sensor的宽须与align_size对齐，对齐后再重新计算出合适的出图尺寸 */
    if (width % align_size) {
        width = ALIGN_DOWN(width, align_size);
        height = 0;
    }

    /* 当出现sensor宽没有16位对齐或者超出scaler范围的时候需要重新计算高 */
    if (height == 0)
        height = width * sensor_info.height / sensor_info.width;

    if (width == sensor_info.width) {
        scaler_enable = 0;
        crop_enable = 0;
    }

    height = ALIGN_DOWN(height, 2);
    output_fmt.pixel_format = CAMERA_PIX_FMT_NV12;
    output_fmt.frame_nums = 2;
    output_fmt.scaler.enable = scaler_enable;
    output_fmt.scaler.width = width;
    output_fmt.scaler.height = height;
    output_fmt.crop.enable = crop_enable;
    output_fmt.crop.top = height > fb.yres ? (height - fb.yres) / 2 : 0;
    output_fmt.crop.top = output_fmt.crop.top % 2 ? output_fmt.crop.top + 1 : output_fmt.crop.top;
    output_fmt.crop.left = (width - fb.xres) / 2;
    output_fmt.crop.left = output_fmt.crop.left % 2 ? output_fmt.crop.left + 1 : output_fmt.crop.left;
    output_fmt.crop.width = fb.xres;
    output_fmt.crop.height = fb.yres > height ? height : fb.yres;
    output_fmt.width = fb.xres;
    output_fmt.height = fb.yres > height ? height : fb.yres;

    /* 确保裁剪图像大小不大于缩放后的图像大小 */
    if (output_fmt.crop.width > output_fmt.scaler.width) {
        fprintf(stderr, "crop width(%d) out of scaler width(%d) fix to(%d)\n", output_fmt.crop.width, output_fmt.scaler.width, output_fmt.scaler.width);
        output_fmt.crop.width = output_fmt.scaler.width;
        output_fmt.width = output_fmt.crop.width;
    }
    if (output_fmt.crop.height > output_fmt.scaler.height) {
        fprintf(stderr, "crop height(%d) out of scaler height(%d) fix to(%d)\n", output_fmt.crop.height, output_fmt.scaler.height, output_fmt.scaler.height);
        output_fmt.crop.height = output_fmt.scaler.height;
        output_fmt.height = output_fmt.crop.height;
    }
    /* 若定位后裁剪大小超出缩放得到的图像大小，取消裁剪 */
    if (((output_fmt.crop.top + output_fmt.crop.height) > output_fmt.scaler.height) ||
        ((output_fmt.crop.left + output_fmt.crop.width) > output_fmt.scaler.width)) {
            output_fmt.crop.left = 0;
            output_fmt.crop.top = 0;
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int fb_fd = -1;
    int isp_fd = -1;

    int loop = 0;
    int loops = 0;

    /* 参数检查 */
    if (argc < 2 || argc > 3)
        usage(argv[0], -1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argv[0], argc == 2 ? 0 : -1);

    if (argc == 2)
        loops = 0;

    if (argc == 3) {
        ret = sscanf(argv[2], "%d", &loops);
        if (ret != 1)
            usage(argv[0], -1);

        if (loops < 0)
            loops = 0;
    }

    isp_fd = isp_open(argv[1]);
    if (isp_fd == -ENODEV) {
        fprintf(stderr, "IMP ISP open device failed\n");
        return -ENODEV;
    }

    fb_fd = fb_open("/dev/fb1", &fb);
    if (fb_fd == -1) {
        fprintf(stderr, "fb open device failed\n");
        ret = -ENODEV;
        goto err_open_fb;
    }

    ret = fb_enable(fb_fd);
    if (ret) {
        fprintf(stderr, "fb enable failed\n");
        goto err_enable_fd;
    }

    ret = fb_pan_display_enable_user_cfg(fb_fd);
    if (ret) {
        fprintf(stderr, "fb enable user cfg failed\n");
        goto err_enable_user_cfg;
    }

    recalc_output_fmt_by_fb(isp_fd);

    isp_set_format(isp_fd, &output_fmt);

    ret = isp_requset_buffer(isp_fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "IMP ISP set request buffer failed\n");
        goto error_isp_request_buffer;
    }

    isp_get_info(isp_fd, &cam);

    ret = isp_mmap(isp_fd, &cam);
    if (ret < 0) {
        isp_free_buffer(isp_fd);
        close(isp_fd);
        return ret;
    }

    ret = isp_power_on(isp_fd);
    if (ret) {
        fprintf(stderr, "IMP ISP power on failed\n");
        goto err_power_on;
    }

    ret = isp_stream_on(isp_fd);
    if (ret) {
        fprintf(stderr, "IMP ISP stream on failed\n");
        goto err_stream_on;
    }

    void *old_frame = NULL;

    for (loop = 1; ((!loops) || loop <= loops); loop++) {
        void *mem = isp_wait_frame(isp_fd);
        if (!mem) {
            fprintf(stderr, "IMP ISP get frame failed\n");
            goto err_wait_frame;
        }

        void *y_mem = mem;
        void *uv_mem = NULL;
        int line_length = 0;

        uv_mem = mem + cam.line_length * cam.height;
        line_length = cam.line_length;

        struct lcdc_layer cfg = {
            .fb_fmt = fb_fmt_NV12,
            .xres = output_fmt.width,
            .yres = output_fmt.height,
            .xpos = 0,
            .ypos = 0,
            .layer_order = lcdc_layer_bottom,
            .layer_enable = 1,
            .scaling = {
                .enable = 0,
            },
            .y = {
                .stride = line_length,
                .mem = (void *)cam.phys_mem + (y_mem - cam.mapped_mem),
            },
            .uv = {
                .stride = line_length,
                .mem = (void *)cam.phys_mem + (uv_mem - cam.mapped_mem),
            },
            .alpha = {
                .enable = 0,
                .value = 0xff,
            },
        };

        ret = fb_pan_display_set_user_cfg(fb_fd, &cfg);
        if (ret)
            fprintf(stderr, "fb set user cfg failed %d\n", ret);

        fb_pan_display(fb_fd, &fb, 0);

        if (old_frame)
            isp_put_frame(isp_fd, old_frame);

        old_frame = mem;
    }

err_wait_frame:
    if (old_frame)
        isp_put_frame(isp_fd, old_frame);
    isp_stream_off(isp_fd);
err_stream_on:
    isp_power_off(isp_fd);
err_power_on:
    isp_free_buffer(isp_fd);
error_isp_request_buffer:
err_enable_user_cfg:
    fb_disable(fb_fd);
err_enable_fd:
    close(fb_fd);
err_open_fb:
    isp_close(isp_fd, &cam);

    return ret;
}
