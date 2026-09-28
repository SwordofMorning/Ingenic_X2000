#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

void system_open_camera(void)
{
    // 这里仅仅是测试ui时的例子
    // 默认情况下 外部的摄像头应用 会一直使用 /dev/fb0 显示, 这里的 /dev/fb1的ui会叠加上去
    // system("cmd_fb enable /dev/fb0");
    // system("cmd_fb clear /dev/fb0 color=0x00000000");
    // system("cmd_fb draw_rect /dev/fb0 color=0xff00ff00 y=880 width=500 height=500");
    // system("cmd_fb display /dev/fb0");
}

void system_close_camera(void)
{
    // system("cmd_fb clear /dev/fb0 color=0x00000000");
}

void system_set_brightness(int brightness)
{
    printf("backligth %d%%\n", brightness);
}

static void brightness_change_event(lv_event_t *e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    system_set_brightness(lv_slider_get_value(slider));
}

static int app_is_show = 0;
static lv_obj_t *camera_root;

static void back_to_desktop_event(lv_event_t *e)
{
    lv_obj_del(camera_root);

    lv_obj_t *desktop = ui_get_desktop();
    if (!desktop)
        desktop = ui_desktop_init();
    lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    app_is_show = 0;
}

static void toggle_camera_ui_event(lv_event_t *e)
{
    lv_obj_t *camera_layer = lv_obj_get_child(camera_root, 0);

    if (app_is_show)
        lv_obj_add_flag(camera_layer, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(camera_layer, LV_OBJ_FLAG_HIDDEN);

    app_is_show = !app_is_show;
}

static ilv_style_t common_layer_attrs[] = {
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_SIZE_CONTENT},
    {LV_style_pad_all, LV_PAD_ALL(10, 10, 100, 0)},
    {LV_style_border_side, LV_BORDER_SIDE_BOTTOM},
    {LV_style_shadow_width, 0},
    {LV_style_bg_opa, 0},

    {0, 0},
};

ilv_style_t app_camera_layer_attrs[] = {
    {LV_style_view, LV_VIEW("obj", "camera_root")},
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_PCT(100)},
    {LV_style_bg_opa, 0x0},
    {LV_style_border_side, LV_BORDER_SIDE_NONE},
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, toggle_camera_ui_event)},
    {LV_style_layout_start},

        {LV_style_view, LV_VIEW("flex_col", "camera_layer")},
        {LV_style_sets, (long)common_layer_attrs},
        {LV_style_min_height, LV_PCT(30)},
        {LV_style_align, LV_ALIGN_BOTTOM_LEFT},
        {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
        {LV_style_bg_opa, 0xbb},
        {LV_style_layout_start},

            {LV_style_view, LV_VIEW("flex_col","brightness")},
            {LV_style_sets, (long)common_layer_attrs},
            {LV_style_layout_start},
                {LV_style_view, LV_VIEW("flex_row", "flex")},
                {LV_style_sets, (long)common_layer_attrs},
                {LV_style_border_side, LV_BORDER_SIDE_NONE},
                {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
                {LV_style_layout_start},
                    {LV_style_view, LV_VIEW("label", "label")},
                    {LV_style_label_text, LV_STR("屏幕亮度")},
                {LV_style_layout_end},

                {LV_style_view, LV_VIEW("slider", "slider")},
                {LV_style_width, LV_PCT(80)},
                {LV_style_height, 40},
                {LV_style_bg_opa, 0xbb},
                {LV_style_add_event, LV_EVENT(LV_EVENT_VALUE_CHANGED, brightness_change_event)},
            {LV_style_layout_end},

            {LV_style_view, LV_VIEW("flex_row", "back")},
            {LV_style_sets, (long)common_layer_attrs},
            {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},
            {LV_style_layout_start},
                {LV_style_view, LV_VIEW("label", "label")},
                {LV_style_label_text, LV_STR("返回桌面")},
                {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},
            {LV_style_layout_end},

            {LV_style_view, LV_VIEW("flex_row", "close")},
            {LV_style_sets, (long)common_layer_attrs},
            {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, toggle_camera_ui_event)},
            {LV_style_layout_start},
                {LV_style_view, LV_VIEW("label", "label")},
                {LV_style_label_text, LV_STR("关闭")},
                {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, toggle_camera_ui_event)},
            {LV_style_layout_end},

        {LV_style_layout_end},
    {LV_style_layout_end},

    {0, 0},
};

void app_camera_click_event(lv_event_t *e)
{
    system_open_camera();

    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    camera_root = ilv_create_view(lv_scr_act(), app_camera_layer_attrs);

    app_is_show = 1;
}

void app_camera_start(void)
{
    app_camera_click_event(NULL);
    toggle_camera_ui_event(NULL);
}
