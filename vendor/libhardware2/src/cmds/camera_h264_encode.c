#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <libhardware2/camera.h>
#include <libhardware2/v4l2_h264_encode.h>
#include <libutils2/simple_bayer16_to_nv12.h>

static struct camera_info info;

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    input=             : the dev path for camera, default /dev/vic0\n");
    fprintf(stderr, "    output=            : the file to save encoded H264 data stream, must be set\n");
    fprintf(stderr, "    encoder=           : the V4L2 video dev path for H264 encode, default /dev/video1\n");
    fprintf(stderr, "    max_frame=         : size of frames will be encoded, default 30\n");
    fprintf(stderr, "    gop_size=          : size between each keyframe, default 10\n");
    fprintf(stderr, "    single_frame=      : save each frame separately or not, default 0\n");
    fprintf(stderr, "    Example: %s output=/tmp/output\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
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

static int parse_ustr(const char *src, const char *prefix,
                    unsigned char *dest, int base)
{
    int len = strlen(prefix);

    if (strncmp(src, prefix, len) || ((strlen(src) - len) >= base))
        return 0;

    memmove(dest, src + len, base);

    return 1;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

int main(int argc, char *argv[])
{
    int cam_fd;
    int ret = 0;
    int output_size;
    struct v4l2_h264_encoder *encoder;
    int gop_size = 10, max_frame = 30, single_frame = 0;;

    prg_name = argv[0];
    unsigned char cam_path[20];
    unsigned char file_path[20];
    unsigned char encoder_path[20];
    memmove(cam_path, "/dev/vic0", 10);
    memmove(encoder_path, "/dev/video1", 12);
    memset(file_path, 0, 20);

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    int i;
    for (i = 1; i < argc; i++) {
        if (parse_ustr(argv[i], "input=", cam_path, 20))
            continue;
        if (parse_ustr(argv[i], "output=", file_path, 20))
            continue;
        if (parse_ustr(argv[i], "encoder=", encoder_path, 20))
            continue;
        if (parse_uint(argv[i], "max_frame=", &max_frame, 10))
            continue;
        if (parse_uint(argv[i], "gop_size=", &gop_size, 10))
            continue;
        if (parse_uint(argv[i], "single_frame=", &single_frame, 10))
            continue;
        error_arg(argv[i]);
    }

    if (strlen(file_path) == 0) {
        fprintf(stderr, "[err] output must be set up\n");
        return -1;
    }

    fprintf(stderr, "file_path    : %s*\n", file_path);
    fprintf(stderr, "input_path   : %s\n", cam_path);
    fprintf(stderr, "encoder_path : %s\n", encoder_path);
    fprintf(stderr, "max_frame    : %d\n", max_frame);
    fprintf(stderr, "gop_size     : %d\n", gop_size);
    fprintf(stderr, "single_frame : %d\n", single_frame);

    cam_fd = camera_open(&info, cam_path);
    if (cam_fd < 0)
        return -1;

    ret = camera_power_on(cam_fd);
    if (ret < 0)
        goto close_cam;

    ret = camera_stream_on(cam_fd);
    if (ret < 0)
        goto close_power;

    unsigned int input_fmt = info.data_fmt;
    unsigned int line_length = 0;
    switch (input_fmt) {
    case CAMERA_PIX_FMT_Y16:
    case CAMERA_PIX_FMT_SBGGR16:
    case CAMERA_PIX_FMT_SGBRG16:
    case CAMERA_PIX_FMT_SGRBG16:
    case CAMERA_PIX_FMT_SRGGB16:
        line_length = info.line_length / 2;
        input_fmt = CAMERA_PIX_FMT_NV12;
        break;
    case CAMERA_PIX_FMT_NV12:
    case CAMERA_PIX_FMT_NV21:
        line_length = info.line_length;
        break;
    default:
        ret = -1;
        fprintf(stderr, "Unsupport input format : %x\n", info.data_fmt);
        goto close_stream;
    }

    struct v4l2_h264_encoder_config config;
    memset(&config, 0, sizeof(struct v4l2_h264_encoder_config));
    config.video_path = encoder_path;
    config.width = info.width;
    config.height = info.height;
    config.line_length = line_length;
    config.input_fmt = input_fmt;
    config.gop_size = gop_size;
    config.bitrate = 400000;

    encoder = v4l2_h264_encoder_open(&config);
    if (!encoder) {
        fprintf(stderr, "Unable to open v4l2 h264 encoder\n");
        ret = -1;
        goto close_stream;
    }

    unsigned char buf[22];
    unsigned int t = 0;

    snprintf(buf, sizeof(buf), "%s00", file_path);
    int file_fd1 = open(buf, O_RDWR | O_CREAT | O_TRUNC);/* 存放全帧视频 */
    if (file_fd1 < 0) {
        fprintf(stderr, "open file %s error.\n", buf);
        goto close_encoder;
    }

    while (1) {
        void *frame = camera_wait_frame(cam_fd);
        if (!frame) {
            ret = -1;
            goto close_file;
        }

        switch (info.data_fmt) {
        case CAMERA_PIX_FMT_Y16:
        case CAMERA_PIX_FMT_SBGGR16:
        case CAMERA_PIX_FMT_SGBRG16:
        case CAMERA_PIX_FMT_SGRBG16:
        case CAMERA_PIX_FMT_SRGGB16:
            simple_bayer16_to_nv12(frame, frame, info.width, info.height, info.line_length);
            break;
        default:
            break;
        }

        if (t == 5)
            v4l2_h264_encoder_set_keyframe(encoder);

        void *mem = v4l2_h264_encoder_work(encoder, frame, &output_size);
        if (!mem) {
            fprintf(stderr, "V4l2 h264 encode work failed\n");
            ret = -1;
            goto close_file;
        }
        camera_put_frame(cam_fd, frame);

        if (single_frame) {
            snprintf(buf, sizeof(buf), "%s.%d", file_path, t);
            int file_fd = open(buf, O_RDWR | O_CREAT | O_TRUNC);/* 单独存放每一帧 */
            if (file_fd < 0) {
                fprintf(stderr, "open file %s error.\n", file_path);
                goto close_file;
            }
            write(file_fd, mem, output_size);
            close(file_fd);
        }

        write(file_fd1, mem, output_size);

        if (t++ >= max_frame) break;
    }

close_file:
    close(file_fd1);

close_encoder:
    v4l2_h264_encoder_close(encoder);

close_stream:
    camera_stream_off(cam_fd);

close_power:
    camera_power_off(cam_fd);

close_cam:
    camera_drop_frames(cam_fd, info.frame_nums);

    camera_close(cam_fd, &info);

    return ret;
}
