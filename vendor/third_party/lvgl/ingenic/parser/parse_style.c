#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "parse_str.h"
#include "parser/ilv_parser.h"

char **ilv_parser_get_line_words(ilv_parser_t *list, int *depth, char **line_ptr);

struct enum_pair a_style[] = {
    {"align", LV_style_align},
    {"arc_w", LV_style_arc_width},
    {"arc_width", LV_style_arc_width},
    {"arc_rounded", LV_style_arc_rounded},
    {"arc_color", LV_style_arc_color},
    {"arc_opa", LV_style_arc_opa},
    {"arc_img_src", LV_style_arc_img_src},
    {"anim", LV_style_anim},
    {"anim_time", LV_style_anim_time},
    {"anim_speed", LV_style_anim_speed},
    {"add_state", LV_style_add_state},
    {"add_flag", LV_style_add_flag},
    {NULL, 0},
};

struct enum_pair b_style[] = {
    {"bg_color", LV_style_bg_color},
    {"bg_opa", LV_style_bg_opa},
    {"bg_grad_color", LV_style_bg_grad_color},
    {"bg_grad_dir", LV_style_bg_grad_dir},
    {"bg_main_stop", LV_style_bg_main_stop},
    {"bg_grad_stop", LV_style_bg_grad_stop},
    {"bg_grad", LV_style_bg_grad},
    {"bg_dither_mode", LV_style_bg_dither_mode},
    {"bg_img_src", LV_style_bg_img_src},
    {"bg_img_opa", LV_style_bg_img_opa},
    {"bg_img_recolor", LV_style_bg_img_recolor},
    {"bg_img_recolor_opa", LV_style_bg_img_recolor_opa},
    {"bg_img_tiled", LV_style_bg_img_tiled},
    {"border_color", LV_style_border_color},
    {"border_opa", LV_style_border_opa},
    {"border_w", LV_style_border_width},
    {"border_width", LV_style_border_width},
    {"border_side", LV_style_border_side},
    {"border_post", LV_style_border_post},
    {"blend_mode", LV_style_blend_mode},
    {"base_dir", LV_style_base_dir},
    {NULL, 0},
};

struct enum_pair c_style[] = {
    {"color", LV_style_text_color},
    {"color_filter_dsc", LV_style_color_filter_dsc},
    {"color_filter_opa", LV_style_color_filter_opa},
    {"clip_corner", LV_style_clip_corner},
    {"column_dsc_array", LV_style_grid_column_dsc_array},
    {"column_dsc", LV_style_grid_column_dsc_array},
    {"column_align", LV_style_grid_column_align},
    {"col_dsc_array", LV_style_grid_column_dsc_array},
    {"col_dsc", LV_style_grid_column_dsc_array},
    {"col_align", LV_style_grid_column_align},
    {"cell", LV_style_grid_cell},
    {"cell_row_span", LV_style_grid_cell_row_span},
    {"cell_row_pos", LV_style_grid_cell_row_pos},
    {"cell_col_span", LV_style_grid_cell_column_span},
    {"cell_col_pos", LV_style_grid_cell_column_pos},
    {"cell_column_span", LV_style_grid_cell_column_span},
    {"cell_column_pos", LV_style_grid_cell_column_pos},
    {"cell_x_align", LV_style_grid_cell_x_align},
    {"cell_y_align", LV_style_grid_cell_y_align},
    {"clear_state", LV_style_clear_state},
    {"clear_flag", LV_style_clear_flag},

    {NULL, 0},
};

struct enum_pair f_style[] = {
    {"font", LV_style_font_name},
    {"flex_flow", LV_style_flex_flow},
    {"flex_main_place", LV_style_flex_main_place},
    {"flex_cross_place", LV_style_flex_cross_place},
    {"flex_track_place", LV_style_flex_track_place},
    {"flex_grow", LV_style_flex_grow},
    {NULL, 0},
};

