#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif

#define LV_SYMBOL_PLANE           "\xEF\x81\xB2"
#define LV_SYMBOL_KEY             "\xEF\x82\x84"
#define LV_SYMBOL_NETWORK          "\xEF\x83\xA8" /*61671, 0xF0E7*/

static lv_obj_t *setting_view;

struct demo_setting_icon {
    char *text;
    lv_event_cb_t event_cb;
};


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

struct demo_setting_icon set_icons[] = {
    {LV_SYMBOL_WIFI"  无线局域网", event_handler},
    {LV_SYMBOL_BLUETOOTH"  蓝牙", event_handler},
    {LV_SYMBOL_GPS"  GPS定位", event_handler},
    {LV_SYMBOL_USB"  USB", event_handler},
    {LV_SYMBOL_BATTERY_FULL"  电池", event_handler},
    {LV_SYMBOL_PLANE"  飞行模式", event_handler},
    {LV_SYMBOL_NETWORK"  以太网", event_handler},
    {NULL, NULL},
    {LV_SYMBOL_KEY "  密码管理", event_handler},
    {LV_SYMBOL_SD_CARD"  SD CARD", event_handler},
    {LV_SYMBOL_DOWNLOAD"  下载管理", event_handler},
    {NULL, NULL},
    {LV_SYMBOL_VOLUME_MID "  音频设置", event_handler},
    {LV_SYMBOL_VIDEO "  视频设置", event_handler},
    {LV_SYMBOL_CALL "  通话设置", event_handler},
    {LV_SYMBOL_ENVELOPE "  邮件设置", event_handler},
    {NULL, NULL},
    {LV_SYMBOL_CLOSE"  关闭", close_event},
};


void app_setting_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    setting_view = lv_list_create(lv_scr_act());
    lv_obj_set_size(setting_view, lv_pct(100), lv_pct(100));

    lv_obj_t * btn;
    lv_obj_t * text;

    int i = 0;

    struct demo_setting_icon *icon;

    for (i = 0; i < ARRAY_SIZE(set_icons); i++) {
        icon = &set_icons[i];
        if (!icon->event_cb) {
            text = lv_list_add_text(setting_view, "");
            lv_obj_set_size(text, lv_pct(100), lv_pct(5));
            continue;
        }

        btn = lv_list_add_btn(setting_view, NULL, NULL);

        lv_obj_add_event_cb(btn, icon->event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_pad_top(btn, 30, 0);
        lv_obj_set_style_pad_bottom(btn, 30, 0);

        text = lv_label_create(btn);
        lv_label_set_text(text, icon->text);
    }
}