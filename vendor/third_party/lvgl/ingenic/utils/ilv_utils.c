#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "libutils2/listmap.h"

#include "utils/ilv_utils.h"

void ilv_set_as_circle(lv_obj_t *obj, int width)
{
    lv_obj_set_size(obj, width, width);
    lv_obj_set_style_radius(obj, width/2, 0);
}

void ilv_set_bg_color_opa(lv_obj_t *obj, lv_color_t color, int opa)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
}
/************************************************************************/

void ilv_drag_event(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);

    lv_indev_t * indev = lv_indev_get_act();
    if(indev == NULL)  return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t x = lv_obj_get_x(obj) + vect.x;
    lv_coord_t y = lv_obj_get_y(obj) + vect.y;
    lv_obj_set_pos(obj, x, y);
}

void ilv_add_drag_event(lv_obj_t *obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(obj, ilv_drag_event, LV_EVENT_PRESSING, NULL);
}
/************************************************************************/

void ilv_drag_parent_event(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(obj);

    lv_indev_t * indev = lv_indev_get_act();
    if(indev == NULL)  return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t x = lv_obj_get_x(parent) + vect.x;
    lv_coord_t y = lv_obj_get_y(parent) + vect.y;
    lv_obj_set_pos(parent, x, y);
}

void ilv_add_drag_parent_event(lv_obj_t *obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(obj, ilv_drag_parent_event, LV_EVENT_PRESSING, NULL);
}

/************************************************************************/
void ilv_close_event(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_del(obj);
}

void ilv_add_short_clicked_close_event(lv_obj_t *obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(obj, ilv_close_event, LV_EVENT_SHORT_CLICKED, NULL);
}

/************************************************************************/
void ilv_close_parent_event(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_del(lv_obj_get_parent(obj));
}

void ilv_add_short_clicked_close_parent_event(lv_obj_t *obj)
{
    lv_obj_add_event(obj, ilv_close_parent_event, LV_EVENT_SHORT_CLICKED, NULL);
}

/************************************************************************/
lv_obj_t *ilv_get_root_user_obj(lv_obj_t *obj)
{
    while (1) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        // lv_scr_act() lv_layer_top() lv_layer_sys() 's parent is NULL
        if (lv_obj_get_parent(parent) == NULL)
            return obj;
        obj = parent;
    }

    return NULL;
}

lv_obj_t *ilv_get_root_obj(lv_obj_t *obj)
{
    while (1) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        // lv_scr_act() lv_layer_top() lv_layer_sys() 's parent is NULL
        if (parent == NULL)
            return obj;
        obj = parent;
    }

    return NULL;
}

/************************************************************************/
lv_style_t *ilv_load_font(const char *path, int height)
{
#if LVGL_VERSION_MAJOR >= 9
    lv_font_t *font = lv_tiny_ttf_create_file(path, height);
#else
    static lv_ft_info_t info;
    lv_ft_info_t info2 = {
        .name = path,
        .weight = height,
        .style = FT_FONT_STYLE_NORMAL,
        .mem = NULL,
    };
    info = info2;
    if(!path || !lv_ft_font_init(&info)) {
        fprintf(stderr, "failed to create font: %s\n", path);
        return NULL;
    }
    lv_font_t *font = info.font;
#endif
    lv_style_t *style = malloc(sizeof(*style));
    lv_style_init(style);

    lv_style_set_text_font(style, font);
    // lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);

    return style;
}

void ilv_del_font(lv_style_t *style)
{
    lv_style_value_t value;
    lv_style_get_prop(style, LV_STYLE_TEXT_FONT, &value);
    if (value.ptr)
#if LVGL_VERSION_MAJOR >= 9
        lv_tiny_ttf_destroy((void *)value.ptr);
#else
        lv_ft_font_destroy((void *)value.ptr);
#endif
    lv_style_reset(style);
    free(style);
}

/************************************************************************/

#include "libutils2/data_array.h"

static ilv_userdata_dsc_t *find_dsc(data_array_t *array, const char *name, int *index)
{
    int i;
    for (i = 0; i < data_array_size(array); i++) {
        ilv_userdata_dsc_t *dsc = data_array_at(array, i);
        if (!strcmp(dsc->name, name)) {
            if (index)
                *index = i;
            return dsc;
        }
    }
    return NULL;
}

static void obj_delete_userdata_event(lv_event_t *e)
{
    data_array_t *array = lv_event_get_user_data(e);
    if (!array)
        return;
    int i;
    for (i = 0; i < data_array_size(array); i++) {
        ilv_userdata_dsc_t *dsc = data_array_at(array, i);
        if (dsc->free_cb)
            dsc->free_cb(dsc->data);
    }
    data_array_delete(array);
}

void ilv_add_user_data(lv_obj_t *obj, 
    const char *name, void *userdata, ilv_userdata_free_t free_cb)
{
    data_array_t *array = lv_obj_get_event_user_data(obj, obj_delete_userdata_event);
    if (!array) {
        array = data_array_create(sizeof(ilv_userdata_dsc_t), 4);
        lv_obj_add_event(obj, obj_delete_userdata_event, LV_EVENT_DELETE, array);
    }

    ilv_userdata_dsc_t *dsc = find_dsc(array, name, NULL);
    if (dsc) {
        if (userdata != dsc->data) {
            fprintf(stderr, "userdata:%s obj:%p userdata %p override by %p\n",
                 dsc->name, obj, dsc->data, userdata);
            if (dsc->free_cb)
                dsc->free_cb(dsc->data);
            dsc->data = userdata;
            dsc->free_cb = free_cb;
        }
    } else {
        ilv_userdata_dsc_t d = {
            .name = name,
            .data = userdata,
            .free_cb = free_cb
        };
        data_array_add(array, &d);
    }
}

void ilv_del_user_data(lv_obj_t *obj, const char *name)
{
    data_array_t *array = lv_obj_get_event_user_data(obj, obj_delete_userdata_event);
    if (!array)
        return;

    int index;
    ilv_userdata_dsc_t *dsc = find_dsc(array, name, &index);
    if (!dsc)
        return;
    if (dsc->free_cb)
        dsc->free_cb(dsc->data);
    data_array_del(array, index);
}

void *ilv_get_user_data(lv_obj_t *obj, const char *name)
{
    data_array_t *array = lv_obj_get_event_user_data(obj, obj_delete_userdata_event);
    if (!array)
        return NULL;
    ilv_userdata_dsc_t *dsc = find_dsc(array, name, NULL);
    return dsc ? dsc->data : NULL;
}

void *ilv_check_malloc_user_data(lv_obj_t *obj, const char *name, int size)
{
    void *data = ilv_get_user_data(obj, name);
    if (!data) {
        data = malloc(size);
        memset(data, 0, size);
        ilv_add_user_data(obj, name, data, free);
    }
    return data;
}

static void obj_delete_data_event(lv_event_t *e)
{
    void **p = lv_event_get_user_data(e);
    ilv_userdata_free_t free_cb = p[1];
    free_cb(p[0]);
    free(p);
}

void ilv_add_delete_data(lv_obj_t *obj, void *data, ilv_userdata_free_t free_cb)
{
    assert(free_cb);

    void **p = malloc(sizeof(*p)*2);
    p[0] = data;
    p[1] = free_cb;

    lv_obj_add_event(obj, obj_delete_data_event, LV_EVENT_DELETE, p);
}
