#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "lvgl_ingenic_support.h"

struct text_doing_timer_data {
    char *text;
    unsigned int v;
    unsigned int end;
};

static void m_free_userdata(void *data)
{
    struct text_doing_timer_data *p = data;
    free(p->text);
    free(p);
}

static struct text_doing_timer_data *m_get_user_data(lv_obj_t *obj)
{
    struct text_doing_timer_data *p = ilv_get_user_data(obj, "timer-count");
    if (!p) {
        p = malloc(sizeof(*p));
        memset(p, 0, sizeof(*p));
        p->end = -1;
        ilv_add_user_data(obj, "timer-count", p, m_free_userdata);
    }
    return p;
}

static void do_set_text(lv_obj_t *obj, struct text_doing_timer_data *p, const char *text)
{
    if (p->text)
        free(p->text);
    p->text = strdup(text);
    lv_label_set_text_fmt(obj, "%s%s", p->text, "...");
    lv_obj_set_width(obj, LV_SIZE_CONTENT);
    lv_obj_update_layout(obj);
    lv_obj_set_width(obj, lv_obj_get_width(obj));
}

static void text_doing_timer_cb(struct _lv_timer_t *timer)
{
    char dot[4+1] = {0};
    lv_obj_t *obj = timer->user_data;
    struct text_doing_timer_data *p = m_get_user_data(obj);
    int v = p->v++;

    if (!p->text)
        do_set_text(obj, p, strdup(lv_label_get_text(obj)));

    v = v%7;
    if (v >= 4)
        v = 7 - v;
    memset(dot, '.', v);

    lv_label_set_text_fmt(obj, "%s%s", p->text, dot);

    if (p->end != -1 && p->v >= p->end)
        lv_obj_del(lv_obj_get_parent(obj));
}

static ilv_style_t text_doing_text_attrs[] = {
    {LV_style_label_text, LV_STR("test")},
    {LV_style_align, LV_ALIGN_CENTER},
    {LV_style_add_timer, LV_TIMER(text_doing_timer_cb, 550)},
    {0, 0},
};

static ilv_style_t text_doing_layer_attrs[] = {
    {LV_style_view, LV_VIEW("obj", "td_layer")},
    {LV_style_shadow_width, 30},
    {LV_style_layout_start},
        {LV_style_view, LV_VIEW("label", "text")},
        {LV_style_sets, LV_PTR(text_doing_text_attrs)},
    {LV_style_layout_end},

    {0, 0},
};

lv_obj_t *ilv_text_doing_view_create(lv_obj_t *parent)
{
    return ilv_create_view(parent, text_doing_layer_attrs);
}

void ilv_text_doing_set_text(lv_obj_t *obj, const char *text)
{
    obj = lv_obj_get_child(obj, 0);
    struct text_doing_timer_data *p = m_get_user_data(obj);
    do_set_text(obj, p, text);
}

void ilv_text_doing_set_timeout(lv_obj_t *obj, int msecs)
{
    obj = lv_obj_get_child(obj, 0);
    struct text_doing_timer_data *p = m_get_user_data(obj);
    lv_timer_t *timer = ilv_get_user_data(obj, "ilv-timer");
    assert(timer);
    p->end = msecs > 0 ? msecs/timer->period : -1;
}

void ilv_text_doing_set_text_color(lv_obj_t *obj, lv_color_t color)
{
    obj = lv_obj_get_child(obj, 0);
    lv_obj_set_style_text_color(obj, color, 0);
}

void ilv_text_doing_set_text_opa(lv_obj_t *obj, int opa)
{
    obj = lv_obj_get_child(obj, 0);
    lv_obj_set_style_text_opa(obj, opa, 0);
}
