#include <stdio.h>
#include "lvgl/lvgl.h"
#include "style/ilv_style.h"
#include "style/ilv_style_meter.h"

static lv_meter_indicator_t *indic = NULL;
static lv_meter_scale_t *scale = NULL;
static lv_obj_t *old_obj = NULL;

int ilv_meter_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (obj != old_obj) {
        indic = NULL;
        scale = NULL;
    }
    old_obj = obj;

    switch (style) {
    case LV_style_meter_add_scale:
        scale = lv_meter_add_scale(obj);
        break;
    case LV_style_meter_indicator_value:
        if (!indic)
            goto err_no_indicator;
        lv_meter_set_indicator_value(obj, indic, value);
        break;
    case LV_style_meter_indicator_start_value:
        if (!indic)
            goto err_no_indicator;
        lv_meter_set_indicator_start_value(obj, indic, value);
        break;
    case LV_style_meter_indicator_end_value:
        if (!indic)
            goto err_no_indicator;
        lv_meter_set_indicator_end_value(obj, indic, value);
        break;
    case LV_style_meter_add_arc: {
        if (!scale)
            goto err_no_scale;
        lv_meter_arc_t *t = (void *) value;
        indic = lv_meter_add_arc(obj, scale, t->width, 
            lv_color_from_int(t->color), t->r_mod);
        break;
    }
    case LV_style_meter_add_needle_line: {
        if (!scale)
            goto err_no_scale;
        lv_meter_needle_line_t *t = (void *) value;
        indic = lv_meter_add_needle_line(obj, scale, t->width, 
            lv_color_from_int(t->color), t->r_mod);
        break;
    }
    case LV_style_meter_add_needle_img: {
        if (!scale)
            goto err_no_scale;
        lv_meter_needle_img_t *t = (void *) value;
        indic = lv_meter_add_needle_img(obj, scale, t->src, 
            t->pivot_x, t->pivot_y);
        break;
    }
    case LV_style_meter_scale_ticks: {
        if (!scale)
            goto err_no_scale;
        lv_meter_scale_ticks_t *t = (void *) value;
        lv_meter_set_scale_ticks(obj, scale, t->cnt, t->width, t->len,
                lv_color_from_int(t->color));
        break;
    }
    case LV_style_meter_scale_major_ticks: {
        if (!scale)
            goto err_no_scale;
        lv_meter_scale_major_ticks_t *t = (void *) value;
        lv_meter_set_scale_major_ticks(obj, scale, t->nth, t->width, t->len,
                lv_color_from_int(t->color), t->label_gap);
        break;
    }    
    case LV_style_meter_scale_range: {
        if (!scale)
            goto err_no_scale;
        lv_meter_scale_range_t *t = (void *) value;
        lv_meter_set_scale_range(obj, scale, t->min, t->max, t->angle_range, t->rotation);
        break;
    }
    case LV_style_meter_add_scale_lines: {
        if (!scale)
            goto err_no_scale;
        lv_meter_scale_lines_t *t = (void *) value;
        indic = lv_meter_add_scale_lines(obj, scale, lv_color_from_int(t->color_start),
                lv_color_from_int(t->color_end), t->local, t->width_mod);
        break;
    }
    default:
        fprintf(stderr, "ilv_meter: ingore unknown sytle: %d\n", style);
        return -1;
    }

    return 0;

err_no_indicator:
    fprintf(stderr, "ilv_meter: must add indicator: %d\n", style);
    return -1;
err_no_scale:
    fprintf(stderr, "ilv_meter: must add scale: %d\n", style);
    return -1;
}
