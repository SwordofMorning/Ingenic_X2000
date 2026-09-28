#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "style/ilv_style.h"
#include "utils/ilv_utils.h"
#include "parser/ilv_config.h"
#include "parser/ilv_parser.h"
#include "libutils2/data_array.h"

static data_array_t *view_types;

void ilv_add_view_type(const char *name, lv_create_view_cb_t create_cb,
     lv_set_view_style_cb_t set_cb, parse_style_cb_t parse_cb)
{
    if (!view_types)
        view_types = data_array_create(sizeof(view_type_t), 16);
    view_type_t t = {name, create_cb, set_cb, parse_cb};
    data_array_add(view_types, &t);
}

static view_type_t cur_vt;

static view_type_t *find_view_type(const char *name)
{
    int i;
    for (i = 0; i < data_array_size(view_types); i++) {
        view_type_t *t = data_array_at(view_types, i);
        if (!strcmp(t->name, name))
            return t;
    }
    return NULL;
}

view_type_t *find_view_type2(const char *name, int n)
{
    int i;
    for (i = 0; i < data_array_size(view_types); i++) {
        view_type_t *t = data_array_at(view_types, i);
        if (!strncmp(t->name, name, n) && t->name[n] == '\0')
            return t;
    }
    return NULL;
}

static view_type_t *set_view_cb(const char *name)
{
    view_type_t *vt = find_view_type(name);

    if (!vt) {
        fprintf(stderr, "ilv_style: can't find view type:%s, use obj\n", name);
        vt = find_view_type("obj");
        assert(vt);
    }

    return vt;
}

static void free_anim(void *data)
{
    long *p = data;
    lv_obj_t *obj = (void *) p[0];
    lv_anim_exec_xcb_t cb = (void *) p[1];
    lv_anim_del(obj, cb);
}

static void free_timer(void *data)
{
    lv_timer_t *t = data;
    lv_timer_del(t);
}

int ilv_style_is_ptr(int style)
{
    if (style == LV_STYLE_BG_GRAD ||
        style == LV_STYLE_BG_IMG_SRC ||
        style == LV_STYLE_ARC_IMG_SRC ||
        style == LV_STYLE_TEXT_FONT ||
        style == LV_STYLE_COLOR_FILTER_DSC ||
        style == LV_STYLE_ANIM ||
        style == LV_STYLE_TRANSITION ||
        style == LV_STYLE_GRID_COLUMN_DSC_ARRAY_ ||
        style == LV_STYLE_GRID_ROW_DSC_ARRAY_)
        return 1;
    return 0;
}

int ilv_style_is_color(int style)
{
    if (style == LV_STYLE_BG_COLOR ||
        style == LV_STYLE_BG_GRAD_COLOR ||
        style == LV_STYLE_BG_IMG_RECOLOR ||
        style == LV_STYLE_BORDER_COLOR ||
        style == LV_STYLE_OUTLINE_COLOR ||
        style == LV_STYLE_SHADOW_COLOR ||
        style == LV_STYLE_IMG_RECOLOR ||
        style == LV_STYLE_LINE_COLOR ||
        style == LV_STYLE_ARC_COLOR ||
        style == LV_STYLE_TEXT_COLOR)
        return 1;
    return 0;
}

int ilv_style_is_pct_size(int style)
{
    if (style == LV_style_width ||
        style == LV_style_min_width ||
        style == LV_style_max_width ||
        style == LV_style_height ||
        style == LV_style_min_height ||
        style == LV_style_max_height ||
        style == LV_style_x ||
        style == LV_style_y ||
        style == LV_style_translate_x ||
        style == LV_style_translate_y ||
        style == LV_style_transform_pivot_x ||
        style == LV_style_transform_pivot_y)
        return 1;
    return 0;
}

char *ilv_check_img_src_realpath(char *src)
{
    if (!src[0])
        return NULL;
    return realpath(src, NULL);
}

static void ilv_set_lv_self_style(lv_obj_t *obj, int style, long value, int selector)
{
    if (style == LV_STYLE_BG_IMAGE_SRC) {
        void *v = ilv_check_img_src_realpath((void *)value);
        if (v) {
            ilv_add_delete_data(obj, v, free);
            value = (long) v;
        }
    }
    if (style == LV_STYLE_ARC_IMG_SRC) {
        void *v = ilv_check_img_src_realpath((void *)value);
        if (v) {
            ilv_add_delete_data(obj, v, free);
            value = (long) v;
        }
    }

    lv_style_value_t v = {0};
    if (ilv_style_is_ptr(style))
        v.ptr = (void *) value;
    else if (ilv_style_is_color(style))
        v.color = lv_color_from_int(value);
    else
        v.num = value;

    lv_obj_set_local_style_prop(obj, style, v, selector);
}

