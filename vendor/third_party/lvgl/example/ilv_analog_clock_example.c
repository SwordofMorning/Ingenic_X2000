#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "view/ilv_rotate_img.h"
#include "lvgl_ingenic_support.h"

#include <time.h>
#include <sys/time.h>

static lv_obj_t *img_second;
static lv_obj_t *img_minute;
static lv_obj_t *img_hour;

static void set_rotate_value(void * indic, int32_t v)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = localtime(&tv.tv_sec);

    float s_angle = (tm->tm_sec*6.0 + tv.tv_usec*6.0/1000000);
    ilv_rotate_img_set_angle(img_second, s_angle*10);

    float m_angle = (tm->tm_min*6.0 + tm->tm_sec*6.0/60);
    ilv_rotate_img_set_angle(img_minute, m_angle*10);

    float h_angle = (tm->tm_hour*30.0 + tm->tm_min*30.0/60);
    ilv_rotate_img_set_angle(img_hour, h_angle*10);

    printf("time: %d.%03d angle:%f\n", tm->tm_sec, tv.tv_usec/1000, s_angle);
}

void ilv_rotate_img_example(lv_obj_t *root)
{
    int ret;
    lvgl_set_fb_show_frame_rate(1);

    lv_obj_t *meter = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter, 640, 640);
    lv_obj_center(meter);

    lv_meter_scale_t * scale_min = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter, scale_min, 0, 60, 360, 270);

    lv_meter_scale_t * scale_hour = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_hour, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY));               /*12 ticks*/
    lv_meter_set_scale_major_ticks(meter, scale_hour, 1, 2, 20, lv_color_black(), 10);    /*Every tick is major*/
    lv_meter_set_scale_range(meter, scale_hour, 1, 12, 330, 300);       /*[1..12] values in an almost full circle*/

    lv_obj_update_layout(meter);
    int dst_w = lv_obj_get_content_width(meter);
    int dst_h = lv_obj_get_content_height(meter);

    ilv_rotate_img_cfg_t cfg = {
        .bilinear = 1,
    };

    img_hour = ilv_rotate_img_create(meter);
    ilv_rotate_img_set_src(img_hour, "/usr/data/res/hour.png");
    ilv_rotate_img_set_src_center(img_hour, 12, 110);
    ilv_rotate_img_set_dst_center(img_hour, dst_w/2, dst_h/2);
    ilv_rotate_img_set_extra_cfg(img_hour, &cfg);

    img_minute = ilv_rotate_img_create(meter);
    ilv_rotate_img_set_src(img_minute, "/usr/data/res/minute.png");
    ilv_rotate_img_set_src_center(img_minute, 12, 150);
    ilv_rotate_img_set_dst_center(img_minute, dst_w/2, dst_h/2);
    ilv_rotate_img_set_extra_cfg(img_minute, &cfg);

    img_second = ilv_rotate_img_create(meter);
    ilv_rotate_img_set_src(img_second, "/usr/data/res/second.png");
    ilv_rotate_img_set_src_center(img_second, 12, 264);
    ilv_rotate_img_set_dst_center(img_second, dst_w/2, dst_h/2);
    ilv_rotate_img_set_extra_cfg(img_second, &cfg);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, set_rotate_value);
    lv_anim_set_values(&a, 0, 60);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_time(&a, 100);
    lv_anim_set_var(&a, NULL);
    lv_anim_start(&a);
}
