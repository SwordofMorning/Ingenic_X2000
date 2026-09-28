#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <libhardware2/rmem.h>
#include <libhardware2/fb.h>

#include <libutils2/nalu_buf.h>
#include <libhardware2/v4l2_h264_decode.h>

#define DUL_Frame //双帧开关

#ifdef DUL_FRAME
#define MAX_BUFFERS 2
#else
#define MAX_BUFFERS 1
#endif

#define page_size 4096
#define ALIGN(d,a) (((d)+((a)-1))/(a)*(a))

static char *prg_name;
static struct fb_device_info fb_info;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);

    fprintf(stderr, "    -h/--help      : show help info\n");
    fprintf(stderr, "    infile=        : input frame path\n");
    fprintf(stderr, "    width=         : input frame width\n");
    fprintf(stderr, "    height=        : input frame height\n");
    fprintf(stderr, "    Example: %s infile=/tmp/test.h264 width=1280 height=720 \n", prg_name);

    exit(status);
}

void fill_nv12_black(void *y_va, void *uv_va, int width, int height)
{
    int y, x;

    unsigned char *y_mem = y_va;
    unsigned char *uv_mem = uv_va;

    // 填充Y分量为黑色
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            y_mem[y * width + x] = 0;        // Y分量的值设为0，表示黑色
        }
    }

    // 填充UV分量为黑色
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width; x += 2) {
            uv_mem[y * width + x] = 128;     // U分量的值设为128，表示黑色
            uv_mem[y * width + x + 1] = 128; // V分量的值设为128，表示黑色
        }
    }
}