void ilv_style_set(lv_obj_t *obj, int style, long value, int selector)
{
    if (style < LV_style_lv_self_end)
        return ilv_set_lv_self_style(obj, style, value, selector);

    if (style >= LV_style_extra_start) {
        int ret = -1;
        if (cur_vt.set_extra_style)
            ret = cur_vt.set_extra_style(obj, style, value, selector);
        if (ret != 0)
            fprintf(stderr, "ilv_style: %s can't set this style: %d\n", cur_vt.name, style);
        return;
    }

    if (style == LV_style_sets) {
        ilv_styles_sets(obj, (void *)value, selector);
        return;
    }

    if (style == LV_style_add_event && value) {
        lv_event_data_t *t = (void *)value;
        if (t->cb) {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event(obj, t->cb, t->event, t->data);
        }
        return;
    }

    if (style == LV_style_add_data && value) {
        lv_data_data_t *t = (void *)value;
        ilv_add_user_data(obj, t->name, t->data, NULL);
        return;
    }

    if (style == LV_style_add_delete && value) {
        lv_delete_data_t *t = (void *)value;
        ilv_add_delete_data(obj, t->data, t->free_cb);
        return;
    }

    if (style == LV_style_add_anim && value) {
        lv_anim_data_t *e = (void *)value;
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, e->exec_cb);
        lv_anim_set_values(&a, e->start, e->end);
        lv_anim_set_repeat_count(&a, e->cnt);
        lv_anim_set_time(&a, e->duration);
        lv_anim_set_var(&a, obj);
        lv_anim_start(&a);
        long *p = malloc(sizeof(long)*2);
        p[0] = (long) obj;
        p[1] = (long) e->exec_cb;
        if (e->cnt >= 0)
            ilv_add_user_data(obj, "ilv-anim", (void *)p, free_anim);
        return;
    }

    if (style == LV_style_add_timer && value) {
        lv_timer_data_t *e = (void *)value;
        lv_timer_t *t = lv_timer_create(e->timer_xcb, e->period, obj);
        if (e->repeat_count > 0)
            lv_timer_set_repeat_count(t, e->repeat_count);
        else
            ilv_add_user_data(obj, "ilv-timer", t, free_timer);
        return;
    }

    if (style == LV_style_add_style && value) {
        lv_style_t **p = (void *)value;
        if (*p)
            lv_obj_add_style(obj, *p, 0);
        return;
    }

    if (style == LV_style_pad_all && value) {
        lv_pad_all_data_t *t = (void *)value;
        ilv_style_set(obj, LV_STYLE_PAD_TOP, t->top, selector);
        ilv_style_set(obj, LV_STYLE_PAD_BOTTOM, t->bottom, selector);
        ilv_style_set(obj, LV_STYLE_PAD_LEFT, t->left, selector);
        ilv_style_set(obj, LV_STYLE_PAD_RIGHT, t->right, selector);
        return;
    }

    if (style == LV_style_layout_flex && value) {
        lv_flex_data_t *t = (void *)value;
        ilv_style_set(obj, LV_STYLE_FLEX_FLOW_, t->flow, selector);
        ilv_style_set(obj, LV_STYLE_FLEX_MAIN_PLACE_, t->main_place, selector);
        ilv_style_set(obj, LV_STYLE_FLEX_CROSS_PLACE_, t->cross_place, selector);
        ilv_style_set(obj, LV_STYLE_FLEX_TRACK_PLACE_, t->track_place, selector);
        ilv_style_set(obj, LV_STYLE_LAYOUT, LV_LAYOUT_FLEX, selector);
        return;
    }

    if (style == LV_style_layout_grid && value) {
        lv_grid_data_t *t = (void *)value;
        ilv_style_set(obj, LV_STYLE_GRID_COLUMN_ALIGN_, t->colum_algin, selector);
        ilv_style_set(obj, LV_STYLE_GRID_ROW_ALIGN_, t->row_align, selector);
        ilv_style_set(obj, LV_STYLE_LAYOUT, LV_LAYOUT_GRID, selector);
        return;
    }

    if (style == LV_style_grid_cell) {
        lv_grid_cell_data_t *t = (void *)value;
        lv_obj_set_grid_cell(obj, t->col_align, t->col_pos, t->col_span,
                                 t->row_align, t->row_pos, t->row_span);
        return;
    }

    if (style == LV_style_scroll_snap_x)
        return lv_obj_set_scroll_snap_x(obj, value);

    if (style == LV_style_scroll_snap_y)
        return lv_obj_set_scroll_snap_y(obj, value);

    if (style == LV_style_layout_dirty)
        return lv_obj_mark_layout_as_dirty(obj);

    if (style == LV_style_view_name)
        return ilv_add_user_data(obj, "ilv-view-name", (void *)value, NULL);

    if (style == LV_style_font_name) {
        void *font;
        if (!ilv_config_get("font", (void *)value, &font))
            lv_obj_add_style(obj, font, 0);
        return;
    }

    if (style == LV_style_add_state)
        return lv_obj_add_state(obj, value);

    if (style == LV_style_clear_state)
        return lv_obj_clear_state(obj, value);

    if (style == LV_style_add_flag)
        return lv_obj_add_flag(obj, value);

    if (style == LV_style_clear_flag)
        return lv_obj_clear_flag(obj, value);

    if (style == LV_style_parser_shortclick && value) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event(obj, ilv_parser_shortclick_event, LV_EVENT_SHORT_CLICKED, (void *)value);
    }
}

