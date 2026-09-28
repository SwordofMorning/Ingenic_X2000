#include <stdio.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

ilv_style_t desktop_app_layout_attrs[] = {
    {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_style_sets, (long)clear_extra_size_attrs},
    {LV_STYLE_GRID_CELL_X_ALIGN_, LV_GRID_ALIGN_CENTER},
    {LV_STYLE_GRID_CELL_Y_ALIGN_, LV_GRID_ALIGN_CENTER},
    {LV_STYLE_GRID_CELL_COLUMN_SPAN_, 1},
    {LV_STYLE_GRID_CELL_ROW_SPAN_, 1},
    {LV_style_layout_dirty, 1},
    {LV_STYLE_BG_OPA, 0},
    {0, 0},
};

#define DEF_APP(row, colum, name, label_name, image_color, click_event) \
    {LV_style_view, LV_VIEW("flex_col", name)}, \
    {LV_style_sets, LV_PTR(desktop_app_layout_attrs)}, \
    {LV_STYLE_GRID_CELL_COLUMN_POS_, colum}, \
    {LV_STYLE_GRID_CELL_ROW_POS_, row}, \
    {LV_style_layout_start}, \
    {LV_style_view, LV_VIEW("obj", "image")}, \
    {LV_STYLE_BG_COLOR, image_color}, \
    {LV_STYLE_BORDER_WIDTH, 0}, \
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, click_event)}, \
    {LV_style_view, LV_VIEW("label", "label")}, \
    {LV_style_label_text, LV_STR(label_name)}, \
    {LV_style_layout_end}

#define DEF_APP_IMG(row, colum, name, label_name, image_src, click_event) \
    {LV_style_view, LV_VIEW("flex_col", name)}, \
    {LV_style_sets, LV_PTR(desktop_app_layout_attrs)}, \
    {LV_STYLE_GRID_CELL_COLUMN_POS_, colum}, \
    {LV_STYLE_GRID_CELL_ROW_POS_, row}, \
    {LV_style_layout_start}, \
    {LV_style_view, LV_VIEW("obj", "image")}, \
    {LV_STYLE_BG_IMG_SRC, LV_STR(image_src)}, \
    {LV_STYLE_BG_OPA, 0}, \
    {LV_STYLE_BORDER_WIDTH, 0}, \
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, click_event)}, \
    {LV_style_view, LV_VIEW("label", "label")}, \
    {LV_style_label_text, LV_STR(label_name)}, \
    {LV_STYLE_TEXT_COLOR, 0xffffff},\
    {LV_style_layout_end}

ilv_style_t desktop_views[] = {
    // 桌面背景
    {LV_style_view, LV_VIEW("obj", "desktop")},
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_PCT(100)},
    {LV_STYLE_BG_COLOR, 0x000000},
    // {LV_STYLE_BG_IMAGE_SRC, LV_STR("./res/desktop_bg.png")},
    {LV_STYLE_BORDER_WIDTH, 0},
    {LV_STYLE_BG_OPA, 0},
    {LV_style_layout_start},

        // 左右滑动页面
        {LV_style_view, LV_VIEW("flex_row", "flex")},
        {LV_STYLE_WIDTH, LV_PCT(100)},
        {LV_STYLE_HEIGHT, LV_PCT(100)},
        {LV_style_scroll_snap_x, LV_SCROLL_SNAP_START},
        {LV_style_sets, (long)clear_extra_size_attrs},
        {LV_STYLE_BG_OPA, 0},
        {LV_style_layout_start},

            // 第一页
            {LV_style_view, LV_VIEW("grid", "page0")},
            {LV_STYLE_WIDTH, LV_PCT(100)},
            {LV_STYLE_HEIGHT, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_STYLE_GRID_COLUMN_DSC_ARRAY_, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_STYLE_GRID_ROW_DSC_ARRAY_, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_sets, (long)clear_extra_size_attrs},
            {LV_STYLE_BG_OPA, 0},
            {LV_style_layout_start},
                // 第一行
                DEF_APP_IMG(1, 0, "camera", "摄像头", "./res/app_img/camera.png", app_camera_click_event),
                DEF_APP_IMG(1, 1, "photo", "相册", "./res/app_img/image.png", NULL),
                DEF_APP_IMG(1, 2, "monitor", "监控", "./res/app_img/monitor.png", NULL),
                DEF_APP_IMG(1, 3, "file", "文件管理", "./res/app_img/file_manager.png", NULL),
                // 第二行
                DEF_APP_IMG(2, 0, "player", "播放器", "./res/app_img/video.png", NULL),
                DEF_APP_IMG(2, 1, "setting", "设置", "./res/app_img/setting.png", app_setting_click_event),
                DEF_APP_IMG(2, 2, "wifi", "wifi", "./res/app_img/wifi.png", NULL),
                DEF_APP_IMG(2, 3, "brightness", "亮度", "./res/app_img/brightness.png", app_brightness_click_event),
                // 第三行
                // 第四行
                // 不放置app,利用预留的高度来抬高前面的图片的位置

            {LV_style_layout_end},

            // 第二页
            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_STYLE_WIDTH, LV_PCT(100)},
            {LV_STYLE_HEIGHT, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_START)},
            {LV_STYLE_GRID_COLUMN_DSC_ARRAY_, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_STYLE_GRID_ROW_DSC_ARRAY_, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_STYLE_BORDER_WIDTH, 0},
            // {LV_STYLE_BG_COLOR, LV_COLOR_BROWN_darken3},
            {LV_STYLE_BG_OPA, 0},
            {LV_style_layout_start},

                DEF_APP_IMG(0, 0, "password", "密码", "./res/app_img/password.png", NULL),
                DEF_APP_IMG(0, 1, "faceapp", "人脸", "./res/app_img/face.png", NULL),
                DEF_APP_IMG(0, 2, "fingerprint", "指纹", "./res/app_img/finger.png", NULL),
                DEF_APP_IMG(0, 3, "date", "时间", "./res/app_img/time.png", NULL),

            {LV_style_layout_end},

        {LV_style_layout_end},

    {LV_style_layout_end},

    {0, 0},
};

static lv_obj_t *desktop_obj;

lv_obj_t *ui_get_desktop(void)
{
    return desktop_obj;
}

lv_obj_t *ui_desktop_init(void)
{
    desktop_obj = ilv_create_view(lv_scr_act(), desktop_views);
    return desktop_obj;
}
