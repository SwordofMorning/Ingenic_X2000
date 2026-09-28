#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include "parser/ilv_parser.h"

lv_obj_t *ilv_flex_col_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    ilv_style_set(obj, LV_style_layout_flex, LV_FLEX_NORMAL(LV_FLEX_FLOW_COLUMN), 0);
    return obj;
}

lv_obj_t *ilv_flex_row_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    ilv_style_set(obj, LV_style_layout_flex, LV_FLEX_NORMAL(LV_FLEX_FLOW_ROW), 0);
    return obj;
}

lv_obj_t *ilv_grid_view_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    ilv_style_set(obj, LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_CENTER), 0);
    return obj;
}

/*****************************************************************************/
int ilv_img_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_style_img_src) {
        void *v = ilv_check_img_src_realpath((void *)value);
        if (v) {
            ilv_add_delete_data(obj, v, free);
            value = (long) v;
        }
        lv_img_set_src(obj, (void *)value);
        return 0;
    }
    return -1;
}

int ilv_img_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    if (!strcmp(key, "path") || !strcmp(key, "src"))
        return ilv_parser_add_style_str(parser, LV_style_img_src, key, val);
    return -1;
}

/*****************************************************************************/
int ilv_label_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_style_label_text) {
        lv_label_set_text(obj, (void *)value);
        return 0;
    }
    return -1;
}

int ilv_label_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    if (!strcmp(key, "text"))
        return ilv_parser_add_style_str(parser, LV_style_label_text, key, val);
    return -1;
}

/*****************************************************************************/
int ilv_textarea_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_style_textarea_text) {
        lv_textarea_set_text(obj, (void *)value);
        return 0;
    }
    if (style == LV_style_textarea_placeholder_text) {
        lv_textarea_set_placeholder_text(obj, (void *)value);
        return 0;
    }
    if (style == LV_style_textarea_oneline) {
        lv_textarea_set_one_line(obj, value);
        return 0;
    }
    return -1;
}

int ilv_textarea_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    if (!strcmp(key, "text"))
        return ilv_parser_add_style_str(parser, LV_style_textarea_text, key, val);
    if (!strcmp(key, "placeholder_text") || !strcmp(key, "bg_text"))
        return ilv_parser_add_style_str(parser, LV_style_textarea_placeholder_text, key, val);
    if (!strcmp(key, "oneline"))
        return ilv_parser_add_style_int(parser, LV_style_textarea_oneline, key, val);
    return -1;
}

/*****************************************************************************/
int ilv_roller_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_style_roller_options_infinite) {
        lv_roller_set_options(obj, (void *)value, LV_ROLLER_MODE_INFINITE);
        return 0;
    }
    if (style == LV_style_roller_options_normal) {
        lv_roller_set_options(obj, (void *)value, LV_ROLLER_MODE_NORMAL);
        return 0;
    }
    if (style == LV_style_roller_row_cnt) {
        lv_roller_set_visible_row_count(obj, value);
        return 0;
    }
    if (style == LV_style_roller_selected) {
        lv_roller_set_selected(obj, value, 0);
        return 0;
    }
    return -1;
}

static char *to_lv_str_list(const char *key, const char *val)
{
    int n, i;
    char **words = parse_str_array(val, ",", &n);
    if (!n) {
        free_parse_words(words);
        fprintf(stderr, "str list: 获取字符串数组失败: %s=%s\n", key, val);
        return NULL;
    }
    char *p = malloc(strlen(val)+2);
    char *s = p;
    for (i = 0; i < n; i++) {
        parse_str2(words[i], &words[i]);
        if (i+1 < n)
            s += sprintf(s, "%s\n", words[i]);
        else
            s += sprintf(s, "%s", words[i]);
    }
    assert(s-p < strlen(val)+2);

    free_parse_words(words);

    return realloc(p, s-p+1);
}

int ilv_roller_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    if (!strcmp(key, "row_cnt"))
        return ilv_parser_add_style_int(parser, LV_style_roller_row_cnt, key, val);
    if (!strcmp(key, "selected"))
        return ilv_parser_add_style_int(parser, LV_style_roller_selected, key, val);

    int type = 0;
    if (!strcmp(key, "options") || !strcmp(key, "options_normal"))
        type = LV_style_roller_options_normal;
    else if (!strcmp(key, "options_infinite"))
        type = LV_style_roller_options_infinite;
    else
        return -1;

    char *p = to_lv_str_list(key, val);
    if (!p)
        return -1;

    ilv_parser_add_style(parser, type, (long)p);
    ilv_parser_add_delete_data(parser, p, free);

    return 0;
}

