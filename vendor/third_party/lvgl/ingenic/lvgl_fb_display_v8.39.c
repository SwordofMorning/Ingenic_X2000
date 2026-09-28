#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libhardware2/fb.h>
#include <pthread.h>
#include <semaphore.h>

#include "lvgl/lvgl.h"

#include "lv_conf.h"

#include <libutils2/data_array.h>
#include <libutils2/boot_time.h>

struct m_queue {
    sem_t sem;
    data_array_t a;
    pthread_spinlock_t lock;
};

static int fb_fd;
static struct fb_device_info fb_info;
static void *draw_tmp_buf;
static void *display_buf[3];
static lv_disp_t *lv_disp;
static int is_enable = 0;
static int fb_buf_cnt = 0;
static int switch_to_next_buf = 1;
static volatile int show_refresh_rate = 0;
static volatile int draw_index = 0;

static struct m_queue avail_queue;
static struct m_queue display_queue;

static data_array_t areas[3];

static void copy_areas(int dst_index, int src_index)
{
    int bytes = fb_bytes_per_pixel(fb_info.fb_fmt);

    int i;
    for (i = 0; i < data_array_size(&areas[dst_index]); i++) {
        int y;
        lv_area_t *a = data_array_at(&areas[dst_index], i);
        int w = a->x2 - a->x1 + 1;
        for (y = a->y1; y <= a->y2; y++) {
            void *src = display_buf[src_index] + y*fb_info.line_length + a->x1*bytes;
            void *dst = display_buf[dst_index] + y*fb_info.line_length + a->x1*bytes;
            memcpy(dst, src, w*bytes);
        }
    }
    // memcpy(display_buf[dst_index], display_buf[src_index], fb_info.line_length*fb_info.yres);
    data_array_reset_size(&areas[dst_index], 0);
}

/*
 * 求得 区域 a (x1,y1 x2,y2) 减去其 子集区域 s 的4个区域
 * 有时候不是每个子区域都存在,所以返回的子区域r中可能多个都是无效的区域
 * 
 * x1,y1      s.x1               s.x2        x2,y1
 *      +********.****************.*********+
 *      *                                   *
 *      *          area 0                   *
 *      *                                   *
 * s.y1 .........+................+.........*
 *      *        .                .         *
 *      * area 1 .      s         . area 2  *
 *      *        .                .         *
 * s.y2 .........+................+.........*
 *      *                                   *
 *      *          area 3                   *
 *      *                                   *
 *      +***********************************+
 * x1,y2                                     x2,y2
*/

static int lv_area_sub_intersect(lv_area_t *a, lv_area_t *s, lv_area_t *r)
{
    int flag = 0;
    lv_area_t invalid_area = {
        .x1 = 0, .x2 = -1,
        .y1 = 0, .y2 = -1,
    };
    if (a->y1 < s->y1) {
        r[0].x1 = a->x1;
        r[0].x2 = a->x2;
        r[0].y1 = a->y1;
        r[0].y2 = s->y1-1;
        flag |= 1 << 0;
    } else {
        r[0] = invalid_area;
    }
    if (a->x1 < s->x1) {
        r[1].x1 = a->x1;
        r[1].x2 = s->x1-1;
        r[1].y1 = s->y1;
        r[1].y2 = s->y2;
        flag |= 1 << 1;
    } else {
        r[1] = invalid_area;
    }
    if (a->x2 > s->x2) {
        r[2].x1 = s->x2+1;
        r[2].x2 = a->x2;
        r[2].y1 = s->y1;
        r[2].y2 = s->y2;
        flag |= 1 << 2;
    } else {
        r[2] = invalid_area;
    }
    if (a->y2 > s->y2) {
        r[3].x1 = a->x1;
        r[3].x2 = a->x2;
        r[3].y1 = s->y2+1;
        r[3].y2 = a->y2;
        flag |= 1 << 3;
    } else {
        r[3] = invalid_area;
    }
    return flag;
}

