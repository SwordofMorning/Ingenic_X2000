#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <turbojpeg.h>
#include <libmedia/video_decoder.h>
#include <libmedia/media_alloter.h>
#include <libmedia/decode/turbo_jpeg_video_decoder.h>

struct turbo_jpeg_video_decoder {
    struct video_decoder decoder;
    struct video_frame *frame;
};

static struct video_decoder *turbo_jpeg_video_decoder_open_decoder(struct video_decoder_param *param)
{
    struct turbo_jpeg_video_decoder *turbo = malloc(sizeof(*turbo));

    memset(turbo, 0, sizeof(*turbo));

    return &turbo->decoder;
}

static void turbo_jpeg_video_decoder_close_decoder(struct video_decoder *decoder)
{
    struct turbo_jpeg_video_decoder *turbo = (void *)decoder;
    if (turbo->frame)
        video_frame_put(turbo->frame);
    free(turbo);
}

static int to_format(int inSubsamp)
{
    switch (inSubsamp) {
    case TJSAMP_444: return VIDEO_yuv444p;
    case TJSAMP_422: return VIDEO_yuv422p;
    case TJSAMP_420: return VIDEO_yuv420p;
    case TJSAMP_GRAY: return VIDEO_y8;
    case TJSAMP_411: return VIDEO_yuv411p;
    default:
        return -1;
    }
}

static int turbo_jpeg_video_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt)
{
    struct turbo_jpeg_video_decoder *turbo = (void *)decoder;
    int ret, err = -1;
    int width, height;
    int inSubsamp = -1, inColorspace;

    if (turbo->frame) {
        video_frame_put(turbo->frame);
        turbo->frame = NULL;
    }

    tjhandle handle = tjInitDecompress();
    if (handle == NULL) {
        fprintf(stderr, "turbojpeg: failed to init\n");
        return -1;
    }

    ret = tjDecompressHeader3(handle, pkt->data, pkt->size,
                     &width, &height, &inSubsamp, &inColorspace);
    if (ret < 0)
    {
        fprintf(stderr, "turbojpeg: faield to read header: %s\n", tjGetErrorStr2(handle));
        goto delete_handle;
    }

    // printf("%d %d %d %d\n", width, height, inColorspace, inSubsamp);

    if (inColorspace != TJCS_YCbCr) {
        fprintf(stderr, "turbojpeg: not support this colorsapce now: %d\n", inColorspace);
        goto delete_handle;
    }

    if (decoder->param.width)
        width = decoder->param.width;
    if (decoder->param.height)
        height = decoder->param.height;

    enum video_frame_format format = to_format(inSubsamp);
    if (format == -1) {
        fprintf(stderr, "turbojpeg: not support this jpeg: %d %d\n", inColorspace, inSubsamp);
        goto delete_handle;
    }

    struct video_frame *frame =
        video_frame_alloc_init(width, height, format, 4);
    if (!frame) {
        fprintf(stderr, "turbojpeg: failed to alloc %dx%d %d\n", width, height, format);
        goto delete_handle;
    }

    ret = tjDecompressToYUV2(
            handle, pkt->data, pkt->size, frame->data[0], width, 4, height, 0);
    if (ret < 0) {
        fprintf(stderr, "turbojpeg: failed to decode: %s\n", tjGetErrorStr2(handle));
        goto delete_handle;
    }

    turbo->frame = frame;

    err = 0;
delete_handle:
    tjDestroy(handle);
    return err;
}

static int turbo_jpeg_video_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame)
{
    struct turbo_jpeg_video_decoder *turbo = (void *)decoder;

    if (!turbo->frame)
        return -MEDIA_EAGAIN;

    *dst_frame = turbo->frame;
    turbo->frame = NULL;

    return 0;
}

struct video_decoder_cb turbo_jpeg_decoder_cb = {
    .open_decoder = turbo_jpeg_video_decoder_open_decoder,
    .close_decoder = turbo_jpeg_video_decoder_close_decoder,
    .send_pkt = turbo_jpeg_video_decoder_send_pkt,
    .get_frame = turbo_jpeg_video_decoder_get_frame,
};

void turbo_jpeg_video_decoder_init_param(struct turbo_jpeg_video_decoder_param *param)
{
    param->param.cb = &turbo_jpeg_decoder_cb;
}