struct enum_pair g_style[] = {
    {"grid_column_dsc_array", LV_style_grid_column_dsc_array},
    {"grid_column_dsc", LV_style_grid_column_dsc_array},
    {"grid_column_align", LV_style_grid_column_align},
    {"grid_col_dsc_array", LV_style_grid_column_dsc_array},
    {"grid_col_dsc", LV_style_grid_column_dsc_array},
    {"grid_col_align", LV_style_grid_column_align},
    {"grid_row_dsc", LV_style_grid_row_dsc_array},
    {"grid_row_dsc_array", LV_style_grid_row_dsc_array},
    {"grid_row_align", LV_style_grid_row_align},
    {"grid_cell", LV_style_grid_cell},
    {"grid_cell_row_span", LV_style_grid_cell_row_span},
    {"grid_cell_row_pos", LV_style_grid_cell_row_pos},
    {"grid_cell_column_span", LV_style_grid_cell_column_span},
    {"grid_cell_column_pos", LV_style_grid_cell_column_pos},
    {"grid_cell_x_align", LV_style_grid_cell_x_align},
    {"grid_cell_y_align", LV_style_grid_cell_y_align},
    {NULL, 0},
};

struct enum_pair i_style[] = {
    {"id", LV_style_view_name},
    {"img_opa", LV_style_img_opa},
    {"img_recolor", LV_style_img_recolor},
    {"img_recolor_opa", LV_style_img_recolor_opa},
    {NULL, 0},
};

struct enum_pair l_style[] = {
    {"layout", LV_style_layout},
    {"line_w", LV_style_line_width},
    {"line_dash_w", LV_style_line_dash_width},
    {"line_width", LV_style_line_width},
    {"line_dash_width", LV_style_line_dash_width},
    {"line_dash_gap", LV_style_line_dash_gap},
    {"line_rounded", LV_style_line_rounded},
    {"line_color", LV_style_line_color},
    {"line_opa", LV_style_line_opa},
    {NULL, 0},
};

struct enum_pair m_style[] = {
    {"min_w", LV_style_min_width},
    {"max_w", LV_style_max_width},
    {"min_h", LV_style_min_height},
    {"max_h", LV_style_max_height},
    {"min_width", LV_style_min_width},
    {"max_width", LV_style_max_width},
    {"min_height", LV_style_min_height},
    {"max_height", LV_style_max_height},
    {NULL, 0},
};

struct enum_pair o_style[] = {
    {"opa", LV_style_opa},
    {"outline_w", LV_style_outline_width},
    {"outline_width", LV_style_outline_width},
    {"outline_color", LV_style_outline_color},
    {"outline_opa", LV_style_outline_opa},
    {"outline_pad", LV_style_outline_pad},
    {NULL, 0},
};

struct enum_pair p_style[] = {
    {"pad_top", LV_style_pad_top},
    {"pad_bottom", LV_style_pad_bottom},
    {"pad_left", LV_style_pad_left},
    {"pad_right", LV_style_pad_right},
    {"pad_row", LV_style_pad_row},
    {"pad_column", LV_style_pad_column},
    {"pad_all", LV_style_pad_all},
    {"part", LV_style_set_part},
    {NULL, 0},
};

struct enum_pair s_style[] = {
    {"shadow_w", LV_style_shadow_width},
    {"shadow_width", LV_style_shadow_width},
    {"shadow_ofs_x", LV_style_shadow_ofs_x},
    {"shadow_ofs_y", LV_style_shadow_ofs_y},
    {"shadow_spread", LV_style_shadow_spread},
    {"shadow_color", LV_style_shadow_color},
    {"shadow_opa", LV_style_shadow_opa},
    {"scroll_snap_x", LV_style_scroll_snap_x},
    {"scroll_snap_y", LV_style_scroll_snap_y},
    {"shortclick", LV_style_parser_shortclick},
    {NULL, 0},
};

struct enum_pair t_style[] = {
    {"text_color", LV_style_text_color},
    {"text_opa", LV_style_text_opa},
    {"text_font", LV_style_text_font},
    {"text_letter_space", LV_style_text_letter_space},
    {"text_line_space", LV_style_text_line_space},
    {"text_decor", LV_style_text_decor},
    {"text_align", LV_style_text_align},
    {"transform_w", LV_style_transform_width},
    {"transform_width", LV_style_transform_width},
    {"transform_height", LV_style_transform_height},
    {"translate_x", LV_style_translate_x},
    {"translate_y", LV_style_translate_y},
    {"transform_zoom", LV_style_transform_zoom},
    {"transform_angle", LV_style_transform_angle},
    {"transform_pivot_x", LV_style_transform_pivot_x},
    {"transform_pivot_y", LV_style_transform_pivot_y},
    {"transition", LV_style_transition},
    {NULL, 0},
};

