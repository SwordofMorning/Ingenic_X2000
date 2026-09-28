
#include <stdio.h>
#include <soc/base.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <cpu/host_cpu.h>

#include <driver/gpio.h>

#include <include_bin.h>
#include <driver/systick.h>
#include <soc/felix_h264_decoder.h>
#include <nalu_buf.h>
#include <assert.h>
#include <common.h>
#include <driver/fb.h>

#include <stdlib.h>
#include <string.h>

#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720


/**
 * 提取视频文件中的h264 流可在pc端用ffmpeg 命令实现：
 * ffmpeg -i input.mp4 -c:v libx264 -profile:v baseline -an -f h264 output.h264
**/
INCBIN(h264, "example/resource/test.h264");


static void nv12_display(struct fbdev_data *fbdev, void *nv12_mem)
{
    struct lcdc_layer cfg0 = {
        .xres = VIDEO_WIDTH,
        .yres = VIDEO_HEIGHT,
        .xpos = 0,
        .ypos = 0,
        .layer_enable = 1,
        .layer_order = lcdc_layer_0,
        .fb_fmt = fb_fmt_NV12,

        .y = {
            .mem = nv12_mem,
            .stride = VIDEO_WIDTH,
        },

        .uv = {
            .mem = nv12_mem + VIDEO_WIDTH * VIDEO_HEIGHT,
            .stride = VIDEO_WIDTH,
        },

        .alpha = {
            .enable = 0,
            .value = 0xff,
        }
    };

    fb_set_cfg(fbdev, &cfg0);

    fb_pan_display(fbdev, 0);
}

void felix_h264_decoder_test(void)
{
    int width = VIDEO_WIDTH, height = VIDEO_HEIGHT;

    void *nv12 = memalign(256, ALIGN(width*height*3/2 * 2, cache_line_size()));
    assert(nv12);

    struct nalu_buf *nalu_buf = nalu_buf_init(256*1024);
    assert(nalu_buf);

    struct felix_h264_decoder_param param = {
        .width = width,
        .height = height,
    };

    struct felix_h264_decoder *decoder = felix_h264_decoder_init(&param);
    if (!decoder)
        goto delete_nalu_buf;

    void *data = (void *)h264Data;
    int size = h264Size;
    int count = 0;
    int i;

    struct fb_mem_info info;
    struct fbdev_data *fbdev = fb_open(0); /* 获得fb0设备 */

    fb_enable(fbdev);
    fb_get_info(fbdev, &info);

    fb_enable_cfg(fbdev);

    uint64_t start = 0;

    int cnt = 0;


    while (1) {

        int len = 0;
        struct nalu_frame *frame = nalu_buf_write(nalu_buf, data, size, &len);
        size -= len;
        data += len;
        if (!frame) {
            if (!len)
                break;
            continue;
        }

        void *dst = nv12 + cnt * (width * height * 3 / 2);

        int ret = felix_h264_decoder_decode(decoder, frame->data, frame->size, dst);
        nalu_frame_delete(frame);
        if (ret) {
            continue;
        }

        nv12_display(fbdev, dst);

        cnt = !cnt;
    }

    disable_irq(IRQ_LCD);
    release_irq(IRQ_LCD);

    disable_irq(IRQ_ROTATE);
    release_irq(IRQ_ROTATE);

    felix_h264_decoder_deinit(decoder);
delete_nalu_buf:
    nalu_buf_deinit(nalu_buf);
    free(nv12);
}

static char to_host_cmd[] = "boot_animation_end";

void boot_animation_example(void)
{
    felix_h264_decoder_test();

    /*通知大核，riscv 小核已经释放资源， 大核应用层mcu_read_str_timeout()接口应用获取，然后再触发相应的驱动*/
    host_cpu_write(to_host_cmd, sizeof(to_host_cmd));
    mcu_notify_host(sizeof(to_host_cmd));
}