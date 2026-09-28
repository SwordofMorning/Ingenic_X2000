#ifndef _ILV_STYLE_ROTATE_IMG_H_
#define _ILV_STYLE_ROTATE_IMG_H_

#include "view/ilv_rotate_img.h"
#include "ilv_style_type.h"

// 参考 ingenic/view/ilv_rotate_img.h

enum {
    LV_style_rotate_img_start_ = LV_style_extra_start,

    LV_style_rotate_img_src,        // 参数 void *, 参考 ilv_rotate_img_set_src()
    LV_style_rotate_img_angle,      // 参数 int, 参考 ilv_rotate_img_set_angle()
    LV_style_rotate_img_src_center, // 参数 lv_point_t 参考 ilv_rotate_img_set_src_center()
    LV_style_rotate_img_dst_center, // 参数 lv_point_t 参考 ilv_rotate_img_set_src_center()
    LV_style_rotate_img_extra_cfg,  // 参数 ilv_rotate_img_cfg_t 参考 ilv_rotate_img_set_extra_cfg
    LV_style_rotate_img_bilinear,   // 参数 int, 参考 ilv_rotate_img_set_bilinear()
};

#define ROTATE_IMG_EXTRA_CFG(bilinear, ignore_alpha, get_fast_version) \
    (long)&(ilv_rotate_img_cfg_t){bilinear, ignore_alpha, get_fast_version}

int ilv_rotate_img_set_style(lv_obj_t *obj, int style, long value, int selector);

struct ilv_parser;
int ilv_rotate_img_parse_style(struct ilv_parser *parser, const char *key, const char *val);

#endif /* _ILV_STYLE_ROTATE_IMG_H_ */
