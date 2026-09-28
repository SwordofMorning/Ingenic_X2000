#ifndef _ILV_STYLE_TYPE_H_
#define _ILV_STYLE_TYPE_H_

#include "lvgl/lvgl.h"

enum {
    LV_style_width = LV_STYLE_WIDTH,
    LV_style_min_width = LV_STYLE_MIN_WIDTH,
    LV_style_max_width = LV_STYLE_MAX_WIDTH,
    LV_style_height = LV_STYLE_HEIGHT,
    LV_style_min_height = LV_STYLE_MIN_HEIGHT,
    LV_style_max_height = LV_STYLE_MAX_HEIGHT,
    LV_style_x = LV_STYLE_X,
    LV_style_y = LV_STYLE_Y,
    LV_style_align = LV_STYLE_ALIGN,
    LV_style_layout = LV_STYLE_LAYOUT,
    LV_style_radius = LV_STYLE_RADIUS,
    LV_style_pad_top = LV_STYLE_PAD_TOP,
    LV_style_pad_bottom = LV_STYLE_PAD_BOTTOM,
    LV_style_pad_left = LV_STYLE_PAD_LEFT,
    LV_style_pad_right = LV_STYLE_PAD_RIGHT,
    LV_style_pad_row = LV_STYLE_PAD_ROW,
    LV_style_pad_column = LV_STYLE_PAD_COLUMN,
    LV_style_base_dir = LV_STYLE_BASE_DIR,
    LV_style_clip_corner = LV_STYLE_CLIP_CORNER,
    LV_style_bg_color = LV_STYLE_BG_COLOR,
    LV_style_bg_opa = LV_STYLE_BG_OPA,
    LV_style_bg_grad_color = LV_STYLE_BG_GRAD_COLOR,
    LV_style_bg_grad_dir = LV_STYLE_BG_GRAD_DIR,
    LV_style_bg_main_stop = LV_STYLE_BG_MAIN_STOP,
    LV_style_bg_grad_stop = LV_STYLE_BG_GRAD_STOP,
    LV_style_bg_grad = LV_STYLE_BG_GRAD,
    LV_style_bg_dither_mode = LV_STYLE_BG_DITHER_MODE,
    LV_style_bg_img_src = LV_STYLE_BG_IMG_SRC,
    LV_style_bg_img_opa = LV_STYLE_BG_IMG_OPA,
    LV_style_bg_img_recolor = LV_STYLE_BG_IMG_RECOLOR,
    LV_style_bg_img_recolor_opa = LV_STYLE_BG_IMG_RECOLOR_OPA,
    LV_style_bg_img_tiled = LV_STYLE_BG_IMG_TILED,
    LV_style_border_color = LV_STYLE_BORDER_COLOR,
    LV_style_border_opa = LV_STYLE_BORDER_OPA,
    LV_style_border_width = LV_STYLE_BORDER_WIDTH,
    LV_style_border_side = LV_STYLE_BORDER_SIDE,
    LV_style_border_post = LV_STYLE_BORDER_POST,
    LV_style_outline_width = LV_STYLE_OUTLINE_WIDTH,
    LV_style_outline_color = LV_STYLE_OUTLINE_COLOR,
    LV_style_outline_opa = LV_STYLE_OUTLINE_OPA,
    LV_style_outline_pad = LV_STYLE_OUTLINE_PAD,
    LV_style_shadow_width = LV_STYLE_SHADOW_WIDTH,
    LV_style_shadow_ofs_x = LV_STYLE_SHADOW_OFS_X,
    LV_style_shadow_ofs_y = LV_STYLE_SHADOW_OFS_Y,
    LV_style_shadow_spread = LV_STYLE_SHADOW_SPREAD,
    LV_style_shadow_color = LV_STYLE_SHADOW_COLOR,
    LV_style_shadow_opa = LV_STYLE_SHADOW_OPA,
    LV_style_img_opa = LV_STYLE_IMG_OPA,
    LV_style_img_recolor = LV_STYLE_IMG_RECOLOR,
    LV_style_img_recolor_opa = LV_STYLE_IMG_RECOLOR_OPA,
    LV_style_line_width = LV_STYLE_LINE_WIDTH,
    LV_style_line_dash_width = LV_STYLE_LINE_DASH_WIDTH,
    LV_style_line_dash_gap = LV_STYLE_LINE_DASH_GAP,
    LV_style_line_rounded = LV_STYLE_LINE_ROUNDED,
    LV_style_line_color = LV_STYLE_LINE_COLOR,
    LV_style_line_opa = LV_STYLE_LINE_OPA,
    LV_style_arc_width = LV_STYLE_ARC_WIDTH,
    LV_style_arc_rounded = LV_STYLE_ARC_ROUNDED,
    LV_style_arc_color = LV_STYLE_ARC_COLOR,
    LV_style_arc_opa = LV_STYLE_ARC_OPA,
    LV_style_arc_img_src = LV_STYLE_ARC_IMG_SRC,
    LV_style_text_color = LV_STYLE_TEXT_COLOR,
    LV_style_text_opa = LV_STYLE_TEXT_OPA,
    LV_style_text_font = LV_STYLE_TEXT_FONT,
    LV_style_text_letter_space = LV_STYLE_TEXT_LETTER_SPACE,
    LV_style_text_line_space = LV_STYLE_TEXT_LINE_SPACE,
    LV_style_text_decor = LV_STYLE_TEXT_DECOR,
    LV_style_text_align = LV_STYLE_TEXT_ALIGN,
    LV_style_opa = LV_STYLE_OPA,
    LV_style_color_filter_dsc = LV_STYLE_COLOR_FILTER_DSC,
    LV_style_color_filter_opa = LV_STYLE_COLOR_FILTER_OPA,
    LV_style_anim = LV_STYLE_ANIM,
    LV_style_anim_time = LV_STYLE_ANIM_TIME,
    LV_style_anim_speed = LV_STYLE_ANIM_SPEED,
    LV_style_transition = LV_STYLE_TRANSITION,
    LV_style_blend_mode = LV_STYLE_BLEND_MODE,
    LV_style_transform_width = LV_STYLE_TRANSFORM_WIDTH,
    LV_style_transform_height = LV_STYLE_TRANSFORM_HEIGHT,
    LV_style_translate_x = LV_STYLE_TRANSLATE_X,
    LV_style_translate_y = LV_STYLE_TRANSLATE_Y,
    LV_style_transform_zoom = LV_STYLE_TRANSFORM_ZOOM,
    LV_style_transform_angle = LV_STYLE_TRANSFORM_ANGLE,
    LV_style_transform_pivot_x = LV_STYLE_TRANSFORM_PIVOT_X,
    LV_style_transform_pivot_y = LV_STYLE_TRANSFORM_PIVOT_Y,

