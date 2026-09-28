#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libhardware2/fb.h>
#include <libhardware2/rmem.h>
#include <libhardware2/rotator.h>

typedef struct {
    int size;
    void *data;
    unsigned long phy_addr;
} pixels_data;

typedef struct rotator_config_data rotate_config;

typedef struct {
    char *name;
    int type;
} type_bundle;

#define SHORTER(x, y)   ((x) < (y) ? (x) : (y))
#define ROTATE_X_Y(x)   ((x) == ROTATOR_ANGLE_90 || (x) == ROTATOR_ANGLE_270)

static type_bundle format_bundle [] = {
    {"RGB555",     ROTATOR_RGB555},
    {"RGB565",     ROTATOR_RGB565},
    {"ARGB8888",   ROTATOR_ARGB8888},  //default
    {"Y8",         ROTATOR_Y8},
    {"YUV422",     ROTATOR_YUV422},
};

static type_bundle order_bundle [] = {
    {"RGB",      ROTATOR_ORDER_RGB_TO_RGB},//default
    {"RBG",      ROTATOR_ORDER_RBG_TO_RGB},
    {"GRB",      ROTATOR_ORDER_GRB_TO_RGB},
    {"GBR",      ROTATOR_ORDER_GBR_TO_RGB},
    {"BRG",      ROTATOR_ORDER_BRG_TO_RGB},
    {"BGR",      ROTATOR_ORDER_BGR_TO_RGB},
};

static type_bundle angle_bundle [] = {
    {"0",     ROTATOR_ANGLE_0},
    {"90",    ROTATOR_ANGLE_90},
    {"180",   ROTATOR_ANGLE_180},
    {"270",   ROTATOR_ANGLE_270},
};

static void cmd_usage(const char *command)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr,
            "    %s <input_file=path> <output_file=path>\n"
            "[src_fmt=fmt] <image_width=value> <image_height=value> \n"
            "<rotate_degree=degree> [color_order=order] [mirror=mirror]\n"
            "[preview=device]\n"
            "    %s [preview=device]",
            command, command);

    fprintf(stderr,
            "\nArguments as follow\n"
            "<input_file>        The pixel data file for rotate.\n"
            "<output_file>       The pixel data file after rotate.\n"
            "[src_fmt]           Pixels format: \n"
            "                    ARGB8888(default) \n"
            "                    RGB565 \n"
            "                    RGB555\n"
            "                    YUV422\n"
            "                    Y8\n"
            "<image_width>       The background image weight(unit:pixel).\n"
            "<image_height>      The background image height(unit:pixel).\n"
            "<rotate_degree>     Rotate degree: 0 / 90 / 180 / 270.\n"
            "[color_order]       Input pixels' order(??? -> RGB): \n"
            "                    RGB(default) RBG / GRB / GBR / BRG / BGR.\n"
            "[mirror]            Mirror convert: horizontal / vertical / both.\n"
            "                    (default: not set).\n"
            "[device]            Preview on fb device: \\dev\\fb0 \n"
            "                    (default: not set).\n");

    fprintf(stderr,
            "Example:\n"
            "    %s input_file=/tmp/logo_495_280_argb8888 \n"
            "output_file=/tmp/logo_495_280_argb8888_270\n"
            "image_width=495 image_height=280 rotate_degree=270\n"
            "preview=/dev/fb0\n",
            command);
}

/*
 * Return  =0: match
 *         <0: not match
 */
static int enum_match(type_bundle *bundle, int size, char *name, int *type)
{
    int i;

    for (i = 0; i < size; i++) {
        if (strcmp(name, bundle[i].name) == 0) {
            *type = bundle[i].type;
            return 0;
        }
    }
    fprintf(stderr, "type_name=%s not found\n", name);
    return -1;
}

/*
 * Return  =1: found
 *         =0: not found
 */
static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str + len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        exit(-1);
    }

    *value = v;
    return 1;
}

/*
 * Return  =1: found
 *         =0: not found
 */
static int parse_string(const char *str, const char *prefix, char **pos)
{
    int len_pre = strlen(prefix);
    int len_src = strlen(str);
    len_src = len_src - len_pre;

    if (strncmp(str, prefix, len_pre))
        return 0;

    if (len_src > 0)
        *pos = ((char *)str + len_pre);
    return 1;
}

unsigned int rotator_bytes_per_pixel(enum rotator_fmt fb_fmt)
{
    switch (fb_fmt) {
    case ROTATOR_ARGB8888:
        return 4;
    case ROTATOR_RGB888:
        return 3;
    case ROTATOR_ARGB1555:
    case ROTATOR_RGB555:
    case ROTATOR_RGB565:
        return 2;
    case ROTATOR_NV12:
    case ROTATOR_YUV422:
        return 2;
    case ROTATOR_Y8:
        return 1;
    default:
        return 2;
    }
}

