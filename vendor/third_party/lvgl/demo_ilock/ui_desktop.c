#include <stdio.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

ilv_style_t desktop_app_layout_attrs[] = {
    {LV_style_width, LV_SIZE_CONTENT},
    {LV_style_height, LV_SIZE_CONTENT},
    {LV_style_sets, (long)clear_extra_size_attrs},
    {LV_style_grid_cell_x_align, LV_GRID_ALIGN_CENTER},
    {LV_style_grid_cell_y_align, LV_GRID_ALIGN_CENTER},
    {LV_style_grid_cell_column_span, 1},
    {LV_style_grid_cell_row_span, 1},
    {LV_style_layout_dirty, 1},
    {LV_style_bg_opa, 0},
    {0, 0},
};

#define DEF_APP(row, colum, name, label_name, image_color, click_event) \
    {LV_style_view, LV_VIEW("flex_col", name)}, \
    {LV_style_sets, LV_PTR(desktop_app_layout_attrs)}, \
    {LV_style_grid_cell_column_pos, colum}, \
    {LV_style_grid_cell_row_pos, row}, \
    {LV_style_layout_start}, \
    {LV_style_view, LV_VIEW("obj", "image")}, \
    {LV_style_bg_color, image_color}, \
    {LV_style_bg_opa, 0xd0}, \
    {LV_style_border_width, 0}, \
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, click_event)}, \
    {LV_style_view, LV_VIEW("label", "label")}, \
    {LV_style_label_text, LV_PTR(label_name)}, \
    {LV_style_layout_end}

ilv_style_t desktop_views[] = {
    // 桌面背景
    {LV_style_view, LV_VIEW("obj", "desktop")},
    {LV_style_width, LV_PCT(100)},
    {LV_style_height, LV_PCT(100)},
    {LV_style_bg_img_src, LV_STR("./res/desktop_bg.png")},
    {LV_style_bg_img_opa, 0xbb},
    {LV_style_border_width, 0},
    // {LV_style_bg_opa, 0xaa},
    {LV_style_layout_start},

        // 左右滑动页面
        {LV_style_view, LV_VIEW("flex_row", "flex")},
        {LV_style_width, LV_PCT(100)},
        {LV_style_height, LV_PCT(100)},
        {LV_style_scroll_snap_x, LV_SCROLL_SNAP_START},
        {LV_style_sets, (long)clear_extra_size_attrs},
        {LV_style_bg_opa, 0},
        {LV_style_layout_start},

            // 第一页
            {LV_style_view, LV_VIEW("grid", "page0")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_sets, (long)clear_extra_size_attrs},
            {LV_style_bg_opa, 0},
            {LV_style_layout_start},
 
                // 第一行
                DEF_APP(0, 0, "camera", "摄像头", LV_COLOR_RED_lighten1, app_camera_click_event),
                DEF_APP(0, 1, "photo", "相册", LV_COLOR_DEEP_PURPLE_lighten1, NULL),
                DEF_APP(0, 2, "monitor", "监控", LV_COLOR_LIGHT_BLUE_lighten1, NULL),
                DEF_APP(0, 3, "file", "文件管理", LV_COLOR_GREEN_lighten1, NULL),
                // 第二行
                DEF_APP(1, 0, "poweroff", "关机", LV_COLOR_YELLOW_lighten1, NULL),
                DEF_APP(1, 1, "setting", "设置", LV_COLOR_DEEP_ORANGE_lighten1, NULL),
                DEF_APP(1, 2, "wifi", "wifi", LV_COLOR_GREY_lighten1, NULL),
                DEF_APP(1, 3, "brightness", "亮度", LV_COLOR_PURPLE_lighten1, NULL),
                // 第三行
                DEF_APP(2, 0, "password", "密码", LV_COLOR_BLUE_lighten1, NULL),
                DEF_APP(2, 1, "faceapp", "人脸", LV_COLOR_TEAL_lighten1, NULL),
                DEF_APP(2, 2, "fingerprint", "指纹", LV_COLOR_LIME_lighten1, NULL),
                DEF_APP(2, 3, "date", "时间", LV_COLOR_ORANGE_lighten1, app_analog_clock_click_event),
                // 第四行
                // 不放置app,利用预留的高度来抬高前面的图片的位置

            {LV_style_layout_end},

            // 第二页
            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_BROWN_darken3},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_BROWN_darken3},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_RED},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_PINK},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_PURPLE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_DEEP_PURPLE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_INDIGO},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_BLUE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_LIGHT_BLUE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_CYAN},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_TEAL},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_GREEN},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_LIGHT_GREEN},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_LIME},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_YELLOW},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_AMBER},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_ORANGE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_DEEP_ORANGE},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_BROWN},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_BLUE_GREY},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_GREY},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_RED},
            {LV_style_bg_opa, 250},

            {LV_style_view, LV_VIEW("obj", "page1")},
            {LV_style_width, LV_PCT(100)},
            {LV_style_height, LV_PCT(100)},
            {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
            {LV_style_grid_column_dsc_array, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
            {LV_style_grid_row_dsc_array, LV_GRID_DSC(170, 170, 170, 80, LV_GRID_TEMPLATE_LAST)},
            {LV_style_border_width, 0},
            {LV_style_bg_color, LV_COLOR_PINK},
            {LV_style_bg_opa, 250},

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
