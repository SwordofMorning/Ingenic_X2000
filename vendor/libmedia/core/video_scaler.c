#include <stdio.h>
#include <assert.h>
#include <libmedia/video_scaler.h>

struct video_scaler *video_scaler_open(struct video_scaler_param *param)
{
    assert(param->cb);
    assert(param->cb->open_scaler);

    struct video_scaler *scaler = param->cb->open_scaler(param);
    if (!scaler)
        return NULL;

    scaler->param = *param;

    return scaler;
}

void video_scaler_close(struct video_scaler *scaler)
{
    assert(scaler->param.cb->close_scaler);
    scaler->param.cb->close_scaler(scaler);
}

int video_scaler_send_frame(struct video_scaler *scaler, struct video_frame *frame)
{
    assert(scaler->param.cb->send_frame);
    return scaler->param.cb->send_frame(scaler, frame);
}

int video_scaler_get_frame(struct video_scaler *scaler, struct video_frame **frame)
{
    assert(scaler->param.cb->get_frame);
    return scaler->param.cb->get_frame(scaler, frame);
}

int video_frame_scale(struct video_scaler_param *param, struct video_frame *src_frame,
                      struct video_frame **dst_frame, int w, int h)
{
    int ret;
    param->dst_width = w;
    param->dst_height = h;

    struct video_scaler *scaler = video_scaler_open(param);
    if (!scaler)
        return -1;

    ret = video_scaler_send_frame(scaler, src_frame);
    if (ret < 0) {
        video_scaler_close(scaler);
        return -1;
    }

    ret = video_scaler_get_frame(scaler, dst_frame);
    if (ret < 0) {
        video_scaler_close(scaler);
        return -1;
    }

    video_scaler_close(scaler);

    return 0;
}
