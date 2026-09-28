#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"

#include "lvgl_ingenic_support.h"

#include <libutils2/message_queue.h>

#include <libutils2/boot_time.h>

#include <sys/types.h>
#include <dirent.h>

#include <libmedia/media_demuxing.h>
#include <libmedia/utils/fifo_utils.h>
#include <libmedia/utils/pkt_parse.h>

#include <linux/keyboard.h>
#include <linux/input.h>
#include "ui_desktop.h"

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

#ifdef APP_LVGL_VIDEO_PATH
#define VIDEO_DIR_NAME APP_LVGL_VIDEO_PATH
#else
#define VIDEO_DIR_NAME "/tmp/sdcard/mmcblk0p1/"
#endif
#define VIDOE_PLAYE_NAME "/usr/bin/video_player"

struct video_user_data{
    char name[512];
    uint64_t duration;
};

static lv_obj_t *main_view;
static lv_obj_t *display_view;
static lv_obj_t *display_view_parent;
static lv_obj_t *slider;
static lv_obj_t *slider_label_start;
static lv_obj_t *slider_label_end;

static pid_t play_pid;
static struct fifo *fifo;

static int thread_stop;

static pthread_t slider_thread;

static inline void convert_sec_to_HMS(const uint32_t sec, uint32_t *h, uint32_t* m, uint32_t *s)
{
    *h = sec / 3600;
    *m = (sec % 3600) / 60;
    *s = (sec % 3600) % 60;
}

static void set_playback_time(lv_obj_t *obj, uint32_t sec)
{
    uint32_t h, m, s;
    convert_sec_to_HMS(sec, &h, &m, &s);
    if (h>0)
        lv_label_set_text_fmt(obj, "#0000ff %02d:%02d:%02d#", h, m, s);
    else
        lv_label_set_text_fmt(obj, "#0000ff %02d:%02d#", m, s);
}

static void set_slider_time(uint64_t time)
{
    uint32_t sec = time / 1000 / 1000;
    int value = lv_slider_get_value(slider);
    if (abs(sec - value) == 0)
        return;

    if (!lv_slider_is_dragged(slider)) {
        lv_slider_set_value(slider, sec, LV_ANIM_OFF);
        set_playback_time(slider_label_start, sec);
    }
}

static uint64_t get_video_duration(char *input_file)
{
    uint64_t dts;
    struct media_demuxing_param demuxing_param;
    ffmpeg_demuxing_init_param(&demuxing_param);

    demuxing_param.input_file = input_file;

    struct media_demuxing *demuxing = demuxing_open(&demuxing_param);

    dts = demuxing_get_duration(demuxing);
    demuxing_close(demuxing);

    return dts;
}

static void *get_video_current_time(void *data)
{
    char buf[128];
    int64_t time = -1;
    thread_stop = 0;
    while (1) {
        time = -1;
        if (thread_stop)
            break;

        fifo_read_pkt(fifo, buf, sizeof(buf), 10);
        pkt_parse_int64(buf, "current_time", &time, 10);

        if (time < 0)
            continue;

        set_slider_time(time);
        usleep(60*1000);
    }

    return NULL;
}

static struct _lv_event_dsc_t *move_event;
static void video_display_gesture_cb(lv_event_t *e);
static void display_view_clicked_cb(lv_event_t *e);
static void display_view_parent_clicked_cb(lv_event_t *e);

