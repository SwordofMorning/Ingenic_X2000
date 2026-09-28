#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <isp.h>
#include <libhardware2/avpu_h264_encode.h>


#define ALIGN_DOWN(n, align)         (n & (~(align - 1)))
#define DEFAULT_ISP_DEVICE           "/dev/mscaler-ch0"

static struct camera_info info;
static int camera_fd;
static int file_fd1;
struct avpu_h264_encoder *encoder;

/* isp格式配置 */
static struct frame_image_format output_fmt = {
    .width              = 0,
    .height             = 0,
    .pixel_format       = CAMERA_PIX_FMT_NV12,

    .scaler.enable      = 0,
    .scaler.width       = 0,
    .scaler.height      = 0,

    .crop.enable        = 0,
    .crop.width         = 0,
    .crop.height        = 0,
    .crop.left          = 0,
    .crop.top           = 0,

    .frame_nums         = 2,
};

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help      : show help info\n");
    fprintf(stderr, "    input=         : the dev path for camera, default /dev/mscaler-ch0\n");
    fprintf(stderr, "    output=        : the file to save encoded H264 data stream, must be set\n");
    fprintf(stderr, "    max_frame=     : size of frames will be encoded, default 30\n");
    fprintf(stderr, "    gop_size=      : size between each keyframe, default 10\n");
    fprintf(stderr, "    bitrate=       : bit rate of frame data transmission, default 400\n");
    fprintf(stderr, "    frame_rate=    : frame rate, default 20\n");
    fprintf(stderr, "    width=         : width of each frame, please align 16, default sensor size\n");
    fprintf(stderr, "    height=        : height of each frame, please align 16, default sensor size\n");
    fprintf(stderr, "           if set width and height will scaling the frame\n");
    fprintf(stderr, "    single_frame=  : save each frame separately or not, default 0\n");
    fprintf(stderr, "    Example: \n"
                        "   %s output=/tmp/output\n"
                        "   %s input=/dev/mscaler-ch0 output=/tmp/output max_frame=30\n"
                            "gop_size=10 bitrate=400 frame_rate=20 width=1280 height=720 single_frame=0 \n",
                            prg_name, prg_name);
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
        return -ENODEV;
    }

    ret = isp_get_sensor_info(camera_fd, &info);
    if (ret < 0) {
        fprintf(stderr, "isp get sensor info failed\n");
        goto close_isp;
    }

    output_fmt.width = ALIGN_DOWN(output_fmt.width, 32);
    output_fmt.height = ALIGN_DOWN(output_fmt.height, 32);

    if (info.width <= output_fmt.width || info.height <= output_fmt.height) {
        output_fmt.scaler.enable = 1;
        output_fmt.scaler.width = output_fmt.width;
        output_fmt.scaler.height = output_fmt.height;
    } else {
        output_fmt.crop.enable = 1;
        output_fmt.crop.width = output_fmt.width;
        output_fmt.crop.height = output_fmt.height;
        output_fmt.crop.left = (info.width - output_fmt.crop.width) / 2;
        output_fmt.crop.top = (info.height - output_fmt.crop.height) / 2;
    }

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

static void parse_cmd_info(const int argc, const char **argv,
                           char *input, char *output, int *max_frame,
                           int *gop_size, int *width, int *height,
                           int *frame_rate, int *bitrate, int *single_frame)
{

    int i;
    for (i = 1; i < argc; i++) {
        if (parse_ustr(argv[i], "input=", input, 20))
            continue;
        if (parse_ustr(argv[i], "output=", output, 64))
            continue;
        if (parse_uint(argv[i], "max_frame=", max_frame, 10))
            continue;
        if (parse_uint(argv[i], "gop_size=", gop_size, 10))
            continue;
        if (parse_uint(argv[i], "width=", width, 10))
            continue;
        if (parse_uint(argv[i], "height=", height, 10))
            continue;
        if (parse_uint(argv[i], "frame_rate=", frame_rate, 10))
            continue;
        if (parse_uint(argv[i], "bitrate=", bitrate, 10))
            continue;
        if (parse_uint(argv[i], "single_frame=", single_frame, 10))
            continue;
        error_arg(argv[i]);
    }

    if (*width < 0 || *height < 0) {
        fprintf(stderr, "width(%d) or height(%d) err!\n", *width, *height);
        usage(-1);
    }
    if (*max_frame <= 0) {
        fprintf(stderr, "max_frame(%d) err!\n", *max_frame);
        usage(-1);
    }
    if (*gop_size <= 0) {
        fprintf(stderr, "gop_size(%d) err!\n", *gop_size);
        usage(-1);
    }
    if (*frame_rate <= 0) {
        fprintf(stderr, "frame_rate(%d) err!\n", *frame_rate);
        usage(-1);
    }
    if (*bitrate <= 0) {
        fprintf(stderr, "bitrate(%d) err!\n", *bitrate);
        usage(-1);
    }
}

