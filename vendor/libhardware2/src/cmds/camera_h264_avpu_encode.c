#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <libutils2/simple_bayer16_to_nv12.h>
#include <libhardware2/avpu_h264_encode.h>
#include <libhardware2/camera.h>

#define ALIGN_DOWN(n, align)            (n & (~(align - 1)))

#define DEFAULT_CAMERA_DEVICE           "/dev/vic"

static struct camera_info g_camera_info;

static const char *g_prg_name;

static void usage(int status)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    input=             : the dev path for camera, default /dev/vic0\n");
    fprintf(stderr, "    output=            : the file to save encoded H264 data stream,\n"
                    "                         it must be set\n");
    fprintf(stderr, "    max_frame=         : size of frames will be encoded, default 30\n");
    fprintf(stderr, "    save_single_frame= : save each frame separately or not, default 0\n");
    fprintf(stderr, "    gop_size=          : size between each keyframe, default 10\n");
    fprintf(stderr, "    frame_rate=        : the frame rate of output h264 file, default 10\n");
    fprintf(stderr, "    bitrate=           : the bit rate of output h264 file, default 400\n");

    fprintf(stderr, "Example:\n"
                    "    %s output=/tmp/test.h264\n"
                    "    %s input=/dev/vic output=/tmp/test.h264 max_frame=30 \n"
                    "save_single_frame=0 gop_size=10 frame_rate=24 bitrate=400\n",
                    g_prg_name, g_prg_name);

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

