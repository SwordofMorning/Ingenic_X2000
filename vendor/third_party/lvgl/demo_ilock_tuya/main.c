#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"
#include "lvgl_ingenic_support.h"
#include "lv_libjpeg_turbo.h"

void register_linux_signal_hanler(const char *app_name);

int main(int argc, char **argv)
{

    if (argc != 1 && argc != 3) {
        printf("usage: %s <fb_dev> <input_dev>\n", argv[0]);
        return -1;
    }

    const char *fb_path = "/dev/fb1";
    const char *tp_path = "/dev/input/event0";

    if (argc == 3) {
        fb_path = argv[1];
        tp_path = argv[2];
    }

    register_linux_signal_hanler(argv[0]);

    /*Initialize LVGL*/
    lv_init();

    ilv_style_add_normal_view_types();

#ifdef APP_lvgl_use_jpeg_turbo
    lv_libjpeg_turbo_init();
#endif

    int ret;
    ret = lvgl_init_fb_display(fb_path);
    assert(!ret);

    ret = lvgl_init_tp_input(tp_path);
    assert(!ret);

    lv_style_t *font = ilv_load_font(realpath("./res/wqy-microhei.ttc", NULL), 35);
    if (font) {
        lv_obj_add_style(lv_scr_act(), font, 0);
        lv_obj_add_style(lv_layer_top(), font, 0);
    } else {
        fprintf(stderr, "init with no font\n");
    }

    lv_obj_t *root = lv_scr_act();
    lv_obj_set_style_bg_opa(root, 0, 0);

    // ui_desktop_init();
    // 默认打开摄像头的界面
    // 摄像头界面退出时会加载 desktop
    app_camera_start();

    while(1) {
        lvgl_usleep_loop(1000);
    }

    return 0;
}

