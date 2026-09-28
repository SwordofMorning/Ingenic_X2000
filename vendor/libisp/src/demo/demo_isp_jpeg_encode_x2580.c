#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <isp.h>
#include <libhardware2/avpu_jpeg_encode.h>


#define ALIGN_DOWN(n, align)         (n & (~(align - 1)))
#define DEFAULT_ISP_DEVICE           "/dev/mscaler-ch0"

static struct camera_info info;
static int camera_fd;
static FILE *file_fp;
struct avpu_jpeg_encoder *encoder;

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
    fprintf(stderr, "    -h/--help      : show help info\n");
    fprintf(stderr, "    input=         : the dev path for camera, default /dev/mscaler-ch0\n");
    fprintf(stderr, "    output=        : the file to save encoded JPEG data stream, must be set\n");
    fprintf(stderr, "    Example: %s output=/tmp/output.jpeg\n", prg_name);
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
        return -ENODEV;
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


int main(int argc, char *argv[])
{
    int ret = 0;
    unsigned int dst_len = 0;

    prg_name = argv[0];
    unsigned char *isp_path = malloc(20);
    unsigned char *file_path = malloc(20);
    memmove(isp_path, DEFAULT_ISP_DEVICE, 18);
    memset(file_path, 0, 20);

    unsigned char *src_buf = NULL;
    unsigned char *dst_buf = NULL;

    /* 参数初始化 */
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
        error_arg(argv[i]);
    }

    if (strlen(file_path) == 0) {
        fprintf(stderr, "[err] output must be set up\n");
        return -1;
    }

    fprintf(stderr, "file_path    : %s*\n", file_path);
    fprintf(stderr, "input_path   : %s\n", isp_path);

    /* 摄像头初始化、开流 */
    camera_fd = init_isp_camera(isp_path);
    if (camera_fd < 0) {
        fprintf(stderr, "Init isp camera failed\n");
        return camera_fd;
    }

    /* jpeg 编码初始化，内存申请 */
    unsigned int align_width = ALIGN_DOWN(output_fmt.width, 16);
    unsigned int align_height = ALIGN_DOWN(output_fmt.height, 16);

    struct avpu_jpeg_encoder_config config;
    memset(&config, 0, sizeof(struct avpu_jpeg_encoder_config));
    config.width = align_width;
    config.height = align_height;
    config.input_fmt = info.data_fmt;

    fprintf(stderr, "width        : %d\n", config.width);
    fprintf(stderr, "height       : %d\n", config.height);

    encoder = avpu_jpeg_encoder_open(&config);
    if (!encoder) {
        fprintf(stderr, "Uanble to open avpu jpeg encoder\n");
        ret = -1;
        goto close_isp_cam;
    }

    unsigned int frame_size = avpu_jpeg_encoder_get_frame_size(encoder);
    src_buf = avpu_encoder_alloc(frame_size);
    if(src_buf == NULL) {
        fprintf(stderr, "avpu encoder alloc(0x%x) failed\n", frame_size);
        ret = -1;
        goto avpu_encoder_alloc_err;
    }

    dst_buf = malloc(frame_size);
    if (dst_buf == NULL) {
        fprintf(stderr, "out frame malloc(0x%x) failed\n", frame_size);
        ret = -1;
        goto dst_buf_alloc_err;
    }
    /* get uv offset of src_buf */
    unsigned int uv_offset = avpu_jpeg_encoder_get_uv_offset(encoder);

    file_fp = fopen(file_path, "wb");
    if (file_fp == NULL) {
        fprintf(stderr, "open file %s error.\n", file_path);
        goto close_encoder;
    }

    /* 取图、编码、保存 */
    i = 3;
    while (i--) {
        void *isp_mem = isp_wait_frame(camera_fd);
        if (isp_mem == NULL)
            goto close_file;

        /* copy y and uv to src_buf to encode, but we can't ensure that
            the height and width of camera_info are aligned at 16.
            So we copy using the requested alignment size, and the offset position
            of the uv is calculated using the original size information of camera_info */
        memcpy(src_buf, isp_mem, frame_size*2/3);
        memcpy(src_buf+uv_offset, isp_mem + info.width*info.height, frame_size/3);

        ret = avpu_jpeg_encoder_work(encoder, src_buf, &dst_len, dst_buf);
        if (ret < 0) {
            fprintf(stderr, "avpu jpeg encode work failed\n");
            ret = -1;
            goto close_file;
        }

        isp_put_frame(camera_fd, isp_mem);

        fwrite(dst_buf, 1, dst_len, file_fp);
        system("sync");
        printf("Save jpeg to File(%s) size=%d\n", file_path, dst_len);
    }

close_file:
    fclose(file_fp);

close_encoder:
    free(dst_buf);
dst_buf_alloc_err:
    avpu_encoder_free(src_buf);
avpu_encoder_alloc_err:
    avpu_jpeg_encoder_close(encoder);

close_isp_cam:
    deinit_isp_camera(camera_fd);

    free(isp_path);
    free(file_path);

    file_fp = NULL;
    camera_fd = -1;

    return ret;
}
