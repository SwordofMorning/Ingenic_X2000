

#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "style/ilv_style_rotate_img.h"
#include "ilv_style_analog_clock.h"
#include "parser/ilv_parser.h"

int ilv_rotate_hour_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    return ilv_rotate_img_set_style(obj, style, value, selector);
}

int ilv_rotate_minute_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    return ilv_rotate_img_set_style(obj, style, value, selector);
}

int ilv_rotate_second_set_style(lv_obj_t *obj, int style, long value, int selector)
{
    return ilv_rotate_img_set_style(obj, style, value, selector);
}


int ilv_rotate_hour_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    return ilv_rotate_img_parse_style(parser, key, val);
}

int ilv_rotate_minute_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    return ilv_rotate_img_parse_style(parser, key, val);
}

int ilv_rotate_second_parse_style(struct ilv_parser *parser, const char *key, const char *val)
{
    return ilv_rotate_img_parse_style(parser, key, val);
}
