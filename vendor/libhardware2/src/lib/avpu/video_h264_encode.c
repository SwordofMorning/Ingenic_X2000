/*
 * avpu应用调用接口
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <libhardware2/avpu_h264_encode.h>

#define ALIGN(d,a)                      (((d)+((a)-1))/(a)*(a))

int avpu_h264_encoder_yuv_init(void **h, struct avpu_h264_encoder_config *config);
int avpu_h264_encoder_yuv_work(void *h, uint32_t vaddr, uint32_t paddr, uint32_t *out_length, uint8_t *out_frame);
int avpu_h264_encoder_yuv_deinit(void *h);

#define pr_err(msg) \
    fprintf(stderr, "error: %s: %s\n", msg, strerror(errno))

struct video_buffer {
    /*source*/
    void *src_map_addr;
    void *src_phy_addr;
    uint32_t frame_size;
    unsigned int width;
    unsigned int height;
    unsigned int uv_offset; /* AVPU高度 16像素对齐 */
};

struct avpu_h264_encoder {
    void *handler;
    struct video_buffer buffer;
};


uint32_t avpu_h264_encoder_get_frame_size(struct avpu_h264_encoder *encoder)
{
    return encoder->buffer.frame_size;
}

uint32_t avpu_h264_encoder_get_uv_offset(struct avpu_h264_encoder *encoder)
{
    return encoder->buffer.uv_offset;
}


struct avpu_h264_encoder *avpu_h264_encoder_open(struct avpu_h264_encoder_config *config)
{
    void *h = NULL;
    int ret;

    ret = avpu_encoder_init();
    if (ret < 0) {
        pr_err("avpu encoder inir");
        return NULL;
    }

    ret = avpu_h264_encoder_yuv_init(&h, config);
    if ((ret < 0) || (h == NULL)) {
        pr_err("video h264 avpu encoder yuv init failed\n");
        ret = -1;
        goto err_encoder_yuvinit;
    }

    struct avpu_h264_encoder *encoder = malloc(sizeof(*encoder));
    assert(encoder);

    int width = config->width;
    int height = config->height;
    int frame_size = width * (ALIGN(height, 16)) * 3 / 2;

    encoder->buffer.width = width;
    encoder->buffer.height = height;
    encoder->buffer.frame_size = frame_size;
    encoder->buffer.uv_offset = width * (ALIGN(height, 16));

    encoder->handler = h;

    return encoder;

err_encoder_yuvinit:
    avpu_encoder_deinit();
    return NULL;
}

int avpu_h264_encoder_work(struct avpu_h264_encoder *encoder, void *map_vaddr, uint32_t *out_length, void *out_frame)
{
    encoder->buffer.src_map_addr = map_vaddr;
    encoder->buffer.src_phy_addr = avpu_encoder_vitr_to_phys(map_vaddr);

    return avpu_h264_encoder_yuv_work(encoder->handler, (uint32_t)encoder->buffer.src_map_addr, (uint32_t)encoder->buffer.src_phy_addr, out_length, out_frame);
}


int avpu_h264_encoder_close(struct avpu_h264_encoder *encoder)
{
    avpu_h264_encoder_yuv_deinit(encoder->handler);

    avpu_encoder_deinit();

    free(encoder);

    return 0;
}