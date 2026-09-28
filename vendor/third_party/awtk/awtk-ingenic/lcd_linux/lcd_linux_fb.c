/**
 * File:   lcd_linux_fb.h
 * Author: AWTK Develop Team
 * Brief:  linux framebuffer lcd
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-09-07 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include <stdio.h>
#include <signal.h>
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/thread.h"
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "tkc/mutex.h"
#include "tkc/semaphore.h"
#include "blend/image_g2d.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "base/lcd_orientation_helper.h"
#include <libhardware2/fb.h>

static struct fb_device_info fb_info;
static int fb;
static int frame_index = 0;

static void on_signal_int(int sig)
{
    tk_quit();
}

static void on_app_exit(void)
{
    fb_disable(fb);
    fb_close(fb, &fb_info);

    log_debug("on_app_exit\n");
}

static ret_t (*lcd_mem_linux_flush_default)(lcd_t *lcd);
static ret_t lcd_mem_linux_flush(lcd_t *lcd)
{
    if (lcd_mem_linux_flush_default)
        lcd_mem_linux_flush_default(lcd);

    fb_pan_display(fb, &fb_info, frame_index);
    return RET_OK;
}

static lcd_t *lcd_linux_create_flushable(struct fb_device_info info)
{
    lcd_t *lcd = NULL;

    uint8_t *fb_mem = (uint8_t *)(info.mapped_mem);

    if (info.bits_per_pixel == 16)
        lcd = lcd_mem_bgr565_create_single_fb(
            info.xres, info.yres, fb_mem);
    if (info.bits_per_pixel == 24 || info.bits_per_pixel == 32)
        lcd = lcd_mem_bgra8888_create_single_fb(
            info.xres, info.yres, fb_mem);

    if (lcd != NULL) {
        lcd_mem_linux_flush_default = lcd->flush;
        lcd->flush = lcd_mem_linux_flush;
        lcd_mem_set_line_length(lcd, info.line_length);
    }

    return lcd;
}

static ret_t lcd_mem_linux_write_buff(lcd_t *lcd)
{
    lcd_mem_t *mem = (lcd_mem_t *)lcd;
    uint8_t *online_fb = (uint8_t *)(fb_info.mapped_mem);
    uint8_t *offline_fb = (uint8_t *)(fb_info.mapped_mem + fb_info.frame_size);

    if (frame_index == 0) {
        lcd_mem_set_offline_fb(mem, online_fb);
        lcd_mem_set_online_fb(mem, offline_fb);
    }
    else {
        lcd_mem_set_offline_fb(mem, offline_fb);
        lcd_mem_set_online_fb(mem, online_fb);
    }

    frame_index = !frame_index;
    fb_pan_display(fb, &fb_info, frame_index);

    return RET_OK;
}

static lcd_t *lcd_linux_create_swappable(struct fb_device_info info)
{
    lcd_t *lcd = NULL;

    uint8_t *online_fb = (uint8_t *)(info.mapped_mem);
    uint8_t *offline_fb = (uint8_t *)(info.mapped_mem + info.frame_size);
    return_value_if_fail(offline_fb != NULL, NULL);

    if (info.bits_per_pixel == 16) {
        lcd = lcd_mem_bgr565_create_double_fb(info.xres, info.yres, online_fb, offline_fb);
    }
    else if (info.bits_per_pixel == 24 || info.bits_per_pixel == 32) {
        lcd = lcd_mem_bgra8888_create_double_fb(info.xres, info.yres, online_fb, offline_fb);
    }

    if (lcd != NULL) {
        lcd->swap = lcd_mem_linux_write_buff;
        lcd_mem_set_line_length(lcd, info.line_length);
    }
    if (lcd == NULL) {
        printf("open lcd fail\n\n");
    }

    return lcd;
}

static lcd_t *lcd_linux_create(struct fb_device_info info)
{
    return_value_if_fail(info.frame_nums > 0, NULL);

    if (info.frame_nums == 1)
        return lcd_linux_create_flushable(info);

    if (info.frame_nums >= 2)
        return lcd_linux_create_swappable(info);

    return NULL;
}

lcd_t *lcd_linux_fb_create(const char *filename)
{
    lcd_t *lcd = NULL;

    return_value_if_fail(filename != NULL, NULL);

    fb = fb_open(filename, &fb_info);

    lcd = lcd_linux_create(fb_info);

    fb_enable(fb);

    atexit(on_app_exit);
    signal(SIGINT, on_signal_int);

    return lcd;
}

unsigned long lcd_linux_fb_get_phyaddr(void *fb_data, int *index)
{
    unsigned long fb_physical_addr = 0;

    if (fb_data < fb_info.mapped_mem || fb_data > fb_info.mapped_mem + (fb_info.frame_size * fb_info.frame_nums))
        return 0;

    fb_physical_addr = fb_info.fix.smem_start + (fb_data - fb_info.mapped_mem);
    if ((fb_physical_addr - fb_info.fix.smem_start) % fb_info.frame_size) {
        printf("lcd_linux_fb: get fb phy addr is error, phy addr = %lx\n", fb_physical_addr);
        assert(0);
    }

    *index = (fb_physical_addr - fb_info.fix.smem_start) / fb_info.frame_size;

    return fb_physical_addr;
}

unsigned int lcd_linux_fb_get_size(int *width, int *height, int *linesize)
{
    *width = fb_info.xres;
    *height = fb_info.yres;
    *linesize = fb_info.line_length;

    return fb_info.frame_size;
}
