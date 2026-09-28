#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <libhardware2/fb.h>
#include <libhardware2/keyboard.h>
#include <libhardware2/adc.h>

static struct fb_device_info info;
static int fd;

static void draw_rect8(unsigned char *p, int x, int y, int x1, int y1, unsigned int color)
{
    p = (void *)p + y*info.line_length + x;

    int i, j;
    for (i = 0; i < y1 - y; i++){
        for (j = 0; j < x1 - x; j++) {
            p[j] = color;
        }
        p = (void *)p + info.line_length;
    }
}

#define COLOR(_c, _from, _to, _len) \
    (((((_c) >> (_from)) & 0xff) >> (8 - (_len))) << (_to))

static void draw_rect16(unsigned short *p, int x, int y, int x1, int y1, unsigned int color)
{
    p = (void *)p + y*info.line_length + x*2;

    color = COLOR(color, 16, 11, 5) |
            COLOR(color, 8, 5, 6) |
            COLOR(color, 0, 0, 5);

    int i, j;
    for (i = 0; i < y1 - y; i++){
        for (j = 0; j < x1 - x; j++) {
            p[j] = color;
        }
        p = (void *)p + info.line_length;
    }
}

static void draw_rect32(unsigned int *p, int x, int y, int x1, int y1, unsigned int color)
{
    p = (void *)p + y*info.line_length + x*4;

    int i, j;
    for (i = 0; i < y1 - y; i++){
        for (j = 0; j < x1 - x; j++) {
            p[j] = color;
        }
        p = (void *)p + info.line_length;
    }
}

static void draw_rect(void *mem, int x, int y, int width, int height, unsigned int color)
{
    int xres = info.xres;
    int yres = info.yres;
    int x1 = (width < 0) ? xres : x + width;
    int y1 = (height < 0) ? yres : y + height;

    if (x1 > xres)
        x1 = xres;

    if (y1 > yres)
        y1 = yres;

    if (x < 0)
        x = 0;

    if (y < 0)
        y = 0;

    int bytes = info.line_length / info.xres;

    if (bytes == 0 || bytes == 3) {
        fprintf(stderr, "not support pix fmt: %d %d %d\n",
             info.bits_per_pixel, info.line_length, info.xres);
        exit(-1);
    }

    if (bytes == 1)
        draw_rect8(mem, x, y, x1, y1, color);

    if (bytes == 2)
        draw_rect16(mem, x, y, x1, y1, color);

    if (bytes >= 4)
        draw_rect32(mem, x, y, x1, y1, color);
}

struct shape {
    unsigned char w;
    unsigned char h;
    unsigned char a[];
};

#define SHAPE(_w, _h, x...) \
    (struct shape *) &(unsigned char []){_w, _h, x}

static struct shape *shapes[] =
{
    /*
     * []
     */
    SHAPE(1, 1,    1),
    /*
     * [][][]
     */
    SHAPE(3, 1,    1, 1, 1),
    /*
     * []
     * []
     * []
     */
    SHAPE(1, 3,    1,
                   1,
                   1),
    /*
     * [][]
     * [][]
     */
    SHAPE(2, 2,    1, 1,
                   1, 1),
    /*
     * [][]
     * [][]
     */
    SHAPE(2, 2,    1, 1,
                   1, 1),
    /*
     * []
     * []
     * [][]
     */
    SHAPE(2, 3,    1, 0,
                   1, 0,
                   1, 1),
    /*
     *   []
     *   []
     * [][]
     */
    SHAPE(2, 3,    0, 1,
                   0, 1,
                   1, 1),
    /*
     *   []
     * [][][]
     */
    SHAPE(3, 2,    0, 1, 0,
                   1, 1, 1),
    /*
     *   [][]
     * [][]
     */
    SHAPE(3, 2,    0, 1, 1,
                   1, 1, 0),
    /*
     * [][]
     *   [][]
     */
    SHAPE(3, 2,    1, 1, 0,
                   0, 1, 1),
};


static void *frames[2];
static int indexs[2];

static int shape_size;
static int shape_y_off;
static int shape_y_cnt;

static unsigned int *shape_map;

static unsigned int colors[] = {0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xff00ffff};

static void draw_shape(void *mem, struct shape *shape, int xpos, int ypos, int color)
{
    int i, j;
    unsigned char *a = shape->a;
    int y = shape_y_off + ypos * shape_size;

    for (j = 0; j < shape->h; j++) {
        int x = xpos * shape_size;
        for (i = 0; i < shape->w; i++) {
            if (*a)
                draw_rect(mem, x, y, shape_size, shape_size, color);
            x += shape_size;
            a++;
        }
        y += shape_size;
    }
}

