#ifndef _ILV_STYLE_NORMAL_VIEWS_H_
#define _ILV_STYLE_NORMAL_VIEWS_H_

enum {
    LV_style_label_start_ = LV_style_extra_start,
    LV_style_label_text,          // const char * 
};

enum {
    LV_style_textarea_start_ = LV_style_extra_start,
    LV_style_textarea_text,          // const char * 
    LV_style_textarea_placeholder_text,          // const char *
    LV_style_textarea_oneline,       // int
};

enum {
    LV_style_img_start_ = LV_style_extra_start,
    LV_style_img_src,            // const char * 或者 lv_img_dsc_t
};

enum {
    LV_style_roller_start_ = LV_style_extra_start,
    LV_style_roller_options_normal,     // const char *
    LV_style_roller_options_infinite,   // const char *
    LV_style_roller_row_cnt,            // uint8_t
    LV_style_roller_selected,           // uint16_t
};

enum {
    LV_style_dropdown_start_ = LV_style_extra_start,
    LV_style_dropdown_options,     // const char *
    LV_style_dropdown_dir,         // uint8_t
    LV_style_dropdown_symbol,      // const char * 或者 lv_img_dsc_t
    LV_style_dropdown_selected,    // const char *
    LV_style_dropdown_selected_highlight, // int
};

#include "style/ilv_style_meter.h"
#include "style/ilv_style_rotate_img.h"
#include "ilv_style_text_doing.h"
#include "ilv_style_analog_clock.h"
#include "ilv_style_digital_clock.h"

void ilv_style_add_normal_view_types(void);

#endif /* _ILV_STYLE_NORMAL_VIEWS_H_ */
