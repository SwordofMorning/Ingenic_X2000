/**
 * File:   main_loop_linux.c
 * Author: AWTK Develop Team
 * Brief:  linux implemented main_loop interface
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2018-09-09 li xianjing <xianjimli@hotmail.com> created
 *
 */

#include "base/idle.h"
#include "base/timer.h"
#include "base/font_manager.h"
#include "base/window_manager.h"
#include "main_loop/main_loop_simple.h"
#include "native_window/native_window_raw.h"

#include "input_thread.h"
#include "lcd_linux_fb.h"
#include "main_loop_linux.h"
#include <linux/input.h>

#ifdef WITH_LINUX_EGL
#define LCD_T lcd_egl_context_t
#else
#define LCD_T lcd_t
#endif

#ifndef FB_DEVICE_FILENAME
#define FB_DEVICE_FILENAME "/dev/fb0"
#endif /*FB_DEVICE_FILENAME*/

#ifndef INPUT_DEVICE_FILENAME
#define INPUT_DEVICE_FILENAME "/dev/input"
#endif /*INPUT_DEVICE_FILENAME*/

static slist_t s_device_threads_list;

static ret_t main_loop_linux_destroy(main_loop_t *l)
{
    main_loop_simple_t *loop = (main_loop_simple_t *)l;

    main_loop_simple_reset(loop);

    native_window_raw_deinit();

    return RET_OK;
}

ret_t input_dispatch_to_main_loop(void *ctx, const event_queue_req_t *evt, const char *msg)
{
    main_loop_simple_t *l = (main_loop_simple_t *)ctx;
    event_queue_req_t event = *evt;
    event_queue_req_t *e = &event;

    if (l != NULL && l->base.queue_event != NULL) {
        switch (e->event.type)
        {
        case EVT_POINTER_DOWN:
        {
            l->pressed = TRUE;
            e->event.size = sizeof(e->pointer_event);
            break;
        }
        case EVT_POINTER_UP:
        {
            l->pressed = FALSE;
            e->event.size = sizeof(e->pointer_event);
            break;
        }
        default:
            break;
        }

        main_loop_queue_event(&(l->base), e);
        // input_dispatch_print(ctx, e, msg);
    }
    else {
        return RET_BAD_PARAMS;
    }
    return RET_OK;
}

static void on_app_exit(void)
{
    slist_deinit(&s_device_threads_list);
    input_thread_global_deinit();
}

static ret_t intput_thread_on(main_loop_simple_t *loop,const char * path)
{
    tk_thread_t *thread = NULL;
    ret_t ret = RET_OK;

    thread = input_thread_run(path, input_dispatch_to_main_loop, loop, loop->w, loop->h);

    if (thread != NULL) {
        ret = slist_append(&s_device_threads_list, thread);
    }

    return ret;
}

main_loop_t *main_loop_init(int w, int h)
{
    main_loop_simple_t *loop = NULL;
    LCD_T *lcd = NULL;

    lcd = lcd_linux_fb_create(FB_DEVICE_FILENAME);
    return_value_if_fail(lcd != NULL, NULL);

    native_window_raw_init(lcd);

    loop = main_loop_simple_init(lcd->w, lcd->h, NULL, NULL);
    loop->base.destroy = main_loop_linux_destroy;

    input_thread_global_init();

    slist_init(&s_device_threads_list, (tk_destroy_t)tk_thread_destroy, NULL);
    intput_thread_on(loop, INPUT_DEVICE_FILENAME);

    atexit(on_app_exit);

    return (main_loop_t *)loop;
}
