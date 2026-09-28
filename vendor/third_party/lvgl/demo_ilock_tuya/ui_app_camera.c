#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <libmedia/utils/fifo_utils.h>
#include <libmedia/utils/pkt_parse.h>


#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "ui_desktop.h"

#define message_fifo_path "/tmp/ui_message_fifo"

static int app_is_show = 0;
static lv_obj_t *camera_root;

static pthread_t message_thread;
static lv_obj_t *message_view;

 static int is_back = 0;

static void *get_message(void *data)
{
    int fifo_fd = open(message_fifo_path, O_RDONLY | O_NONBLOCK);
    if (fifo_fd < 0) {
        fprintf(stderr, "fifo: failed to open fifo for %s\n", message_fifo_path);
        return NULL;
    }

    char buffer[1024] = {0};
    int ret;
    int display_time = 0;
    is_back = 0;

    while(!is_back) {
        memset(buffer, 0, sizeof(buffer));
        ret = read(fifo_fd, buffer, sizeof(buffer));
        if (!ret) {
            usleep(1000*10);
            if (display_time >= 0)
                display_time -= 10 * 1000;
            else
                lv_obj_add_flag(message_view, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_label_set_text(message_view, buffer);
        lv_obj_clear_flag(message_view, LV_OBJ_FLAG_HIDDEN);
        display_time = 1 * 1000 * 1000;
    }

    close(fifo_fd);
    unlink(message_fifo_path);
    return NULL;
}

void init_get_message(void)
{
    int ret;
    if (access(message_fifo_path, F_OK)) {
        ret = mkfifo(message_fifo_path, 0777);
        if (ret < 0) {
            fprintf(stderr, "fifo: failed to create: %s\n", message_fifo_path);
            return;
        }
    }

    message_view = lv_label_create(camera_root);

    lv_obj_set_style_border_side(message_view, LV_BORDER_SIDE_FULL, 5);
    lv_obj_set_style_radius(message_view, 10, 10);

    lv_obj_align(message_view, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(message_view, 0xbb, 0);
    lv_obj_add_flag(message_view, LV_OBJ_FLAG_HIDDEN);


    pthread_create(&message_thread, NULL, get_message, NULL);
}

static void back_to_desktop_event(lv_event_t *e)
{
    is_back = 1;

    pthread_join(message_thread, NULL);

    lv_obj_del(camera_root);

    lv_obj_t *desktop = ui_get_desktop();
    if (!desktop)
        desktop = ui_desktop_init();
    lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    app_is_show = 0;
}

static void toggle_camera_ui_event(lv_event_t *e)
{
    lv_obj_t *camera_layer = lv_obj_get_child(camera_root, 0);

    if (app_is_show)
        lv_obj_add_flag(camera_layer, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(camera_layer, LV_OBJ_FLAG_HIDDEN);

    app_is_show = !app_is_show;
}

static ilv_style_t common_layer_attrs[] = {
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_BOTTOM},
    {LV_STYLE_SHADOW_WIDTH, 0},
    {LV_STYLE_BG_OPA, 0},

    {0, 0},
};

ilv_style_t app_camera_layer_attrs[] = {
    {LV_style_view, LV_VIEW("obj", "camera_root")},
    {LV_STYLE_WIDTH, LV_PCT(100)},
    {LV_STYLE_HEIGHT, LV_PCT(100)},
    {LV_STYLE_BG_OPA, 0x0},
    {LV_STYLE_BORDER_SIDE, LV_BORDER_SIDE_NONE},
    {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, toggle_camera_ui_event)},
    {LV_style_layout_start},

        {LV_style_view, LV_VIEW("flex_col", "camera_layer")},
        {LV_style_sets, (long)common_layer_attrs},
        {LV_STYLE_MIN_HEIGHT, LV_PCT(10)},
        {LV_STYLE_ALIGN, LV_ALIGN_BOTTOM_LEFT},
        {LV_STYLE_BG_OPA, 0xbb},
        {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},
        {LV_style_layout_start},
            {LV_style_view, LV_VIEW("flex_row", "back")},
            {LV_STYLE_WIDTH, LV_SIZE_CONTENT},
            {LV_STYLE_HEIGHT, LV_SIZE_CONTENT},
            {LV_STYLE_ALIGN, LV_ALIGN_CENTER},
            {LV_STYLE_BG_OPA, 0},
            {LV_STYLE_BORDER_SIDE, 0},
            {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},
            {LV_style_layout_start},

                {LV_style_view, LV_VIEW("label", "label")},
                {LV_style_label_text, LV_STR("返回桌面")},
                {LV_style_add_event, LV_EVENT(LV_EVENT_SHORT_CLICKED, back_to_desktop_event)},

            {LV_style_layout_end},
        {LV_style_layout_end},
    {LV_style_layout_end},

    {0, 0},
};

void app_camera_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    camera_root = ilv_create_view(lv_scr_act(), app_camera_layer_attrs);

    app_is_show = 1;

    init_get_message();

    toggle_camera_ui_event(NULL);

}

void app_camera_start(void)
{
    app_camera_click_event(NULL);
    toggle_camera_ui_event(NULL);
}
