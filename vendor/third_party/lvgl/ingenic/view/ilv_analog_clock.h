#ifndef _ILV_ANALOG_CLOCK_H_
#define _ILV_ANALOG_CLOCK_H_

#include "lvgl/lvgl.h"

lv_obj_t *ilv_analog_clock_view_create(lv_obj_t *parent);
lv_obj_t *ilv_rotate_hour_view_create(lv_obj_t *parent);
lv_obj_t *ilv_rotate_minute_view_create(lv_obj_t *parent);
lv_obj_t *ilv_rotate_second_view_create(lv_obj_t *parent);

#endif /* _ILV_ANALOG_CLOCK_H_ */
