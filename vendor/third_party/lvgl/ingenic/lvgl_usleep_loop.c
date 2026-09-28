
#include <unistd.h>
#include <libutils2/boot_time.h>

#include "lvgl/lvgl.h"

void lvgl_usleep_loop(int period_us)
{
    uint64_t old = boot_time_usecs();

    while (1) {
        lv_timer_handler();

        uint64_t now = boot_time_usecs();
        if (now-old < period_us)
            usleep(period_us - (now-old));

        now = boot_time_usecs();
        lv_tick_inc((now-old) / 1000);

        old = now;
    }
}

void lvgl_usleep_once(int period_us)
{
    static uint64_t old = 0;
    if (old == 0)
        old = boot_time_usecs();

    lv_timer_handler();

    uint64_t now = boot_time_usecs();
    if (now-old < period_us)
        usleep(period_us - (now-old));

    now = boot_time_usecs();
    lv_tick_inc((now-old) / 1000);

    old = now;
}

int is_ctrl_key_pressed = 0;

int lv_get_ctrl_key_pressed(void)
{
    return is_ctrl_key_pressed;
}

void lv_set_clipboard_text(const char *text)
{
}