static void signal_handler(int signum)
{
    close(file_fd1);

    avpu_h264_encoder_close(encoder);

    deinit_isp_camera(camera_fd);

    file_fd1 = -1;
    camera_fd = -1;

    exit(-1);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int gop_size = 10, max_frame = 30, single_frame = 0;
    int width = 0, height = 0, frame_rate = 20, bitrate = 400;

    prg_name = argv[0];
    unsigned char *isp_path = malloc(20);
    unsigned char *file_path = malloc(64);
    memmove(isp_path, DEFAULT_ISP_DEVICE, 18);
    memset(file_path, 0, 64);

    unsigned char *src_buf = NULL;
    unsigned char *dst_buf = NULL;
    unsigned int dst_len = 0;
    unsigned int encode_frm_size = 0;
    unsigned int uv_offset = 0;

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    parse_cmd_info((const int)argc, (const char**)argv,
                           isp_path, file_path, &max_frame,
                           &gop_size, &width, &height,
                           &frame_rate, &bitrate, &single_frame);

    if (strlen(file_path) == 0) {
        fprintf(stderr, "[err] output must be set up\n");
        return -1;
    }

    fprintf(stderr, "file_path    : %s*\n", file_path);
    fprintf(stderr, "input_path   : %s\n", isp_path);
    fprintf(stderr, "max_frame    : %d\n", max_frame);
    fprintf(stderr, "gop_size     : %d\n", gop_size);
    fprintf(stderr, "frame_rate   : %d\n", frame_rate);
    fprintf(stderr, "bitrate      : %d\n", bitrate);
    fprintf(stderr, "single_frame : %d\n", single_frame);
    fprintf(stderr, "set_width    : %d\n", width);
    fprintf(stderr, "set_height   : %d\n", height);

    output_fmt.width = width;
    output_fmt.height = height;

    signal(SIGINT, signal_handler);

    camera_fd = init_isp_camera(isp_path);
    if (camera_fd < 0) {
        fprintf(stderr, "Init isp camera failed\n");
        return camera_fd;
    }

    struct avpu_h264_encoder_config config;
    memset(&config, 0, sizeof(struct avpu_h264_encoder_config));
    config.width      = output_fmt.width;
    config.height     = output_fmt.height;
    config.frame_rate = frame_rate;
    config.input_fmt  = info.data_fmt;
    config.gop_size   = gop_size;
    config.bitrate    = bitrate;

    fprintf(stderr, "align_width  : %d\n", config.width);
    fprintf(stderr, "align_height : %d\n", config.height);

    encoder = avpu_h264_encoder_open(&config);
    if (!encoder) {
        fprintf(stderr, "Uanble to open avpu h264 encoder\n");
        ret = -1;
        goto close_isp_cam;
    }

    encode_frm_size = avpu_h264_encoder_get_frame_size(encoder);

    src_buf = avpu_encoder_alloc(encode_frm_size);
    if(src_buf == NULL) {
        fprintf(stderr, "avpu encoder alloc(0x%x) failed\n", encode_frm_size);
        ret = -1;
        goto avpu_encoder_alloc_err;
    }

    dst_buf = malloc(encode_frm_size);
    if (dst_buf == NULL) {
        fprintf(stderr, "out frame malloc(0x%x) failed\n", encode_frm_size);
        ret = -1;
        goto dst_buf_alloc_err;
    }

    /* get uv offset of src_buf */
    uv_offset = avpu_h264_encoder_get_uv_offset(encoder);

    unsigned char file_buf[20];
    snprintf(file_buf, sizeof(file_buf), "%s00", file_path);
    file_fd1 = open(file_buf, O_RDWR | O_CREAT | O_TRUNC);
    if (file_fd1 < 0) {
        fprintf(stderr, "open file %s error.\n", file_buf);
        goto close_encoder;
    }

    /* 获取frame_size(非对齐大小) */
    struct frame_image_format fmt;
    isp_get_format(camera_fd, &fmt);

    void *isp_mem = isp_wait_frame(camera_fd);
    if (isp_mem == NULL)
        goto close_file;

    isp_put_frame(camera_fd, isp_mem);

    unsigned int t = 0;
    while (1) {
        isp_mem = isp_wait_frame(camera_fd);
        if (isp_mem == NULL)
            goto close_file;

        /* copy y and uv to src_buf to encode, but we can't ensure that
            the height and width of camera_info are aligned at 16.
            So we copy using the requested alignment size, and the offset position
            of the uv is calculated using the original size information of camera_info */
        memcpy(src_buf, isp_mem, uv_offset);
        memcpy(src_buf+uv_offset, isp_mem + info.width*info.height, encode_frm_size/3);

        isp_put_frame(camera_fd, isp_mem);

        ret = avpu_h264_encoder_work(encoder, src_buf, &dst_len, dst_buf);
        if (ret < 0) {
            fprintf(stderr, "avpu h264 encode work failed\n");
            ret = -1;
            goto close_file;
        }

        if (single_frame) {
            snprintf(file_buf, sizeof(file_buf), "%s_%d", file_path, t);
            int file_fd = open(file_buf, O_RDWR | O_CREAT | O_TRUNC);
            if (file_fd < 0) {
                fprintf(stderr, "open file %s error.\n", file_path);
                goto close_file;
            }
            write(file_fd, dst_buf, dst_len);
            close(file_fd);
        }

        write(file_fd1, dst_buf, dst_len);

        if (t++ >= max_frame) break;
    }

close_file:
    close(file_fd1);

close_encoder:
    free(dst_buf);
dst_buf_alloc_err:
    avpu_encoder_free(src_buf);
avpu_encoder_alloc_err:
    avpu_h264_encoder_close(encoder);

close_isp_cam:
    deinit_isp_camera(camera_fd);

    file_fd1 = -1;
    camera_fd = -1;

    return ret;
}
