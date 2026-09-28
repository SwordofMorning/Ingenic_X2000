#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "view/ilv_rotate_img.h"
#include "style/ilv_style_rotate_img.h"
#include "parser/ilv_parser.h"

int ilv_rotate_img_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    switch (style) {
    case LV_style_rotate_img_src: {
        void *p = realpath((void *)value, NULL);
        if (p)
            ilv_add_user_data(obj, "ilv-rotate-img-src", p, free);
        else
            p = (void *) value;
        ilv_rotate_img_set_src(obj, p);
        break;
    }
    case LV_style_rotate_img_angle:
        ilv_rotate_img_set_angle(obj, value);
        break;
    case LV_style_rotate_img_src_center: {
        lv_point_t *p = (void *)value;
        ilv_rotate_img_set_src_center(obj, p->x, p->y);
        break;
    }
    case LV_style_rotate_img_dst_center: {
        lv_point_t *p = (void *)value;
        ilv_rotate_img_set_dst_center(obj, p->x, p->y);
        break;
    }
    case LV_style_rotate_img_extra_cfg: {
        ilv_rotate_img_cfg_t *cfg = (void *)value;
        ilv_rotate_img_set_extra_cfg(obj, cfg);
        break;
    }
    case LV_style_rotate_img_bilinear:
        ilv_rotate_img_set_bilinear(obj, value);
        break;
    default:
        fprintf(stderr, "ilv_rotate_img: ingore unknown sytle: %d\n", style);
        return -1;
    }
    return 0;
}

static struct enum_pair rotate_img_style[] = {
    {"src", LV_style_rotate_img_src},
    {"path", LV_style_rotate_img_src},
    {"angle", LV_style_rotate_img_angle},
    {"src_center", LV_style_rotate_img_src_center},
    {"dst_center", LV_style_rotate_img_dst_center},
    {"bilinear", LV_style_rotate_img_bilinear},
    {NULL, 0},
};

int ilv_rotate_img_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    long type;

    if (parse_enum(rotate_img_style, key, &type))
        return -1;

    switch (type) {
    case LV_style_rotate_img_src:
        return ilv_parser_add_style_str(parser, type, key, val);
    case LV_style_rotate_img_angle:
        return ilv_parser_add_style_int(parser, type, key, val);
    case LV_style_rotate_img_src_center:
        return ilv_parser_add_style_point(parser, type, key, val);
    case LV_style_rotate_img_dst_center:
        return ilv_parser_add_style_point(parser, type, key, val);
    case LV_style_rotate_img_bilinear:
        return ilv_parser_add_style_int(parser, type, key, val);
    }

    return 0;
}
