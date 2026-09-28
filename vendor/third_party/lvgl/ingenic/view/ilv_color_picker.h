#ifndef _ILV_COLOR_PICKER_H_
#define _ILV_COLOR_PICKER_H_

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"

typedef void (*ilv_color_pick_event_t)(lv_obj_t *picker, lv_color_t color);

#define color_picker_obj_ilv_data_name "color-picker-obj"
#define color_picker_cb_ilv_data_name "color-picker-cb"

lv_obj_t *ilv_color_picker_create(lv_obj_t *parent, lv_obj_t *obj, ilv_color_pick_event_t cb);

lv_obj_t *ilv_color_picker_btn_create(lv_obj_t *parent, lv_obj_t *obj, ilv_color_pick_event_t cb);

#endif /* _ILV_COLOR_PICKER_H_ */
