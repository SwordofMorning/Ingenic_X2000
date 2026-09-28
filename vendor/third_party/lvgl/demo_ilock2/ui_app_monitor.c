#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include "../ingenic/view/ilv_h264_view.h"

#include "lvgl/lvgl.h"


#define view_width 640
#define view_height 360

static int app_is_show = 0;

static lv_obj_t *app_monitor_view;
static lv_obj_t *video_ch[4];
static lv_obj_t *video_event_view;

char file_name[][64] = {
    [0] = {"/usr/data/1_360p.h264"},
    [1] = {"/usr/data/2_360p.h264"},
    [2] = {"/usr/data/3_360p.h264"},
    [3] = {"/usr/data/4_360p.h264"},
};


static void app_video_view_click(lv_event_t *e);
static void toggle_video_ui_event(lv_event_t *e);
static void back_to_destop(lv_event_t *e);
static void back_to_monitor_event(lv_event_t *e);


static void back_to_destop(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(app_monitor_view);
    app_monitor_view = NULL;
}


static void back_to_monitor_event(lv_event_t *e)
{
    lv_obj_t *video = lv_obj_get_parent(video_event_view);

    lv_obj_del(video_event_view);
    video_event_view = NULL;

    lv_obj_set_size(video, 640, 360);
    lv_obj_add_event_cb(video, app_video_view_click, LV_EVENT_CLICKED, NULL);

    int i = 0;
    for (i = 0; i < 4; i++) {
        if (!video_ch[i])
            continue;

        ilv_h264_clear_hidden_flag(video_ch[i]);
        lv_obj_clear_flag(lv_obj_get_child(app_monitor_view, 0), LV_OBJ_FLAG_HIDDEN);
    }

    app_is_show = 0;
}

static void toggle_video_ui_event(lv_event_t *e)
{
    lv_obj_t *root = lv_obj_get_child(video_event_view, 0);

    if (app_is_show)
        lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);

    app_is_show = !app_is_show;
}

static ilv_style_t common_layer_attrs[] = {
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_BG_OPA, 0},
    {0, 0},
};


static ilv_style_t app_video_layer_attrs[] = {
    {LV_style_view,  LV_VIEW("obj", "video_event_view")},
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_PCT(100)},
    {LV_STYLE_BG_OPA, 0x0},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_NONE},
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, toggle_video_ui_event)},
    {LV_style_layout_start},
        {LV_style_view, LV_VIEW("flex_col", "video_layer")},
        {LV_style_sets, (long)common_layer_attrs},
        {LV_STYLE_MIN_HEIGHT, LV_PCT(10)},
        {LV_STYLE_ALIGN, LV_ALIGN_BOTTOM_LEFT},
        {LV_STYLE_BG_OPA, 0xbb},
        {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_monitor_event)},
        {LV_style_layout_start},
            {LV_style_view, LV_VIEW("flex_row", "back")},
            {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
            {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
            {LV_STYLE_ALIGN, LV_ALIGN_CENTER},
            {LV_STYLE_BG_OPA, 0},
            {LV_STYLE_BORDER_SIDE, 0},
            {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_monitor_event)},
            {LV_style_layout_start},

                {LV_style_view, LV_VIEW("label","label")},
                {LV_style_label_text, LV_STR("返回桌面")},
                {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_monitor_event)},

            {LV_style_layout_end},
        {LV_style_layout_end},
    {LV_style_layout_end},
    {0, 0},
};

void app_video_view_click(lv_event_t *e)
{
    lv_obj_t *video = lv_event_get_target(e);
    lv_obj_set_size(video, 1280, 720);

    int i = 0;
    for (i = 0; i < 4; i++) {
        if ((void *)video != video_ch[i] && video_ch[i])
            ilv_h264_add_hidden_flag(video_ch[i]);
    }
    lv_obj_add_flag(lv_obj_get_child(app_monitor_view, 0), LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_event_cb(video, app_video_view_click);

    video_event_view = ilv_create_view(video, app_video_layer_attrs);



    /*这里搞不懂为啥滑动的复位 是设置 lv_scr_act 才有效果，app_monitor_view 才是设置了自动布局+可滑动呀*/
    // lv_obj_scroll_to_y(app_monitor_view, 0, 0);
    lv_obj_scroll_to_y(lv_scr_act(), 0, 0);

}



static ilv_style_t icon_layout_attrs[] = {
    {LV_STYLE_BORDER_WIDTH, 1},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_OUTLINE_WIDTH, 0},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_RADIUS, 0},
    {LV_style_layout_dirty, 1},
    {0, 0},
};


