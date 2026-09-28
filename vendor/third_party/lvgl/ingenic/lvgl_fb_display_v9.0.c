#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libhardware2/fb.h>

#include "lvgl/lvgl.h"

#include "lv_conf.h"

static int fb_fd;
static struct fb_device_info fb_info;
static void *draw_tmp_buf;
static void *display_buf[2];
static lv_disp_t *lv_disp;
static int is_enable = 0;
static int display_index = 0;

static void fb_display_flush(struct _lv_disp_t *disp, const lv_area_t * area, uint8_t * px_map)
{
    if (lv_disp_flush_is_last(disp)) {
        if (!is_enable) {
            fb_enable(fb_fd);
            is_enable = 1;
        }
        fb_pan_display(fb_fd, &fb_info, display_index);
        if (display_buf[0] != display_buf[1])
            display_index = !display_index;
    }

    lv_disp_flush_ready(disp);
}

int lvgl_init_fb_display(const char *fb_path)
{
    if (!fb_path)
        fb_path = "/dev/fb0";

    fb_fd = fb_open(fb_path, &fb_info);
    if (fb_fd == -1)
        return -1;

    if (!fb_info.frame_nums) {
        fprintf(stderr, "fb: %s no frame buffer\n", fb_path);
        return -1;
    }

    switch (fb_info.fb_fmt) {
    case fb_fmt_RGB565:
        if (LV_COLOR_DEPTH != 16) {
            fprintf(stderr, "lvgl not match color: %d %d\n", fb_info.fb_fmt, LV_COLOR_DEPTH);
            return -1;
        }
        break;
    case fb_fmt_ARGB8888:
    case fb_fmt_RGB888:
        if (LV_COLOR_DEPTH != 32) {
            fprintf(stderr, "lvgl not match color: %d %d\n", fb_info.fb_fmt, LV_COLOR_DEPTH);
            return -1;
        }
        break;
    default:
        fprintf(stderr, "lvgl not support this format: %d\n", fb_info.fb_fmt);
        fb_close(fb_fd, &fb_info);
        return -1;
    }

    int bytes = fb_bytes_per_pixel(fb_info.fb_fmt);
    int xres = fb_info.xres;
    int yres = fb_info.yres;

    draw_tmp_buf = malloc(xres*100*bytes);

    display_buf[0] = fb_info.mapped_mem;
    display_buf[1] = fb_info.mapped_mem + (1%fb_info.frame_nums)*fb_info.frame_size;

    lv_disp_t *disp = lv_disp_create(xres, yres);
    lv_disp_set_flush_cb(disp, fb_display_flush);
    lv_disp_set_draw_buffers(disp, display_buf[0], display_buf[1], xres*fb_info.line_length, LV_DISP_RENDER_MODE_DIRECT);

    return 0;
}
