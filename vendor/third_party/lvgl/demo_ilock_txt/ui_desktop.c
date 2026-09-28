#include <stdio.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

#include "parser/ilv_config.h"
#include "parser/ilv_parser.h"

lv_obj_t *obj_desktop;
lv_obj_t *obj_camera;

ilv_parser_t *parser_desktop;

int app_is_show = 0;

static void add_short_click_event(lv_obj_t *obj, lv_event_cb_t cb, void *data)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, cb, LV_EVENT_SHORT_CLICKED, data);
}

static lv_obj_t *create_app_view(lv_obj_t *parent, const char *name)
{
    char app_name[128];
    sprintf(app_name, "app_%s", name);

    ilv_style_t *styles = ilv_parser_get_view_style_by_name(parser_desktop, app_name);
    if (!styles) {
        fprintf(stderr, "desktop: %s not defined, please define it\n", app_name);
        return NULL;
    }

    return ilv_create_view(parent, styles);
}

static void back_to_desktop_event_cb(lv_event_t *e)
{
    if (!obj_desktop)
        return;

    lv_obj_t *obj = lv_event_get_user_data(e);

    lv_obj_del(obj);
    lv_obj_clear_flag(obj_desktop, LV_OBJ_FLAG_HIDDEN);

    app_is_show = 0;
}

static void toggle_app_ui_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (app_is_show)
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);

    app_is_show = !app_is_show;
}

/******************************************************************/

static void brightness_change_event_cb(lv_event_t *e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    // system_set_brightness(lv_slider_get_value(slider));
    printf("brightness: %d\n", lv_slider_get_value(slider));
}

static void app_camera_click_event_cb(lv_event_t *e)
{
    const char *name = lv_event_get_user_data(e);
    obj_camera = create_app_view(lv_scr_act(), name);
    if (!obj_camera)
        return;

    if (obj_desktop)
        lv_obj_add_flag(obj_desktop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *app_content = lv_obj_get_child(obj_camera, 0);

    add_short_click_event(obj_camera, toggle_app_ui_event_cb, app_content);

    lv_obj_t *obj;
    obj = ilv_search_child(obj_camera, "close");
    if (obj) {
        add_short_click_event(obj, toggle_app_ui_event_cb, app_content);
        obj = lv_obj_get_child(obj, 0);
        add_short_click_event(obj, toggle_app_ui_event_cb, app_content);
    }

    obj = ilv_search_child(obj_camera, "back");
    if (obj) {
        add_short_click_event(obj, back_to_desktop_event_cb, obj_camera);
        obj = lv_obj_get_child(obj, 0);
        add_short_click_event(obj, back_to_desktop_event_cb, obj_camera);
    }

    obj = ilv_search_child(obj_camera, "brightness");
    if (obj) {
        lv_obj_add_event_cb(obj, brightness_change_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    app_is_show = 1;
}

/******************************************************************/

lv_obj_t *ui_desktop_init(void)
{
    parser_desktop = ilv_parse_file("res/ui_desktop.txt");
    if (!parser_desktop)
        return NULL;

    if (ilv_parser_get_view_cnt(parser_desktop) == 0) {
        fprintf(stderr, "no view find\n");
        return NULL;
    }

    ilv_style_t *styles = ilv_parser_get_view_style_by_name(parser_desktop, "desktop");
    if (!styles) {
        fprintf(stderr, "no desktop view find\n");
        return NULL;
    }

    ilv_parser_apply_configs(parser_desktop);

    obj_desktop = ilv_create_view(lv_scr_act(), styles);
    ilv_parser_add_to_view(obj_desktop, parser_desktop);

    lv_obj_t *obj = ilv_search_child(obj_desktop, "camera");
    if (obj) {
        obj = lv_obj_get_child(obj, 0);
        add_short_click_event(obj, app_camera_click_event_cb, (void *)"camera");
    }

    return obj_desktop;
}
