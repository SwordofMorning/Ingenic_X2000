#include <stdio.h>
#include <libutils2/cmyk_to_rgb.h>
#include <libhardware2/fb.h>
#include <string.h>
#include <stdlib.h>
#include "jpeglib.h"
#include "jerror.h"

static char *command;
static void usage(int status)
{
    fprintf(stderr, "Usage: %s jpeg_path\n", command);
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "\t%s /etc/logo.jpeg\n", command);

    exit(status);
}

#define min(_x, _y) ((_x) < (_y) ? (_x) : (_y))

int main(int argc, char **argv)
{
    int fd;
    struct fb_device_info info;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * infile;
    JSAMPARRAY buffer;
    int row_stride;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    fd = fb_open("/dev/fb0", &info);
    if (fd < 0) {
        fprintf(stderr, "/dev/fb0 not found!\n");
        exit(-1);
    }

    // 支持RGB666 RGB888
    if (info.bits_per_pixel < 18) {
        fprintf(stderr, "error:display not supprot bits_per_pixel: %d\n", info.bits_per_pixel);
        fb_close(fd, &info);
        return -1;
    }

    // 绑定标准错误处理结构
    cinfo.err = jpeg_std_error(&jerr);

    // 初始化JPEG对象
    jpeg_create_decompress(&cinfo);

    // 指定图像文件
    infile = fopen(argv[1], "rb");
    if (infile == NULL) {
        fprintf(stderr, "fopen fail: not found %s\n",argv[1]);
        fb_close(fd, &info);
        return -1;
    }
    jpeg_stdio_src(&cinfo, infile);

    // 读取图像信息
    (void) jpeg_read_header(&cinfo, TRUE);

    // 设定解压缩参数，此处我们将图像长宽缩小为原图的1/2，目前支持1/1,1/2,1/4,1/8
    cinfo.scale_num=1;
    cinfo.scale_denom=1;

    // 开始解压缩图像
    (void) jpeg_start_decompress(&cinfo);

    // 分配缓冲区空间
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    int display_width = min(cinfo.output_width, info.xres);
    int display_height = min(cinfo.output_height, info.yres);

    int fb_x_offset = (info.xres - display_width) / 2;
    int fb_y_offset = (info.yres - display_height) / 2;

    int jpeg_x_offset = (cinfo.output_width - display_width) / 2;
    int jpeg_y_offset = (cinfo.output_height - display_height) / 2;

    unsigned char *jpeg_rgb_buffer = NULL;
    if (cinfo.out_color_space == JCS_CMYK) {
        jpeg_rgb_buffer = (unsigned char *)malloc(row_stride * sizeof(unsigned char));
    }

    // 定位到fb应该显示的第一行
    unsigned int *fb_mem = info.mapped_mem + info.line_length * fb_y_offset;

    // 定位到jpeg应该显示的第一行（即跳过图片不显示的上半部分）
    int w,h;
    for (h = 0; h < jpeg_y_offset; h++) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
    }

    // 逐行解码jpeg图片，并拷贝到fb
    for (h = 0; h < display_height; h++) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        unsigned char *jpeg_rgb = buffer[0];

        if (cinfo.out_color_space == JCS_CMYK) {
            cmyk_to_rgb24((unsigned int *)buffer[0], jpeg_rgb_buffer, row_stride, row_stride, cinfo.output_width, 1);
            jpeg_rgb = jpeg_rgb_buffer;
        }

        unsigned int *fb_rgb = fb_mem;
        fb_rgb += fb_x_offset;
        jpeg_rgb += jpeg_x_offset * 3;

        for (w = 0; w < display_width; w++) {
            unsigned int r = jpeg_rgb[w*3 + 0];
            unsigned int g = jpeg_rgb[w*3 + 1];
            unsigned int b = jpeg_rgb[w*3 + 2];

            fb_rgb[w] = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
        }

        fb_mem = (void *)fb_mem + info.line_length;
    }

    // 将jpeg解码的行定位到最后，否则jpeg库会报错退出
    cinfo.output_scanline = cinfo.output_height;

    // 显示
    fb_enable(fd);
    fb_pan_display(fd, &info, 0);

    // 结束解压缩操作
    (void) jpeg_finish_decompress(&cinfo);

    // 释放资源
    if (cinfo.out_color_space == JCS_CMYK) {
        free(jpeg_rgb_buffer);
    }
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    fb_close(fd, &info);

    return 0;
}