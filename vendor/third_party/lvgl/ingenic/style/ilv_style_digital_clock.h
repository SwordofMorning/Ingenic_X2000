#ifndef _ILV_STYLE_DIGITAL_H_
#define _ILV_STYLE_DIGITAL_H_

#include "view/ilv_digital_clock.h"
#include "ilv_style_type.h"

enum {
    LV_style_digital_time_start_ = LV_style_extra_start,

    LV_style_digital_time_chiness,         // 参数 int, 参考 ilv_digital_time_set_chiness
    LV_style_digital_time_fill_zero,       // 参数 int, 参考 ilv_digital_time_set_fill_zero
};

int ilv_digital_time_set_style(lv_obj_t *obj, int style, long value, int selector);

struct ilv_parser;
int ilv_digital_time_parse_style(struct ilv_parser *parser, const char *key, const char *val);

#endif /* _ILV_STYLE_DIGITAL_H_ */