ilv_style_t test_monitor_attrs[] = {
    {LV_style_view, LV_VIEW("obj", "monitor")},
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_SIZE_CONTENT},
    {LV_style_layout_flex, LV_FLEX_NORMAL(LV_FLEX_FLOW_ROW_WRAP)},
    {LV_style_flex_main_place, LV_FLEX_ALIGN_START},
    {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
    {LV_style_pad_row, 0},
    {LV_style_pad_column, 0},
    {LV_style_bg_color, 0xffffff},
    {LV_style_bg_opa, 0},
    {LV_style_border_width, 0},
    {LV_style_layout_dirty, 1},
    {LV_style_layout_start},
        {LV_style_view, LV_VIEW("btn", "BTN")},
        {LV_STYLE_WIDTH, LV_PCT(100)},
        {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
        {LV_style_layout_flex, LV_FLEX_NORMAL(LV_FLEX_FLOW_ROW)},
        {LV_style_sets, (long)icon_layout_attrs},
        {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_destop)},
        {LV_style_layout_start},
            {LV_style_view, LV_VIEW("obj", "image")},
            {LV_STYLE_BG_IMG_SRC, LV_STR("./res/icon_img/close.png")},
            {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
            {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
            {LV_STYLE_BG_OPA, 0},
            {LV_style_border_width, 0},
        {LV_style_layout_end},
    {LV_style_layout_end},
    {0,0},
};


void killall_signal_handler(int signum) {
    if (signum == SIGINT) {
        if (app_monitor_view)
            lv_obj_del(app_monitor_view);
        exit(0);
    }
}


static ilv_style_t video_view_attrs[] = {
    {LV_style_width, view_width},
    {LV_style_height, view_height},
    {LV_style_border_color, LV_COLOR_WHITE},
    {LV_style_border_width, 2},
    {LV_style_border_opa, 0xff},
    {LV_style_pad_all, LV_PAD_ALL(0,0,0,0)},
    {LV_style_outline_width, 0},
};


void app_monitor_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);


    if (signal(SIGINT, killall_signal_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to set up signal handler");
        return;
    }

    app_monitor_view = ilv_create_view(lv_scr_act(), test_monitor_attrs);

    video_ch[0] = ilv_h264_create(app_monitor_view);
    ilv_h264_set_src(video_ch[0], file_name[0]);
    ilv_styles_sets(video_ch[0], video_view_attrs, 0);
    lv_obj_add_event_cb(video_ch[0], app_video_view_click, LV_EVENT_SHORT_CLICKED, NULL);

    video_ch[1] = ilv_h264_create(app_monitor_view);
    ilv_h264_set_src(video_ch[1], file_name[1]);
    ilv_styles_sets(video_ch[1], video_view_attrs, 0);
    lv_obj_add_event_cb(video_ch[1], app_video_view_click, LV_EVENT_SHORT_CLICKED, NULL);

    video_ch[2] = ilv_h264_create(app_monitor_view);
    ilv_h264_set_src(video_ch[2], file_name[2]);
    ilv_styles_sets(video_ch[2], video_view_attrs, 0);
    lv_obj_add_event_cb(video_ch[2], app_video_view_click, LV_EVENT_SHORT_CLICKED, NULL);


    video_ch[3] = ilv_h264_create(app_monitor_view);
    ilv_h264_set_src(video_ch[3], file_name[3]);
    ilv_styles_sets(video_ch[3], video_view_attrs, 0);
    lv_obj_add_event_cb(video_ch[3], app_video_view_click, LV_EVENT_SHORT_CLICKED, NULL);
}