static void display_view_parent_clicked_cb(lv_event_t *e)
{
    lv_obj_clear_flag(display_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(display_view_parent, LV_OBJ_FLAG_HIDDEN);
}

static void display_view_clicked_cb(lv_event_t *e)
{
    lv_obj_clear_flag(display_view_parent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(display_view, LV_OBJ_FLAG_HIDDEN);
}

static void video_display_gesture_cb(lv_event_t *e)
{
    lv_indev_t *tp_input = lv_indev_get_act();
    lv_point_t new_point;
    static lv_point_t old_point;
    lv_indev_get_point(tp_input, &new_point);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED && !move_event) {
        move_event = lv_obj_add_event_cb(display_view, video_display_gesture_cb, LV_EVENT_PRESSING, NULL);
        old_point = new_point;
        return;
    }

    if (code == LV_EVENT_RELEASED && move_event) {
        lv_obj_remove_event_dsc(display_view, move_event);
        move_event = NULL;
        return;
    }

    if (code != LV_EVENT_PRESSING)
        return;

    int mul = lv_slider_get_max_value(slider) * 100 / 600;
    // int slilder_value = lv_slider_get_value(slider);

    int changed = abs(old_point.x - new_point.x) * mul / 100;
    if (changed <= 2) {
        // old_point = new_point;
        return;
    }

    changed = changed / 2;

    if (old_point.x > new_point.x) {
        fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=%d\nto_time=0\n", KEY_LEFT, changed * 1000 * 1000);
    } else {
        fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=%d\nto_time=0\n", KEY_RIGHT, changed * 1000 * 1000);
    }
    old_point = new_point;

    return;
}

static void video_list_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    struct video_user_data *user_data = lv_obj_get_user_data(btn);

    uint32_t sec = user_data->duration / 1000 / 1000;
    set_playback_time(slider_label_end, sec);

    lv_obj_add_flag(main_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(display_view, LV_OBJ_FLAG_HIDDEN);
    lv_slider_set_range(slider, 0, sec);

    lv_obj_add_event_cb(display_view, video_display_gesture_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(display_view, video_display_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(display_view, display_view_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(display_view_parent, display_view_parent_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_clear_flag(display_view, LV_OBJ_FLAG_SCROLLABLE);

    char *args[] = {VIDOE_PLAYE_NAME, user_data->name, NULL};
    fifo = create_process_open_fifo(args, &play_pid);
    if (!fifo)
        printf("create process open fifo err\n");

    pthread_create(&slider_thread, NULL, get_video_current_time, NULL);
}

static void display_view_btn_event(lv_event_t *e)
{
    int key = (int)lv_event_get_user_data(e);

    fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=0\nto_time=0\n", key);

    if (key == KEY_PAUSE) {
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        if (strcmp("resume", lv_label_get_text(label)) == 0)
            lv_label_set_text(label, "pause");
        else
            lv_label_set_text(label, "resume");

        lv_obj_center(label);
    }

    if (key == KEY_END) {
        thread_stop = 1;
        pthread_join(slider_thread, NULL);

        lv_obj_add_flag(display_view_parent, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(display_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(main_view, LV_OBJ_FLAG_HIDDEN);

        close_fifo_wait_process(fifo, play_pid);
        fifo_delete_pid(play_pid);
        fifo = NULL;

        int ret = lv_obj_remove_event_cb(display_view, video_display_gesture_cb);
        ret |= lv_obj_remove_event_cb(display_view, display_view_clicked_cb) << 1;
        ret |= lv_obj_remove_event_cb(display_view_parent, display_view_parent_clicked_cb) << 2;
    }

}

lv_obj_t *lv_create_one_btn(lv_obj_t *parent, char *btn_name, lv_coord_t width, lv_coord_t height, void *func, void *param)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);

    // lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);

    lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING);

    lv_obj_set_scrollbar_mode(btn, 0);

    if (func)
        lv_obj_add_event_cb(btn, func, LV_EVENT_CLICKED, param);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_recolor(label, 1);
    lv_label_set_text_fmt(label, "#ffffff %s#", btn_name);
    lv_obj_center(label);

    return btn;
}

static void slider_event(lv_event_t *e)
{
    int value = lv_slider_get_value(slider);
    set_playback_time(slider_label_start, value);
    fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=0\nto_time=%d\n", KEY_SETUP, value);
}


static void change_to_ui_view(lv_event_t *e)
{
    system("uvc_preview_on.sh");

    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_clear_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(main_view);
}

static lv_obj_t *ingenic_player_create_main_view(void)
{
    lv_obj_t *list1 = lv_list_create(lv_scr_act());
    lv_obj_t * btn;
    lv_obj_set_size(list1, lv_pct(100), lv_pct(100));

    btn = lv_list_add_text(list1, "视频列表");
    lv_obj_set_size(btn, lv_pct(100), lv_pct(5));


    DIR* dp = opendir(VIDEO_DIR_NAME);
    if(!dp)
        printf("failed to open dir\n");

    struct dirent *dirp;
    int size;
    // lv_obj_t *btn;

    while (dp) {
        dirp = readdir(dp);
        if (!dirp)
            break;

        size = strlen(dirp->d_name);
        if (size < 5)
            continue;
        if (strcmp(dirp->d_name + (size - 4), ".mp4") != 0 && strcmp(dirp->d_name + (size - 4), ".mp3") != 0)
            continue;

        btn = lv_list_add_btn(list1, LV_SYMBOL_VIDEO, dirp->d_name);
        lv_obj_set_style_pad_top(btn, 30, 0);
        lv_obj_set_style_pad_bottom(btn, 30, 0);

        struct video_user_data *btn_userdata = malloc(sizeof(*btn_userdata));
        snprintf(btn_userdata->name, sizeof(btn_userdata->name), "%s%s", VIDEO_DIR_NAME, dirp->d_name);
        btn_userdata->duration = get_video_duration(btn_userdata->name);

        lv_obj_set_user_data(btn, btn_userdata);
        lv_obj_add_event_cb(btn, video_list_btn_cb, LV_EVENT_CLICKED, NULL);
    }

    btn = lv_list_add_text(list1, "");
    lv_obj_set_size(btn, lv_pct(100), lv_pct(5));

    btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, "关闭");
    lv_obj_add_event_cb(btn, change_to_ui_view, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_top(btn, 30, 0);
    lv_obj_set_style_pad_bottom(btn, 30, 0);

    closedir(dp);

    return list1;
}

static void ingenic_player_create_disp_view(void)
{
    if (display_view)
        return;

    display_view_parent = lv_obj_create(lv_scr_act());
    lv_obj_set_size(display_view_parent, lv_pct(100), lv_pct(100));
    lv_obj_align(display_view_parent, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(display_view_parent, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_bg_opa(display_view_parent, LV_OPA_0, 0);

    display_view = lv_obj_create(lv_scr_act());
    lv_obj_set_size(display_view, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(display_view, LV_OPA_0, 0);

    lv_obj_t *btn1 = lv_create_one_btn(display_view, ">>", 125, 50, display_view_btn_event, (void *)KEY_RIGHT);
    lv_obj_t *btn2 = lv_create_one_btn(display_view, "<<", 125, 50, display_view_btn_event, (void *)KEY_LEFT);
    lv_obj_t *btn3 = lv_create_one_btn(display_view, "pause", 125, 50, display_view_btn_event, (void *)KEY_PAUSE);
    lv_obj_t *btn4 = lv_create_one_btn(display_view, "return", 125, 50, display_view_btn_event, (void *)KEY_END);

    lv_obj_align(btn1, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_align(btn2, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(btn3, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_align(btn4, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    slider = lv_slider_create(display_view);
    lv_obj_set_size(slider, lv_pct(100), lv_pct(2));
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, lv_pct(85));
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLOATING);
    // lv_obj_clear_flag(slider, LV_OBJ_FLAG_SCROLLABLE);

    lv_slider_set_range(slider, 0, 100);
    lv_obj_add_event_cb(slider, slider_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(slider, slider_event, LV_EVENT_VALUE_CHANGED, NULL);

    /*Create a label below the slider*/
    slider_label_start = lv_label_create(display_view);
    lv_label_set_recolor(slider_label_start, 1);
    lv_label_set_text(slider_label_start, "#0000ff 00:00#");
    lv_obj_align_to(slider_label_start, slider, LV_ALIGN_TOP_LEFT, 0, -40);
    lv_obj_add_flag(slider_label_start, LV_OBJ_FLAG_FLOATING);
    // lv_obj_clear_flag(slider_label_start, LV_OBJ_FLAG_SCROLLABLE);

    slider_label_end = lv_label_create(display_view);
    lv_label_set_recolor(slider_label_end, 1);
    lv_label_set_text(slider_label_end, "#0000ff 00:00#");
    lv_obj_align_to(slider_label_end, slider, LV_ALIGN_TOP_RIGHT, -15, -40);
    lv_obj_add_flag(slider_label_end, LV_OBJ_FLAG_FLOATING);
    // lv_obj_clear_flag(slider_label_end, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_flag(display_view_parent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(display_view, LV_OBJ_FLAG_HIDDEN);
}

void app_player_click_event(lv_event_t *e)
{
    lv_obj_t *desktop = ui_get_desktop();
    if (desktop)
        lv_obj_add_flag(desktop, LV_OBJ_FLAG_HIDDEN);

    main_view = ingenic_player_create_main_view();
    ingenic_player_create_disp_view();

    system("uvc_preview_off.sh");
}