long ilv_style_get(lv_obj_t *obj, int style, int selector)
{
    if (style >= LV_style_lv_self_end) {
        fprintf(stderr, "ilv_style: just support get lvgl self style\n");
        return 0;
    }

    lv_style_value_t v = {0};
    lv_obj_get_local_style_prop(obj, style, &v, selector);
    if (ilv_style_is_ptr(style))
        return (long) v.ptr;
    else if (ilv_style_is_color(style))
        return lv_color_to_int(v.color);
    else
        return v.num;
}

ilv_style_t *ilv_styles_sets(lv_obj_t *obj, ilv_style_t *styles, int selector)
{
    while (styles->style) {
        if (styles->style >= LV_style_view && styles->style <= LV_style_layout_end)
            break;
        if (styles->style == LV_style_set_part)
            selector = styles->value;
        else
            ilv_style_set(obj, styles->style, styles->value, selector);
        styles++;
    }
    return styles;
}

static lv_obj_t *create_view(lv_obj_t *parent, ilv_style_t *s, view_type_t *vt_save)
{
    int style = s->style;
    lv_view_data_t *t = (void *)s->value;

    if (style != LV_style_view) {
        fprintf(stderr, "ilv_style: ignore not view style:%d\n", style);
        return NULL;
    }
 
    view_type_t *vt = set_view_cb(t->type);
    *vt_save = cur_vt;
    cur_vt = *vt;

    lv_obj_t *obj = vt->create_view(parent);
    // printf("create: %s:%s %p parent=%p\n", t->type, t->name, obj, parent);

    if (t->name)
        ilv_add_user_data(obj, "ilv-view-name", (void *)t->name, NULL);

    return obj;
}

static ilv_style_t *add_childs(lv_obj_t *parent, ilv_style_t *s, int depth, lv_obj_t **ret)
{
    lv_obj_t *obj = NULL;
    view_type_t vt_save, vt_save2;
    int selector = 0;

    for (; s->style; ) {
        // printf("[%d] style: %d\n", depth, s->style);
        obj = create_view(parent, s++, &vt_save);
        if (obj)
            break;
    }

    lv_obj_t *first_obj = obj;

    for (; s->style; ) {
        // printf("[%d] style: %d\n", depth, s->style);
        switch (s->style) {
        case LV_style_view:
            obj = create_view(parent, s, &vt_save2);
            selector = 0;
            break;
        case LV_style_add_child:
            ilv_create_view(obj, (void *)s->value);
            break;
        case LV_style_layout_start:
            selector = 0;
            s = add_childs(obj, s+1, depth+1, NULL);
            selector = 0;
            continue;
        case LV_style_layout_end:
            s++;
            if (depth > 0)
                goto out;
            fprintf(stderr, "ilv: ignore redundant LV_style_layout_end\n");
            break;
        case LV_style_set_part:
            selector = s->value;
            break;
        default:
            ilv_style_set(obj, s->style, s->value, selector);
            break;
        }

        s++;
    }

out:
    if (ret)
        *ret = first_obj;
    cur_vt = vt_save;

    return s;
}

lv_obj_t *ilv_create_view(lv_obj_t *parent, ilv_style_t *styles)
{
    lv_obj_t *obj = NULL;
    add_childs(parent, styles, 0, &obj);
    return obj;
}

lv_obj_t *ilv_get_child(lv_obj_t *parent, const char *name)
{
    int i;
    int cnt = lv_obj_get_child_cnt(parent);

    for (i = 0; i < cnt; i++) {
        lv_obj_t *c = lv_obj_get_child(parent, i);
        void *v = ilv_get_user_data(c, "ilv-view-name");
        if (v && !strcmp(v, name))
            return c;
    }

    return NULL;
}

