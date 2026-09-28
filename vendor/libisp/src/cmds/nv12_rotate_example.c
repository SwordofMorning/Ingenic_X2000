/*
 * NV12 旋转示例
 * 该文件不参与编译，可复制到 isp_main.c 测试功能
 * 注意：lisisp_cmds.mk 需添加 LDFLAGS-y := -L../libhardware2/output/ -lhardware2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <signal.h>
#include <isp.h>

#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <libhardware2/rmem.h>
#include <libhardware2/rotator.h>
#define ALIGN(x, n) (((x) + (n) - 1) - ((x) + (n) - 1) % (n))
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))


enum {
    cmd_init,
    cmd_power_on,
    cmd_power_off,
    cmd_stream_on,
    cmd_stream_off,
    cmd_info,
    cmd_get_frame,
    cmd_drop_all,
    cmd_deinit,
};

static struct camera_info cam;
static struct camera_info cfg_info;

static struct frame_image_format output_fmt = {
    .pixel_format       = CAMERA_PIX_FMT_NV12,
    .frame_nums         = 2,

    .scaler.enable      = 0,
    .crop.enable        = 0,
};

static const char *prg_name;

static const unsigned int support_fmts[] = {
        CAMERA_PIX_FMT_NV12,
        CAMERA_PIX_FMT_NV21,
        CAMERA_PIX_FMT_YVU420,
        CAMERA_PIX_FMT_JZ420B,
        CAMERA_PIX_FMT_GREY,
};

static void usage(int status)
{
    int i;
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                        : show help info\n");

    fprintf(stderr, "    support format:\n");
    for (i = 0; i < ARRAY_SIZE(support_fmts); i++) {
        char fmt_a = (char)(support_fmts[i] >> 0);
        char fmt_b = (char)(support_fmts[i] >> 8);
        char fmt_c = (char)(support_fmts[i] >> 16);
        char fmt_d = (char)(support_fmts[i] >> 24);
        fprintf(stderr, "        %C%C%C%C\n", fmt_a, fmt_b, fmt_c, fmt_d);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "    init device_path [width=value] [height=value] [frame_nums] [format]: config camera\n");
    fprintf(stderr, "    Example: %s init /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "             %s init /dev/mscaler0-ch0  width=640 height=480 frame_nums=3 format=nv12\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    power_on device_path            : power on camera\n");
    fprintf(stderr, "    Example: %s power_on /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    power_off device_path           : power off camera\n");
    fprintf(stderr, "    Example: %s power_off /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    stream_on device_path           : stream on camera, start capture\n");
    fprintf(stderr, "    Example: %s stream_on /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    stream_off device_path          : stream off camera, stop capture\n");
    fprintf(stderr, "    Example: %s stream_off /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    info device_path                : show camera info\n");
    fprintf(stderr, "    Example: %s info /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    get_frame device_path           : get one frame data to stdout\n");
    fprintf(stderr, "    Example: %s get_frame /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    drop_all_frames device_path     : drop all captured frames\n");
    fprintf(stderr, "    Example: %s drop_all_frames /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    deinit device_path              : drop all captured frames\n");
    fprintf(stderr, "    Example: %s deinit /dev/mscaler0-ch0 \n", prg_name);
    fprintf(stderr, "\n");
    exit(status);
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

static void recalc_output_fmt(int fd)
{
    struct camera_info sensor_info;
    int crop_enable = 1;
    int width = cfg_info.width;
    int height = cfg_info.height;

    isp_get_sensor_info(fd, &sensor_info);

    if (width > sensor_info.width || width <= 0)
        width = sensor_info.width;

    if (height > sensor_info.height || height <= 0)
        height = sensor_info.height;

    if (width % 16) {
        width = ALIGN(width, 16);
        if (width > sensor_info.width)
            width = width - 16;
    }

    if (width == sensor_info.width && height == sensor_info.height)
        crop_enable = 0;

    if (cfg_info.frame_nums <= 0)
        cfg_info.frame_nums = 3;

    if (cfg_info.data_fmt < 0)
        cfg_info.data_fmt = CAMERA_PIX_FMT_NV12;

    output_fmt.pixel_format = cfg_info.data_fmt;
    output_fmt.frame_nums = cfg_info.frame_nums;
    output_fmt.scaler.enable = 0;
    output_fmt.crop.enable = crop_enable;
    output_fmt.crop.top = (sensor_info.height - height) / 2;
    output_fmt.crop.left = (sensor_info.width - width) / 2;
    output_fmt.crop.width = width;
    output_fmt.crop.height = height;
    output_fmt.width = width;
    output_fmt.height = height;
}

static int output_fmt_parse_int(const char *str, const char *prefix, int *value, int base)
{
    int i;
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    for (i = 0; i < ARRAY_SIZE(support_fmts); i++) {
        char fmt_a = str[len + 0];
        char fmt_b = str[len + 1];
        char fmt_c = str[len + 2];
        char fmt_d = str[len + 3];
        unsigned int tmp_fmt = camera_fourcc(fmt_a, fmt_b, fmt_c, fmt_d);

        if (tmp_fmt != support_fmts[i])
            continue;

        *value = tmp_fmt;
        return 1;
    }

    fprintf(stderr, "error: is not valid: %s\n", str);
    usage(-1);

    return 0;
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

int main(int argc, char *argv[])
{
    int ret = 0;
    int fd = -1;
    int width = -1;
    int height = -1;
    int frame_nums = -1;
    int data_fmt = -1;
    int i, cmd;

    prg_name = argv[0];

    while (1) {
        /* 参数检查 */
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "init")) {
            if (argc < 3)
                usage(-1);

            for (i = 3; i < argc; i++) {
                if (parse_int(argv[i], "width=", &width, 10))
                    continue;

                if (parse_int(argv[i], "height=", &height, 10))
                    continue;

                if (parse_int(argv[i], "frame_nums=", &frame_nums, 10))
                    continue;

                if (output_fmt_parse_int(argv[i], "format=", &data_fmt, 20))
                    continue;

                error_arg(argv[i]);
            }

            cfg_info.width = width;
            cfg_info.height = height;
            cfg_info.frame_nums = frame_nums;
            cfg_info.data_fmt = data_fmt;

            cmd = cmd_init;
            break;
        }

        if (!strcmp(argv[1], "power_on")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_power_on;
            break;
        }

        if (!strcmp(argv[1], "power_off")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_power_off;
            break;
        }

        if (!strcmp(argv[1], "stream_on")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_stream_on;
            break;
        }

        if (!strcmp(argv[1], "stream_off")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_stream_off;
            break;
        }

        if (!strcmp(argv[1], "info")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_info;
            break;
        }

        if (!strcmp(argv[1], "get_frame")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_get_frame;
            break;
        }

        if (!strcmp(argv[1], "drop_all_frames")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_drop_all;
            break;
        }

        if (!strcmp(argv[1], "deinit")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_deinit;
            break;
        }

        error_arg(argv[1]);
    }

    fd = isp_open(argv[2]);
    if (fd == -ENODEV)
        return -ENODEV;

    if (cmd == cmd_init) {
        recalc_output_fmt(fd);
        ret = isp_set_format(fd, &output_fmt);
        if (!ret)
            ret = isp_requset_buffer(fd, &output_fmt);
        goto close_fd;
    }

    isp_mmap(fd, &cam);

    if (cmd == cmd_power_on) {
        ret = isp_power_on(fd);
        goto close_fd;
    }

    if (cmd == cmd_power_off) {
        ret = isp_power_off(fd);
        goto close_fd;
    }

    if (cmd == cmd_stream_on) {
        ret = isp_stream_on(fd);
        goto close_fd;
    }

    if (cmd == cmd_stream_off) {
        ret = isp_stream_off(fd);
        goto close_fd;
    }

    if (cmd == cmd_info) {
        ret = isp_get_info(fd, &cam);
        char fmt_a = (char)(cam.data_fmt >> 0);
        char fmt_b = (char)(cam.data_fmt >> 8);
        char fmt_c = (char)(cam.data_fmt >> 16);
        char fmt_d = (char)(cam.data_fmt >> 24);
        printf("name        : %s\n", cam.name);
        printf("width       : %d\n", cam.width);
        printf("height      : %d\n", cam.height);
        printf("fps         : %d\n", cam.fps);
        printf("data_fmt    : %C%C%C%C\n", fmt_a, fmt_b, fmt_c, fmt_d);
        printf("line_length : %d\n", cam.line_length);
        printf("frame_size  : %d\n", cam.frame_size);
        printf("frame_nums  : %d\n", cam.frame_nums);
        printf("phys_mem    : %08lx\n", cam.phys_mem);
        printf("mapped_mem  : %p\n", cam.mapped_mem);
        goto close_fd;
    }

    if (cmd == cmd_get_frame) {
        void *mem = isp_wait_frame(fd);

        struct rotator_config_data y_rotator_config;

        int rmem_fd = rmem_open();
        if (rmem_fd < 0)
            exit(-1);
        int rotator_fd = rotator_open();
        if (rotator_fd < 0)
            exit(-1);

        void *mmap_dst;
        unsigned long dst_phy;
        unsigned int y_pixel_byte;

        y_pixel_byte = 1;
        y_rotator_config.src_fmt = ROTATOR_NV12;
        y_rotator_config.dst_fmt = ROTATOR_NV12;

        mmap_dst = rmem_alloc(rmem_fd, &dst_phy, cam.width * cam.height * y_pixel_byte * 1.5);
        if (mmap_dst == NULL)
            exit(-1);

        unsigned int y_src_line_length = cam.width * y_pixel_byte;
        unsigned int y_dst_line_length = cam.height * y_pixel_byte;

        y_rotator_config.frame_width        = cam.width;
        y_rotator_config.frame_height       = cam.height;
        y_rotator_config.src_stride         = y_src_line_length / y_pixel_byte;
        y_rotator_config.dst_stride         = y_dst_line_length / y_pixel_byte;
        y_rotator_config.convert_color      = ROTATOR_ORDER_RGB_TO_RGB; //RGB→RGB
        y_rotator_config.horizontal_mirror  = ROTATOR_NO_MIRROR;        //不水平翻转
        y_rotator_config.vertical_mirror    = ROTATOR_NO_MIRROR;        //不垂直翻转
        y_rotator_config.rotate_angle       = ROTATOR_ANGLE_270;         //90度
        y_rotator_config.src_buf            = (void *)cam.phys_mem;
        y_rotator_config.dst_buf            = (void *)dst_phy;

        rotator_complete_conversion(rotator_fd, &y_rotator_config);

        memcpy(mem, mmap_dst, cam.width * cam.height * y_pixel_byte * 1.5);
        rotator_close(rotator_fd);
        rmem_close(rmem_fd);

        if (mem)
            fwrite(mem, 1, cam.frame_size, stdout);

        isp_put_frame(fd, mem);
        goto close_fd1;
    }

    if (cmd == cmd_drop_all) {
        ret = isp_drop_frames(fd, cam.frame_nums);
        goto close_fd;
    }

    if (cmd == cmd_deinit) {
        ret = isp_free_buffer(fd);
        goto close_fd;
    }

close_fd:
    isp_close(fd, &cam);
    return ret;
close_fd1:
    close(fd);

    return ret;
}
