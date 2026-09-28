#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"

#include "lvgl_ingenic_support.h"

int main(int argc, char *argv[])
{
    const char *fb_path = "/dev/fb0";
    const char *tp_path = "/dev/input/event0";

    lv_init();

    int ret;
    ret = lvgl_init_fb_display(fb_path);
    assert(!ret);

    ret = lvgl_init_tp_input(tp_path);
    // assert(!ret);

    lv_demo_widgets();

    lvgl_usleep_loop(10*1000);

    return 0;
}