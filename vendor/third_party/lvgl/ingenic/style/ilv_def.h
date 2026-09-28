#ifndef _ILV_DEF_H_
#define _ILV_DEF_H_

#include "lvgl/lvgl.h"

#if LVGL_VERSION_MAJOR >= 9
#define LV_STYLE_BG_IMG_SRC     LV_STYLE_BG_IMAGE_SRC 
#define LV_STYLE_ARC_IMG_SRC    LV_STYLE_ARC_IMAGE_SRC
#define LV_STYLE_BG_IMG_RECOLOR    LV_STYLE_BG_IMAGE_RECOLOR
#define LV_STYLE_IMG_RECOLOR    LV_STYLE_IMAGE_RECOLOR
#define lv_obj_set_style_bg_img_src lv_obj_set_style_bg_image_src

enum {
#if LV_USE_FLEX
    LV_LAYOUT_FLEX_ = LV_LAYOUT_FLEX,
#endif

#if LV_USE_GRID
    LV_LAYOUT_GRID_ = LV_LAYOUT_GRID,
#endif

    LV_LAYOUT_COUNT_,
};

enum {
    LV_SYTLE_START_ = _LV_STYLE_LAST_BUILT_IN_PROP,

#if LV_USE_FLEX
    LV_STYLE_FLEX_FLOW_ = LV_STYLE_FLEX_FLOW,
    LV_STYLE_FLEX_MAIN_PLACE_ = LV_STYLE_FLEX_MAIN_PLACE,
    LV_STYLE_FLEX_CROSS_PLACE_ = LV_STYLE_FLEX_CROSS_PLACE,
    LV_STYLE_FLEX_TRACK_PLACE_ = LV_STYLE_FLEX_TRACK_PLACE,
    LV_STYLE_FLEX_GROW_ = LV_STYLE_FLEX_GROW,
#endif

#if LV_USE_GRID
    LV_STYLE_GRID_COLUMN_DSC_ARRAY_ = LV_STYLE_GRID_COLUMN_DSC_ARRAY,
    LV_STYLE_GRID_ROW_DSC_ARRAY_ = LV_STYLE_GRID_ROW_DSC_ARRAY,
    LV_STYLE_GRID_COLUMN_ALIGN_ = LV_STYLE_GRID_COLUMN_ALIGN,
    LV_STYLE_GRID_ROW_ALIGN_ = LV_STYLE_GRID_ROW_ALIGN,

    LV_STYLE_GRID_CELL_ROW_SPAN_ = LV_STYLE_GRID_CELL_ROW_SPAN,
    LV_STYLE_GRID_CELL_ROW_POS_ = LV_STYLE_GRID_CELL_ROW_POS,
    LV_STYLE_GRID_CELL_COLUMN_SPAN_ = LV_STYLE_GRID_CELL_COLUMN_SPAN,
    LV_STYLE_GRID_CELL_COLUMN_POS_ = LV_STYLE_GRID_CELL_COLUMN_POS,
    LV_STYLE_GRID_CELL_X_ALIGN_ = LV_STYLE_GRID_CELL_X_ALIGN,
    LV_STYLE_GRID_CELL_Y_ALIGN_ = LV_STYLE_GRID_CELL_Y_ALIGN,
#endif

};

#endif


#if LVGL_VERSION_MAJOR < 9

#define LV_STYLE_BG_IMAGE_SRC   LV_STYLE_BG_IMG_SRC
#define LV_STYLE_ARC_IMAGE_SRC  LV_STYLE_ARC_IMG_SRC
#define LV_STYLE_BG_IMAGE_RECOLOR  LV_STYLE_BG_IMG_RECOLOR
#define LV_STYLE_IMAGE_RECOLOR  LV_STYLE_IMG_RECOLOR
#define lv_obj_set_style_bg_image_src lv_obj_set_style_bg_img_src
#define lv_obj_add_event lv_obj_add_event_cb

enum {
#if LV_USE_FLEX
    LV_LAYOUT_FLEX_,  // --> LV_LAYOUT_FLEX
#endif

#if LV_USE_GRID
    LV_LAYOUT_GRID_,  // --> LV_LAYOUT_GRID
#endif

    ILV_LAYOUT_COUNT_,
};

enum {
    LV_SYTLE_START_ = _LV_STYLE_LAST_BUILT_IN_PROP,

#if LV_USE_FLEX
    LV_STYLE_FLEX_FLOW_,
    LV_STYLE_FLEX_MAIN_PLACE_,
    LV_STYLE_FLEX_CROSS_PLACE_,
    LV_STYLE_FLEX_TRACK_PLACE_,
    LV_STYLE_FLEX_GROW_,
#endif

#if LV_USE_GRID
    LV_STYLE_GRID_COLUMN_DSC_ARRAY_,
    LV_STYLE_GRID_ROW_DSC_ARRAY_,
    LV_STYLE_GRID_COLUMN_ALIGN_,
    LV_STYLE_GRID_ROW_ALIGN_,

    LV_STYLE_GRID_CELL_ROW_SPAN_,
    LV_STYLE_GRID_CELL_ROW_POS_,
    LV_STYLE_GRID_CELL_COLUMN_SPAN_,
    LV_STYLE_GRID_CELL_COLUMN_POS_,
    LV_STYLE_GRID_CELL_X_ALIGN_,
    LV_STYLE_GRID_CELL_Y_ALIGN_,
#endif

    LV_SYTLE_LAST_,
};

#define lv_malloc lv_mem_alloc
#define lv_free lv_mem_free

#define LV_RESULT_INVALID LV_RES_INV
#define LV_RESULT_OK LV_RES_OK
#define lv_result_t lv_res_t

#define lv_image_decoder_t lv_img_decoder_t
#define lv_image_decoder_dsc_t lv_img_decoder_dsc_t
#define lv_image_header_t lv_img_header_t
#define lv_image_src_t lv_img_src_t
#define LV_IMAGE_SRC_FILE LV_IMG_SRC_FILE
#define lv_image_decoder_create lv_img_decoder_create
#define lv_image_decoder_set_info_cb lv_img_decoder_set_info_cb
#define lv_image_decoder_set_open_cb lv_img_decoder_set_open_cb
#define lv_image_decoder_set_close_cb lv_img_decoder_set_close_cb
#define lv_image_src_get_type lv_img_src_get_type

#include <inttypes.h>
/* platform-specific printf format for int32_t, usually "d" or "ld" */
#define LV_PRId32 PRId32
#define LV_PRIu32 PRIu32
#define LV_PRIx32 PRIx32
#define LV_PRIX32 PRIX32

#define LV_COLOR_FORMAT_RGB888 LV_IMG_CF_TRUE_COLOR

static inline uint32_t lv_color_to_int(lv_color_t c)
{
    uint8_t * tmp = (uint8_t *) &c;
    return tmp[0] + (tmp[1] << 8) + (tmp[2] << 16);
}

static inline lv_color_t lv_color_from_int(uint32_t v)
{
    void * p = (void *)&v;
    return *((lv_color_t *)p);
}
#endif

#endif /* _ILV_DEF_H_ */