lv_obj_t *ilv_get_child2(lv_obj_t *parent,...)
{
    lv_obj_t *obj = NULL;
    va_list args;

    va_start(args, parent);

    while (1) {
        char *name = va_arg(args, char *);
        if (!name)
            return obj;
        obj = ilv_get_child(parent, name);
        if (!obj)
            return NULL;
        parent = obj;
    }

    va_end(args);

    return NULL;
}

lv_obj_t *ilv_search_child(lv_obj_t *parent, const char *name)
{
    int i;
    int cnt = lv_obj_get_child_cnt(parent);

    for (i = 0; i < cnt; i++) {
        lv_obj_t *c = lv_obj_get_child(parent, i);
        void *v = ilv_get_user_data(c, "ilv-view-name");
        if (v && !strcmp(v, name))
            return c;
        c = ilv_search_child(c, name);
        if (c)
            return c;
    }

    return NULL;
}

int ilv_check_view_name(ilv_style_t *s, const char *name)
{
    if (s->style == LV_style_view) {
        lv_view_data_t *t = (void *)s->value;
        if (t->name && !strcmp(t->name, name))
            return 1;
        s++;
    }

    while (s->style) {
        if (s->style == LV_style_view)
            break;
        if (s->style == LV_style_view_name) {
            if (!strcmp((char *)s->value, name))
                return 1;
        }
        s++;
    }

    return 0;
}

int ilv_check_view_type(ilv_style_t *s, const char *type)
{
    if (s->style == LV_style_view) {
        lv_view_data_t *t = (void *)s->value;
        if (t->type && !strcmp(t->type, type))
            return 1;
    }
    return 0;
}

ilv_style_t clear_extra_size_attrs[] = {
    {LV_style_pad_all, LV_PAD_ALL(0, 0, 0, 0)},
    {LV_style_radius, 0},
    {LV_style_shadow_width, 0},
    {LV_style_border_width, 0},
    {LV_style_outline_width, 0},
    {0, 0},
};

// ilv_style_t desktop_views2[] = {
//     {LV_style_view, LV_VIEW("obj", "desktop")},
//     {LV_STYLE_WIDTH, LV_PCT(100)},
//     {LV_STYLE_HEIGHT, LV_PCT(100)},
//     {LV_STYLE_BG_IMAGE_SRC, LV_PTR("/usr/data/720x1280.png")},
//     {LV_STYLE_BG_OPA, 0},
//     {LV_STYLE_ADD_CHILD, (long)(ilv_style_t[]){

//         {LV_style_view, LV_VIEW("obj", "flex")},
//         {LV_STYLE_WIDTH, LV_PCT(100)},
//         {LV_STYLE_HEIGHT, LV_PCT(100)},
//         {LV_STYLE_LAYOUT_FLEX, FLEX_NORMAL(LV_FLEX_FLOW_ROW)},
//         {LV_style_scroll_snap_x, LV_SCROLL_SNAP_START},
//         {LV_style_sets, (long)clear_extra_size_attrs},
//         {LV_STYLE_BG_OPA, 0},
//         {LV_STYLE_ADD_CHILD, (long)(ilv_style_t[]){

//             {LV_style_view, LV_VIEW("obj", "f0")},
//             {LV_STYLE_WIDTH, LV_PCT(100)},
//             {LV_STYLE_HEIGHT, LV_PCT(100)},
//             {LV_style_layout_grid, LV_GRID(LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_END)},
//             {LV_STYLE_GRID_COLUMN_DSC_ARRAY, LV_GRID_DSC(60, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
//             {LV_STYLE_GRID_ROW_DSC_ARRAY, LV_GRID_DSC(160, 160, 160, 160, LV_GRID_TEMPLATE_LAST)},
//             {LV_style_sets, (long)clear_extra_size_attrs},
//             {LV_STYLE_BG_OPA, 0},
//             {LV_STYLE_ADD_CHILD, (long)(ilv_style_t[]){

//                 {LV_style_view, LV_VIEW("obj", "camera")},
//                 {LV_style_sets, LV_PTR(desktop_app_layout_attrs)},
//                 {LV_STYLE_ADD_CHILD, (long)(ilv_style_t[]){
//                     {LV_style_view, LV_VIEW("obj", "image")},
//                     {LV_STYLE_BG_COLOR, LV_COLOR_RED_lighten1},
//                     {LV_STYLE_BORDER_WIDTH, 0},
//                     {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, app_camera_click_event)},
//                 }},
//             }},
//         }},
//     }},

// };