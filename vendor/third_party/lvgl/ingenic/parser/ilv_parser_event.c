#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "parser/ilv_parser.h"
#include <unistd.h>
#include <stdio.h>

static void m_close_event(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *last_user = lv_event_get_user_data(e);

    lv_obj_clear_flag(last_user, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del(obj);
}

void ilv_parser_shortclick_event(lv_event_t *e)
{
    const char *src = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *obj = target;
    ilv_parser_t *parser = NULL;
    ilv_style_t *styles = NULL;

    if (!access(src, F_OK)) {
        parser = ilv_parse_file(src);
        if (parser)
            styles = ilv_parser_get_default_view_style(parser);
    }

    if (!styles) {
        while (1) {
            ilv_parser_t *parser = ilv_parser_get_from_view(obj);
            if (parser) {
                styles = ilv_parser_get_view_style_by_name(parser, src);
                break;
            }
            lv_obj_t *parent = lv_obj_get_parent(obj);
            // lv_scr_act() lv_layer_top() lv_layer_sys() 's parent is NULL
            if (lv_obj_get_parent(parent) == NULL)
                break;
            obj = parent;
        }
    }

    if (!styles) {
        fprintf(stderr, "ilv_parser: no src:%s found for view\n", src);
        return;
    }

    lv_obj_t *last_user = ilv_get_root_user_obj(target);
    lv_obj_t *root = lv_obj_get_parent(last_user);

    obj = ilv_create_view(root, styles);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(obj, m_close_event, LV_EVENT_SHORT_CLICKED, last_user);
    if (parser)
        ilv_parser_add_to_view(obj, parser);

    lv_obj_add_flag(last_user, LV_OBJ_FLAG_HIDDEN);
}
