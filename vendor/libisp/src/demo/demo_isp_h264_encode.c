#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <isp.h>
#include <libhardware2/v4l2_h264_encode.h>

static struct camera_info info;
static int camera_fd;
static int file_fd1;
struct v4l2_h264_encoder *encoder;

/* isp格式配置 */
static struct frame_image_format output_fmt = {
    .width              = 0,
    .height             = 0,
    .pixel_format       = CAMERA_PIX_FMT_NV12,

    .scaler.enable      = 0,
    .scaler.width       = 0,
    .scaler.height      = 0,

    .frame_nums         = 2,
};

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    input=             : the dev path for camera, default /dev/mscaler0-ch0\n");
    fprintf(stderr, "    output=            : the file to save encoded H264 data stream, must be set\n");
    fprintf(stderr, "    encoder=           : the V4L2 video dev path for H264 encode, default /dev/video1\n");
    fprintf(stderr, "    max_frame=         : size of frames will be encoded, default 30\n");
    fprintf(stderr, "    gop_size=          : size between each keyframe, default 10\n");
    fprintf(stderr, "    bitrate=           : bit rate of frame data transmission, default 400000\n");
    fprintf(stderr, "    width=             : width of each frame, please align 16, default sensor size\n");
    fprintf(stderr, "    height=            : height of each frame, please align 16, default sensor size\n");
    fprintf(stderr, "           if set width and height will scaling the frame\n");
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

static int init_isp_camera(char * isp_cam_path)
{
    int ret;
    int camera_fd;
    camera_fd = isp_open(isp_cam_path);
    if (camera_fd < 0) {
        fprintf(stderr, "isp open failed\n");
        return -1;
    }

    ret = isp_get_sensor_info(camera_fd, &info);
    if (ret < 0) {
        fprintf(stderr, "isp get sensor info failed\n");
        goto close_isp;
    }

    if (output_fmt.width != 0 || output_fmt.height != 0)
        output_fmt.scaler.enable = 1;

    output_fmt.scaler.width = output_fmt.width ? output_fmt.width : info.width;
    output_fmt.scaler.height = output_fmt.height ? output_fmt.height : info.height;

    output_fmt.width = output_fmt.scaler.width;
    output_fmt.height = output_fmt.scaler.height;

    ret = isp_set_format(camera_fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set format failed\n");
        goto close_isp;
    }

    ret = isp_requset_buffer(camera_fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set request buffer failed\n");
        goto close_isp;
    }

    isp_get_info(camera_fd, &info);

    ret = isp_mmap(camera_fd, &info);
    if (ret < 0)
        goto free_isp_buffer;

    ret = isp_power_on(camera_fd);
    if (ret < 0)
        goto free_isp_buffer;

    ret = isp_stream_on(camera_fd);
    if (ret < 0)
        goto power_off_isp;

    return camera_fd;

power_off_isp:
    isp_power_off(camera_fd);
free_isp_buffer:
    isp_free_buffer(camera_fd);
close_isp:
    isp_close(camera_fd, &info);

    return ret;
}

static void deinit_isp_camera(int camera_fd)
{
    isp_drop_frames(camera_fd, info.frame_nums);

    isp_stream_off(camera_fd);

    isp_power_off(camera_fd);

    isp_free_buffer(camera_fd);

    isp_close(camera_fd, &info);
}