struct enum_pair misc_style[] = {
    {"w", LV_style_width},
    {"h", LV_style_height},
    {"x", LV_style_x},
    {"y", LV_style_y},
    {"width", LV_style_width},
    {"height", LV_style_height},
    {"radius", LV_style_radius},
    {"name", LV_style_view_name},
    {"row_dsc_array", LV_style_grid_row_dsc_array},
    {"row_dsc", LV_style_grid_row_dsc_array},
    {"row_align", LV_style_grid_row_align},

    {NULL, 0},
};

static int find_style(const char *name)
{
    struct enum_pair *p = misc_style;

    switch (name[0]) {
    case 'a': p = a_style; break;
    case 'b': p = b_style; break;
    case 'c': p = c_style; break;
    case 'f': p = f_style; break;
    case 'g': p = g_style; break;
    case 'i': p = i_style; break;
    case 'l': p = l_style; break;
    case 'm': p = m_style; break;
    case 'o': p = o_style; break;
    case 'p': p = p_style; break;
    case 's': p = s_style; break;
    case 't': p = t_style; break;
    default: break;
    }

    int i;
    for (i = 0; p[i].name; i++) {
        if (!strcmp(p[i].name, name))
            return p[i].value;
    }

    return -1;
}

int parse_pct_int(const char *str, long *value)
{
    char *e;
    long v = strtol(str, &e, 0);
    if (e == str)
        return -1;
    if (!strcmp(e, "%"))
        v = LV_PCT(v);
    else if (*e != '\0')
        return -1;
    *value = v;
    return 0;
}

static void add_style(ilv_parser_t *list, int type, long value)
{
    ilv_parser_add_style(list, type, value);
}

static struct enum_pair align_values[] = {
    {"center", LV_ALIGN_CENTER},
    {"left_mid", LV_ALIGN_LEFT_MID},
    {"right_mid", LV_ALIGN_RIGHT_MID},
    {"top_left", LV_ALIGN_TOP_LEFT},
    {"top_mid", LV_ALIGN_TOP_MID},
    {"top_right", LV_ALIGN_TOP_RIGHT},
    {"bottom_left", LV_ALIGN_BOTTOM_LEFT},
    {"bottom_mid", LV_ALIGN_BOTTOM_MID},
    {"bottom_right", LV_ALIGN_BOTTOM_RIGHT},
    {"default", LV_ALIGN_DEFAULT},
    {NULL, 0},
};

static struct enum_pair flex_flow_values[] = {
    {"row", LV_FLEX_FLOW_ROW},
    {"col", LV_FLEX_FLOW_COLUMN},
    {"column", LV_FLEX_FLOW_COLUMN},
    {"row_wrap", LV_FLEX_FLOW_ROW_WRAP},
    {"row_reverse", LV_FLEX_FLOW_ROW_REVERSE},
    {"row_wrap_reverse", LV_FLEX_FLOW_ROW_WRAP_REVERSE},
    {"col_wrap", LV_FLEX_FLOW_COLUMN_WRAP},
    {"col_reverse", LV_FLEX_FLOW_COLUMN_REVERSE},
    {"col_wrap_reverse", LV_FLEX_FLOW_COLUMN_WRAP_REVERSE},
    {"column_wrap", LV_FLEX_FLOW_COLUMN_WRAP},
    {"column_reverse", LV_FLEX_FLOW_COLUMN_REVERSE},
    {"column_wrap_reverse", LV_FLEX_FLOW_COLUMN_WRAP_REVERSE},
    {NULL, 0},
};