    LV_style_start_ = _LV_STYLE_LAST_BUILT_IN_PROP,

#if LV_USE_FLEX
    LV_style_flex_flow,
    LV_style_flex_main_place,
    LV_style_flex_cross_place,
    LV_style_flex_track_place,
    LV_style_flex_grow,
#endif

#if LV_USE_GRID
    LV_style_grid_column_dsc_array,// 参数 lv_coord_t[] 参考 lv_style_set_grid_column_dsc_array
    LV_style_grid_row_dsc_array,   // 参数 lv_coord_t[] 参考 lv_style_set_grid_row_dsc_array
    LV_style_grid_column_align,    // 参数 lv_grid_align_t 参考 lv_style_set_grid_column_align
    LV_style_grid_row_align,       // 参数 lv_grid_align_t 参考 lv_style_set_grid_row_align

    LV_style_grid_cell_row_span,    // 参数 uint8_t 参考 lv_obj_set_grid_cell row_span
    LV_style_grid_cell_row_pos,     // 参数 uint8_t 参考 lv_obj_set_grid_cell row_pos
    LV_style_grid_cell_column_span, // 参数 uint8_t 参考 lv_obj_set_grid_cell column_span
    LV_style_grid_cell_column_pos,  // 参数 uint8_t 参考 lv_obj_set_grid_cell column_pos
    LV_style_grid_cell_x_align,     // 参数 lv_grid_align_t 参考 lv_obj_set_grid_cell column_align/x_align
    LV_style_grid_cell_y_align,     // 参数 lv_grid_align_t 参考 lv_obj_set_grid_cell row_align/y_align
#endif

