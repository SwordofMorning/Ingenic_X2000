#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <libhardware2/camera.h>
#include <libhardware2/avpu_jpeg_encode.h>


static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    outfile=           : the file to save encoded JPEG data stream, must be set\n");
    fprintf(stderr, "    infile=            : the dev path for input file(NV12)\n");
    fprintf(stderr, "    width=             : input frame width\n");
    fprintf(stderr, "    height=            : input frame height\n");
    fprintf(stderr, "    Example: %s infile=1920_1080.nv12 width=1920 height=1080 outfile=/tmp/camera.jpeg\n", prg_name);
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

int main(int argc, char *argv[])
{
    int ret = -1;
    int width = 0;
    int height = 0;
    const char *str = NULL;
    const char *src_file_path = "/tmp/test.nv12";
    const char *dst_file_path = "/tmp/test.jpeg";
    unsigned char *src_buf = NULL;
    unsigned char *dst_buf = NULL;
    int i = 0;
    FILE *dst_file_fp;
    FILE *src_file_fp;

    prg_name = argv[0];
    if (argc < 4) {
        usage(-1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            usage(0);
        if (parse_int(argv[i], "width=", &width, 10))
            continue;
        if (parse_int(argv[i], "height=", &height, 10))
            continue;
        if ((str = parse_str(argv[i], "infile="))) {
            src_file_path = str;
            continue;
        }
        if ((str = parse_str(argv[i], "outfile="))) {
            dst_file_path = str;
            continue;
        }
    }

    src_file_fp = fopen(src_file_path, "rb");
    if (!src_file_fp) {
        fprintf(stderr, "open infile %s fail\n", src_file_path);
        return -1;
    }

    dst_file_fp = fopen(dst_file_path, "wb");
    if (!dst_file_fp) {
        fprintf(stderr, "open outfile %s fail\n", dst_file_path);
        ret = -1;
        goto err_dst_file_open;
    }

    struct avpu_jpeg_encoder_config config = {
        .width = width,
        .height = height,
        .input_fmt = CAMERA_PIX_FMT_NV12,
    };
    struct avpu_jpeg_encoder *encoder = avpu_jpeg_encoder_open(&config);
    if (encoder == NULL) {
        fprintf(stderr, "open jpeg(avpu) encoder failed\n");
        ret = -1;
        goto err_encoder_open;
    }

    /* 申请AVPU管理的内存,包括虚拟地址和物理连续地址 */
    int frame_size = avpu_jpeg_encoder_get_frame_size(encoder);
    src_buf = avpu_encoder_alloc(frame_size);
    if(src_buf == NULL) {
        fprintf(stderr, "avpu encoder alloc(%d) failed\n", frame_size);
        ret = -1;
        goto err_encoder_alloc;
    }

    /* 申请jpeg在用户空间保存数据空间 */
    uint32_t dst_len = 0;
    dst_buf = malloc(frame_size);
    if (dst_buf == NULL) {
        fprintf(stderr, "out frame malloc(%d) failed\n", frame_size);
        ret = -1;
        goto err_dst_buf_alloc;
    }

    /* 读取NV12帧数据 */
    fseek(src_file_fp, 0, SEEK_SET);
    fread(src_buf, 1, width * height, src_file_fp);
    fread(src_buf + avpu_jpeg_encoder_get_uv_offset(encoder), 1, width * height / 2, src_file_fp);

    ret = avpu_jpeg_encoder_work(encoder, src_buf, &dst_len, dst_buf);
    if (ret < 0) {
        fprintf(stderr, "avpu jpeg encoder yuv failed\n");
        goto err_encoder_yuvencode;
    }

    fflush(stdout);
    fwrite(dst_buf, 1, dst_len, dst_file_fp);

    fprintf(stderr, "avpu jpeg encode success\n");

err_encoder_yuvencode:
    free(dst_buf);
err_dst_buf_alloc:
    avpu_encoder_free(src_buf);
err_encoder_alloc:
    avpu_jpeg_encoder_close(encoder);
err_encoder_open:
    fclose(dst_file_fp);
err_dst_file_open:
    fclose(src_file_fp);
    return ret;
}
