#include <stdio.h>
#include "lvgl/src/core/lv_disp.h"
#include "lvgl/src/misc/lv_assert.h"
#include "lvgl/src/draw/lv_img_decoder.h"
#include "lvgl/src/misc/lv_fs.h"
#include "lvgl/src/misc/lv_txt.h"
#include "lvgl/src/misc/lv_math.h"
#include "lvgl/src/misc/lv_log.h"

#include "ilv_h264_view.h"
#include "ingenic_h264_view_client.h"

#define MY_CLASS &ilv_h264_class

static int view_id = 0;

static void ilv_h264_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void ilv_h264_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void ilv_h264_event(const lv_obj_class_t * class_p, lv_event_t * e);


/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t ilv_h264_class = {
    .constructor_cb = ilv_h264_constructor,
    .destructor_cb = ilv_h264_destructor,
    .event_cb = ilv_h264_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(ilv_h264_t),
    .base_class = &lv_obj_class
};


static void ilv_h264_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    ilv_h264_t * img = (ilv_h264_t *)obj;

    img->src       = NULL;
    img->src_type  = LV_IMG_SRC_UNKNOWN;
    img->w         = lv_obj_get_width(obj);
    img->h         = lv_obj_get_height(obj);
    img->offset.x  = 0;
    img->offset.y  = 0;

    lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void ilv_h264_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    ilv_h264_t * img = (ilv_h264_t *)obj;

    if (img->name) {
        ingenic_h264_view_client_delete(img->name);
        lv_mem_free(img->name);
        img->name = NULL;
    }

    if(img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_SYMBOL) {
        lv_mem_free((void *)img->src);
        img->src      = NULL;
        img->src_type = LV_IMG_SRC_UNKNOWN;
    }
}


lv_obj_t * ilv_h264_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void ilv_h264_add_hidden_flag(lv_obj_t *obj)
{
    ilv_h264_t *img = (void *)obj;

    ingenic_h264_view_client_stop_display(img->name);
    ingenic_h264_view_client_stop_catch(img->name);

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void ilv_h264_clear_hidden_flag(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

lv_point_t get_obj_screen_coords(lv_obj_t * obj)
{
    lv_point_t abs_pos;
    abs_pos.x = 0;
    abs_pos.y = 0;

    while(obj != NULL) {
        abs_pos.x += lv_obj_get_x(obj);
        abs_pos.y += lv_obj_get_y(obj);
        obj = lv_obj_get_parent(obj);
    }

    return abs_pos;
}

static void reset_size(lv_obj_t *obj)
{
    ilv_h264_t *img = (void *)obj;

    lv_coord_t obj_w = lv_obj_get_width(obj);
    lv_coord_t obj_h = lv_obj_get_height(obj);


    lv_coord_t w = lv_obj_get_style_transform_width(obj, LV_PART_MAIN);
    lv_coord_t h = lv_obj_get_style_transform_height(obj, LV_PART_MAIN);
    lv_area_t coords;
    lv_area_copy(&coords, &obj->coords);
    coords.x1 -= w;
    coords.x2 += w;
    coords.y1 -= h;
    coords.y2 += h;


    if (!obj_w || !obj_h)
        return;

    lv_coord_t border_width = lv_obj_get_style_border_width(obj, LV_PART_MAIN);
    lv_coord_t pleft = lv_obj_get_style_pad_left(obj, LV_PART_MAIN) + border_width;
    lv_coord_t pright = lv_obj_get_style_pad_right(obj, LV_PART_MAIN) + border_width;
    lv_coord_t ptop = lv_obj_get_style_pad_top(obj, LV_PART_MAIN) + border_width;
    lv_coord_t pbottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN) + border_width;

    lv_point_t offset = get_obj_screen_coords(obj);

    int offset_x = offset.x + pleft;
    int offset_y = offset.y + ptop;

    int img_w = obj_w - pleft - pright;
    int img_h = obj_h - ptop - pbottom;


    if (img_w == img->w && img_h == img->h && img->offset.x == offset_x && img->offset.y == offset_y)
        return;


    img->offset.x = coords.x1;
    img->offset.y = coords.y1;

    img->w = obj_w - pleft - pright;
    img->h = obj_h - ptop - pbottom;

    lv_obj_refresh_self_size(obj);

    if (img->src)
        ingenic_h264_view_client_set_area(img->name, img->w, img->h, img->offset.x, img->offset.y);
}

static void draw_img(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    ilv_h264_t * img = (ilv_h264_t *)obj;


    if(code == LV_EVENT_COVER_CHECK) {
        lv_cover_check_info_t * info = lv_event_get_param(e);
        if(info->res == LV_COVER_RES_MASKED) return;
        if(img->src_type == LV_IMG_SRC_UNKNOWN || img->src_type == LV_IMG_SRC_SYMBOL) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        /*With not LV_OPA_COVER images can't cover an area */
        if(lv_obj_get_style_img_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        const lv_area_t * clip_area = lv_event_get_param(e);
        if(_lv_area_is_in(clip_area, &obj->coords, 0) == false) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }
    }
    else if(code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST) {

        lv_coord_t border_width = lv_obj_get_style_border_width(obj, LV_PART_MAIN);
        lv_coord_t pleft = lv_obj_get_style_pad_left(obj, LV_PART_MAIN) + border_width;
        lv_coord_t pright = lv_obj_get_style_pad_right(obj, LV_PART_MAIN) + border_width;
        lv_coord_t ptop = lv_obj_get_style_pad_top(obj, LV_PART_MAIN) + border_width;
        lv_coord_t pbottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN) + border_width;

        lv_res_t res = lv_obj_event_base(MY_CLASS, e);
        if(res != LV_RES_OK) return;


        if(code == LV_EVENT_DRAW_MAIN) {
            if(img->h == 0 || img->w == 0) return;

            lv_area_t img_max_area;
            lv_area_copy(&img_max_area, &obj->coords);

            img_max_area.x2 = img_max_area.x1 + lv_area_get_width(&obj->coords) - 1;
            img_max_area.y2 = img_max_area.y1 + lv_area_get_height(&obj->coords) - 1;

            img_max_area.x1 += pleft;
            img_max_area.y1 += ptop;
            img_max_area.x2 -= pright;
            img_max_area.y2 -= pbottom;
        }

        if (img->name) {
            reset_size(obj);

            ingenic_h264_view_client_start_catch(img->name);
            ingenic_h264_view_client_start_display(img->name);
        }
    }
}



