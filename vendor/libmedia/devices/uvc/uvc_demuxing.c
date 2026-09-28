#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <libmedia/uvc_demuxing.h>

#include <linux/videodev2.h>
#include <libmedia/media_packet.h>

struct uvc_demuxing {
    struct media_demuxing demuxing;

    struct v4l2_camera *camera;
    struct v4l2_camera_format *fmt;

    struct v4l2_camera_buffer buffers[3];
};

static int to_format(int video_format)
{
    switch (video_format)
    {
    case V4L2_PIX_FMT_YUYV: return VIDEO_yuyv;
    case V4L2_PIX_FMT_NV12: return VIDEO_nv12;
    case V4L2_PIX_FMT_H264: return VIDEO_pkt_h264;
    case V4L2_PIX_FMT_MJPEG: return VIDEO_pkt_mjpeg;
    default:
        return -1;
    }
}

static struct media_demuxing *uvc_demuxing_open(struct media_demuxing_param *param)
{
    int ret;
    struct uvc_demuxing_param *p = (void *)param;
    assert(p);
    assert(p->uvc_device);

    struct v4l2_camera_format *uvc_format = &p->uvc_format;

    struct uvc_demuxing *uvc = malloc(sizeof(*uvc));
    assert(uvc);

    struct v4l2_camera *camera = v4l2_camera_open(p->uvc_device);
    if (!camera) {
        fprintf(stderr, "uvc_demuxing: failed to open uvc device\n");
        goto free_uvc;
    }

    ret = v4l2_camera_detect_format(camera, uvc_format);
    if (ret) {
        fprintf(stderr, "uvc_demuxing: failed to detect format\n");
        goto close_camera;
    }

    int frame_format = to_format(uvc_format->format);
    if (ret == -1) {
        fprintf(stderr, "uvc_demuxing: not support this format: %d\n", frame_format);
        return NULL;
    }

    ret = v4l2_camera_create_buffer(camera, uvc_format, 3);
    if (ret) {
        fprintf(stderr, "uvc_demuxing: failed to create buffer\n");
        goto close_camera;
    }

    ret = v4l2_camera_stream_on(camera);
    if (ret) {
        fprintf(stderr, "uvc_demuxing: failed to stream on\n");
        goto close_camera;
    }

    uvc->fmt = uvc_format;
    uvc->camera = camera;

    return &uvc->demuxing;

close_camera:
    v4l2_camera_close(camera);
free_uvc:
    free(uvc);
    return NULL;
}

static void uvc_demuxing_close(struct media_demuxing *demuxing)
{
    struct uvc_demuxing *uvc = (void *)demuxing;
    struct v4l2_camera *camera = uvc->camera;

    v4l2_camera_stream_off(camera);
    v4l2_camera_close(camera);

    free(uvc);
}

static void demuxing_pkt_free(void *handle, struct media_packet *pkt)
{
    struct uvc_demuxing *uvc = handle;

    struct v4l2_camera_buffer *buf = (void *)pkt->pdata;

    v4l2_camera_queue_buffer(uvc->camera, buf);

    media_packet_free(pkt);

}

static int uvc_demuxing_one_pkt(struct media_demuxing *media_demuxing, struct media_packet **dst_pkt)
{
    int ret;
    struct uvc_demuxing *uvc = (void *)media_demuxing;

    struct v4l2_camera_format *uvc_format = uvc->fmt;

    struct v4l2_camera_buffer buf;
    ret = v4l2_camera_dequeue_buffer(uvc->camera, &buf, 1000);
    if (ret) {
        fprintf(stderr, "uvc_demuxing: failed to dequeue buffer\n");
        return -1;
    }

    int index = buf.index;
    uvc->buffers[index] = buf;

    struct media_packet *pkt = media_packet_alloc();
    pkt->data = buf.data[0];
    pkt->size = buf.size[0];
    pkt->handle = uvc;
    pkt->pdata = &uvc->buffers[index];
    pkt->put_packet = demuxing_pkt_free;
    pkt->type = to_format(uvc_format->format);
    *dst_pkt = pkt;

    media_packet_get(pkt);

    return 0;
}

static int uvc_demuxing_get_video_param(struct media_demuxing *demuxing, struct video_decoder_param *param)
{
    struct uvc_demuxing *uvc = (void *)demuxing;
    param->codec_name = to_format(uvc->fmt->format) == VIDEO_pkt_h264 ? "h264" : "mjpeg";
    param->width   = uvc->fmt->width;
    param->height  = uvc->fmt->height;
    param->extradata = NULL;
    param->extradata_size = 0;

    return 0;
}

struct media_demuxing_cb uvc_demuxing_cb = {
    .open_demuxing = uvc_demuxing_open,
    .close_demuxing = uvc_demuxing_close,
    .demuxing_packet = uvc_demuxing_one_pkt,
    .get_video_param = uvc_demuxing_get_video_param,
};

void uvc_demuxing_init_param(struct media_demuxing_param *param)
{
    param->cb = &uvc_demuxing_cb;
}