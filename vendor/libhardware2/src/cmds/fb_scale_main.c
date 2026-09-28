#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <libhardware2/rmem.h>
#include <libhardware2/fb.h>

static char *prg_name;

static int usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);

    fprintf(stderr, "Note : fb_scale logo_640_360_nv12.yuv logo put in libhardware2/src/cmds/logo_640_360_nv12.yuv\n");
    fprintf(stderr, "In project build directory run `adb push ../libhardware2/src/cmds/logo_640_360_nv12.yuv /usr/data` command push logo file to board\n");
    fprintf(stderr, "\n");

    fprintf(stderr,"Usage1:%s <fb_device=device_path> <file=file_path> <src_width=width> <src_height=height> <src_stride=stride> <scale_width=width> <scale_height=height>\n", prg_name);
    fprintf(stderr,"Example1:\n");
    fprintf(stderr,"\t%s fb_device=/dev/fb0 file=/usr/data/logo_640_360_nv12.yuv src_width=640 src_height=360 src_stride=640 scale_width=600 scale_height=300\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
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

static char *parse_str(char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}

static int read_src(FILE *fp, void *y_mem, void *uv_mem, int height, int width, int stride)
{
    int i = 0;
    size_t read_size = 0;

    for (i = 0; i < height; i++) {
        read_size = fread(y_mem, sizeof(char), width, fp);
        if(read_size != width) {
            fprintf(stderr, "scale: fread y fail, read_size(%d) != width(%d)\n", read_size, width);
            return -1;
        }

        y_mem += stride;
    }

    for (i = 0; i < height / 2; i++) {
        read_size = fread(uv_mem, sizeof(char), width, fp);
        if(read_size != width) {
            fprintf(stderr, "scale: fread uv fail, read_size(%d) != width(%d)\n", read_size, width);
            return -1;
        }

        uv_mem += stride;
    }

    return 0;
}


int main(int argc, char **argv)
{
    int i, ret = 0;
    FILE * infile;
    int rmem_fd = -1;
    int display_fb = -1;
    int src_width = -1;
    int src_height = -1;
    int src_stride = -1;
    int scale_width = -1;
    int scale_height = -1;
    const char *str = NULL;
    void *mmap_y = NULL;
    void *mmap_uv = NULL;
    unsigned long y_phy;
    unsigned long uv_phy;
    const char *fb_device = NULL;
    const char *src_file_path = NULL;
    struct fb_device_info fb_info;

    prg_name = argv[0];

    if (argc < 3)
        usage(-1);

    for (i = 1; i < argc; i++) {
        if (parse_int(argv[i], "src_width=", &src_width, 10))
            continue;
        if (parse_int(argv[i], "src_height=", &src_height, 10))
            continue;
        if (parse_int(argv[i], "src_stride=", &src_stride, 10))
            continue;
        if (parse_int(argv[i], "scale_width=", &scale_width, 10))
            continue;
        if (parse_int(argv[i], "scale_height=", &scale_height, 10))
            continue;

        if ((str = parse_str(argv[i], "file="))) {
            src_file_path = str;
            continue;
        }
        if ((str = parse_str(argv[i], "fb_device="))) {
            fb_device = str;
            continue;
        }
    }

    if (src_stride < src_width)
        src_stride = src_width;

    display_fb = fb_open(fb_device, &fb_info);
    if (display_fb < 0) {
        fprintf(stderr, "fb_scale : open %s device fail\n", fb_device);
        return -1;
    }

    if (scale_width > fb_info.xres || scale_width <= 0) {
        fprintf(stderr, "fb_scale : scale width fb display is not supported \n");
        goto close_fb;
    }

    if (scale_height > fb_info.yres || scale_height <= 0) {
        fprintf(stderr, "fb_scale : scale height fb display is not supported \n");
        goto close_fb;
    }

    rmem_fd = rmem_open();
    if (rmem_fd < 0) {
        fprintf(stderr, "fb_scale : open rmem device fail\n");
        goto close_fb;
    }

    mmap_y = rmem_alloc(rmem_fd, &y_phy, src_stride * src_height);
    if (mmap_y == NULL) {
        fprintf(stderr, "fb_scale : alloc rmem space fail\n");
        goto rmem_fb;
    }

    mmap_uv = rmem_alloc(rmem_fd, &uv_phy, src_stride * src_height / 2);
    if (mmap_uv == NULL) {
        fprintf(stderr, "fb_scale : alloc rmem space fail\n");
        goto free_mmap_y;
    }

    infile = fopen(src_file_path, "rb");
    if (infile == NULL) {
        fprintf(stderr, "fb_scale : open %s fail\n", src_file_path);
        goto free_mmap_uv;
    }

    ret = read_src(infile, mmap_y, mmap_uv, src_height, src_width, src_stride);
    if(ret < 0) {
        fprintf(stderr, "fb_scale: read fail\n");
        goto close_infile;
    }

    fb_enable(display_fb);

    struct lcdc_layer layer_cfg0 = {
        .fb_fmt = fb_fmt_NV12,
        .xres = src_stride,
        .yres = src_height,
        .xpos = 0,
        .ypos = 0,

        .layer_order = lcdc_layer_0,
        .layer_enable = 1,

        .y = {
            .mem = (void *)y_phy,
            .stride = src_stride,
        },
        .uv = {
            .mem = (void *)uv_phy,
            .stride = src_stride,
        },

        .alpha = {
            .enable = 1,
            .value = 0xff,
        },
        .scaling = {
            .enable = 1,
            .xres = scale_width,
            .yres = scale_height,
        },
    };


    ret = fb_pan_display_set_user_cfg(display_fb, &layer_cfg0);
    if (ret) {
        fprintf(stderr, "fb_scale: fd set user config fail\n");
        goto disable_fb;
    }

    ret = fb_pan_display_enable_user_cfg(display_fb);
    if (ret) {
        fprintf(stderr, "fb_scale: fd enable user config fail\n");
        goto disable_fb;
    }

    ret = fb_pan_display(display_fb, &fb_info, 0);
    if (ret) {
        fprintf(stderr, "fb_scale: fd enable display fail\n");
        goto disable_fb;
    }

    return 0;

disable_fb:
    fb_disable(display_fb);
close_infile:
    fclose(infile);
free_mmap_uv:
    rmem_free(rmem_fd, mmap_uv, uv_phy, src_width * src_height / 2);
free_mmap_y:
    rmem_free(rmem_fd, mmap_y, y_phy, src_width * src_height);
rmem_fb:
    rmem_close(rmem_fd);
close_fb:
    fb_close(display_fb, &fb_info);

    return ret;
}