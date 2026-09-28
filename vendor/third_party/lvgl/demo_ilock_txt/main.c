#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"
#include "lvgl_ingenic_support.h"
#include "lv_libjpeg_turbo.h"
#include "parser/ilv_config.h"
#include "parser/ilv_parser.h"

void register_linux_signal_hanler(const char *app_name);

int main(int argc, char **argv)
{
    const char *fb_path = "/dev/fb0";
    const char *tp_path = "/dev/input/event0";

    register_linux_signal_hanler(argv[0]);

    /*Initialize LVGL*/
    lv_init();

#ifdef APP_lvgl_use_jpeg_turbo
    lv_libjpeg_turbo_init();
#endif

    int ret;
    ret = lvgl_init_fb_display(fb_path);
    assert(!ret);

    ret = lvgl_init_tp_input(tp_path);
    // assert(!ret);

    lvgl_set_fb_show_frame_rate(1);

    ilv_style_add_normal_view_types();

    ilv_config_init();

    lv_obj_t *root = lv_scr_act();
    lv_obj_set_style_bg_opa(root, 0, 0);

    lv_obj_t *desktop = ui_desktop_init();
    if (!desktop)
        return 0;

    while(1) {
        lvgl_usleep_once(1000);
    }

    return 0;
}