/*****************************************************************************/
int ilv_dropdown_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_style_dropdown_options) {
        lv_dropdown_set_options(obj, (void *)value);
        return 0;
    }
    if (style == LV_style_dropdown_dir) {
        lv_dropdown_set_dir(obj, value);
        return 0;
    }
    if (style == LV_style_dropdown_symbol) {
        void *v = ilv_check_img_src_realpath((void *)value);
        if (v) {
            ilv_add_delete_data(obj, v, free);
            value = (long) v;
        }
        lv_dropdown_set_symbol(obj, (void *)value);
        return 0;
    }
    if (style == LV_style_dropdown_selected) {
        lv_dropdown_set_selected(obj, value);
        return 0;
    }
    if (style == LV_style_dropdown_selected_highlight) {
        lv_dropdown_set_selected_highlight(obj, value);
        return 0;
    }
    return -1;
}

int ilv_dropdown_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    if (!strcmp(key, "dir"))
        return ilv_parser_add_style_int(parser, LV_style_dropdown_dir, key, val);
    if (!strcmp(key, "symbol") || !strcmp(key, "src"))
        return ilv_parser_add_style_str(parser, LV_style_dropdown_symbol, key, val);
    if (!strcmp(key, "selected"))
        return ilv_parser_add_style_str(parser, LV_style_dropdown_selected, key, val);
    if (!strcmp(key, "highlight"))
        return ilv_parser_add_style_int(parser, LV_style_dropdown_selected_highlight, key, val);

    if (!strcmp(key, "options")) {
        char *p = to_lv_str_list(key, val);
        if (!p)
            return -1;

        ilv_parser_add_style(parser, LV_style_dropdown_options, (long)p);
        ilv_parser_add_delete_data(parser, p, free);

        return 0;
    }

    return -1;
}

/*****************************************************************************/
void ilv_style_add_normal_view_types(void)
{
    ilv_add_view_type("obj", lv_obj_create, NULL, NULL);
    ilv_add_view_type("label", lv_label_create, ilv_label_set_style, ilv_label_parse_style);
    ilv_add_view_type("btn", lv_btn_create, NULL, NULL);
    ilv_add_view_type("textarea", lv_textarea_create, ilv_textarea_set_style, ilv_textarea_parse_style);
    ilv_add_view_type("slider", lv_slider_create, NULL, NULL);
    ilv_add_view_type("list", lv_list_create, NULL, NULL);
    ilv_add_view_type("flex_row", ilv_flex_row_view_create, NULL, NULL);
    ilv_add_view_type("flex_col", ilv_flex_col_view_create, NULL, NULL);
    ilv_add_view_type("grid", ilv_grid_view_create, NULL, NULL);
    ilv_add_view_type("meter", lv_meter_create, ilv_meter_set_style, NULL);
    ilv_add_view_type("switch", lv_switch_create, NULL, NULL);
    ilv_add_view_type("img", lv_img_create, ilv_img_set_style, ilv_img_parse_style);
    ilv_add_view_type("arc", lv_arc_create, NULL, NULL);
    ilv_add_view_type("roller", lv_roller_create, ilv_roller_set_style, ilv_roller_parse_style);
    ilv_add_view_type("dropdown", lv_dropdown_create, ilv_dropdown_set_style, ilv_dropdown_parse_style);


    ilv_add_view_type("rotate_img", ilv_rotate_img_create, ilv_rotate_img_set_style, ilv_rotate_img_parse_style);

    ilv_add_view_type("text_doing", ilv_text_doing_view_create, ilv_text_doing_set_style, ilv_text_doing_parse_style);
    ilv_add_view_type("analog_clock", ilv_analog_clock_view_create, NULL, NULL);
    ilv_add_view_type("rotate_hour", ilv_rotate_hour_view_create, ilv_rotate_hour_set_style, ilv_rotate_hour_parse_style);
    ilv_add_view_type("rotate_minute", ilv_rotate_minute_view_create, ilv_rotate_minute_set_style, ilv_rotate_minute_parse_style);
    ilv_add_view_type("rotate_second", ilv_rotate_second_view_create, ilv_rotate_second_set_style, ilv_rotate_second_parse_style);

    ilv_add_view_type("digital_second", ilv_digital_second_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_minute", ilv_digital_minute_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_hour", ilv_digital_hour_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_day", ilv_digital_day_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_month", ilv_digital_month_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_year", ilv_digital_year_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_weekday", ilv_digital_weekday_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_year_full", ilv_digital_year_full_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_msec", ilv_digital_msec_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);
    ilv_add_view_type("digital_cmsec", ilv_digital_cmsec_view_create, ilv_digital_time_set_style, ilv_digital_time_parse_style);

}