    LV_style_lv_self_end,

    LV_style_view = 1000,          // lv_view_data_t
    LV_style_add_child,            // ilv_style_t[]
    LV_style_layout_start,         //
    LV_style_layout_end,           //

    LV_style_sets,                 // ilv_style_t[]
    LV_style_copy = LV_style_sets, // ilv_style_t[]
    LV_style_add_event,            // lv_event_data_t
    LV_style_add_data,             // lv_data_data_t
    LV_style_add_delete,           // lv_delete_data_t
    LV_style_add_anim,             // lv_anim_data_t
    LV_style_add_timer,            // lv_timer_data_t
    LV_style_add_style,            // lv_style_t **

    LV_style_pad_all,              // lv_pad_all_data_t
    LV_style_layout_flex,          // lv_flex_data_t
    LV_style_layout_grid,          // lv_grid_data_t
    LV_style_scroll_snap_x,        // lv_scroll_snap_t
    LV_style_scroll_snap_y,        // lv_scroll_snap_t
    LV_style_layout_dirty,         //
    LV_style_view_name,            // const char *
    LV_style_font_name,            // const char *
    LV_style_grid_cell,            // lv_grid_cell_data_t
    LV_style_add_state,            // lv_state_t
    LV_style_clear_state,          // lv_state_t
    LV_style_add_flag,             // lv_obj_flag_t
    LV_style_clear_flag,           // lv_obj_flag_t
    LV_style_set_part,             // lv_part_t
    LV_style_parser_shortclick,    // const char *

    LV_style_extra_start = 3000,
};

typedef struct ilv_style {
    int style;
    long value;
} ilv_style_t;

typedef struct lv_view_data {
    const char *type;
    const char *name;
} lv_view_data_t;

typedef struct lv_data_data {
    const char *name;
    void *data;
} lv_data_data_t;

typedef struct lv_delete_data {
    void *data;
    void (*free_cb)(void *data);
} lv_delete_data_t;

typedef struct lv_event_data {
    int event;
    lv_event_cb_t cb;
    void *data;
} lv_event_data_t;

typedef struct lv_anim_data {
    int32_t start;
    int32_t end;
    uint32_t duration;
    lv_anim_exec_xcb_t exec_cb;
    uint16_t cnt;
} lv_anim_data_t;

typedef struct lv_timer_data {
    lv_timer_cb_t timer_xcb;
    uint32_t period;
    int32_t repeat_count;
} lv_timer_data_t;

typedef struct lv_pad_all_data {
    uint8_t top;
    uint8_t bottom;
    uint8_t left;
    uint8_t right;
} lv_pad_all_data_t;

typedef struct lv_flex_data {
    uint8_t flow;
    uint8_t main_place;
    uint8_t cross_place;
    uint8_t track_place;
} lv_flex_data_t;

typedef struct lv_grid_data {
    uint8_t colum_algin;
    uint8_t row_align;
} lv_grid_data_t;

typedef struct lv_grid_cell_data {
    uint8_t col_align;
    uint8_t col_pos;
    uint8_t col_span;
    uint8_t row_align;
    uint8_t row_pos;
    uint8_t row_span;
} lv_grid_cell_data_t;

#endif /* _ILV_STYLE_TYPE_H_ */
