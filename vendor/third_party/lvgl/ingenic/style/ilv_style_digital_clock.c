

#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "ilv_style_digital_clock.h"
#include "parser/ilv_parser.h"

int ilv_digital_time_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    switch (style) {
    case LV_style_digital_time_chiness:
        ilv_digital_time_set_chiness(obj, value);
        break;
    case LV_style_digital_time_fill_zero:
        ilv_digital_time_set_fill_zero(obj, value);
        break;
    default:
        fprintf(stderr, "ilv_digital_time: ingore unknown sytle: %d\n", style);
        return -1;
    }
    return 0;
}

static struct enum_pair digital_time_style[] = {
    {"chiness", LV_style_digital_time_chiness},
    {"fill_zero", LV_style_digital_time_fill_zero},

    {NULL, 0},
};

int ilv_digital_time_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    long type;

    if (parse_enum(digital_time_style, key, &type))
        return -1;

    switch (type) {
    case LV_style_digital_time_chiness:
        return ilv_parser_add_style_int(parser, type, key, val);
    case LV_style_digital_time_fill_zero:
        return ilv_parser_add_style_int(parser, type, key, val);
    }

    return 0;
}
