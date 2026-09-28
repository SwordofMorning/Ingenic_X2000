#ifndef __ilv_h264_VIEW_H__
#define __ilv_h264_VIEW_H__

#include "lvgl/src/lv_conf_internal.h"

#include "lvgl/src/core/lv_obj.h"
#include "lvgl/src/misc/lv_fs.h"
#include "lvgl/src/draw/lv_draw.h"


typedef struct {
    lv_obj_t obj;
    void * src; /*Image source: Pointer to an array or a file or a symbol*/
    void *name;
    lv_point_t offset;
    lv_coord_t w;                       /*Width of the image (Handled by the library)*/
    lv_coord_t h;                       /*Height of the image (Handled by the library)*/
    uint8_t src_type : 2;               /*See: lv_img_src_t*/
} ilv_h264_t;


lv_obj_t * ilv_h264_create(lv_obj_t * parent);
void ilv_h264_set_src(lv_obj_t *obj, char *src);
void ilv_h264_add_hidden_flag(lv_obj_t *obj);
void ilv_h264_clear_hidden_flag(lv_obj_t *obj);


#endif