/*
 * Return  =0: parse ok
 *         <0: parse err
 */
static int parse_cmd_info(int argc, const char **argv, rotate_config *config,
                          char **input_path, char **output_path, char **device)
{
    int ret = 0;
    int parse_ok;
    int i;
    char *src_fmt = NULL;
    char *angle = NULL;
    char *color_order = NULL;
    char *mirror = NULL;

    config->src_fmt             = ROTATOR_ARGB8888;
    config->convert_color       = ROTATOR_ORDER_RGB_TO_RGB;
    config->vertical_mirror     = ROTATOR_NO_MIRROR;
    config->horizontal_mirror   = ROTATOR_NO_MIRROR;

    for (i = 1; i < argc; i++) {
        parse_ok = 0;
        parse_ok |= parse_string((const char *)argv[i], "input_file=", input_path);
        parse_ok |= parse_string((const char *)argv[i], "output_file=", output_path);
        parse_ok |= parse_string((const char *)argv[i], "src_fmt=", &src_fmt);
        parse_ok |= parse_uint((const char *)argv[i], "image_width=", &config->frame_width, 10);
        parse_ok |= parse_uint((const char *)argv[i], "image_height=", &config->frame_height, 10);
        parse_ok |= parse_string((const char *)argv[i], "rotate_degree=", &angle);
        parse_ok |= parse_string((const char *)argv[i], "color_order=", &color_order);
        parse_ok |= parse_string((const char *)argv[i], "mirror=", &mirror);
        parse_ok |= parse_string((const char *)argv[i], "preview=", device);
        if (!parse_ok) {
            fprintf(stderr, "unknown parameter %s \n", argv[i]);
            ret--;
        }
    }

    if (src_fmt)
        ret += enum_match(format_bundle,
                          sizeof(format_bundle) / sizeof(format_bundle[0]),
                          src_fmt, (int *)&(config->src_fmt));
    if (angle)
        ret += enum_match(angle_bundle,
                          sizeof(angle_bundle) / sizeof(angle_bundle[0]), angle,
                          (int *)&(config->rotate_angle));
    if (color_order)
        ret += enum_match(order_bundle,
                          sizeof(order_bundle) / sizeof(order_bundle[0]),
                          color_order, (int *)&(config->convert_color));

    if (mirror != NULL) {
        if (strcmp("horizontal", mirror) == 0)
            config->horizontal_mirror = ROTATOR_MIRROR;
        else if (strcmp("vertical", mirror) == 0)
            config->vertical_mirror = ROTATOR_MIRROR;
        else if (strcmp("both", mirror) == 0) {
            config->horizontal_mirror = ROTATOR_MIRROR;
            config->vertical_mirror = ROTATOR_MIRROR;
        } else
            ret--;
    }

    config->dst_fmt = config->src_fmt;
    config->src_stride = config->frame_width;

    config->dst_stride = config->frame_width;
    if (ROTATE_X_Y(config->rotate_angle))
        config->dst_stride = config->frame_height;

    return ret;
}

int support_fb_fmt(enum fb_fmt fb_fmt, enum rotator_fmt dst_fmt)
{
    switch (fb_fmt) {
    case fb_fmt_RGB555:
        return dst_fmt == ROTATOR_RGB555;
        break;
    case fb_fmt_RGB565:
        return dst_fmt == ROTATOR_RGB565;
        break;
    case fb_fmt_RGB888:
        return dst_fmt == ROTATOR_RGB888;
        break;
    case fb_fmt_ARGB8888:
        return dst_fmt == ROTATOR_ARGB8888;
        break;
    case fb_fmt_yuv422:
        return dst_fmt == ROTATOR_YUV422;
        break;
    case fb_fmt_NV21:
    case fb_fmt_NV12:
        return dst_fmt == ROTATOR_NV12;
        break;
    default:
        return 0;
        break;
    }
}

