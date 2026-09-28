#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

lv_obj_t *brightness_obj;

void system_set_brightness(int brightness)
{
    char cmd_brightness[120];
    printf("ui set brightness %d\n", brightness);

    sprintf(cmd_brightness, "fb_brightness_ctl.sh %d", brightness);
    system(cmd_brightness);
}

static void brightness_change_event(lv_event_t *e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    system_set_brightness(lv_slider_get_value(slider));
}

static ilv_style_t common_layer_attrs[] = {
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {0, 0},
};

static void back_to_ui_desktop(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(brightness_obj);
}


ilv_style_t brightness_views[] = {

    {LV_style_view, LV_VIEW("obj", "camera_root")},
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_PCT(100)},
    {LV_STYLE_BG_OPA, 0x0},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_NONE},
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_ui_desktop)},
    {LV_style_layout_start},


        {LV_style_view, LV_VIEW("flex_col", "brightness")},
        {LV_style_sets, (long)common_layer_attrs},
        {LV_STYLE_ALIGN, LV_ALIGN_BOTTOM_LEFT},
        {LV_style_layout_start},
            {LV_style_view, LV_VIEW("flex_row", "flex")},
            {LV_style_sets, (long)common_layer_attrs},
            {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_NONE},
            {LV_style_pad_all, LV_PAD_ALL(30, 0, 0, 0)},
            {LV_style_layout_start},
                {LV_style_view, LV_VIEW("label", "label")},
                {LV_style_label_text, LV_STR("屏幕亮度")},
            {LV_style_layout_end},

            {LV_style_view, LV_VIEW("slider", "slider")},
            {LV_STYLE_WIDTH, LV_PCT(80)},
            {LV_STYLE_HEIGHT, 50},
            {LV_STYLE_BG_OPA, 0xbb},
            {LV_style_add_event, LV_EVENT(LV_EVENT_VALUE_CHANGED, brightness_change_event)},
        {LV_style_layout_end},
};


void app_brightness_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    brightness_obj = ilv_create_view(lv_scr_act(), brightness_views);
}