static void signal_handler(int signum)
{
    close(file_fd1);

    v4l2_h264_encoder_close(encoder);

    deinit_isp_camera(camera_fd);

    file_fd1 = -1;
    camera_fd = -1;

    exit(-1);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int output_size = 0;
    int gop_size = 10, max_frame = 30, single_frame = 0;
    int width = 0, height = 0, bitrate = 400000;

    prg_name = argv[0];
    unsigned char *isp_path = malloc(20);
    unsigned char *file_path = malloc(20);
    unsigned char *encoder_path = malloc(20);
    memmove(isp_path, "/dev/mscaler0-ch0", 18);
    memmove(encoder_path, "/dev/video1", 12);
    memset(file_path, 0, 20);

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    int i;
    for (i = 1; i < argc; i++) {
        if (parse_ustr(argv[i], "input=", isp_path, 20))
            continue;
        if (parse_ustr(argv[i], "output=", file_path, 20))
            continue;
        if (parse_ustr(argv[i], "encoder=", encoder_path, 20))
            continue;
        if (parse_uint(argv[i], "max_frame=", &max_frame, 10))
            continue;
        if (parse_uint(argv[i], "gop_size=", &gop_size, 10))
            continue;
        if (parse_uint(argv[i], "width=", &width, 10))
            continue;
        if (parse_uint(argv[i], "height=", &height, 10))
            continue;
        if (parse_uint(argv[i], "bitrate=", &bitrate, 10))
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
    fprintf(stderr, "input_path   : %s\n", isp_path);
    fprintf(stderr, "encoder_path : %s\n", encoder_path);
    fprintf(stderr, "max_frame    : %d\n", max_frame);
    fprintf(stderr, "gop_size     : %d\n", gop_size);
    fprintf(stderr, "bitrate      : %d\n", bitrate);
    fprintf(stderr, "single_frame : %d\n", single_frame);

    output_fmt.width = width;
    output_fmt.height = height;

    signal(SIGINT, signal_handler);

    camera_fd = init_isp_camera(isp_path);
    if (camera_fd < 0) {
        fprintf(stderr, "Init isp camera failed\n");
        return camera_fd;
    }

    struct v4l2_h264_encoder_config config;
    memset(&config, 0, sizeof(struct v4l2_h264_encoder_config));
    config.video_path = encoder_path;
    config.width = output_fmt.width;
    config.height = output_fmt.height;
    config.line_length = info.line_length;
    config.input_fmt = info.data_fmt;
    config.gop_size = gop_size;
    config.bitrate = bitrate;

    fprintf(stderr, "width        : %d\n", config.width);
    fprintf(stderr, "height       : %d\n", config.height);

    encoder = v4l2_h264_encoder_open(&config);
    if (!encoder) {
        fprintf(stderr, "Uanble to open v4l2 h264 encoder\n");
        ret = -1;
        goto close_isp_cam;
    }

    unsigned char buf[20];
    unsigned int t = 0;

    snprintf(buf, sizeof(buf), "%s00", file_path);
    file_fd1 = open(buf, O_RDWR | O_CREAT | O_TRUNC);
    if (file_fd1 < 0) {
        fprintf(stderr, "open file %s error.\n", buf);
        goto close_encoder;
    }

    struct frame_info frame_info;
    while (1) {
        ret = isp_dqbuf_wait(camera_fd, &frame_info);
        if (ret)
            goto close_file;

        if (t == 5)
            v4l2_h264_encoder_set_keyframe(encoder);

        /**
         * 使用v4l2_h264_encoder_work接口，底层会分配空间作为vpu的输入.
         *
         * 使用v4l2_h264_encoder_work_by_phy_mem接口,
         *          底层直接使用inputmem的内存作为VPU输入源,内存占用更少.(但要求mem必须是物理上连续的)
        */

        // void *mem = v4l2_h264_encoder_work(encoder, frame, &output_size);
        void *mem = v4l2_h264_encoder_work_by_phy_mem(encoder, frame_info.vaddr, frame_info.paddr, &output_size);
        if (!mem) {
            fprintf(stderr, "V4l2 h264 encode work failed, ret : %d, %p\n", ret, mem);
            ret = -1;
            goto close_file;
        }
        isp_put_frame(camera_fd, frame_info.vaddr);

        if (single_frame) {
            snprintf(buf, sizeof(buf), "%s.%d", file_path, t);
            int file_fd = open(buf, O_RDWR | O_CREAT | O_TRUNC);
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

close_isp_cam:
    deinit_isp_camera(camera_fd);

    file_fd1 = -1;
    camera_fd = -1;

    return ret;
}