static int play_on_device(char *device, void *src_buf, rotate_config *config)
{
    struct fb_device_info fb_info;
    int fb_fd;

    fb_fd = fb_open(device, &fb_info);
    if (fb_fd < 0) {
        fprintf(stderr, "%s open fail! ignored\n", device);
        return -1;
    }
    if (!support_fb_fmt(fb_info.fb_fmt, config->dst_fmt)) {
        fprintf(stderr, "fb format not comportable with rotated format\n");
        return -1;
    }

    int rotate_bpp = rotator_bytes_per_pixel(config->dst_fmt);
    int frame_bpp = fb_bytes_per_pixel(fb_info.fb_fmt);
    int display_x = SHORTER(fb_info.xres, config->frame_width);
    int display_y = SHORTER(fb_info.yres, config->frame_height);
    if (ROTATE_X_Y(config->rotate_angle)) {
        display_x = SHORTER(fb_info.xres, config->frame_height);
        display_y = SHORTER(fb_info.yres, config->frame_width);
    }
    char *mmap_tra = src_buf;
    char *mmap_fb_tra = fb_info.mapped_mem;

    for (int i = 0; i < display_y; i++) {
        for (int j = 0; j < display_x; j++) {
            // fb buffer is Little-Endian (each pixel) and 16 /32 aligned
            for (int z = 0; z < rotate_bpp; z++) {
                *(mmap_fb_tra + (frame_bpp - z - 1)) = *mmap_tra++;
            }
            mmap_fb_tra += frame_bpp;
        }
        mmap_fb_tra += fb_info.line_length - display_x * frame_bpp;
        if (ROTATE_X_Y(config->rotate_angle))
            mmap_tra += config->frame_height * rotate_bpp - display_x * frame_bpp;
        else
            mmap_tra += config->frame_width * rotate_bpp - display_x * frame_bpp;
    }
    fb_enable(fb_fd);
    fb_pan_display(fb_fd, &fb_info, 0);
    fb_close(fb_fd, &fb_info);
    return 0;
}

// void static debug_para(rotate_config data)
// {
//     printf("src_buf: %p\n", data.src_buf);
//     printf("dst_buf: %p\n", data.dst_buf);
//     printf("frame_height: %u\n", data.frame_height);
//     printf("frame_width: %u\n", data.frame_width);
//     printf("src_stride: %u\n", data.src_stride);
//     printf("dst_stride: %u\n", data.dst_stride);
//     printf("src_fmt: %d\n", data.src_fmt);
//     printf("convert_color: %d\n", data.convert_color);
//     printf("dst_fmt: %d\n", data.dst_fmt);
//     printf("horizontal_mirror: %d\n", data.horizontal_mirror);
//     printf("vertical_mirror: %d\n", data.vertical_mirror);
//     printf("rotate_angle: %d\n", data.rotate_angle);
// }

void filling_src_data_rgb565(unsigned int src_xres, unsigned int src_yres, void *mem);
void filling_src_data_argb8888(unsigned int src_xres, unsigned int src_yres, void *mem);

int rotator_no_input(char *device)
{
    int ret = 0;
    struct fb_device_info fb_info;
    struct rotator_config_data rotator_config;

    int rmem_fd = rmem_open();
    if (rmem_fd < 0)
        return -1;

    int rotator_fd = rotator_open();
    if (rotator_fd < 0)
        return -1;

    if (device == NULL) {
        device = "/dev/fb0";
    }
    int fb_fd = fb_open(device, &fb_info);
    if (fb_fd < 0) {
        fprintf(stderr, "%s open fail!\n", device);
        return -1;
    }

    void *mmap_src = NULL;
    unsigned long src_phy;
    unsigned int pixel_byte;

    unsigned int src_xres = fb_info.yres;
    unsigned int src_yres = fb_info.xres;

    if (fb_info.fb_fmt == fb_fmt_RGB565) {
        pixel_byte = 2;
        mmap_src = rmem_alloc(rmem_fd, &src_phy, src_xres * src_yres * 2);
        if (mmap_src == NULL)
            return -1;

        filling_src_data_rgb565(src_xres, src_yres, mmap_src);
        rotator_config.src_fmt = ROTATOR_RGB565;    //源数据格式
        rotator_config.dst_fmt = ROTATOR_RGB565;    //目标数据格式

    } else if (fb_info.fb_fmt == fb_fmt_ARGB8888 || fb_info.fb_fmt == fb_fmt_RGB888) {
        pixel_byte = 4;
        mmap_src = rmem_alloc(rmem_fd, &src_phy, src_xres * src_yres * 4);
        if (mmap_src == NULL)
            return -1;

        filling_src_data_argb8888(src_xres, src_yres, mmap_src);
        rotator_config.src_fmt = ROTATOR_ARGB8888;  //源数据格式
        rotator_config.dst_fmt = ROTATOR_ARGB8888;  //目标数据格式

    } else {
        fprintf(stderr, "fmt %d is not support!\n", fb_info.fb_fmt);
        return -1;
    }

    unsigned int src_line_length = src_xres * pixel_byte;
    unsigned int dst_line_length = fb_info.line_length;

    rotator_config.frame_width        = src_xres;
    rotator_config.frame_height       = src_yres;
    rotator_config.src_stride         = src_line_length / pixel_byte;
    rotator_config.dst_stride         = dst_line_length / pixel_byte;
    rotator_config.convert_color      = ROTATOR_ORDER_RGB_TO_RGB; //RGB→RGB
    rotator_config.horizontal_mirror  = ROTATOR_NO_MIRROR;        //不水平翻转
    rotator_config.vertical_mirror    = ROTATOR_NO_MIRROR;        //不垂直翻转
    rotator_config.rotate_angle       = ROTATOR_ANGLE_90;         //90度
    rotator_config.src_buf            = (void *)src_phy;
    rotator_config.dst_buf            = (void *)fb_info.fix.smem_start;

    rotator_complete_conversion(rotator_fd, &rotator_config);

    fb_enable(fb_fd);
    fb_pan_display(fb_fd, &fb_info, 0);

    fb_close(fb_fd, &fb_info);
    rotator_close(rotator_fd);
    rmem_close(rmem_fd);

    return ret;
}