static struct enum_pair flex_align_values[] = {
    {"start", LV_FLEX_ALIGN_START},
    {"end", LV_FLEX_ALIGN_END},
    {"center", LV_FLEX_ALIGN_CENTER},
    {"space_evenly", LV_FLEX_ALIGN_SPACE_EVENLY},
    {"space_around", LV_FLEX_ALIGN_SPACE_AROUND},
    {"space_between", LV_FLEX_ALIGN_SPACE_BETWEEN},
    {"align_start", LV_FLEX_ALIGN_START},
    {"align_end", LV_FLEX_ALIGN_END},
    {"align_center", LV_FLEX_ALIGN_CENTER},
    {"align_space_evenly", LV_FLEX_ALIGN_SPACE_EVENLY},
    {"align_space_around", LV_FLEX_ALIGN_SPACE_AROUND},
    {"align_space_between", LV_FLEX_ALIGN_SPACE_BETWEEN},
    {NULL, 0},
};

static struct enum_pair grid_align_values[] = {
    {"start", LV_GRID_ALIGN_START},
    {"end", LV_GRID_ALIGN_END},
    {"center", LV_GRID_ALIGN_END},
    {"stretch", LV_GRID_ALIGN_STRETCH},
    {"space_evenly", LV_GRID_ALIGN_SPACE_EVENLY},
    {"space_around", LV_GRID_ALIGN_SPACE_AROUND},
    {"space_between", LV_GRID_ALIGN_SPACE_BETWEEN},
    {"align_start", LV_GRID_ALIGN_START},
    {"align_end", LV_GRID_ALIGN_END},
    {"align_center", LV_GRID_ALIGN_END},
    {"align_stretch", LV_GRID_ALIGN_STRETCH},
    {"align_space_evenly", LV_GRID_ALIGN_SPACE_EVENLY},
    {"align_space_around", LV_GRID_ALIGN_SPACE_AROUND},
    {"align_space_between", LV_GRID_ALIGN_SPACE_BETWEEN},
    {NULL, 0},
};

static struct enum_pair state_values[] = {
    {"default", LV_STATE_DEFAULT},
    {"checked", LV_STATE_CHECKED},
    {"focused", LV_STATE_FOCUSED},
    {"focus_key", LV_STATE_FOCUS_KEY},
    {"edited", LV_STATE_EDITED},
    {"hovered", LV_STATE_HOVERED},
    {"pressed", LV_STATE_PRESSED},
    {"scrolled", LV_STATE_SCROLLED},
    {"disabled", LV_STATE_DISABLED},
    {"user_1", LV_STATE_USER_1},
    {"user_2", LV_STATE_USER_2},
    {"user_3", LV_STATE_USER_3},
    {"user_4", LV_STATE_USER_4},
    {"any", LV_STATE_ANY},
    {NULL, 0},
};

static struct enum_pair flag_values[] = {
    {"hidden", LV_OBJ_FLAG_HIDDEN},
    {"clickable", LV_OBJ_FLAG_CLICKABLE},
    {"click_focusable", LV_OBJ_FLAG_CLICK_FOCUSABLE},
    {"checkable", LV_OBJ_FLAG_CHECKABLE},
    {"scrollable", LV_OBJ_FLAG_SCROLLABLE},
    {"scroll_elastic", LV_OBJ_FLAG_SCROLL_ELASTIC},
    {"scroll_momentum", LV_OBJ_FLAG_SCROLL_MOMENTUM},
    {"scroll_one", LV_OBJ_FLAG_SCROLL_ONE},
    {"scroll_chain_hor", LV_OBJ_FLAG_SCROLL_CHAIN_HOR},
    {"scroll_chain_ver", LV_OBJ_FLAG_SCROLL_CHAIN_VER},
    {"scroll_chain", LV_OBJ_FLAG_SCROLL_CHAIN},
    {"scroll_on_focus", LV_OBJ_FLAG_SCROLL_ON_FOCUS},
    {"scroll_with_arrow", LV_OBJ_FLAG_SCROLL_WITH_ARROW},
    {"snappable", LV_OBJ_FLAG_SNAPPABLE},
    {"press_lock", LV_OBJ_FLAG_PRESS_LOCK},
    {"event_bubble", LV_OBJ_FLAG_EVENT_BUBBLE},
    {"gesture_bubble", LV_OBJ_FLAG_GESTURE_BUBBLE},
    {"adv_hittest", LV_OBJ_FLAG_ADV_HITTEST},
    {"ignore_layout", LV_OBJ_FLAG_IGNORE_LAYOUT},
    {"floating", LV_OBJ_FLAG_FLOATING},
    {"overflow_visible", LV_OBJ_FLAG_OVERFLOW_VISIBLE},
    {"layout_1", LV_OBJ_FLAG_LAYOUT_1},
    {"layout_2", LV_OBJ_FLAG_LAYOUT_2},
    {"widget_1", LV_OBJ_FLAG_WIDGET_1},
    {"widget_2", LV_OBJ_FLAG_WIDGET_2},
    {"user_1", LV_OBJ_FLAG_USER_1},
    {"user_2", LV_OBJ_FLAG_USER_2},
    {"user_3", LV_OBJ_FLAG_USER_3},
    {"user_4", LV_OBJ_FLAG_USER_4},
    {NULL, 0},
};

