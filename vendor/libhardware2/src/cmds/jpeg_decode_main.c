#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <linux/videodev2.h>

#include <libhardware2/v4l2_jpeg_decode.h>

static char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);

    fprintf(stderr, "    -h/--help      : show help info\n");
    fprintf(stderr, "    width=         : input frame width\n");
    fprintf(stderr, "    height=        : input frame height\n");
    fprintf(stderr, "    infile=        : input frame path\n");
    fprintf(stderr, "    outfile=       : output frame path, default /tmp/out_frm.raw\n");

    fprintf(stderr, "    Example: %s width=1280 height=720 infile=/tmp/test.jpg outfile=/tmp/out_frm.raw\n", prg_name);

    exit(status);
}

int read_file(unsigned char **buf, const char *path) {
    FILE * fp;
    int len = -1;
    fp = fopen(path, "rb");
    if (NULL != fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        *buf = malloc(len);
        fread(*buf, 1, len, fp);

        fclose(fp);
    }
    return len;
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
    const char *src_file_path = "/tmp/test.jpg";
    const char *dst_file_path = "/tmp/out_frm.raw";
    unsigned char *src_buf = NULL;
    int src_size = 0;
    int i = 0;
    FILE *dst_file_fp;

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

    src_size = read_file(&src_buf, src_file_path);
    if (src_size <= 0) {
        fprintf(stderr, "open infile %s fail\n", src_file_path);
        return -1;
    }

    dst_file_fp = fopen(dst_file_path, "wb");
    if (!dst_file_fp) {
        fprintf(stderr, "open outfile %s fail\n", dst_file_path);
        goto close_file;
    }

    struct v4l2_jpeg_decoder_config config = {
        .width = width,
        .height = height,
        .video_path = "/dev/video1",
        .output_fmt = V4L2_PIX_FMT_NV12
    };
    
    struct v4l2_jpeg_decoder *decoder = v4l2_jpeg_decoder_open(&config);
    if (!decoder)
        goto close_file;

    int y_size = config.width * config.height;
    void *out_mem[2];
    ret = v4l2_jpeg_decoder_work(decoder, src_buf, src_size, out_mem);
    if (ret)
        goto close_decoder;

    fwrite(out_mem[0], 1, y_size, dst_file_fp);         // write y
    fwrite(out_mem[1], 1, y_size / 2, dst_file_fp);     // write uv

close_decoder:
    v4l2_jpeg_decoder_close(decoder);
close_file:
    free(src_buf);
    fclose(dst_file_fp);

    return ret;
}
