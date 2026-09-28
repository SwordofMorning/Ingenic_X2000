#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>

#include "lvgl/lvgl.h"

#include "lv_conf.h"

static lv_indev_t *tp_indev;

struct tp_input_reader {
    int tp_fd;
    struct pollfd poll_fd;
    int x;
    int y;
    int is_press;
};

static int read_input_event(struct tp_input_reader *reader)
{
    struct input_event event;
    int ret;

    int x = -1;
    int y = -1;

    ret = poll(&reader->poll_fd, 1, 0);
    if (ret <= 0)
        return 0;

    while(1) {
        read(reader->poll_fd.fd, &event, sizeof(struct input_event));

        if (event.type == EV_ABS) {
            if (event.code == ABS_MT_POSITION_X)
                x = event.value;
            if (event.code == ABS_MT_POSITION_Y)
                y = event.value;
        }

        if (event.type == EV_SYN) {
            if(event.code == SYN_MT_REPORT) {
                if (x >= 0 && y >= 0) {
                    reader->x = x;
                    reader->y = y;

                    reader->is_press = 1;
                } else {
                    reader->is_press = 0;
                }
            }

            if (event.code == SYN_REPORT)
                break;
        }
    }

    return 1;
}

static struct tp_input_reader *tp_input_reader_open(const char *input_device)
{
    struct tp_input_reader *reader = malloc(sizeof(*reader));
    assert(reader);

    int tp_fd = open(input_device, O_RDONLY);
    if (tp_fd < 0) {
        fprintf(stderr, "tp_input_reader: failed to open input device %s\n", input_device);
        return NULL;
    }

    reader->poll_fd.fd = tp_fd;
    reader->poll_fd.events = POLLIN;

    reader->tp_fd = tp_fd;
    reader->is_press = 0;
    reader->x = 0;
    reader->y = 0;

    return reader;
}

static struct tp_input_reader *tp_reader;

static void tp_read_event(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void) indev_drv;      /*Unused*/

    while (read_input_event(tp_reader));

    // printf("%d,%d,%d\n", tp_reader->x, tp_reader->y, tp_reader->is_press);

    /*Store the collected data*/
    data->point.x = tp_reader->x;
    data->point.y = tp_reader->y;
    data->state = tp_reader->is_press ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

int lvgl_init_tp_input(const char *tp_path)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = tp_read_event;

    tp_reader = tp_input_reader_open(tp_path);
    if (!tp_reader)
        return -1;

    tp_indev = lv_indev_drv_register(&indev_drv);
    return 0;
}