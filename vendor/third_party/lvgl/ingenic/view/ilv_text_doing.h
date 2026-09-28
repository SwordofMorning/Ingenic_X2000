#ifndef _ILV_TEXT_DOING_H_
#define _ILV_TEXT_DOING_H_

#include "lvgl/lvgl.h"

lv_obj_t *ilv_text_doing_view_create(lv_obj_t *parent);

void ilv_text_doing_set_text(lv_obj_t *obj, const char *text);

void ilv_text_doing_set_timeout(lv_obj_t *obj, int msecs);

void ilv_text_doing_set_text_color(lv_obj_t *obj, lv_color_t color);

void ilv_text_doing_set_text_opa(lv_obj_t *obj, int opa);

#endif /* _ILV_TEXT_DOING_H_ */
