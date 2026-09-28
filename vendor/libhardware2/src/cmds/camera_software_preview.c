#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <libhardware2/camera.h>
#include <libhardware2/fb.h>
#include <time.h>
#include <stdint.h>
#include <libutils2/bayer16_to_rgb.h>
#include <libutils2/yuv422_to_rgb.h>
#include <libutils2/nv12_to_rgb.h>
#include <libutils2/raw8_to_rgb.h>

#define min(a, b) ((a) > (b)) ? (b) : (a)
#define next_line(addr, len) ((void *)(addr) + (len))

static struct camera_info cam_info;
static struct fb_device_info fb_info;
static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                          : show help info\n");
    fprintf(stderr, "    cam_device_path  fb_device_path    : fb show camera data\n");
    fprintf(stderr, "    Example: %s  /dev/camera /dev/fb0\n", prg_name);
    fprintf(stderr, "    x2000-Example: %s  /dev/vic0 /dev/fb1\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

static inline unsigned int get_fb_offset(int w, int h)
{
    int fb_offset = 0;

    if (fb_info.xres > w)
        fb_offset += fb_bytes_per_pixel(fb_info.fb_fmt) * (fb_info.xres - w) / 2;

    if (fb_info.yres > h)
        fb_offset += fb_info.line_length * (fb_info.yres - h) / 2;

    return fb_offset;
}

static inline unsigned int get_cam_offset(int w, int h)
{
    int cam_y_offset = 0;

    if (cam_info.width > w)
        cam_y_offset += cam_bytes_per_pixel(cam_info.data_fmt) * (cam_info.width - w) / 2;

    if (cam_info.height > h)
        cam_y_offset += cam_info.line_length * (cam_info.height - h) / 2;

    return cam_y_offset;
}

static inline unsigned int get_cam_uv_offset(int w, int h)
{
    int cam_uv_offset = cam_info.width * cam_info.height;

    if (cam_info.width > w)
        cam_uv_offset += cam_bytes_per_pixel(cam_info.data_fmt) * (cam_info.width - w) / 2;

    if (cam_info.height > h)
        cam_uv_offset += cam_info.line_length * (cam_info.height - h) / 4;

    return cam_uv_offset;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int cam_fd;
    int fb_fd = -1;
    int indexs[2];
    void *frames[2];
    int num = 0;
    int w = 0, h = 0;
    int fb_offset = 0;
    int cam_offset = 0;
    int cam_uv_offset = 0;

    prg_name = argv[0];

    if (argc < 3)
        usage(-1);

    cam_fd = camera_open(&cam_info, argv[1]);
    if (cam_fd < 0)
        return -1;

    ret = camera_power_on(cam_fd);
    if (ret)
        goto close_cam;

    ret = camera_stream_on(cam_fd);
    if (ret)
        goto power_down_cam;

    fb_fd = fb_open(argv[2], &fb_info);
    if (fb_fd < 0)
        goto stream_off_cam;

    if (fb_info.frame_nums <= 0) {
        fprintf(stderr, "fb: no frame found\n");
        ret = -1;
        goto close_fb;
    }

    indexs[0] = (fb_info.frame_nums < 2) ? 0 : 1;
    indexs[1] = 0;

    /* 计算实际显示图像尺寸 */
    w = min(fb_info.xres, cam_info.width);
    h = min(fb_info.yres, cam_info.height);

    /* 根据显示图像尺寸，cam、fb地址计算对应偏移
     * uv偏移仅在cam_fmt为nv12格式时使用
     */
    fb_offset = get_fb_offset(w, h);
    cam_offset = get_cam_offset(w, h);
    cam_uv_offset = get_cam_uv_offset(w, h);

    ret = fb_enable(fb_fd);
    if (ret)
        goto close_fb;

    while (1) {
        void *cam_mem = camera_wait_frame(cam_fd);
        if (!cam_mem)
            goto disable_fb;

        frames[num] = fb_info.mapped_mem + fb_info.frame_size * num;

        if (camera_fmt_is_16BIT(cam_info.data_fmt)) {
            bayer16_to_rgb((cam_mem + cam_offset), cam_info.data_fmt, cam_info.line_length,
                           (frames[num] + fb_offset), fb_info.fb_fmt, fb_info.line_length,
                           w, h);
        } else if (camera_fmt_is_YUV422(cam_info.data_fmt)) {
            yuv422_to_rgb((cam_mem + cam_offset), cam_info.data_fmt, cam_info.line_length,
                          (frames[num] + fb_offset), fb_info.fb_fmt, fb_info.line_length,
                          w, h);
        } else if (camera_fmt_is_NV12(cam_info.data_fmt)) {
            nv12_to_rgb((cam_mem + cam_offset), (cam_mem + cam_uv_offset), cam_info.data_fmt, cam_info.line_length,
                        (frames[num] + fb_offset), fb_info.fb_fmt, fb_info.line_length,
                        w, h);
        } else if (camera_fmt_is_8BIT(cam_info.data_fmt)) {
            if (cam_info.data_fmt == CAMERA_PIX_FMT_GREY) {
                y8_to_rgb((cam_mem + cam_offset), cam_info.data_fmt, cam_info.line_length,
                          (frames[num] + fb_offset), fb_info.fb_fmt, fb_info.line_length,
                          w, h);
            } else {
                raw8_to_rgb((cam_mem + cam_offset), cam_info.data_fmt, cam_info.line_length/2,
                            (frames[num] + fb_offset), fb_info.fb_fmt, fb_info.line_length,
                            w, h);
            }
        } else {
            fprintf(stderr, "this camera fmt not support now: %x\n", cam_info.data_fmt);
            goto disable_fb;
        }

        ret = fb_pan_display(fb_fd, &fb_info, num);
        if (ret)
            goto disable_fb;

        ret = camera_put_frame(cam_fd, cam_mem);
        if (ret)
            goto disable_fb;

        num = indexs[num];
    }

    return 0;

disable_fb:
    fb_disable(fb_fd);

close_fb:
    fb_close(fb_fd, &fb_info);

stream_off_cam:
    camera_stream_off(cam_fd);

power_down_cam:
    camera_power_off(cam_fd);

close_cam:
    camera_close(cam_fd, &cam_info);
    return -1;
}