int read_file(unsigned char **buf, const char *path) {
    FILE * fp;
    int len = -1;
    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error fopen\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *buf = malloc(len);
    if (!(*buf)) {
        fprintf(stderr, "memory alloc failed\n");
        len = -1;
    } else {
        len = fread(*buf, 1, len, fp);
    }

    fclose(fp);

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

static void fb_pan_display_nv12(int fb, int width, int height, int stride, unsigned long y_phy, unsigned long uv_phy)
{
    struct lcdc_layer layer_cfg = {
        .fb_fmt = fb_fmt_NV12,
        .xres = width,
        .yres = height,
        .xpos = 0,
        .ypos = 0,

        .layer_order = lcdc_layer_0,
        .layer_enable = 1,

        .y = {
            .mem = (void *)y_phy,
            .stride = stride,
        },

        .uv = {
            .mem = (void *)uv_phy,
            .stride = stride,
        },

        .alpha = {
            .enable = 0,
            .value = 0xff,
        },
    };

    if (width != fb_info.xres || height != fb_info.yres) {
        layer_cfg.scaling.enable = 1;
        layer_cfg.scaling.xres = fb_info.xres;
        layer_cfg.scaling.yres = fb_info.yres;
    }

    fb_pan_display_set_user_cfg(fb, &layer_cfg);

    fb_pan_display(fb, &fb_info, 0);
}

int main(int argc, char *argv[])
{
    int ret = -1;
    int i = 0;

    const char *file_path = "/tmp/test.h264";
    unsigned char *file_data = NULL;
    int file_size;
    const char *str = NULL;
    int width = 0;
    int height = 0;

    int num = 0;
    int indexs[2] = {0, (MAX_BUFFERS < 2) ? 0 : 1};

    prg_name = argv[0];
    if (argc < 3) {
        usage(-1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            usage(0);
        if ((str = parse_str(argv[i], "infile="))) {
            file_path = str;
            continue;
        }
        if (parse_int(argv[i], "width=", &width, 10))
            continue;
        if (parse_int(argv[i], "height=", &height, 10))
            continue;
    }

    file_size = read_file(&file_data, file_path);
    if (file_size <= 0) {
        fprintf(stderr, "read file %s fail\n", file_path);
        return -1;
    }

    /*-----------解码先输出到out_mem,再拷贝到rmem-------------*/
    void *out_mem[2];

    int rmem_fd;
    int y_size = ALIGN(width, 128) * height;
    int uv_size = ALIGN(width, 128) * height / 2;

    void *y_va[MAX_BUFFERS];
    void *uv_va[MAX_BUFFERS];
    unsigned long y_pa[MAX_BUFFERS];
    unsigned long uv_pa[MAX_BUFFERS];

    rmem_fd = rmem_open();
    if (rmem_fd < 0) {
        fprintf(stderr, "decoder open rmem failed: %s\n", strerror(errno));
        goto err_rmem_open;
    }

    for (int i = 0; i < MAX_BUFFERS; i++) {
        y_va[i] = rmem_alloc(rmem_fd, &y_pa[i], ALIGN(y_size, page_size));
        uv_va[i] = rmem_alloc(rmem_fd, &uv_pa[i], ALIGN(uv_size, page_size));
        if(!y_va[i] || !uv_va[i])
            goto err_rmem_alloc;
    }

    /*------------------fb-----------------------*/
    int fb = fb_open("/dev/fb0", &fb_info);
    if (fb < 0) {
        fprintf(stderr, "Failed to open fb0\n");
        goto err_fb_open;
    }

    ret = fb_enable(fb);
    if (ret < 0) {
        fprintf(stderr, "Failed to enable fb0\n");
        goto err_fb_enble;
    }

    ret = fb_pan_display_enable_user_cfg(fb);
    if (ret < 0) {
        fprintf(stderr, "Failed to enable user_cfg\n");
        goto err_fb_enable_ucfg;
    }

    /*------------------h264--------------------*/
    struct v4l2_h264_decoder_config config = {
        .width = width,
        .height = height,
        .video_path = "/dev/video2",
        .output_fmt = V4L2_PIX_FMT_NV12
    };

    struct v4l2_h264_decoder *decoder = v4l2_h264_decoder_open(&config);
    if (!decoder)
        goto err_decoder_open;



    void *input_data = file_data;
    int input_size = file_size;
    struct nalu_buf *nalu_buf = nalu_buf_init(256*1024);

    while(1) {
        int len = 0;
        struct nalu_frame *frame = nalu_buf_write(nalu_buf, input_data, input_size, &len);
        input_data += len;
        input_size -= len;
        if (!frame) {
            if (!len)
                break;
            continue;
        }

        ret = v4l2_h264_decoder_work(decoder, frame->data, frame->size, out_mem);
        if (ret) {
            fprintf(stderr, "decoder work failed: %s\n", strerror(errno));
            goto err_decoder_work;
        }


        memcpy(y_va[num], out_mem[0], y_size);
        memcpy(uv_va[num], out_mem[1], uv_size);
        rmem_cache_sync(rmem_fd, y_va[num], y_size, rmem_cache_mem_to_dev);
        rmem_cache_sync(rmem_fd, uv_va[num], uv_size, rmem_cache_mem_to_dev);

        fb_pan_display_nv12(fb, width, height, config.linesize, y_pa[num], uv_pa[num]);

        ret = v4l2_h264_decoder_work_release(decoder);
        if (ret)
            goto err_decoder_work;

        nalu_frame_delete(frame);

        num = indexs[!num];
    }

err_decoder_work:
    v4l2_h264_decoder_close(decoder);
    /* 最后刷一帧全黑后,再退出 */
    fill_nv12_black(y_va[num], uv_va[num], config.linesize, config.colunmsize);
    rmem_cache_sync(rmem_fd, y_va[num], y_size, rmem_cache_mem_to_dev);
    rmem_cache_sync(rmem_fd, uv_va[num], uv_size, rmem_cache_mem_to_dev);
    fb_pan_display_nv12(fb, width, height, config.linesize, y_pa[num], uv_pa[num]);


err_decoder_open:
    fb_pan_display_disable_user_cfg(fb);

err_fb_enable_ucfg:
    fb_disable(fb);

err_fb_enble:
    fb_close(fb, &fb_info);

err_fb_open:
    for (int i = 0; i < MAX_BUFFERS; i++) {
        rmem_free(rmem_fd, y_va[i], y_pa[i], ALIGN(y_size, page_size));
        rmem_free(rmem_fd, uv_va[i], uv_pa[i], ALIGN(uv_size, page_size));
    }

err_rmem_alloc:
    rmem_close(rmem_fd);

err_rmem_open:
    free(file_data);

    return ret;
}