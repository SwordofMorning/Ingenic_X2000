#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "view/ilv_analog_clock.h"
#include "style/ilv_style_meter.h"
#include "style/ilv_style_rotate_img.h"
#include "utils/ilv_time.h"

static ilv_style_t analog_clock_layer_attrs[] = {
    {LV_style_view, LV_VIEW("meter", "meter")},
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_PCT(100)},
    {LV_style_align, LV_ALIGN_CENTER},
    {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
    {LV_style_meter_add_scale, 0},
    {LV_style_meter_scale_ticks, METE_SCALE_TICKS(61, 1, 10, LV_COLOR_GREY)},
    {LV_style_meter_scale_range, METE_SCALE_RANGE(0, 60, 360, 270)},
    {LV_style_meter_add_scale, 0},
    {LV_style_meter_scale_ticks, METE_SCALE_TICKS(12, 0, 0, LV_COLOR_GREY)},
    {LV_style_meter_scale_major_ticks, METE_SCALE_MAJOR_TICKS(1, 2, 20, LV_COLOR_BLACK, 10)},
    {LV_style_meter_scale_range, METE_SCALE_RANGE(1, 12, 330, 300)},
    {0, 0},
};

lv_obj_t *ilv_analog_clock_view_create(lv_obj_t *parent)
{
    return ilv_create_view(parent, analog_clock_layer_attrs);
}

/*************************************************************************/
static void rotate_hour_timer_cb(struct _lv_timer_t *timer)
{
    lv_obj_t *obj = timer->user_data;

    struct tm tm;
    ilv_get_time(&tm, NULL);

    float h_angle = (tm.tm_hour*30.0 + tm.tm_min*30.0/60);
    ilv_rotate_img_set_angle(obj, h_angle*10);
}

static ilv_style_t rotate_hour_attrs[] = {
    {LV_style_view, LV_VIEW("rotate_img", "hour")},
    {LV_style_add_timer, LV_TIMER(rotate_hour_timer_cb, 100)},
    {0, 0},
};

lv_obj_t *ilv_rotate_hour_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = ilv_create_view(parent, rotate_hour_attrs);
    return obj;
}

/*************************************************************************/
static void rotate_minute_timer_cb(struct _lv_timer_t *timer)
{
    lv_obj_t *obj = timer->user_data;

    struct tm tm;
    ilv_get_time(&tm, NULL);

    float m_angle = (tm.tm_min*6.0 + tm.tm_sec*6.0/60);
    ilv_rotate_img_set_angle(obj, m_angle*10);
}

static ilv_style_t rotate_minute_attrs[] = {
    {LV_style_view, LV_VIEW("rotate_img", "minute")},
    {LV_style_add_timer, LV_TIMER(rotate_minute_timer_cb, 100)},
    {0, 0},
};

lv_obj_t *ilv_rotate_minute_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = ilv_create_view(parent, rotate_minute_attrs);
    return obj;
}

/*************************************************************************/
static void rotate_second_timer_cb(struct _lv_timer_t *timer)
{
    lv_obj_t *obj = timer->user_data;

    struct tm tm;
    int usecs;
    ilv_get_time(&tm, &usecs);

    float s_angle = (tm.tm_sec*6.0 + usecs*6.0/1000000);
    ilv_rotate_img_set_angle(obj, s_angle*10);
}

static ilv_style_t rotate_second_attrs[] = {
    {LV_style_view, LV_VIEW("rotate_img", "second")},
    {LV_style_add_timer, LV_TIMER(rotate_second_timer_cb, 10)},
    {0, 0},
};

lv_obj_t *ilv_rotate_second_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = ilv_create_view(parent, rotate_second_attrs);
    return obj;
}