static void draw_map(void *mem)
{
    int i, j;
    unsigned int *p = shape_map;

    for (i = 0; i < shape_y_cnt; i++) {
        int y = shape_y_off + i*shape_size;
        for (j = 0; j < 10; j++) {
            int color = p[i*10 + j];
            int x = j * shape_size;
            if (color)
                draw_rect(mem, x, y, shape_size, shape_size, color);
        }
    }
}

static int shape_start(struct shape *shape)
{
    return (10 - shape->w) / 2;
}

static void shape_rotate(struct shape *src, struct shape *dst)
{
    int x, y;
    int x0, y0;

    dst->w = src->h;
    dst->h = src->w;

    for (y = 0; y < src->h; y++) {
        x0 = src->h - y - 1;
        for (x = 0; x < src->w; x++) {
            y0 = x;
            dst->a[dst->w*y0 + x0] = src->a[src->w*y + x];
        }
    }
}

static int check_shape_pos(struct shape *shape, int xpos, int ypos)
{
    unsigned int *p = shape_map;
    unsigned char *a = shape->a;
    int i, j;
    int y = ypos;

    for (j = 0; j < shape->h; j++) {
        int x = xpos;
        for (i = 0; i < shape->w; i++) {
            if (*a && p[y*10 + x])
                return 1;
            x++;
            a++;
        }
        y++;
    }

    return 0;
}

static void add_to_shape_map(struct shape *shape, int xpos, int ypos, int color)
{
    unsigned int *p = shape_map;
    unsigned char *a = shape->a;
    int i, j;
    int y = ypos;

    if (!color)
        color = 1;

    for (j = 0; j < shape->h; j++) {
        int x = xpos;
        for (i = 0; i < shape->w; i++) {
            if (*a)
                p[y*10 + x] = color;
            x++;
            a++;
        }
        y++;
    }
}

static int check_map_clear(void)
{
    unsigned int *p = shape_map;
    int i, j;
    int is_ok = 1;

    for (j = 0; j < shape_y_cnt; j++) {
        is_ok = 1;
        p = shape_map + 10 * j;
        for (i = 1; i < 10; i++) {
            if (!p[i]) {
                is_ok = 0;
                break;
            }
        }

        if (is_ok)
            break;
    }

    if (!is_ok)
        return 0;

    if (j != 0)
        memmove(shape_map+10, shape_map, (p - shape_map)*4);

    memset(shape_map, 0, 10*4);

    return 1;
}

enum {
    ADC_LEFT,
    ADC_RIGHT,
    ADC_OK,
    ADC_DOWN,

    ADC_NULL,
};

static int adc_values[] = {
    [ADC_LEFT] = 338,
    [ADC_RIGHT] = 512,
    [ADC_OK] = 851,
    [ADC_DOWN] = 168,
};

static long adc_handle;

static int old_key = ADC_NULL;
static u_int64_t old_time;
static int old_key_delta;

uint64_t boot_time_msecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

int get_adc_key(void)
{
    int value = adc_get_value(adc_handle);
    int *values = adc_values;
    int key = ADC_NULL;
    uint64_t new_time;

    for (int i = 0; i < ADC_NULL; i++) {
        if ((values[i] - 20) <= value && value <= (values[i] + 20)) {
            key = i;
            break;
        }
    }

    if (key == ADC_NULL)
        old_key = ADC_NULL;

    new_time = boot_time_msecs();

    if (old_key != key) {
        old_time = new_time;
        old_key = key;
        old_key_delta = 200;
        return key;
    }

    if (new_time - old_time >= old_key_delta) {
        old_time = new_time;
        old_key_delta = 50;
        return key;
    }

    return ADC_NULL;
}