static void add_area(int index, lv_area_t *s)
{
    data_array_t tmp;
    data_array_init(&tmp, sizeof(lv_area_t), 32);

    int i;
    for (i = 0; i < data_array_size(&areas[index]); i++) {
        lv_area_t *a = data_array_at(&areas[index], i);
        lv_area_t b;
        if (_lv_area_intersect(&b, a, s) == false) {
            data_array_add(&tmp, a);
            continue;
        }
        lv_area_t c[4];
        int flag = lv_area_sub_intersect(a, &b, c);
        if (flag & (1<<0)) data_array_add(&tmp, &c[0]);
        if (flag & (1<<1)) data_array_add(&tmp, &c[1]);
        if (flag & (1<<2)) data_array_add(&tmp, &c[2]);
        if (flag & (1<<3)) data_array_add(&tmp, &c[3]);
    }

    data_array_reset_size(&areas[index], 0);
    for (i = 0; i < data_array_size(&tmp); i++) {
        lv_area_t *a = data_array_at(&tmp, i);
        data_array_add(&areas[index], a);
        // printf("a: %d,%d %d,%d\n", a->x1,a->x2, a->y1,a->y2);
    }
    data_array_add(&areas[index], s);

    data_array_deinit(&tmp);
}

static void fb_show_frame_rate(void)
{
    static double old = 0;
    static int count = 0;

    if (!show_refresh_rate)
        return;

    double t = boot_time_secs();
    if (old == 0)
        old = t;

    count++;

    if (t-old >= 1.0) {
        printf("refresh rate: %.2f\n", count/(t-old));
        old = t;
        count = 0;
    }
}

static int get_index(struct m_queue *queue)
{
    sem_wait(&queue->sem);

    pthread_spin_lock(&queue->lock);
    int index = *(int *)data_array_at(&queue->a, 0);
    data_array_del(&queue->a, 0);
    pthread_spin_unlock(&queue->lock);

    return index;
}

static void add_index(struct m_queue *queue, int index)
{
    pthread_spin_lock(&queue->lock);
    data_array_add(&queue->a, &index);
    pthread_spin_unlock(&queue->lock);

    sem_post(&queue->sem);
}

static void fb_display_flush(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int bytes = fb_bytes_per_pixel(fb_info.fb_fmt);
    int xres = fb_info.xres;
    int yres = fb_info.yres;

    if(area->x2 < 0 || area->y2 < 0 ||
       area->x1 > xres - 1 || area->y1 > yres - 1) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (switch_to_next_buf) {
        if (fb_buf_cnt >= 2) {
            int old_index = draw_index;
            draw_index = get_index(&avail_queue);
            copy_areas(draw_index, old_index);
        }
        switch_to_next_buf = 0;
    }

    /* Truncate the area to the screen
     * 注意:color_p 是否也要跟着变? lvgl 原生的驱动没有变
     */
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > xres - 1 ? xres - 1 : area->x2;
    int32_t act_y2 = area->y2 > yres - 1 ? yres - 1 : area->y2;

    lv_coord_t w = (act_x2 - act_x1 + 1);

    if (fb_buf_cnt >= 2) {
        lv_area_t save;
        save.x1 = act_x1;
        save.x2 = act_x2;
        save.y1 = act_y1;
        save.y2 = act_y2;

        add_area((draw_index+1)%fb_buf_cnt, &save);
        if (fb_buf_cnt == 3)
            add_area((draw_index+2)%fb_buf_cnt, &save);
    }

    void *draw_buf = display_buf[draw_index];

    int y;
    for (y = act_y1; y <= act_y2; y++) {
        void *p = draw_buf + y*fb_info.line_length + act_x1*bytes;
        memcpy(p, color_p, w*bytes);
        color_p += w;
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        if (!is_enable) {
            fb_enable(fb_fd);
            is_enable = 1;
        }
        if (fb_buf_cnt == 1) {
            fb_pan_display(fb_fd, &fb_info, draw_index);
            fb_show_frame_rate();
        } else {
            // 标记当前帧绘制完成,可以显示了
            // sem_post(&display_sem[draw_index]);
            add_index(&display_queue, draw_index);
            switch_to_next_buf = 1;
        }
    }

    lv_disp_flush_ready(disp_drv);
}

