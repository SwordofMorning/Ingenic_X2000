#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

static lv_obj_t *setting_view = NULL;

static void close_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(setting_view);
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);

    printf("event = %d\n", event);
}

ilv_style_t icon_layout_attrs[] = {
    {LV_STYLE_BORDER_WIDTH, 1},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_OUTLINE_WIDTH, 0},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_RADIUS, 0},
    {LV_style_pad_all, LV_PAD_ALL(30, 30, 10, 0)},
    {LV_style_layout_dirty, 1},
    {LV_style_bg_color, 0xffffff},

    {LV_style_set_part, LV_STATE_PRESSED},
    {LV_style_bg_color, LV_COLOR_GREY},
    {0, 0},
};


#define DEF_SET_ICON(name, label_name, image_src, click_event)\
    {LV_style_view, LV_VIEW("btn", name)},\
    {LV_style_width, LV_PCT(100)},\
    {LV_style_height, LV_SIZE_CONTENT},\
    {LV_style_layout_flex,  LV_FLEX_NORMAL(LV_FLEX_FLOW_ROW)},\
    {LV_style_sets, (long)icon_layout_attrs},\
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, click_event)}, \
    {LV_style_layout_start}, \
        {LV_style_view, LV_VIEW("obj", "img")}, \
        {LV_style_bg_img_src, LV_STR(image_src)}, \
        {LV_style_width, LV_SIZE_CONTENT},\
        {LV_style_height, LV_SIZE_CONTENT},\
        {LV_style_bg_opa, 0}, \
        {LV_style_border_width, 0}, \
        {LV_style_view, LV_VIEW("label", "label")}, \
        {LV_style_label_text, LV_STR(label_name)}, \
        {LV_style_text_color, 0x000000}, \
    {LV_style_layout_end}

static ilv_style_t app_setting_layer_attrs[] = {
    {LV_style_view, LV_VIEW("list","SETTING")},
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_PCT(100)},
    {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
    {LV_style_layout_start},
        DEF_SET_ICON("wifi", " 无线局域网", "./res/icon_img/wifi.png", event_handler),
        DEF_SET_ICON("bluetoolth", " 蓝牙", "./res/icon_img/bluetooth.png", event_handler),
        DEF_SET_ICON("usb", " USB链接", "./res/icon_img/usb.png", event_handler),
        DEF_SET_ICON("battery", " 电池", "./res/icon_img/battery-full.png", event_handler),
        DEF_SET_ICON("plane", " 飞行模式", "./res/icon_img/plane.png", event_handler),
        DEF_SET_ICON("sitemap", " 以太网", "./res/icon_img/sitemap.png", event_handler),
        DEF_SET_ICON("key", " 密码管理", "./res/icon_img/key.png", event_handler),
        DEF_SET_ICON("fingerprint", " 指纹管理", "./res/icon_img/fingerprint.png", event_handler),
        DEF_SET_ICON("download", " 下载管理", "./res/icon_img/download.png", event_handler),
        DEF_SET_ICON("sdcard", " 存储卡", "./res/icon_img/sd-card.png", event_handler),
        DEF_SET_ICON("voluem", " 音频设置", "./res/icon_img/volume-down.png", event_handler),
        DEF_SET_ICON("video", " 视频设置", "./res/icon_img/film.png", event_handler),
        DEF_SET_ICON("phone", " 通话设置", "./res/icon_img/phone.png", event_handler),
        DEF_SET_ICON("envelope", " 邮件设置", "./res/icon_img/envelope.png", event_handler),
        DEF_SET_ICON("close", " 关闭", "./res/icon_img/close.png", close_event),
    {LV_style_layout_end},

    {0, 0},
};


void app_setting_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    setting_view = ilv_create_view(lv_scr_act(), app_setting_layer_attrs);

    return;
}