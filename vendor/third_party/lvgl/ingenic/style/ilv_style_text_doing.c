

#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "view/ilv_text_doing.h"
#include "ilv_style_text_doing.h"
#include "parser/ilv_parser.h"

int ilv_text_doing_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    switch (style) {
    case LV_style_text_doing_text:
        ilv_text_doing_set_text(obj, (void *)value);
        break;
    case LV_style_text_doing_timeout:
        ilv_text_doing_set_timeout(obj, value);
        break;
    case LV_style_text_doing_text_color:
        ilv_text_doing_set_text_color(obj, lv_color_from_int(value));
        break;
    case LV_style_text_doing_text_opa:
        ilv_text_doing_set_text_opa(obj, value);
        break;
    default:
        fprintf(stderr, "ilv_text_doing: ingore unknown sytle: %d\n", style);
        return -1;
    }
    return 0;
}

static struct enum_pair text_doing_style[] = {
    {"text", LV_style_text_doing_text},
    {"timeout", LV_style_text_doing_timeout},
    {"color", LV_style_text_doing_text_color},
    {"text_color", LV_style_text_doing_text_color},
    {"text_opa", LV_style_text_doing_text_opa},

    {NULL, 0},
};

int ilv_text_doing_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    long type;

    if (parse_enum(text_doing_style, key, &type))
        return -1;

    switch (type) {
    case LV_style_text_doing_text:
        return ilv_parser_add_style_str(parser, type, key, val);
    case LV_style_text_doing_timeout:
        return ilv_parser_add_style_int(parser, type, key, val);
    case LV_style_text_doing_text_color:
        return ilv_parser_add_style_color_opa(parser, type, key, val);
    case LV_style_text_doing_text_opa:
        return ilv_parser_add_style_int(parser, type, key, val);
    }

    return 0;
}