static void *pan_display_thread(void *data)
{
    int old = -1, index;

    while (1) {
        index = get_index(&display_queue);

        fb_pan_display(fb_fd, &fb_info, index);

        fb_show_frame_rate();

        if (old != -1)
            add_index(&avail_queue, old);

        // printf("display: %d add %d\n", index, old);

        old = index;
     }

    return NULL;
}

void lvgl_set_fb_show_frame_rate(int enable)
{
    show_refresh_rate = enable;
}

void lvgl_set_fb_force_frame_cnt(int frame_cnt)
{
    fb_buf_cnt = LV_MIN(frame_cnt, 3);
}

int lvgl_init_fb_display(const char *fb_path)
{
    if (!fb_path)
        fb_path = "/dev/fb0";

    fb_fd = fb_open(fb_path, &fb_info);
    if (fb_fd == -1)
        return -1;

    if (!fb_info.frame_nums) {
        fprintf(stderr, "fb: %s no frame buffer\n", fb_path);
        return -1;
    }

    switch (fb_info.fb_fmt) {
    case fb_fmt_RGB565:
        if (LV_COLOR_DEPTH != 16) {
            fprintf(stderr, "lvgl not match color: %d %d\n", fb_info.fb_fmt, LV_COLOR_DEPTH);
            return -1;
        }
        break;
    case fb_fmt_ARGB8888:
    case fb_fmt_RGB888:
        if (LV_COLOR_DEPTH != 32) {
            fprintf(stderr, "lvgl not match color: %d %d\n", fb_info.fb_fmt, LV_COLOR_DEPTH);
            return -1;
        }
        break;
    default:
        fprintf(stderr, "lvgl not support this format: %d\n", fb_info.fb_fmt);
        fb_close(fb_fd, &fb_info);
        return -1;
    }

    fb_buf_cnt = fb_buf_cnt ?: LV_MIN(fb_info.frame_nums, 3);

    int bytes = fb_bytes_per_pixel(fb_info.fb_fmt);
    int xres = fb_info.xres;
    int yres = fb_info.yres;

    display_buf[0] = fb_info.mapped_mem;
    display_buf[1] = fb_info.mapped_mem + (1%fb_buf_cnt)*fb_info.frame_size;
    display_buf[2] = fb_info.mapped_mem + (2%fb_buf_cnt)*fb_info.frame_size;
    static lv_disp_draw_buf_t disp_buf;

    sem_init(&display_queue.sem, 0, 0);
    pthread_spin_init(&display_queue.lock, 0);
    data_array_init(&display_queue.a, sizeof(int), 3);
    
    sem_init(&avail_queue.sem, 0, 0);
    pthread_spin_init(&avail_queue.lock, 0);
    data_array_init(&avail_queue.a, sizeof(int), 3);

    if (fb_buf_cnt >= 2) {
        data_array_init(&areas[0], sizeof(lv_area_t), 32);
        data_array_init(&areas[1], sizeof(lv_area_t), 32);
        add_index(&avail_queue, 0);
        add_index(&avail_queue, 1);
    }
    if (fb_buf_cnt == 3) {
        data_array_init(&areas[2], sizeof(lv_area_t), 32);
        add_index(&avail_queue, 2);
    }

    if (fb_buf_cnt >= 2) {
        pthread_t thread;
        pthread_create(&thread, NULL, pan_display_thread, NULL);
    }

    draw_tmp_buf = malloc(xres*200*bytes);
    memset(draw_tmp_buf, 0x1, xres*200*bytes);
    lv_disp_draw_buf_init(&disp_buf, draw_tmp_buf, NULL, xres*200);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fb_display_flush;
    disp_drv.hor_res = xres;
    disp_drv.ver_res = yres;
    disp_drv.screen_transp = 1;
    disp_drv.full_refresh = 0;
    disp_drv.direct_mode = 0;
    lv_disp = lv_disp_drv_register(&disp_drv);

    return 0;
}
