#ifndef _UI_DESKTOP_H_
#define _UI_DESKTOP_H_

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"

lv_obj_t *ui_desktop_init(void);
lv_obj_t *ui_get_desktop(void);

void app_camera_click_event(lv_event_t *e);
void app_camera_start(void);

void app_player_click_event(lv_event_t *e);

void app_setting_click_event(lv_event_t *e);

void app_brightness_click_event(lv_event_t *e);


#endif /* _UI_DESKTOP_H_ */
