#ifndef _ILV_STYLE_ANALOG_CLOCK_H_
#define _ILV_STYLE_ANALOG_CLOCK_H_

#include "view/ilv_analog_clock.h"

int ilv_rotate_hour_set_style(lv_obj_t *obj, int style, long value, int selector);

int ilv_rotate_minute_set_style(lv_obj_t *obj, int style, long value, int selector);

int ilv_rotate_second_set_style(lv_obj_t *obj, int style, long value, int selector);

struct ilv_parser;

int ilv_rotate_hour_parse_style(struct ilv_parser *parser, const char *key, const char *val);

int ilv_rotate_minute_parse_style(struct ilv_parser *parser, const char *key, const char *val);

int ilv_rotate_second_parse_style(struct ilv_parser *parser, const char *key, const char *val);

#endif /* _ILV_STYLE_ANALOG_CLOCK_H_ */
