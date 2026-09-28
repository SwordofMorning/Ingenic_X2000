#ifndef _ILV_STYLE_METER_H_
#define _ILV_STYLE_METER_H_

#include <stdint.h>

#include "ilv_style_type.h"

// see lvgl/src/extra/widgets/meter/lv_meter.h

enum {
    LV_style_meter_start_ = LV_style_extra_start,

    LV_style_meter_add_scale,             // 参数 参考 lv_meter_add_scale()
    LV_style_meter_scale_ticks,           // 参数 lv_meter_scale_ticks_t
    LV_style_meter_scale_major_ticks,     // 参数 lv_meter_scale_major_ticks_t
    LV_style_meter_scale_range,           // 参数 lv_meter_scale_range_t
    LV_style_meter_add_needle_line,       // 参数 lv_meter_needle_line_t
    LV_style_meter_add_needle_img,        // 参数 lv_meter_needle_img_t
    LV_style_meter_add_arc,               // 参数 lv_meter_arc_t
    LV_style_meter_add_scale_lines,       // 参数 lv_meter_scale_lines_t
    LV_style_meter_indicator_value,       // 参数 int32_t 参考 lv_meter_set_indicator_value()
    LV_style_meter_indicator_start_value, // 参数 int32_t 参考 lv_meter_set_indicator_start_value()
    LV_style_meter_indicator_end_value,   // 参数 int32_t 参考 lv_meter_set_indicator_end_value()
};

int ilv_meter_set_style(lv_obj_t *obj, int style, long value, int selector);

// see lv_meter_set_scale_ticks()
typedef struct lv_meter_scale_ticks {
    uint16_t cnt;    // 多少个刻度(以下用线表示,参考模拟时钟的样式)
    uint16_t width;  // 线的宽度
    uint16_t len;    // 线的长度
    uint32_t color;  // 线的颜色
} lv_meter_scale_ticks_t ;

#define METE_SCALE_TICKS(cnt, width, len, color) \
    (long)&(lv_meter_scale_ticks_t){cnt, width, len, color}

// see lv_meter_set_scale_major_ticks()
typedef struct lv_meter_scale_major_ticks {
    uint16_t nth;      // 每多少个刻度是主刻度,主刻度还会显示刻度的值
    uint16_t width;    // 主刻度的宽度
    uint16_t len;      // 主刻度的长度
    uint32_t color;    // 主刻度的颜色
    int16_t label_gap; // 主刻度和主刻度上显示的数字的距离
} lv_meter_scale_major_ticks_t;

#define METE_SCALE_MAJOR_TICKS(nth, width, len, color, label_gap) \
    (long)&(lv_meter_scale_major_ticks_t){nth, width, len, color, label_gap}

// see lv_meter_set_scale_range()
typedef struct lv_meter_scale_range {
    int32_t min;           // 刻度的最小值
    int32_t max;           // 刻度的最大值
                           // 当有主刻度的时候,最大值最小值会影响主刻度值的显示
    uint32_t angle_range;  // 圆弧的角度值,360度就是一个完整的圆
    uint32_t rotation;     // 顺时针3点钟,默认为起始, 这里指定他的偏移,比如偏移270度可以偏移到12点钟上去
} lv_meter_scale_range_t;

#define METE_SCALE_RANGE(min_, max_, angle_range, rotation) \
    (long)&(lv_meter_scale_range_t){min_, max_, angle_range, rotation}

// see lv_meter_add_needle_line()
typedef struct lv_meter_needle_line {
    uint16_t width;
    uint32_t color;
    int16_t r_mod;
} lv_meter_needle_line_t;

// see lv_meter_add_needle_img()
typedef struct lv_meter_needle_img {
    const void * src;
    lv_coord_t pivot_x;
    lv_coord_t pivot_y;
} lv_meter_needle_img_t;

// see lv_meter_add_arc()
typedef struct lv_meter_arc {
    uint16_t width;
    uint32_t color;
    int16_t r_mod;
} lv_meter_arc_t;

// see lv_meter_add_scale_lines()
typedef struct lv_meter_scale_lines {
    uint32_t color_start;
    uint32_t color_end;
    bool local;
    int16_t width_mod;
} lv_meter_scale_lines_t;

#endif /* _ILV_STYLE_METER_H_ */