static void parse_cmd_info(const int argc, const char **argv,
                           char *input, char *output, int *max_frame,
                           int *save_single_frame, int *gop_size,
                           int *frame_rate, int *bitrate)
{
    int i = 0;
    for ( i = 1; i < argc; i++) {
        if (parse_ustr(argv[i], "input=", input, 20))
            continue;
        if (parse_ustr(argv[i], "output=", output, 128))
            continue;
        if (parse_uint(argv[i], "max_frame=", max_frame, 10))
            continue;
        if (parse_uint(argv[i], "save_single_frame=", save_single_frame, 10))
            continue;
        if (parse_uint(argv[i], "gop_size=", gop_size, 10))
            continue;
        if (parse_uint(argv[i], "frame_rate=", frame_rate, 10))
            continue;
        if (parse_uint(argv[i], "bitrate=", bitrate, 10))
            continue;
        error_arg(argv[i]);
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

int main(int argc, char *argv[])
{
    int cam_fd = 0;
    int ret = 0;
    int max_frame = 30, save_single_frame = 0;
    int gop_size = 10, frame_rate = 24, bitrate = 400;

    char cam_path[20] = DEFAULT_CAMERA_DEVICE;
    char file_save_path[128];
    char save_file_name[128];
    memset(file_save_path, 0, sizeof(file_save_path));
    memset(save_file_name, 0, sizeof(save_file_name));

    unsigned char *src_buf = NULL;
    unsigned char *dst_buf = NULL;
    g_prg_name = argv[0];

    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            usage(0);
        }
    }

    parse_cmd_info((const int)argc, (const char **)argv,
                    cam_path, file_save_path, &max_frame,
                    &save_single_frame, &gop_size,
                    &frame_rate, &bitrate);

    fprintf(stderr, "input             : %s\n", cam_path);
    fprintf(stderr, "output            : %s\n", file_save_path);
    fprintf(stderr, "max_frame         : %d\n", max_frame);
    fprintf(stderr, "save_single_frame : %d\n", save_single_frame);
    fprintf(stderr, "gop_size          : %d\n", gop_size);
    fprintf(stderr, "frame_rate        : %d\n", frame_rate);
    fprintf(stderr, "bitrate           : %d\n", bitrate);

    if (strlen(file_save_path) == 0) {
        fprintf(stderr, "output must be set!\n");
        usage(-1);
    }

    cam_fd = camera_open(&g_camera_info, cam_path);
    if (cam_fd < 0) {
        return -1;
    }

    ret = camera_power_on(cam_fd);
    if (ret < 0)
        goto camera_power_on_err;

    ret = camera_stream_on(cam_fd);
    if (ret < 0)
        goto camera_stream_on_err;

    unsigned int camera_input_fmt = g_camera_info.data_fmt;
    unsigned int line_length = 0;
    switch (camera_input_fmt) {
    case CAMERA_PIX_FMT_GREY:
        line_length = g_camera_info.line_length;
        camera_input_fmt = CAMERA_PIX_FMT_GREY;
        break;
    case CAMERA_PIX_FMT_Y16:
    case CAMERA_PIX_FMT_SBGGR16:
    case CAMERA_PIX_FMT_SGBRG16:
    case CAMERA_PIX_FMT_SGRBG16:
    case CAMERA_PIX_FMT_SRGGB16:
        fprintf(stderr, "Note: camera input format not NV12 and we software change to NV12.\n");
        line_length = g_camera_info.line_length / 2;
        camera_input_fmt = CAMERA_PIX_FMT_Y16;
        break;
    case CAMERA_PIX_FMT_NV12:
    case CAMERA_PIX_FMT_NV21:
        line_length = g_camera_info.line_length;
        break;
    default:
        ret = -1;
        fprintf(stderr, "Unsupport camera input format : %#x\n", g_camera_info.data_fmt);
        goto came_input_fmt_err;
    }

    unsigned int align_width = ALIGN_DOWN(line_length, 16);
    unsigned int align_height = ALIGN_DOWN(g_camera_info.height, 16);

    struct avpu_h264_encoder_config config = {
        .width = align_width,
        .height = align_height,
        .frame_rate = frame_rate,
        .input_fmt = CAMERA_PIX_FMT_NV12,
        .gop_size = gop_size,
        .bitrate = bitrate,
    };

    struct avpu_h264_encoder *encoder = avpu_h264_encoder_open(&config);
    if (encoder == NULL) {
        fprintf(stderr, "open jpeg(avpu) encoder failed\n");
        ret = -1;
        goto avpu_h264_encoder_open_err;
    }

    unsigned int frame_size = avpu_h264_encoder_get_frame_size(encoder);

    src_buf = avpu_encoder_alloc(frame_size);
    if(src_buf == NULL) {
        fprintf(stderr, "avpu encoder alloc(%d) failed\n", frame_size);
        ret = -1;
        goto avpu_encoder_alloc_err;
    }

    unsigned int dst_len = 0;
    dst_buf = malloc(frame_size);
    if (dst_buf == NULL) {
        fprintf(stderr, "out frame malloc(%d) failed\n", frame_size);
        ret = -1;
        goto dst_buf_alloc_err;
    }
    /* get uv offset of src_buf */
    unsigned int uv_offset = avpu_h264_encoder_get_uv_offset(encoder);

    unsigned int t = 0;
    memset(save_file_name, 0, sizeof(save_file_name));
    snprintf(save_file_name, sizeof(save_file_name), "%s", file_save_path);
    int h264_fd = open(save_file_name, O_RDWR | O_CREAT | O_TRUNC);
    if (h264_fd < 0) {
        ret = -1;
        fprintf(stderr, "open file %s error.\n", save_file_name);
        goto open_h264_fd_err;
    }

    while (1) {
        void *frame = camera_wait_frame(cam_fd);
        if (!frame) {
            ret = -1;
            goto close_h264_file;
        }

        switch (camera_input_fmt) {
        case CAMERA_PIX_FMT_GREY: {
            memcpy(src_buf, frame, frame_size*2/3);
            memset(src_buf+uv_offset, 0x80, frame_size/3);
            break;
        }
        case CAMERA_PIX_FMT_Y16: {
            simple_bayer16_to_nv12(src_buf, frame, g_camera_info.width, g_camera_info.height,
                                   g_camera_info.line_length);
            break;
        }
        case CAMERA_PIX_FMT_NV12:
        case CAMERA_PIX_FMT_NV21: {
            /* copy y and uv to src_buf to encode, but we can't ensure that
               the height and width of g_camera_info are aligned at 16.
               So we copy using the requested alignment size, and the offset position
               of the uv is calculated using the original size information of g_camera_info */
            memcpy(src_buf, frame, frame_size*2/3);
            memcpy(src_buf+uv_offset, frame+g_camera_info.width*g_camera_info.height, frame_size/3);
            break;
        }
        default:
            fprintf(stderr, "Unsupport camera input format %#x\n", camera_input_fmt);
            goto close_h264_file;
        }

        camera_put_frame(cam_fd, frame);

        ret = avpu_h264_encoder_work(encoder, src_buf, &dst_len, dst_buf);
        if (ret < 0) {
            fprintf(stderr, "avpu h264 encode work failed\n");
            ret = -1;
            goto close_h264_file;
        }

        if (save_single_frame) {
            memset(save_file_name, 0, sizeof(save_file_name));
            snprintf(save_file_name, sizeof(save_file_name), "%s.frame_%d", file_save_path, t);
            int file_fd = open(save_file_name, O_RDWR | O_CREAT | O_TRUNC);
            if (file_fd < 0) {
                fprintf(stderr, "open file %s error.\n", save_file_name);
                goto close_h264_file;
            }
            write(file_fd, dst_buf, dst_len);
            close(file_fd);
        }

        write(h264_fd, dst_buf, dst_len);

        if ( ++t == max_frame ) {
            break;
        }
    }

close_h264_file:
    close(h264_fd);
open_h264_fd_err:
    free(dst_buf);
dst_buf_alloc_err:
    avpu_encoder_free(src_buf);
avpu_encoder_alloc_err:
    avpu_h264_encoder_close(encoder);
avpu_h264_encoder_open_err:
came_input_fmt_err:
    camera_stream_off(cam_fd);
camera_stream_on_err:
    camera_power_off(cam_fd);
camera_power_on_err:
    camera_drop_frames(cam_fd, g_camera_info.frame_nums);
    camera_close(cam_fd, &g_camera_info);

    return ret;
}