static void ilv_h264_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_event_code_t code = lv_event_get_code(e);



    /*Ancestor events will be called during drawing*/
    if(code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
        /*Call the ancestor's event handler*/
        lv_res_t res = lv_obj_event_base(MY_CLASS, e);
        if(res != LV_RES_OK) return;
    }

    lv_obj_t * obj = lv_event_get_target(e);
    ilv_h264_t * img = (ilv_h264_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        /*Refresh the file name to refresh the symbol text size*/
        if(img->src_type == LV_IMG_SRC_SYMBOL) {
            ilv_h264_set_src(obj, img->src);
        }
        else {
            /*With transformation it might change*/
            lv_obj_refresh_ext_draw_size(obj);
        }
    }

    else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {

    }
    else if(code == LV_EVENT_HIT_TEST) {
        lv_hit_test_info_t * info = lv_event_get_param(e);
        lv_area_t a;
        lv_obj_get_click_area(obj, &a);
        info->res = _lv_area_is_point_on(&a, info->point, 0);
    }
    else if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t * p = lv_event_get_param(e);
        p->x = img->w;
        p->y = img->h;
    }
    else if(code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
        draw_img(e);
    }
}

void ilv_h264_set_src(lv_obj_t *obj, char *src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_obj_invalidate(obj);

    ilv_h264_t *img  = (void *)obj;

    lv_img_src_t src_type = lv_img_src_get_type(src);

    /*If the new source type is unknown free the memories of the old source*/
    if(src_type == LV_IMG_SRC_UNKNOWN) {
        LV_LOG_WARN("lv_img_set_src: unknown image type");
        if(img->src_type == LV_IMG_SRC_SYMBOL || img->src_type == LV_IMG_SRC_FILE) {
            lv_mem_free((void *)img->src);
        }
        img->src      = NULL;
        img->src_type = LV_IMG_SRC_UNKNOWN;
        return;
    }

    /*Save the source*/
    if(src_type == LV_IMG_SRC_VARIABLE) {
        /*If memory was allocated because of the previous `src_type` then free it*/
        if(img->src_type == LV_IMG_SRC_FILE) {
            lv_mem_free((void *)img->src);
        }
        img->src = src;
    }
    else if(src_type == LV_IMG_SRC_FILE) {
        /*If the new and the old src are the same then it was only a refresh.*/
        if(img->src != src) {
            const void * old_src = NULL;
            /*If memory was allocated because of the previous `src_type` then save its pointer and free after allocation.
             *It's important to allocate first to be sure the new data will be on a new address.
             *Else `img_cache` wouldn't see the change in source.*/
            if (img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_SYMBOL)
                old_src = img->src;

            char * new_str = lv_mem_alloc(strlen(src) + 1);
            LV_ASSERT_MALLOC(new_str);
            if (new_str == NULL)
                return;

            strcpy(new_str, src);
            img->src = new_str;

            if (old_src)
                lv_mem_free((void *)old_src);

            if (img->name) {
                ingenic_h264_view_client_delete(img->name);
                lv_mem_free(img->name);
                img->name = NULL;
            }

            char name[64] = {0};
            sprintf(name, "video_view_%d\n", view_id++);
            img->name = lv_mem_alloc(strlen(name) + 1);
            strcpy(img->name, name);

            int ret;
            ret = ingenic_h264_view_client_create(img->name, img->src);
            if(ret < 0) {
                fprintf(stderr, "faield to set src\n");
                lv_mem_free(img->src);
                lv_mem_free(img->name);
                img->src = NULL;
                img->name = NULL;
                return;
            }
        }
    }

    lv_obj_update_layout(lv_obj_get_parent(obj));

    img->src_type = src_type;

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);

    return;
}


