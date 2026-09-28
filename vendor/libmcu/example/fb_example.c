#include <stdio.h>
#include <string.h>

#include <soc/base.h>
#include <delay.h>
#include <driver/systick.h>
#include <driver/fb.h>

void fb_example(void)
{
    struct fb_mem_info info;
    struct fbdev_data *fbdev = fb_open(0); /* 获得fb0设备 */

    fb_enable(fbdev);
    fb_get_info(fbdev, &info);

    int i, j, time = 3;

    while (time--) { /* 按红绿蓝顺序每秒钟切换一个颜色 */
        unsigned int *p = info.fb_mem;

        for (j = 0; j < info.yres; j++) {
            for (i = 0; i < info.xres; i++) {
                *p++ = 0xffff0000;
            }
        }

        fb_pan_display(fbdev, 0);
        mdelay(1*1000);

        p = info.fb_mem;
        for (j = 0; j < info.yres; j++) {
            for (i = 0; i < info.xres; i++) {
                *p++ = 0xff00ff00;
            }
        }

        fb_pan_display(fbdev, 0);
        mdelay(1*1000);

        p = info.fb_mem;
        for (j = 0; j < info.yres; j++) {
            for (i = 0; i < info.xres; i++) {
                *p++ = 0xff0000ff;
            }
        }

        fb_pan_display(fbdev, 0);
        mdelay(1*1000);
    }

    fb_disable(fbdev);
    printf("fb_example done\n");
}