static struct enum_pair text_align_values[] = {
    {"auto", LV_TEXT_ALIGN_AUTO},
    {"left", LV_TEXT_ALIGN_LEFT},
    {"center", LV_TEXT_ALIGN_CENTER},
    {"right", LV_TEXT_ALIGN_RIGHT},
    {NULL, 0},
};

static struct enum_pair part_values[] = {
    {"default", LV_STATE_DEFAULT},
    {"checked", LV_STATE_CHECKED},
    {"focused", LV_STATE_FOCUSED},
    {"focus_key", LV_STATE_FOCUS_KEY},
    {"edited", LV_STATE_EDITED},
    {"hovered", LV_STATE_HOVERED},
    {"pressed", LV_STATE_PRESSED},
    {"scrolled", LV_STATE_SCROLLED},
    {"disabled", LV_STATE_DISABLED},
    {"user_1", LV_STATE_USER_1},
    {"user_2", LV_STATE_USER_2},
    {"user_3", LV_STATE_USER_3},
    {"user_4", LV_STATE_USER_4},
    {"main", LV_PART_MAIN},
    {"scrollbar", LV_PART_SCROLLBAR},
    {"indicator", LV_PART_INDICATOR},
    {"knob", LV_PART_KNOB},
    {"selected", LV_PART_SELECTED},
    {"items", LV_PART_ITEMS},
    {"ticks", LV_PART_TICKS},
    {"cursor", LV_PART_CURSOR},
    {"any", LV_PART_ANY},
    {NULL, 0},
};

static int parse_grid_cell(ilv_parser_t *list, int type, const char *key, const char *val)
{
    long col_pos = 0, row_pos = 0, col_span = 1, row_span = 1;
    long col_align = LV_GRID_ALIGN_CENTER, row_align = LV_GRID_ALIGN_CENTER;
    int n, ret = -1;
    char **words = parse_str_array(val, ",", &n);
    switch (n) {
    case 6:
        if (parse_int(words[5], &row_span)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "row_span", key, val);
            break;
        }
        if (parse_int(words[4], &col_span)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "col_span", key, val);
            break;
        }
    case 4:
        if (parse_enum(grid_align_values, words[3], &row_align)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "row_align", key, val);
            break;
        }
        if (parse_enum(grid_align_values, words[2], &col_align)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "col_align", key, val);
            break;
        }
    case 2:
        if (parse_int(words[1], &row_pos)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "row_pos", key, val);
            break;
        }
        if (parse_int(words[0], &col_pos)) {
            fprintf(stderr, "error: 无法获取cell %s属性值: %s=%s\n", "col_pos", key, val);
            break;
        }
        ret = 0;
        break;
    default:
        fprintf(stderr, "error: cell属性值个数太多或者太少: %d %s=%s\n", n, key, val);
        break;
    }

    free_parse_words(words);

    if (ret == 0) {
        lv_grid_cell_data_t *d = malloc(sizeof(*d));
        d->col_align = col_align; d->row_align = row_align;
        d->col_pos = col_pos; d->row_pos = row_pos;
        d->col_span = col_span; d->row_span = row_span;
        add_style(list, type, (long)d);
        ilv_parser_add_delete_data(list, d, free);
    }

    return ret;
}

