#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "view/ilv_rotate_img.h"
#include "lvgl_ingenic_support.h"

// 使用libutils2 编译出的 cmd_ratate_test 生成
static int get_fast_version_720x1280_to_720x1280_32bits(int angle)
{
    switch(angle) {
    case 172: case 175: 
        return 0;
    case 174: case 176: 
        return 1;
    case 0: case 2: case 3: case 4: case 5: case 6: case 7: 
    case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: 
    case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23: 
    case 24: case 25: case 26: case 27: case 28: case 29: case 31: case 33: 
    case 35: case 36: case 37: case 38: case 39: case 40: case 41: case 42: 
    case 141: case 143: case 144: case 147: case 148: case 149: case 150: case 151: 
    case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159: 
    case 162: case 163: case 164: case 165: case 166: case 167: case 168: case 169: 
    case 170: case 171: case 173: case 177: case 178: case 179: case 180: case 182: 
    case 183: case 184: case 185: case 186: case 187: case 188: case 189: case 190: 
    case 191: case 192: case 193: case 194: case 195: case 196: case 198: case 199: 
    case 200: case 201: case 202: case 203: case 204: case 205: case 206: case 207: 
    case 208: case 209: case 210: case 211: case 212: case 213: case 214: case 215: 
    case 216: case 218: case 219: case 221: case 223: case 321: case 323: case 324: 
    case 327: case 328: case 329: case 330: case 331: case 332: case 333: case 334: 
    case 335: case 336: case 337: case 338: case 339: case 342: case 343: case 344: 
    case 345: case 346: case 347: case 348: case 349: case 350: case 351: case 352: 
    case 353: case 354: case 355: case 356: case 357: case 360: 
        return 2;
    case 1: case 62: case 68: case 117: case 119: case 122: case 123: 
    case 181: case 259: case 262: case 310: case 311: case 313: case 315: case 358: 
    case 359: 
        return 3;
    default:
        return 4;
    }
}

static void init_rotate_setting(lv_obj_t *obj, void *src_data, int src_w, int src_h)
{
    static lv_img_dsc_t img_dsc;

    img_dsc.header.always_zero = 0;
    img_dsc.header.w = src_w;
    img_dsc.header.h = src_h;
    img_dsc.data_size = src_w*src_h*4;
    img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    img_dsc.data = src_data;

    ilv_rotate_img_set_src(obj, &img_dsc);
    // ilv_rotate_img_set_src(obj, "/usr/data/res/desktop_bg.png");

    ilv_rotate_img_set_src_center(obj, src_w/2, src_h/2);

    ilv_rotate_img_cfg_t cfg = {
        .bilinear = 0,
        .ignore_alpha = 0,
        .get_fast_version = get_fast_version_720x1280_to_720x1280_32bits,
    };
    ilv_rotate_img_set_extra_cfg(obj, &cfg);

    // lv_obj_t *parent = lv_obj_get_parent(obj);
    // lv_obj_update_layout(parent);
    // int dst_w = lv_obj_get_content_width(parent);
    // int dst_h = lv_obj_get_content_height(parent);
    // ilv_rotate_img_set_dst_center(obj, dst_w/2, dst_h/2);
}

static void set_rotate_value(void * indic, int32_t v)
{
    lv_obj_t *obj = indic;
    ilv_rotate_img_set_angle(obj, v);
    printf("v: %d\n", v);
}

static void set_color_bar(uint8_t *src, int s_w, int s_h, int color, int src_bg)
{
    int i, j;
    int bits = 32;

    for (j = 0; j < s_h; j++) {
        for (i = 0; i < s_w; i++) {
            if (j%20 < 10 || j > (s_h-3)) {
                if (bits == 8)
                    src[j*s_w+i] = color&0xff;
                if (bits == 16)
                    ((uint16_t *)src)[j*s_w+i] = color&0xffff;
                if (bits == 32)
                    ((uint32_t *)src)[j*s_w+i] = color;
            }
            else {
                if (bits == 8)
                    src[j*s_w+i] = src_bg&0xff;
                if (bits == 16)
                    ((uint16_t *)src)[j*s_w+i] = src_bg&0xffff;
                if (bits == 32)
                    ((uint32_t *)src)[j*s_w+i] = src_bg;
            }
        }
    }
}

void ilv_rotate_img_example(lv_obj_t *root)
{
    int ret;
    // lvgl_set_fb_show_frame_rate(1);

    int src_w = 720, src_h = 1280;
    int width = 720, height = 1280;

    lv_obj_t *bg = lv_obj_create(root);
    lv_obj_set_size(bg, width, height);
    lv_obj_set_style_bg_color(bg, lv_palette_lighten(LV_PALETTE_BLUE, 2), 0);
    lv_obj_set_style_pad_left(bg, 10, 0);
    lv_obj_set_style_pad_right(bg, 10, 0);
    lv_obj_center(bg);

    lv_obj_t *img_obj = ilv_rotate_img_create(bg);
    lv_obj_center(img_obj);

    uint32_t *src_data;
    lv_img_decoder_dsc_t dsc;
    char *img_path = "/usr/data/res/desktop_bg.png";
    ret = lv_img_decoder_open(&dsc, img_path, lv_palette_lighten(LV_PALETTE_BLUE, 1), 0);
    if (ret == LV_RES_INV) {
        fprintf(stderr, "failed to open:%s, use color bar\n", img_path);
        src_data = malloc(src_w*src_h*4);
        set_color_bar((void *)src_data, src_w, src_h, 0xff00ff00, 0x000000);
    } else {
        src_w = dsc.header.w;
        src_h = dsc.header.h;
        src_data = (void *)dsc.img_data;
    }

    init_rotate_setting(img_obj, src_data, src_w, src_h);

    // ilv_rotate_img_set_angle(img_obj, 90*10);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, set_rotate_value);
    lv_anim_set_values(&a, 0, 360*10);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_time(&a, 30000);
    lv_anim_set_var(&a, img_obj);
    lv_anim_start(&a);
}
