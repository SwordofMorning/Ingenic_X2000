#ifndef _ILV_DIGITAL_CLOCK_H_
#define _ILV_DIGITAL_CLOCK_H_

#include "lvgl/lvgl.h"

lv_obj_t *ilv_digital_time_view_create(lv_obj_t *parent);

void ilv_digital_time_set_fill_zero(lv_obj_t *obj, int is_fill_zero);

void ilv_digital_time_set_chiness(lv_obj_t *obj, int is_chiness);

lv_obj_t *ilv_digital_second_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_minute_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_hour_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_day_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_month_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_year_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_weekday_view_create(lv_obj_t *parent);

lv_obj_t *ilv_digital_year_full_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_msec_view_create(lv_obj_t *parent);
lv_obj_t *ilv_digital_cmsec_view_create(lv_obj_t *parent);

#endif /* _ILV_DIGITAL_CLOCK_H_ */
