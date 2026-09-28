#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <libmedia/read/uvc_video_reader.h>
#include <libmedia/decode/hw_jpeg_decoder.h>
#include <libmedia/video_reader.h>
#include <libmedia/video_decoder.h>

#include <linux/videodev2.h>

struct uvc_video_reader {
    struct video_reader reader;

    struct v4l2_camera *camera;
    struct v4l2_camera_format *fmt;

    struct video_decoder *video_decoder;

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

static int is_compress_format(int frame_format)
{
    if (frame_format == VIDEO_pkt_mjpeg || frame_format == VIDEO_pkt_h264)
        return 1;
    return 0;
}

static struct video_reader *uvc_video_reader_open(struct video_reader_param *param)
{
    int ret;
    struct video_decoder *decoder = NULL;
    struct uvc_video_reader_param *p = (void *)param;
    assert(p);
    assert(p->uvc_device);

    struct v4l2_camera_format *uvc_format = &p->uvc_format;

    struct uvc_video_reader *uvc = malloc(sizeof(*uvc));
    assert(uvc);

    struct v4l2_camera *camera = v4l2_camera_open(p->uvc_device);
    if (!camera) {
        fprintf(stderr, "uvc_reader: failed to open uvc device\n");
        goto free_uvc;
    }

    ret = v4l2_camera_detect_format(camera, uvc_format);
    if (ret) {
        fprintf(stderr, "uvc_reader: failed to detect format\n");
        goto close_camera;
    }

    int frame_format = to_format(uvc_format->format);
    if (ret == -1) {
        fprintf(stderr, "uvc_reader: not support this format: %d\n", frame_format);
        return NULL;
    }

    if (is_compress_format(frame_format)) {
        if (p->decoder_param) {
            decoder = video_decoder_open(p->decoder_param);
            if (!decoder) {
                fprintf(stderr, "uvc_reader: failed to open video decoder \n");
                return NULL;
            }
        } else {
            fprintf(stderr, "uvc_reader: compress format not set decoder param \n");
            return NULL;
        }
    }

    ret = v4l2_camera_create_buffer(camera, uvc_format, 3);
    if (ret) {
        fprintf(stderr, "uvc_reader: failed to create buffer\n");
        goto close_camera;
    }

    ret = v4l2_camera_stream_on(camera);
    if (ret) {
        fprintf(stderr, "uvc_reader: failed to stream on\n");
        goto close_camera;
    }

    uvc->fmt = uvc_format;
    uvc->camera = camera;
    uvc->video_decoder = decoder;

    return &uvc->reader;

close_camera:
    v4l2_camera_close(camera);
free_uvc:
    free(uvc);
    return NULL;
}

static void uvc_video_reader_close(struct video_reader *reader)
{
    struct uvc_video_reader *uvc = (void *)reader;
    struct v4l2_camera *camera = uvc->camera;

    v4l2_camera_stream_off(camera);
    v4l2_camera_close(camera);

    free(uvc);
}

static void uvc_video_reader_put_video(void *handle, struct video_frame *frame)
{
    struct uvc_video_reader *uvc = handle;

    struct v4l2_camera_buffer *buf = (void *)frame->pdata;

    v4l2_camera_queue_buffer(uvc->camera, buf);

    free(frame);
}

static struct video_frame *uvc_init_media_frame(struct uvc_video_reader *uvc, struct v4l2_camera_buffer *buf)
{
    int ret;
    struct v4l2_camera_format *fmt = uvc->fmt;
    struct video_decoder *video_decoder = NULL;
    int index = buf->index;

    uvc->buffers[index] = *buf;

    if (uvc->video_decoder)
        video_decoder = uvc->video_decoder;

    int frame_format = to_format(fmt->format);

    if (video_decoder) {
        struct media_packet pkt = {
            .data = buf->data[0],
            .size = buf->size[0],
            .type = frame_format,
        };

        ret = video_decoder_send_pkt(video_decoder, &pkt);
        if (ret) {
            fprintf(stderr, "uvc_reader: (decoder)failed send pkt\n");
            v4l2_camera_queue_buffer(uvc->camera, buf);
            return NULL;
        }

        struct video_frame *m_frame = NULL;

        ret = video_decoder_get_frame(video_decoder, &m_frame);
        if (ret)
            fprintf(stderr, "uvc_reader: (decoder)failed get frame\n");

        ret = v4l2_camera_queue_buffer(uvc->camera, buf);
        if (ret) {
            fprintf(stderr, "uvc_reader: (camera)failed queue buffer\n");
            return NULL;
        }

        return m_frame;

    } else {
        struct video_frame *frame = malloc(sizeof(*frame));
        enum video_frame_format format = to_format(fmt->format);
        video_frame_init(frame, uvc->fmt->width, uvc->fmt->height, format,
                             0, buf->data[0], 0, uvc, uvc_video_reader_put_video);
        frame->pdata = &uvc->buffers[index];

        return frame;
    }
    return NULL;
}

static int uvc_video_reader_read_video(struct video_reader *reader, struct video_frame **frame)
{
    int ret;
    struct uvc_video_reader *uvc = (void *)reader;
    struct v4l2_camera_buffer buf;

    ret = v4l2_camera_dequeue_buffer(uvc->camera, &buf, 0);

    if (ret < 0) {
        fprintf(stderr, "uvc_reader: failed to dequeue buffer\n");
        return -1;
    }

    if (ret == 1)
        return 1;

    *frame = uvc_init_media_frame(uvc, &buf);
    if (!frame) {
        fprintf(stderr, "uvc_reader: failed to init frame\n");
        return -1;
    }

    return 0;
}

struct video_reader_cb uvc_video_read_cb = {
    .open_reader = uvc_video_reader_open,
    .close_reader = uvc_video_reader_close,
    .read_video = uvc_video_reader_read_video,
};

void uvc_video_reader_init_param(
    struct uvc_video_reader_param *uvc_param)
{
    struct video_reader_param *param = &uvc_param->param;

    param->width = uvc_param->uvc_format.width;
    param->height = uvc_param->uvc_format.height;
    param->pixel_fmt = VIDEO_nv12;
    param->ignore_video_encoder_linesize = 1;
    param->cb = &uvc_video_read_cb;
}