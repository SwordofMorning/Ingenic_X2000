#include <stdio.h>
#include <assert.h>

#include <libmedia/video_rotater.h>


struct video_rotater *video_rotater_open(struct video_rotater_param *param)
{
    assert(param->cb);
    assert(param->cb->open_rotater);

    struct video_rotater *rotater = param->cb->open_rotater(param);
    if (!rotater)
        return NULL;

    rotater->param = *param;

    return rotater;
}

void video_rotater_close(struct video_rotater *rotater)
{
    assert(rotater->param.cb->close_rotater);
    rotater->param.cb->close_rotater(rotater);
}

int video_rotater_convert_video_frame(struct video_rotater *rotater, struct video_frame *src_frame, struct video_frame **dst_frame)
{
    assert(rotater->param.cb->convert_video_frame);
    return rotater->param.cb->convert_video_frame(rotater, src_frame, dst_frame);
}

void video_rotater_set_rotate(struct video_rotater *rotater, enum video_rotate_angle angle, int hflip, int vflip)
{
    rotater->param.hflip = hflip;
    rotater->param.vflip = vflip;
    rotater->param.rotate_angle = angle;
}
