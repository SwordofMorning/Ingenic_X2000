#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ilv_digital_clock.h"
#include "utils/ilv_time.h"

enum ilv_tm_type {
    TM_sec,
    TM_min,
    TM_hour,
    TM_mday,
    TM_mon,
    TM_year,
    TM_wday,

    TM_year_full,
    TM_msec,
    TM_cmsec,
};

struct m_data {
    int type;
    int is_chiness;
    int is_fill_zero;

    int value;
};

static struct m_data *get_data(lv_obj_t *obj)
{
    struct m_data *data = lv_obj_get_user_data(obj);
    if (!data) {
        data = malloc(sizeof(*data));
        data->type = TM_sec;
        data->is_chiness = 0;
        data->is_fill_zero = 1;
        data->value = -1;
        lv_obj_set_user_data(obj, data);
        ilv_add_delete_data(obj, data, free);
    }
    return data;
}

static void digital_time_timer_cb(struct _lv_timer_t *timer)
{
    lv_obj_t *obj = timer->user_data;
    struct m_data *data = get_data(obj);

    struct tm tm;
    int usecs;
    ilv_get_time(&tm, &usecs);

    const char ch[][4] = {"零","一","二","三","四","五","六","七","八","九" };
    const char en[][4] = {"0","1","2","3","4","5","6","7","8","9"};

    const char (*p)[4] = data->is_chiness ? &ch[0] : &en[0];

    int value = 0;
    switch (data->type) {
    case TM_sec: value = tm.tm_sec; break;
    case TM_min: value = tm.tm_min; break;
    case TM_hour: value = tm.tm_hour; break;
    case TM_mday: value = tm.tm_mday; break;
    case TM_mon: value = tm.tm_mon+1; break;
    case TM_year: value = tm.tm_year; break;
    case TM_wday: value = tm.tm_wday; break;
    case TM_year_full: value = tm.tm_year+1900; break;
    case TM_msec: value = usecs/1000;  break;
    case TM_cmsec: value = usecs/1000/10; break;
    }

    if (data->value == value)
        return;
    data->value = value;

    int v0 = value%10;
    int v1 = (value/10)%10;
    int v2 = (value/100)%10;
    int v3 = (value/1000)%10;

    int fill = data->is_fill_zero;
    int cnt = 2;
    if (data->type == TM_year_full)
        cnt = 4;
    if (data->type == TM_msec)
        cnt = 3;
    if (data->type == TM_wday)
        cnt = 1;

    if (cnt >=4 && (fill || v3))
        lv_label_set_text_fmt(obj, "%s%s%s%s", p[v3], p[v2], p[v1], p[v0]);
    else if (cnt >= 3 && (fill || v2))
        lv_label_set_text_fmt(obj, "%s%s%s", p[2], p[v1], p[v0]);
    else if (cnt >= 2 && (fill || v1))
        lv_label_set_text_fmt(obj, "%s%s", p[v1], p[v0]);
    else
        lv_label_set_text_fmt(obj, "%s", p[v0]);
}

static ilv_style_t digital_time_attrs[] = {
    {LV_style_view, LV_VIEW("label", "second")},
    {LV_style_add_timer, LV_TIMER(digital_time_timer_cb, 10)},
    {0, 0},
};

static void ilv_digital_time_set_type(lv_obj_t *obj, enum ilv_tm_type type)
{
    struct m_data *data = get_data(obj);
    data->type = type;
}

void ilv_digital_time_set_fill_zero(lv_obj_t *obj, int is_fill_zero)
{
    struct m_data *data = get_data(obj);
    if (data->is_fill_zero != is_fill_zero)
        data->value = -1;
    data->is_fill_zero = is_fill_zero;
}

void ilv_digital_time_set_chiness(lv_obj_t *obj, int is_chiness)
{
    struct m_data *data = get_data(obj);
    if (data->is_chiness != is_chiness)
        data->value = -1;
        
    data->is_chiness = is_chiness;
}

static lv_obj_t *create_view(lv_obj_t *parent, enum ilv_tm_type type)
{
    lv_obj_t *obj = ilv_create_view(parent, digital_time_attrs);
    ilv_digital_time_set_type(obj, type);
    return obj;
}

lv_obj_t *ilv_digital_second_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_sec);
}

lv_obj_t *ilv_digital_minute_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_min);
}

lv_obj_t *ilv_digital_hour_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_hour);
}

lv_obj_t *ilv_digital_day_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_mday);
}

lv_obj_t *ilv_digital_month_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_mon);
}

lv_obj_t *ilv_digital_year_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_year);
}

lv_obj_t *ilv_digital_weekday_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_wday);
}

lv_obj_t *ilv_digital_year_full_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_year_full);
}

lv_obj_t *ilv_digital_msec_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_msec);
}

lv_obj_t *ilv_digital_cmsec_view_create(lv_obj_t *parent)
{
    return create_view(parent, TM_cmsec);
}