static void copy_to_chars(long *src, uint8_t *dst, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

static void copy_to_coords(long *src, lv_coord_t *dst, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

static int parse_value(ilv_parser_t *list, int type, const char *key, const char *val)
{
    long value;

    switch (type) {
    case LV_style_width: case LV_style_height:
        if (!strcmp(val, "size_content") || !strcmp(val, "content")) {
            add_style(list, type, LV_SIZE_CONTENT);
            return 0;
        }
    case LV_style_min_width: case LV_style_max_width:
    case LV_style_min_height: case LV_style_max_height:
    case LV_style_x: case LV_style_y:
    case LV_style_translate_x: case LV_style_translate_y:
    case LV_style_transform_pivot_x: case LV_style_transform_pivot_y:
        if (parse_pct_int(val, &value)) {
            fprintf(stderr, "error: 无法获取几何属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_bg_color:
    case LV_style_bg_img_recolor: case LV_style_border_color:
    case LV_style_outline_color: case LV_style_shadow_color:
    case LV_style_img_recolor: case LV_style_line_color:
    case LV_style_arc_color: case LV_style_text_color:
    case LV_style_bg_grad_color:
        return ilv_parser_add_style_color_opa(list, type, key, val);

    case LV_style_text_font:
    case LV_style_color_filter_dsc:
    case LV_style_anim:
    case LV_style_transition:
    case LV_style_bg_grad:
        fprintf(stderr, "error: 暂不支持此属性: %s=%s\n", key, val);
        return -1;

    case LV_style_grid_column_dsc_array:
    case LV_style_grid_row_dsc_array: {
        long *ptr;
        int n = parse_alloc_int_array(val, &ptr);
        if (n <= 0) {
            fprintf(stderr, "error: 无法获取grid dsc属性值: %s=%s\n", key, val);
            return -1;
        }
        lv_coord_t *p = malloc(sizeof(*p)*(n+1));
        copy_to_coords(ptr, p, n);
        p[n] = LV_GRID_TEMPLATE_LAST;

        add_style(list, type, (long)p);
        ilv_parser_add_delete_data(list, p, free);
        free(ptr);
        return 0;
    }

    case LV_style_pad_all: {
        long *ptr;
        int n = parse_alloc_int_array(val, &ptr);
        if (n <= 0) {
            fprintf(stderr, "error: 无法获取pad_all属性值: %s=%s\n", key, val);
            return -1;
        }
        lv_pad_all_data_t *p = malloc(sizeof(*p));
        memset(p, 0, sizeof(*p));
        if (n == 1)
            memset(p, ptr[0], 4);
        else
            copy_to_chars(ptr, (void *)p, n>4?4:n);

        add_style(list, type, (long)p);
        ilv_parser_add_delete_data(list, p, free);
        free(ptr);
        return 0;
    }

    case LV_style_grid_cell:
        return parse_grid_cell(list, type, key, val);

    case LV_style_grid_column_align:
    case LV_style_grid_row_align:
    case LV_style_grid_cell_x_align:
    case LV_style_grid_cell_y_align:
        if (parse_enum(grid_align_values, val, &value)) {
            fprintf(stderr, "error: 无法获取grid对齐属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_text_align:
            if (parse_enum(text_align_values, val, &value)) {
            fprintf(stderr, "error: 无法获取对齐属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_align:
        if (parse_enum(align_values, val, &value)) {
            fprintf(stderr, "error: 无法获取对齐属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_flex_flow:
        if (parse_enum(flex_flow_values, val, &value)) {
            fprintf(stderr, "error: 无法获取flex flow属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_flex_main_place:
    case LV_style_flex_cross_place:
    case LV_style_flex_track_place:
        if (parse_enum(flex_align_values, val, &value)) {
            fprintf(stderr, "error: 无法获取flex对齐属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_add_state:
    case LV_style_clear_state:
        if (parse_enum(state_values, val, &value)) {
            fprintf(stderr, "error: 无法获取state属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_add_flag:
    case LV_style_clear_flag:
        if (parse_enum(flag_values, val, &value)) {
            fprintf(stderr, "error: 无法获取flag属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_set_part:
        if (parse_enum(part_values, val, &value)) {
            fprintf(stderr, "error: 无法获取part属性值: %s=%s\n", key, val);
            return -1;
        }
        add_style(list, type, value);
        return 0;

    case LV_style_bg_img_src:
    case LV_style_view_name:
    case LV_style_font_name:
    case LV_style_parser_shortclick:
        return ilv_parser_add_style_str(list, type, key, val);

    default:
        return ilv_parser_add_style_int(list, type, key, val);
    }
}

static void parse_attrs(ilv_parser_t *list, char **words)
{
    int i;
    for (i = 0; words[i]; i++) {
        char *val = strchr(words[i], '=');
        if (val == NULL) {
            fprintf(stderr, "warn: 属性值后面应该接 '=': %s\n", words[i]);
            continue;
        }

        if (val[1] == '\0') {
            fprintf(stderr, "error: 忽略空值属性: %s\n", words[i]);
            continue;
        }
        *val++ = '\0';
        char *key = words[i];

        if (list->vt && list->vt->parse_cb) {
            if (!list->vt->parse_cb(list, key, val))
                goto restore_word;
        }

        if (key[0] == 'i' && !strcmp(key, "is_root")) {
            if (!strcmp(val, "1")) {
                list->root_index = data_array_size(list->groups);
                goto restore_word;
            }
        }

        int type = find_style(key);
        if (type <= 0) {
            fprintf(stderr, "warn: 未知的属性: %s=%s\n", key, val);
            goto restore_word;
        }

        parse_value(list, type, key, val);
    restore_word:
        val[-1] = '=';
    }
}

static int get_next_view_words(ilv_parser_t *list, char **line_ptr)
{
    char **words = NULL;
    char *line;
    int depth;

    while (1) {
        words = ilv_parser_get_line_words(list, &depth, &line);
        list->save_words = words;
        if (!words)
            return 0;

        char *w0 = words[0];
        int wlen = strlen(w0);
        if (w0[wlen-1] == ':' && w0[0] != ':') {
            if (line_ptr)
                *line_ptr = line;
            else
                free(line);
            return depth;
        }

        if (w0[0] == ':')
            fprintf(stderr, "error: 控件类型不能为空 : (%s)\n", line);
        if (!list->cur_group) {
            fprintf(stderr, "error: 控件类型后面应该接 ':' : (%s)\n", line);
            fprintf(stderr, "error: 属性没有依附控件 或 控件类型后面应该接 ':' : (%s)\n", line);
        } else {
            parse_attrs(list, words);
        }

        free(line);
        free_parse_words(words);
    }

    return 0;
}

static void parse_view(ilv_parser_t *list, char **words)
{
    int wlen = strlen(words[0]);
    view_type_t *vt = find_view_type2(words[0], wlen-1);
    if (!vt) {
        fprintf(stderr, "warn: 未知的控件类型: %s, 使用控件 obj:\n", words[0]);
        vt = find_view_type2("obj", strlen("obj"));
    }

    list->vt = vt;

    lv_view_data_t *data = malloc(sizeof(*data));
    data->type = strndup(words[0], wlen-1);
    data->name = NULL;

    ilv_parser_add_delete_data(list, data, free);
    add_style(list, LV_style_view, (long)data);

    parse_attrs(list, &words[1]);

    free_parse_words(words);
}

static int parse_child_views(ilv_parser_t *list, int depth)
{
    int new_depth;

    add_style(list, LV_style_layout_start, 0);

    parse_view(list, list->save_words);

    while (1) {
        new_depth = get_next_view_words(list, NULL);
        if (!list->save_words)
            break;

        if (new_depth < depth)
            break;

        if (new_depth > depth) {
            new_depth = parse_child_views(list, new_depth);
            if (new_depth < depth)
                break;
        }

        parse_view(list, list->save_words);
    }

    add_style(list, LV_style_layout_end, 0);

    return new_depth;
}

int parse_view_group(ilv_parser_t *list)
{
    int group_depth = list->save_depth;

    parse_view(list, list->save_words);

    int depth = get_next_view_words(list, NULL);
    if (!list->save_words)
        return 0;

    if (depth <= group_depth)
        return depth;

    return parse_child_views(list, depth);
}