void filling_src_data_rgb565(unsigned int src_xres, unsigned int src_yres, void *mem)
{
    int i,j;
    unsigned short *p = mem;

    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0xf800;
        }
    }
    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0x07e0;
        }
    }
    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0x001f;
        }
    }
}


void filling_src_data_argb8888(unsigned int src_xres, unsigned int src_yres, void *mem)
{
    int i,j;
    unsigned int *p = mem;

    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0xffff0000;
        }
    }
    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0xff00ff00;
        }
    }
    for (i = 0; i < src_yres / 3; i++) {
        for (j = 0; j < src_xres; j++) {
            *p++=0xff0000ff;
        }
    }
}

int main(int argc, char **argv)
{
    int ret = 0;
    int rmem_fd = 0;
    int rotator_fd = 0;
    FILE *input_file = NULL;
    FILE *output_file = NULL;
    long input_size = 0;
    char *input_path = NULL;
    char *output_path = NULL;
    char *device = "/dev/fb0";
    void *mmap_src = NULL;
    void *mmap_dst = NULL;
    unsigned long src_phy = 0;
    unsigned long dst_phy = 0;
    rotate_config config;
    memset((void *)&config, 0, sizeof(rotate_config));

    if (argc < 6) {
        if (argc == 1) {
            ret = rotator_no_input(NULL);
            return ret;
        }
        if (argc == 2) {
            if (parse_string((const char *)argv[1], "preview=", &device))
                ret = rotator_no_input(device);
            if (strcmp((const char *)argv[1], "-h") == 0 ||
                strcmp((const char *)argv[1], "--help") == 0 ||
                strcmp((const char *)argv[1], "help") == 0)
                cmd_usage(argv[0]);
            return ret;
        }
        fprintf(stderr, "error: parameters counts error\n");
        cmd_usage(argv[0]);
        goto exit_err;
    }

    ret = parse_cmd_info((const int)argc, (const char **)argv, &config,
                         &input_path, &output_path, &device);
    if (ret < 0) {
        fprintf(stderr, "error: parameters is not valid\n");
        goto exit_err;
    }

    rmem_fd = rmem_open();
    if (rmem_fd < 0)
        goto exit_err;

    rotator_fd = rotator_open();
    if (rotator_fd < 0)
        goto rotator_open_err;

    input_file = fopen(input_path, "rb");
    if (!input_file) {
        fprintf(stderr, "%s: open file %s error.\n", argv[0], input_path);
        goto input_open_err;
    }

    output_file = fopen(output_path, "wb");
    if (!output_file) {
        fprintf(stderr, "%s: open file %s error.\n", argv[0], output_path);
        goto output_open_err;
    }

    fseek(input_file, 0, SEEK_END);
    input_size = ftell(input_file);
    rewind(input_file);

    if (input_size <= 0) {
        fprintf(stderr, "%s: mem alloc size error, input=%ld, output=%ld\n",
                argv[0], input_size, input_size);
        goto mem_alloc_err;
    }

    mmap_src = rmem_alloc(rmem_fd, &src_phy, input_size);
    mmap_dst = rmem_alloc(rmem_fd, &dst_phy, input_size);

    fread(mmap_src, sizeof(unsigned char), input_size, input_file);

    config.src_buf = (void *)src_phy;
    config.dst_buf = (void *)dst_phy;

    // debug_para(config);

    rotator_complete_conversion(rotator_fd, &config);

    fwrite(mmap_dst, sizeof(char), input_size, output_file);

    play_on_device(device, mmap_dst, &config);

    rmem_free(rmem_fd, mmap_src, src_phy, input_size);
    rmem_free(rmem_fd, mmap_dst, dst_phy, input_size);

mem_alloc_err:
    fclose(output_file);
output_open_err:
    fclose(input_file);
input_open_err:
    rotator_close(rotator_fd);
rotator_open_err:
    rmem_close(rmem_fd);
exit_err:
    return ret;
}
