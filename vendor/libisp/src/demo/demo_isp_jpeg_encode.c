#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <isp.h>
#include <libhardware2/v4l2_jpeg_encode.h>

static struct camera_info info;

/* isp格式配置 */
static struct frame_image_format output_fmt = {
    .width              = 0,
    .height             = 0,
    .pixel_format       = CAMERA_PIX_FMT_NV12,
    .frame_nums         = 2,
};

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    output=            : the file to save encoded JPEG data stream, must be set\n");
    fprintf(stderr, "    input=             : the dev path for camera, default /dev/mscaler0-ch0\n");
    fprintf(stderr, "    encoder=           : the V4L2 video dev path for JPEG encode, default /dev/video1\n");
    fprintf(stderr, "    Example: %s output=/tmp/isp.jpeg\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
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

    output_fmt.width = info.width;
    output_fmt.height = info.height;

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

int main(int argc, char *argv[])
{
    int ret = 0;
    int output_size = 0;
    int camera_fd;

    prg_name = argv[0];
    unsigned char *isp_path = malloc(20);
    unsigned char *file_path = malloc(20);
    unsigned char *encoder_path = malloc(20);
    memmove(isp_path, "/dev/mscaler0-ch0", 18);
    memmove(encoder_path, "/dev/video1", 12);
    memset(file_path, 0, 20);

    struct v4l2_jpeg_encoder *encoder;

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
        error_arg(argv[i]);
    }

    if (strlen(file_path) == 0) {
        fprintf(stderr, "[err] output must be set up\n");
        return -1;
    }

    fprintf(stderr, "file_path    : %s\n", file_path);
    fprintf(stderr, "input_path   : %s\n", isp_path);
    fprintf(stderr, "encoder_path : %s\n", encoder_path);

    camera_fd = init_isp_camera(isp_path);
    if (camera_fd < 0) {
        fprintf(stderr, "Init isp camera failed\n");
        return camera_fd;
    }

    struct v4l2_jpeg_encoder_config config;
    memset(&config, 0, sizeof(struct v4l2_jpeg_encoder_config));
    config.video_path = encoder_path;
    config.width = info.width;
    config.height = info.height;
    config.line_length = info.line_length;
    config.input_fmt = info.data_fmt;
    config.quality = 2;

    encoder = v4l2_jpeg_encoder_open(&config);
    if (!encoder) {
        fprintf(stderr, "Uanble to open v4l2 jpeg encoder\n");
        ret = -1;
        goto close_isp_cam;
    }

    int file_fd = open(file_path, O_RDWR | O_CREAT | O_TRUNC);
    if (file_fd < 0) {
        fprintf(stderr, "open file %s error.\n", file_path);
        goto close_encoder;
    }

    void *frame = isp_wait_frame(camera_fd);
    if (!frame) {
        ret = -1;
        goto close_file;
    }

    void *mem = v4l2_jpeg_encoder_work(encoder, frame, &output_size);
    if (!mem) {
        fprintf(stderr, "V4l2 jpeg encode work failed, ret : %d, %p\n", ret, mem);
        ret = -1;
        goto close_file;
    }
    isp_put_frame(camera_fd, frame);

    write(file_fd, mem, output_size);

close_file:
    close(file_fd);

close_encoder:
    v4l2_jpeg_encoder_close(encoder);

close_isp_cam:
    deinit_isp_camera(camera_fd);

    return ret;
}
