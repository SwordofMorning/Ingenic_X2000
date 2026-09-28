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

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

#ifdef APP_LVGL_VIDEO_PATH
#define VIDEO_DIR_NAME APP_LVGL_VIDEO_PATH
#else
#define VIDEO_DIR_NAME "/usr/data/"
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

#define SLIDER_MAXIMUM     200
// 单位 -- us
static int slider_unit;

static pid_t play_pid;
static struct fifo *fifo;

static int thread_stop;

static pthread_t slider_thread;

static inline void time_to_HMSMS(uint64_t time, uint32_t *h, uint32_t* m, uint32_t *s, uint32_t *ms)
{
    time /= 1000;     //us -> ms
    *ms = time % 1000;
    time /= 1000;     //ms -> s
    *s = time % 60;
    time /= 60;       //s -> min
    *m = time % 60;
    time /= 60;       //min -> h
    *h = time;
}

static void set_playback_time(lv_obj_t *obj, uint64_t time)
{
    uint32_t h, m, s, ms;
    time_to_HMSMS(time, &h, &m, &s, &ms);
    if (h > 0)
        lv_label_set_text_fmt(obj, "#0000ff %02d:%02d:%02d#", h, m, s);
    else
        lv_label_set_text_fmt(obj, "#0000ff %02d:%02d.%03d#", m, s, ms);
}

static void set_slider_time(uint64_t time)
{
    int value = lv_slider_get_value(slider);

    printf("value = %d / %d, time(us) = %lld\n", value, SLIDER_MAXIMUM, time);

    if (!lv_slider_is_dragged(slider)) {
        lv_slider_set_value(slider, time / slider_unit, LV_ANIM_OFF);
        set_playback_time(slider_label_start, time);
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
        printf("add event\n");
        old_point = new_point;
        return;
    }

    if (code == LV_EVENT_RELEASED && move_event) {
        lv_obj_remove_event_dsc(display_view, move_event);
        move_event = NULL;
        printf("remove event\n");
        return;
    }

    if (code != LV_EVENT_PRESSING)
        return;

    // 滑动整个屏 移动1min
    lv_coord_t slider_width = lv_obj_get_width(slider);
    int unit_width = slider_width / 60 > 0 ? slider_width / 60 : 1;

    int second = abs(old_point.x - new_point.x) / unit_width;
    if (second <= 2)
        return;

    printf("skip second = %d\n", second);

    if (old_point.x > new_point.x)
        fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=%d\nto_time=0\n", KEY_LEFT, second * 1000 * 1000);
    else
        fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=%d\nto_time=0\n", KEY_RIGHT, second * 1000 * 1000);

    old_point = new_point;

    return;
}

static void video_list_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    struct video_user_data *user_data = lv_obj_get_user_data(btn);

    slider_unit = user_data->duration / SLIDER_MAXIMUM;
    if (slider_unit == 0)
        slider_unit = 1;

    set_playback_time(slider_label_end, user_data->duration);

    lv_obj_add_flag(main_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(display_view, LV_OBJ_FLAG_HIDDEN);
    lv_slider_set_range(slider, 0, SLIDER_MAXIMUM);

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
        printf("ret = %d\n", ret);
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
    uint64_t time_us = lv_slider_get_value(slider) * slider_unit;
    set_playback_time(slider_label_start, time_us);
    fifo_write_pkt2(fifo, 100, "key_type=%d\nseek_time=0\nto_time=%llu\n", KEY_SETUP, time_us);
    // 防止滑动太快产生过多的时间跳转请求，video_player端需要依次处理(解码)，而无法及时响应松手后真正想跳转到的时间。
    usleep(50 * 1000);
}

static lv_obj_t *ingenic_player_create_main_view(void)
{
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_list_add_text(list, "video list");

    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    DIR* dp = opendir(VIDEO_DIR_NAME);
    if(!dp)
        printf("failed to open dir\n");

    struct dirent *dirp;
    int size;
    lv_obj_t *btn;

    while(1) {
        dirp = readdir(dp);
        if (!dirp)
            break;

        size = strlen(dirp->d_name);
        if (size < 5)
            continue;
        if (strcmp(dirp->d_name + (size - 4), ".mp4") != 0 && strcmp(dirp->d_name + (size - 4), ".mp3") != 0)
            continue;

        btn = lv_list_add_btn(list, NULL, dirp->d_name);

        struct video_user_data *btn_userdata = malloc(sizeof(*btn_userdata));
        snprintf(btn_userdata->name, sizeof(btn_userdata->name), "%s%s", VIDEO_DIR_NAME, dirp->d_name);
        btn_userdata->duration = get_video_duration(btn_userdata->name);
        printf("btn_userdata->name = %s, btn_userdata->duration = %lld\n", btn_userdata->name, btn_userdata->duration);

        lv_obj_set_user_data(btn, btn_userdata);
        lv_obj_add_event_cb(btn, video_list_btn_cb, LV_EVENT_CLICKED, NULL);
    }

    closedir(dp);

    return list;
}

static void ingenic_player_create_disp_view(void)
{
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

int main(int argc, char *argv[])
{
    const char *fb_path = "/dev/fb1";
    const char *tp_path = "/dev/input/event0";

    lv_init();

    printf("argv[0] = %s\n", argv[0]);
    printf("argv[0] len = %d\n", strlen(argv[0]));

    int ret;
    ret = lvgl_init_fb_display(fb_path);
    assert(!ret);

    ret = lvgl_init_tp_input(tp_path);
    assert(!ret);

    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_0, 0);

    main_view = ingenic_player_create_main_view();
    ingenic_player_create_disp_view();

    lvgl_usleep_loop(10*1000);
}
