#include <types.h>
#include <felix_drv.h>
#include <stdlib.h>
#include <string.h>

#include <felix_h264_decoder.h>
#include <cpu/io.h>

struct felix_h264_decoder {
    struct ingenic_vdec_ctx *ctx;
    void *buf[3];
    int size[3];
    int width;
    int height;
};


extern struct vpu_ops ingenic_vpu_ops;
extern int vpu_on(void);

int ingenic_vcodec_felix_set_param(void *data, int width, int height, int format)
{
	struct ingenic_vdec_ctx *ctx = (struct ingenic_vdec_ctx *)data;
	int ret = 0;
	ctx->sWidth = width;
	ctx->sHeight = height;

	ctx->avctx->width = width;
	ctx->avctx->height = height;

	h264_set_vpu_ops(ctx->h, ctx, &ingenic_vpu_ops);

	// VPU ON
	vpu_on();

	h264_set_output_format(ctx->h, format, width, height);

	return 0;
}

struct felix_h264_decoder *felix_h264_decoder_init(struct felix_h264_decoder_param *param)
{
    ingenic_felix_init();

    struct felix_h264_decoder *decoder = malloc(sizeof(*decoder));
    memset(decoder, 0, sizeof(*decoder));

    decoder->ctx = ingenic_felix_ctx_init();

    decoder->width = param->width;
    decoder->height = param->height;


    int ret = ingenic_vcodec_felix_set_param(decoder->ctx, param->width, param->height, 2);
    if (ret) {
        printf("h264: failed to set param\n");
        goto deinit_ctx;
    }

    return decoder;
deinit_ctx:
    ingenic_felix_ctx_deinit(decoder->ctx);
    free(decoder);
    ingenic_felix_deinit();
    return NULL;
}

static void *check_alloc_mem(struct felix_h264_decoder *decoder, int index, int size)
{
    if (decoder->buf[index]) {
        if (decoder->size[index] >= size)
            return decoder->buf[index];
        free(decoder->buf[index]);
    }

    decoder->buf[index] = memalign(256, ALIGN(size, cache_line_size()));
    if (!decoder->buf[index])
        printf("h264: faield alloc tmp buf[%d]: %d\n", index, size);
    decoder->size[index] = size;
    return decoder->buf[index];
}

/*
 * felix_buffer_t 这个结构体本身没有什么意义
 * 主要是结构体里面的 frame.buf
 */
static void init_addr(felix_buffer_t *addr, AVBuffer *buf0, AVBuffer *buf1, void *data0, int sz0, void *data1, int sz1)
{
    memset(addr, 0, sizeof(*addr));
    addr->vaddr = (unsigned long) data0;
    addr->paddr = virt_to_phys(data0);
    addr->index = 0;
    addr->size = sz0 + sz1;
    addr->frame.buf[0] = buf0;
    addr->frame.buf[1] = buf1;
    if (buf0) {
        buf0->buffer = data0;
        buf0->buffer_pa = virt_to_phys(data0);
        buf0->size = sz0;
    }
    if (buf1) {
        buf1->buffer = data1;
        buf1->buffer_pa = virt_to_phys(data1);
        buf1->size = sz1;
    }
}

int felix_h264_decoder_decode_nv12_separate(
    struct felix_h264_decoder *decoder, void *src_, int src_size, void *y_, void *uv_)
{
	struct ingenic_vdec_ctx *ctx = decoder->ctx;
	AVCodecContext *avctx = ctx->avctx;
    H264Context *h = ctx->h;

    int width = decoder->width;
    int height = decoder->height;

    void *src = src_;
    void *y = y_;
    void *uv = uv_;



    if ((unsigned long)src % 256) {
        src = check_alloc_mem(decoder, 0, src_size);
        if (!src)
            return -ENOMEM;
        memcpy(src, src_, src_size);
    }

    if ((unsigned long)y % 256) {
        y = check_alloc_mem(decoder, 1, width*height);
        if (!y)
            return -ENOMEM;
    }

    if ((unsigned long)uv % 256) {
        uv = check_alloc_mem(decoder, 2, width*height/2);
        if (!uv)
            return -ENOMEM;
    }

    AVPacket avpkt;
    avpkt.data = src;
    avpkt.data_pa = virt_to_phys(src);
    avpkt.size = src_size;

	felix_buffer_t dst_addr;
    AVBuffer dst_buf0;
    AVBuffer dst_buf1;
    init_addr(&dst_addr, &dst_buf0, &dst_buf1, y, width*height, uv, width*height/2);

    h264_enqueue_frame(h, &dst_addr.frame);

    int got_frame = 0;
    int ret = h264_decode_frame(avctx, &ctx->dec_frame, &got_frame, &avpkt);
    if (ret < 0) {
        printf("h264 decoder: failed to decode: %d\n", ret);

        h264_dequeue_frame(h);
        return ret;
    }


    h264_dequeue_frame(h);

    if (y != y_)
        memcpy(y_, y, width*height);

    if (uv != uv_)
        memcpy(uv_, uv, width*height/2);

    if (!got_frame) {
        printf("h264 decoder: not get frame\n");
        return -EAGAIN;
    }


    return 0;
}

int felix_h264_decoder_decode(
    struct felix_h264_decoder *decoder, void *src, int src_size, void *nv12)
{
    return felix_h264_decoder_decode_nv12_separate(
        decoder, src, src_size, nv12, nv12+decoder->width*decoder->height);
}

void felix_h264_decoder_deinit(struct felix_h264_decoder *decoder)
{
    ingenic_felix_ctx_deinit(decoder->ctx);
    if (decoder->buf[0]) free(decoder->buf[0]);
    if (decoder->buf[1]) free(decoder->buf[1]);
    if (decoder->buf[2]) free(decoder->buf[2]);
    free(decoder);
    ingenic_felix_deinit();
}

