#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/fb.h>

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                   : show help info\n");
    fprintf(stderr, "    enable dev_path             : enable fb\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    disable dev_path            : disable fb\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    display dev_path args...    : display fb\n");
    fprintf(stderr, "       frame_index=   the frame index, default 0\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    info dev_path               : show fb info\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    clear dev_path args...      : clear fb to color \n");
    fprintf(stderr, "       color=         the frame color (ARGB), default 0x00\n");
    fprintf(stderr, "       frame_index=   the frame index, default 0\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    draw_rect dev_path args...  : draw rect \n");
    fprintf(stderr, "       color=         the frame color (ARGB), default 0x00\n");
    fprintf(stderr, "       frame_index=   the frame index, default 0\n");
    fprintf(stderr, "       x=             y start position, default 0\n");
    fprintf(stderr, "       y=             y start position, defatul 0\n");
    fprintf(stderr, "       width=         rect width, defalut -1, fix to screen width\n");
    fprintf(stderr, "       height=        rect height, defalut -1, fix to screen width\n");

    exit(status);
}

enum {
    cmd_enable,
    cmd_disable,
    cmd_display,
    cmd_info,
    cmd_clear,
    cmd_draw_rect
};

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

static int parse_int(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtol(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int cmd = -1;
    int frame_index = 0;
    int color = 0;
    int x = 0, y = 0;
    int width = -1, height = -1;

    prg_name = argv[0];

    while (1) {
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "enable")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_enable;
            break;
        }

        if (!strcmp(argv[1], "disable")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_disable;
            break;
        }

        if (!strcmp(argv[1], "info")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_info;
            break;
        }

        if (!strcmp(argv[1], "display")) {
            if (argc < 3)
                usage(-1);

            int i;
            for (i = 3; i < argc; i++) {
                if (parse_uint(argv[i], "frame_index=", &frame_index, 10))
                    continue;
                error_arg(argv[i]);
            }

            cmd = cmd_display;
            break;
        }

        if (!strcmp(argv[1], "clear")) {
            if (argc < 3)
                usage(-1);

            int i;
            for (i = 3; i < argc; i++) {
                if (parse_uint(argv[i], "frame_index=", &frame_index, 10))
                    continue;
                if (parse_uint(argv[i], "color=", &color, 16))
                    continue;
                error_arg(argv[i]);
            }
            cmd = cmd_clear;
            break;
        }

        if (!strcmp(argv[1], "draw_rect")) {
            if (argc < 3)
                usage(-1);

            int i;
            for (i = 3; i < argc; i++) {
                if (parse_uint(argv[i], "frame_index=", &frame_index, 10))
                    continue;
                if (parse_uint(argv[i], "color=", &color, 16))
                    continue;
                if (parse_int(argv[i], "x=", &x, 10))
                    continue;
                if (parse_int(argv[i], "y=", &y, 10))
                    continue;
                if (parse_int(argv[i], "width=", &width, 10))
                    continue;
                if (parse_int(argv[i], "height=", &height, 10))
                    continue;
                error_arg(argv[i]);
            }

            cmd = cmd_draw_rect;
            break;
        }

        fprintf(stderr, "error: not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    fd = fb_open(argv[2], &info);

    if (fd < 0)
        exit(-1);

    if (cmd == cmd_enable) {
        ret = fb_enable(fd);
        goto close_fd;
    }

    if (cmd == cmd_disable) {
        ret = fb_disable(fd);
        goto close_fd;
    }

    if (cmd == cmd_display) {
        ret = fb_pan_display(fd, &info, frame_index);
        goto close_fd;
    }

    if (cmd == cmd_info) {
        printf("address xres yres line_len frames total_size\n");
        printf("%08lx %d %d %d  %d %u\n",
             info.fix.smem_start, info.xres, info.yres, info.line_length, info.frame_nums, info.fix.smem_len);
        goto close_fd;
    }

    if (frame_index >= info.frame_nums) {
        fprintf(stderr, "fb: frame index out of range: %d %d\n", frame_index, info.frame_nums);
        ret = -1;
        goto close_fd;
    }

    void *fb_base = info.mapped_mem + info.frame_size * frame_index;
    draw_rect(fb_base, x, y, width, height, color);

close_fd:
    fb_close(fd, &info);
    return ret;
}