int main(int argc, char *argv[])
{
    int return_value = -1;

    fd = fb_open("/dev/fb0", &info);
    if (fd < 0)
        return -1;

    if (info.frame_nums < 1) {
        fprintf(stderr, "fb: no frame found\n");
        goto m_close_fb;
    }

    shape_size = info.xres / 10;
    if (shape_size < 1) {
        fprintf(stderr, "fb: xres too small: %d\n", info.xres);
        goto m_close_fb;
    }

    if (info.yres / shape_size < 5) {
        fprintf(stderr, "fb: yres too small: %d\n", info.yres);
        goto m_close_fb;
    }

    shape_y_off = info.yres % shape_size;
    shape_y_cnt = info.yres / shape_size;

    shape_map = malloc(shape_y_cnt * 10 * sizeof(int));
    memset(shape_map, 0, shape_y_cnt * 10 * sizeof(int));

    void *mem = info.mapped_mem;

    frames[0] = mem;
    frames[1] = (info.frame_nums < 2) ? mem : mem + info.frame_size;
    indexs[0] = (info.frame_nums < 2) ? 0 : 1;
    indexs[1] = 0;

    if (fb_enable(fd))
        goto m_free_mem;

    adc_handle = adc_enable(0);
    if (adc_handle == -1)
        goto m_free_mem;

    int shape_nums = sizeof(shapes)/sizeof(shapes[0]);
    int color_nums = sizeof(colors)/sizeof(colors[0]);
    int n = 0;

    while (1) {
        struct shape *shape = shapes[random() % shape_nums];
        unsigned char tmp[2 + shape->w*shape->h];
        memcpy(tmp, shape, sizeof(tmp));
        shape = (void *)tmp;

        int color = colors[random() % color_nums];

        int x = shape_start(shape);
        int y = 0;

        int i = 0;
        for (i = 0; i < shape_y_cnt; i++) {
            if (check_shape_pos(shape, x, y))
                break;

            draw_rect(frames[n], 0, 0, -1, -1, 0);
            draw_shape(frames[n], shape, x, y, color);

            draw_map(frames[n]);

            fb_pan_display(fd, &info, n);
            n = indexs[n];

            uint64_t start = boot_time_msecs();

            while (1) {
                int update = 0;
                int key = get_adc_key();

                if (key == ADC_LEFT) {
                    if (x) {
                        if (!check_shape_pos(shape, x-1, y)) {
                            x--;
                            update = 1;
                        }
                    }
                }

                if (key == ADC_RIGHT) {
                    if (x + shape->w < 10) {
                        if (!check_shape_pos(shape, x+1, y)) {
                            x++;
                            update = 1;
                        }
                    }
                }

                if (key == ADC_DOWN) {
                    if (y + shape->h < shape_y_cnt) {
                        if (!check_shape_pos(shape, x, y+1)) {
                            y++;
                            update = 1;
                        }
                    }
                }

                if (key == ADC_OK) {
                    if (x + shape->w < 10) {
                        unsigned char tmp2[2 + shape->w*shape->h];
                        struct shape *tmp_shape = (void *)tmp2;
                        shape_rotate(shape, tmp_shape);

                        if (y + shape->h <= shape_y_cnt) {
                            if (!check_shape_pos(tmp_shape, x, y)) {
                                memcpy(shape, tmp_shape, sizeof(tmp2));
                                update = 1;
                            }
                        }
                    }
                }

                if (update) {
                    draw_rect(frames[n], 0, 0, -1, -1, 0);
                    draw_shape(frames[n], shape, x, y, color);

                    draw_map(frames[n]);

                    fb_pan_display(fd, &info, n);
                    n = indexs[n];
                }

                uint64_t end = boot_time_msecs();
                if (end - start >= 300)
                    break;

                usleep(10*1000);
            }

            usleep(300*1000);

            y++;
            if (y + shape->h > shape_y_cnt)
                break;
        }

        if (y == 0) {
            draw_rect(frames[n], 0, 0, -1, -1, 0);
            draw_shape(frames[n], shape, x, y, color);

            draw_map(frames[n]);

            fb_pan_display(fd, &info, n);
            n = indexs[n];

            printf("game over!\n");
            break;
        }

        add_to_shape_map(shape, x, y-1, color);

        while (check_map_clear()) {
            draw_rect(frames[n], 0, 0, -1, -1, 0);
            draw_map(frames[n]);

            fb_pan_display(fd, &info, n);
            n = indexs[n];

            usleep(300*1000);
        }
    }

    return_value = 0;
    adc_disable(adc_handle);
m_free_mem:
    free(shape_map);
m_close_fb:
    fb_close(fd, &info);
    return return_value;
}


void show_adc_key(void)
{
    while (1) {
        int key = get_adc_key();
        if (key != ADC_NULL)
            printf("key: %d\n", key);
    }
}

void show_shapes(void)
{
    int n = 0;
    int i;

    for (i = 0; i < sizeof(shapes)/sizeof(shapes[0]); i++) {
        draw_rect(frames[n], 0, 0, -1, -1, 0);
        draw_shape(frames[n], shapes[i], shape_start(shapes[i]), shape_y_off + 200, 0xffff0000);

        fb_pan_display(fd, &info, n);

        usleep(1000*1000);
    }
}
