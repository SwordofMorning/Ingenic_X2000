/**
 * @file ilv_rotate_img.h
 *
 */

#ifndef ILV_ROTATE_IMG_H
#define ILV_ROTATE_IMG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/src/lv_conf_internal.h"

#if LV_USE_IMG != 0

#include "lvgl/src/core/lv_obj.h"
#include "lvgl/src/misc/lv_fs.h"
#include "lvgl/src/draw/lv_draw.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * Data of image
 */
typedef struct {
    lv_obj_t obj;
    const void * src; /*Image source: Pointer to an array or a file or a symbol*/
    lv_point_t offset;
    lv_coord_t w;          /*Width of the image (Handled by the library)*/
    lv_coord_t h;          /*Height of the image (Handled by the library)*/
    lv_point_t pivot;     /*rotation center of the image*/
    uint8_t src_type : 2;  /*See: lv_img_src_t*/
    uint8_t cf : 5;        /*Color format from `lv_img_color_format_t`*/
    uint8_t is_rotate:1;
    uint8_t is_rotate_dst_set:1;
    uint8_t is_rotate_src_set:1;
    rotate_dsc_t rotate_dsc;
    lv_coord_t rotate_dst_x;
    lv_coord_t rotate_dst_y;
} ilv_rotate_img_t;

typedef struct {
    uint8_t bilinear:1;                 // use bilinear interpolation or not,
                                        // bilinear is slower but better effect 
    uint8_t ignore_alpha:1;             // ignore alpha or not
                                        // if all alpha is 0xff, can set it to speed up
    int (*get_fast_version)(int angle); // method cb to get the fast version of rotate method,
                                        // when rotate near angle
} ilv_rotate_img_cfg_t;

extern const lv_obj_class_t ilv_rotate_img_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an image object
 * @param parent pointer to an object, it will be the parent of the new image
 * @return pointer to the created image
 */
lv_obj_t * ilv_rotate_img_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the image data to display on the object
 * @param obj       pointer to an image object
 * @param src_img   1) pointer to an ::ilv_rotate_img_dsc_t descriptor (converted by LVGL's image converter) (e.g. &my_img) or
 *                  2) path to an image file (e.g. "S:/dir/img.bin")or
 *                  3) a SYMBOL (e.g. LV_SYMBOL_OK)
 */
void ilv_rotate_img_set_src(lv_obj_t * obj, const void * src);

/**
 * Set the rotation angle of the image.
 * The image will be rotated around the set pivot set by `lv_img_set_pivot()`
 * Note that indexed and alpha only images can't be transformed.
 * @param obj       pointer to an image object
 * @param angle     rotation angle in degree with 0.1 degree resolution (0..3600: clock wise)
 */
void ilv_rotate_img_set_angle(lv_obj_t * obj, int angle);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point.
 * @param obj       pointer to an image object
 * @param x         rotation center x of the image
 * @param y         rotation center y of the image
 */
void ilv_rotate_img_set_src_center(lv_obj_t * obj, int x, int y);

/**
 * Set the rotation center on the dst/parent
 * @param obj       pointer to an image object
 * @param x         rotation center x of the image
 * @param y         rotation center y of the image
 */
void ilv_rotate_img_set_dst_center(lv_obj_t * obj, int x, int y);

/**
 * Set the extra rotate cfg
 * @param obj       pointer to an image
 * @param cfg       the extra rotate cfg
 */
void ilv_rotate_img_set_extra_cfg(lv_obj_t * obj, ilv_rotate_img_cfg_t *cfg);

/**
 * Set the extra rotate cfg bilinear interpolation
 * @param obj       pointer to an image
 * @param enable_bilinear  1: enable bilinear 0: disable bilinear
 */
void ilv_rotate_img_set_bilinear(lv_obj_t *obj, int enable_bilinear);

/**
 * Set an offset for the source of an image so the image will be displayed from the new origin.
 * @param obj       pointer to an image
 * @param x         the new offset along x axis.
 */
void ilv_rotate_img_set_offset_x(lv_obj_t * obj, lv_coord_t x);

/**
 * Set an offset for the source of an image.
 * so the image will be displayed from the new origin.
 * @param obj       pointer to an image
 * @param y         the new offset along y axis.
 */
void ilv_rotate_img_set_offset_y(lv_obj_t * obj, lv_coord_t y);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the source of the image
 * @param obj       pointer to an image object
 * @return          the image source (symbol, file name or ::lv-img_dsc_t for C arrays)
 */
const void * ilv_rotate_img_get_src(lv_obj_t * obj);

/**
 * Get the offset's x attribute of the image object.
 * @param img       pointer to an image
 * @return          offset X value.
 */
lv_coord_t ilv_rotate_img_get_offset_x(lv_obj_t * obj);

/**
 * Get the offset's y attribute of the image object.
 * @param obj       pointer to an image
 * @return          offset Y value.
 */
lv_coord_t ilv_rotate_img_get_offset_y(lv_obj_t * obj);

void ilv_rotate_img_get_src_center(lv_obj_t * obj, int *src_x, int *src_y);

void ilv_rotate_img_get_dst_center(lv_obj_t * obj, int *dst_x, int *dst_y);

int ilv_rotate_img_get_angle(lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_IMG*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*ILV_ROTATE_IMG_H*/
