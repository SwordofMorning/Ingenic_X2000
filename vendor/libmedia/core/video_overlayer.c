#include <stdio.h>
#include <assert.h>
#include <libmedia/video_overlayer.h>

struct video_overlayer *video_overlayer_open(struct video_overlayer_param *param, struct video_overlayer_param *fg_param, int fg_len)
{
    assert(param->cb);
    assert(param->cb->open_overlayer);

    struct video_overlayer *overlayer = param->cb->open_overlayer(param, fg_param, fg_len);
    if (!overlayer)
        return NULL;

    overlayer->param = *param;

    return overlayer;
}

void video_overlayer_close(struct video_overlayer *overlayer)
{
    assert(overlayer->param.cb->close_overlayer);
    overlayer->param.cb->close_overlayer(overlayer);
}

int video_overlayer_send_bg_frame(struct video_overlayer *overlayer, struct video_frame *frame)
{
    assert(overlayer->param.cb->send_bg_frame);
    return overlayer->param.cb->send_bg_frame(overlayer, frame);
}

int video_overlayer_send_fg_frame(struct video_overlayer *overlayer, struct video_frame *frame, int index)
{
    assert(overlayer->param.cb->send_fg_frame);
    return overlayer->param.cb->send_fg_frame(overlayer, frame, index);
}

int video_overlayer_get_frame(struct video_overlayer *overlayer, struct video_frame **frame)
{
    assert(overlayer->param.cb->get_frame);
    return overlayer->param.cb->get_frame(overlayer, frame);
}