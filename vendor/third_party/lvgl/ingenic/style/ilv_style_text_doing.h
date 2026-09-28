#ifndef _ILV_STYLE_TEXT_DOING_H_
#define _ILV_STYLE_TEXT_DOING_H_

#include "view/ilv_text_doing.h"
#include "ilv_style_type.h"

// 参考 ingenic/view/ilv_text_doing.h

enum {
    LV_style_text_doing_start_ = LV_style_extra_start,

    LV_style_text_doing_text,         // 参数 const char *, 参考 ilv_text_doing_set_text()
    LV_style_text_doing_timeout,      // 参数 int, 参考 ilv_text_doing_set_timeout()
    LV_style_text_doing_text_color,   // 参数 int, 16进制颜色值
    LV_style_text_doing_text_opa,   // 参数 int, 颜色透明度
};

int ilv_text_doing_set_style(lv_obj_t *obj, int style, long value, int selector);

struct ilv_parser;
int ilv_text_doing_parse_style(struct ilv_parser *parser, const char *key, const char *val);

#endif /* _ILV_STYLE_TEXT_DOING_H_ */
