#ifndef _ILV_UTILS_H_
#define _ILV_UTILS_H_

#include "lvgl/lvgl.h"
#include "style/ilv_style.h"
#include "utils/ilv_color.h"

typedef void (*ilv_userdata_free_t)(void *data);

typedef struct ilv_userdata_dsc_ {
    const char *name;
    void *data;
    ilv_userdata_free_t free_cb;
} ilv_userdata_dsc_t;

void ilv_add_user_data(lv_obj_t *obj,
     const char *name, void *userdata, ilv_userdata_free_t free_cb);

void ilv_del_user_data(lv_obj_t *obj, const char *name);

void *ilv_get_user_data(lv_obj_t *obj, const char *name);

void *ilv_check_malloc_user_data(lv_obj_t *obj, const char *name, int size);

void ilv_add_delete_data(lv_obj_t *obj, void *data, ilv_userdata_free_t free_cb);

lv_style_t *ilv_load_font(const char *path, int height);
void ilv_del_font(lv_style_t *style);

void ilv_set_as_circle(lv_obj_t *obj, int width);
void ilv_set_bg_color_opa(lv_obj_t *obj, lv_color_t color, int opa);

void ilv_drag_event(lv_event_t * e);
void ilv_add_drag_event(lv_obj_t *obj);

void ilv_drag_parent_event(lv_event_t * e);
void ilv_add_drag_parent_event(lv_obj_t *obj);

void ilv_close_event(lv_event_t * e);
void ilv_add_short_clicked_close_event(lv_obj_t *obj);

void ilv_close_parent_event(lv_event_t * e);
void ilv_add_short_clicked_close_parent_event(lv_obj_t *obj);

lv_obj_t *ilv_get_root_obj(lv_obj_t *obj);
lv_obj_t *ilv_get_root_user_obj(lv_obj_t *obj);

#endif /* _ILV_UTILS_H_ */
