#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"
#include "view/ilv_rotate_img.h"
#include "style/ilv_style_meter.h"
#include "style/ilv_style_rotate_img.h"

#define METER_WIDTH 640
#define METER_HEIGHT 640

static int app_is_show = 0;
static lv_obj_t *analog_clock_root;

static lv_obj_t *img_second;
static lv_obj_t *img_minute;
static lv_obj_t *img_hour;

static void back_to_desktop_event(lv_event_t *e)
{
    lv_obj_del(analog_clock_root);

    lv_obj_t *desktop = ui_get_desktop();
    if (!desktop)
        desktop = ui_desktop_init();
    lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    img_second = NULL;
    img_minute = NULL;
    img_hour = NULL;

    app_is_show = 0;
}

static void set_time_value(void * indic, int32_t v)
{
    lv_obj_t *meter = (void *)indic;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = localtime(&tv.tv_sec);

    if (!img_second)
        img_second = ilv_get_child(meter, "second");
    if (!img_minute)
        img_minute = ilv_get_child(meter, "minute");
    if (!img_hour)
        img_hour = ilv_get_child(meter, "hour");

    if (!img_second)
        return;

    float s_angle = (tm->tm_sec*6.0 + tv.tv_usec*6.0/1000000);
    ilv_rotate_img_set_angle(img_second, s_angle*10);

    float m_angle = (tm->tm_min*6.0 + tm->tm_sec*6.0/60);
    ilv_rotate_img_set_angle(img_minute, m_angle*10);

    float h_angle = (tm->tm_hour*30.0 + tm->tm_min*30.0/60);
    ilv_rotate_img_set_angle(img_hour, h_angle*10);

    // printf("time: %d.%03d angle:%f\n", tm->tm_sec, tv.tv_usec/1000, s_angle);
}

ilv_style_t app_analog_clock_layer_attrs[] = {
    {LV_style_view, LV_VIEW("obj", "analog_clock_root")},
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_PCT(100)},
    {LV_style_bg_opa, 0x0},
    {LV_style_border_side, LV_BORDER_SIDE_NONE},
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},
    {LV_style_layout_start},

        {LV_style_view, LV_VIEW("meter", "meter")},
        {LV_style_width, METER_WIDTH},
        {LV_style_height, METER_HEIGHT},
        {LV_style_align, LV_ALIGN_CENTER},
        {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
        {LV_style_meter_add_scale, 0},
        {LV_style_meter_scale_ticks, METE_SCALE_TICKS(61, 1, 10, LV_COLOR_GREY)},
        {LV_style_meter_scale_range, METE_SCALE_RANGE(0, 60, 360, 270)},
        {LV_style_meter_add_scale, 0},
        {LV_style_meter_scale_ticks, METE_SCALE_TICKS(12, 0, 0, LV_COLOR_GREY)},
        {LV_style_meter_scale_major_ticks, METE_SCALE_MAJOR_TICKS(1, 2, 20, LV_COLOR_BLACK, 10)},
        {LV_style_meter_scale_range, METE_SCALE_RANGE(1, 12, 330, 300)},
        {LV_style_add_anim, LV_ANIM(0, 100, 1000, set_time_value, LV_ANIM_REPEAT_INFINITE)},
        {LV_style_layout_start},

            {LV_style_view, LV_VIEW("rotate_img", "hour")},
            {LV_style_rotate_img_src, LV_STR("./res/hour.png")},
            {LV_style_rotate_img_src_center, LV_POINT(12, 110)},
            {LV_style_rotate_img_dst_center, LV_POINT(METER_WIDTH/2, METER_HEIGHT/2)},
            {LV_style_rotate_img_extra_cfg, ROTATE_IMG_EXTRA_CFG(1, 0, NULL)},

            {LV_style_view, LV_VIEW("rotate_img", "minute")},
            {LV_style_rotate_img_src, LV_STR("./res/minute.png")},
            {LV_style_rotate_img_src_center, LV_POINT(12, 150)},
            {LV_style_rotate_img_dst_center, LV_POINT(METER_WIDTH/2, METER_HEIGHT/2)},
            {LV_style_rotate_img_extra_cfg, ROTATE_IMG_EXTRA_CFG(1, 0, NULL)},

            {LV_style_view, LV_VIEW("rotate_img", "second")},
            {LV_style_rotate_img_src, LV_STR("./res/second.png")},
            {LV_style_rotate_img_src_center, LV_POINT(12, 264)},
            {LV_style_rotate_img_dst_center, LV_POINT(METER_WIDTH/2, METER_HEIGHT/2)},
            {LV_style_rotate_img_extra_cfg, ROTATE_IMG_EXTRA_CFG(1, 0, NULL)},

        {LV_style_layout_end},
    {LV_style_layout_end},

    {0, 0},
};

void app_analog_clock_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    analog_clock_root = ilv_create_view(lv_scr_act(), app_analog_clock_layer_attrs);
    set_time_value(ilv_get_child(analog_clock_root, "meter"), 0);
    app_is_show = 1;
}

void app_analog_clock_start(void)
{
    app_analog_clock_click_event(NULL);
}
