#include "lvgl/lvgl.h"
#include "ilv_color_picker.h"
#include "utils/ilv_utils.h"
#include <stdio.h>

ilv_style_t color_picker_attrs[] = {
    {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_PAD_LEFT, 0},
    {LV_STYLE_PAD_RIGHT, 0},
    {LV_STYLE_PAD_TOP, 0},
    {LV_STYLE_PAD_BOTTOM, 0},
    {LV_STYLE_BG_OPA, 0x33},
    {0, 0},
};

ilv_style_t color_picker_line_attrs[] = {
    {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_LAYOUT, LV_LAYOUT_FLEX_},
    {LV_STYLE_FLEX_FLOW_, LV_FLEX_FLOW_ROW},
    {LV_STYLE_FLEX_MAIN_PLACE_, LV_FLEX_ALIGN_START},
    {LV_STYLE_FLEX_CROSS_PLACE_, LV_FLEX_ALIGN_CENTER},
    {LV_STYLE_FLEX_TRACK_PLACE_, LV_FLEX_ALIGN_CENTER},
    {LV_STYLE_PAD_LEFT, 0},
    {LV_STYLE_PAD_RIGHT, 0},
    {LV_STYLE_PAD_TOP, 0},
    {LV_STYLE_PAD_BOTTOM, 0},
    {LV_STYLE_RADIUS, 0},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_BG_OPA, 0x33},
    {0, 0},
};

ilv_style_t color_picker_item_attrs[] = {
    {LV_STYLE_WIDTH, 40},
    {LV_STYLE_HEIGHT, 20},
    {LV_STYLE_PAD_LEFT, 0},
    {LV_STYLE_PAD_RIGHT, 0},
    {LV_STYLE_PAD_TOP, 0},
    {LV_STYLE_PAD_BOTTOM, 0},
    {LV_STYLE_RADIUS, 0},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_BORDER_WIDTH, 0},
    {LV_STYLE_OUTLINE_WIDTH, 0},
    {LV_STYLE_BG_OPA, 0xff},
    {0, 0},
};

ilv_style_t color_picker_item_btn_attrs[] = {
    {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_PAD_TOP, 5},
    {LV_STYLE_PAD_BOTTOM, 5},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_BORDER_WIDTH, 0},
    {LV_STYLE_OUTLINE_WIDTH, 0},
    {LV_STYLE_BG_OPA, 0x33},
    {0, 0},
};

static lv_obj_t *create_list_line(lv_obj_t *parent)
{
    lv_obj_t *line = lv_obj_create(parent);
    ilv_styles_sets(line, color_picker_line_attrs, 0);
    return line;
}

static lv_obj_t *create_list_line_text(lv_obj_t *parent, const char *text)
{
    lv_obj_t *line = create_list_line(parent);

    lv_obj_t *label = lv_label_create(line);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return line;
}

static void color_short_clicked_event(lv_event_t *e)
{
    lv_obj_t *c = lv_event_get_target(e);
    lv_obj_t *line = lv_obj_get_parent(c);
    lv_obj_t *picker = lv_obj_get_parent(line);
    ilv_color_pick_event_t cb = lv_event_get_user_data(e);
    if (cb)
        cb(picker, lv_obj_get_style_bg_color(c, 0));
}

static lv_obj_t *create_color_item(lv_obj_t *parent, ilv_color_pick_event_t cb, lv_color_t color)
{
    lv_obj_t *c = lv_obj_create(parent);
    ilv_styles_sets(c, color_picker_item_attrs, 0);
    lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(c, color_short_clicked_event, LV_EVENT_SHORT_CLICKED, cb);

    lv_obj_set_style_bg_color(c, color, 0);

    return c;
}

static lv_obj_t *create_item_btn(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_btn_create(parent);
    ilv_styles_sets(btn, color_picker_item_btn_attrs, 0);

    lv_color_t color = lv_obj_get_style_bg_color(parent, 0);
    lv_color_t text_color = lv_obj_get_style_text_color(parent, 0);
    lv_obj_set_style_text_color(btn, text_color, 0);
    lv_obj_set_style_bg_color(btn, color, LV_PART_SELECTED);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

lv_obj_t *ilv_color_picker_create(lv_obj_t *parent, lv_obj_t *obj, ilv_color_pick_event_t cb)
{
    lv_obj_t *list = lv_list_create(parent);
    ilv_styles_sets(list, color_picker_attrs, 0);
    ilv_add_user_data(list, color_picker_obj_ilv_data_name, obj, NULL);
    ilv_add_user_data(list, color_picker_cb_ilv_data_name, cb, NULL);

    lv_obj_t *line_close = create_item_btn(list, "关闭");
    lv_obj_add_flag(line_close, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(line_close, ilv_drag_parent_event, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event(line_close, ilv_close_parent_event, LV_EVENT_SHORT_CLICKED, NULL);

    const char *color_strs[] = {"红色", "粉红", "紫色", "深紫",
                                "靛蓝", "蓝色", "浅蓝", "青色",
                                "青色", "绿色", "浅绿", "青柠",
                                "黄色", "琥珀", "橙色", "深橙",
                                "棕色", "蓝灰", "灰色", };
    int i, j;
    for (i = 0; i < sizeof(color_strs)/sizeof(color_strs[0]); i++) {
        lv_obj_t *line = create_list_line_text(list, color_strs[i]);
        create_color_item(line, cb, lv_palette_main(i));
        for (j = 4; j >= 1; j--)
            create_color_item(line, cb, lv_palette_darken(i, j));
        for (j = 1; j <= 5; j++)
            create_color_item(line, cb, lv_palette_lighten(i, j));
    }

    lv_obj_update_layout(list);
    lv_obj_set_width(line_close, lv_obj_get_width(list));

    return list;
}

static void m_color_pick_event(lv_obj_t *picker, lv_color_t color)
{
    lv_obj_t *btn = ilv_get_user_data(picker, color_picker_obj_ilv_data_name);
    ilv_color_pick_event_t cb = ilv_get_user_data(btn, color_picker_cb_ilv_data_name);

    lv_obj_set_style_bg_color(btn, color, 0);

    if (cb)
        cb(btn, color);
}

static void m_color_btn_short_clicked_event(lv_event_t *e)
{
    lv_obj_t *c = lv_event_get_target(e);

    ilv_color_picker_create(lv_scr_act(), c, m_color_pick_event);
}

lv_obj_t *ilv_color_picker_btn_create(lv_obj_t *parent, lv_obj_t *obj, ilv_color_pick_event_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(btn, m_color_btn_short_clicked_event, LV_EVENT_SHORT_CLICKED, NULL);

    if (obj)
        lv_obj_set_style_bg_color(btn, lv_obj_get_style_bg_color(obj, 0), 0);
    lv_obj_set_style_bg_opa(btn, 255, 0);

    ilv_add_user_data(btn, color_picker_obj_ilv_data_name, obj, NULL);
    ilv_add_user_data(btn, color_picker_cb_ilv_data_name, cb, NULL);

    return btn;
}
