#ifndef _ILV_STYLE_H_
#define _ILV_STYLE_H_

#include "lvgl/lvgl.h"
#include "ilv_def.h"
#include "ilv_style_type.h"
#include "ilv_style_normal_views.h"

#define S(x, v) {LV_style_##x, (long)(v)}

#define LV_STR(str)  (long)(str)
#define LV_PTR(ptr)  (long)(ptr)
#define LV_VIEW(type,name) (long)&(lv_view_data_t){type , name}
#define LV_EVENT(x...) (long)&(lv_event_data_t){x}
#define LV_DATA(name,data) (long)&(lv_data_data_t){name, data}
#define LV_ANIM(x...) (long)&(lv_anim_data_t){x}
#define LV_TIMER(x...) (long)&(lv_timer_data_t){x}
#define LV_POINT(x,y) (long)&(lv_point_t){x,y}
#define LV_FLEX(x...) (long)&(lv_flex_data_t){x}
#define LV_GRID(x...) (long)&(lv_grid_data_t){x}
#define LV_PAD_ALL(x...) (long)&(lv_pad_all_data_t){x}
#define LV_GRID_DSC(x...) (long)(lv_coord_t[]){x}

#define LV_FLEX_NORMAL(flow) LV_FLEX(flow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER)

typedef lv_obj_t *(*lv_create_view_cb_t)(lv_obj_t *parent);
typedef int (*lv_set_view_style_cb_t)(lv_obj_t *obj, int style, long value, int selector);
typedef int (*lv_parse_view_style_cb_t)(lv_obj_t *obj, char *);

struct ilv_parser;
typedef int (*parse_style_cb_t)(struct ilv_parser *parser, const char *key, const char *val);

void ilv_add_view_type(const char *name, lv_create_view_cb_t create_cb,
     lv_set_view_style_cb_t set_cb, parse_style_cb_t parse_cb);

typedef struct view_type {
    const char *name;
    lv_create_view_cb_t create_view;
    lv_set_view_style_cb_t set_extra_style;
    parse_style_cb_t parse_cb;
} view_type_t;

view_type_t *find_view_type2(const char *name, int n);

int ilv_style_is_ptr(int attr);

int ilv_style_is_color(int attr);

int ilv_style_is_pct_size(int style);

char *ilv_check_img_src_realpath(char *src);

void ilv_style_set(lv_obj_t *obj, int attr, long value, int selector);

long ilv_style_get(lv_obj_t *obj, int attr, int selector);

ilv_style_t *ilv_styles_sets(lv_obj_t *obj, ilv_style_t *styles, int selector);

lv_obj_t *ilv_create_view(lv_obj_t *parent, ilv_style_t *styles);

lv_obj_t *ilv_get_child(lv_obj_t *parent, const char *name);

lv_obj_t *ilv_get_child2(lv_obj_t *parent, ...);

// 搜索所有子view
lv_obj_t *ilv_search_child(lv_obj_t *parent, const char *name);

int ilv_check_view_name(ilv_style_t *s, const char *name);

int ilv_check_view_type(ilv_style_t *s, const char *type);

extern ilv_style_t clear_extra_size_attrs[];

#endif /* _ILV_STYLE_H_ */
