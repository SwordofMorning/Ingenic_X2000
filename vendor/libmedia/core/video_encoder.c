#include <libmedia/video_encoder.h>
#include <assert.h>


struct video_encoder *video_encoder_open(struct video_encoder_param *param)
{
    assert(param->cb->open_encoder);

    struct video_encoder *encoder = param->cb->open_encoder(param);
    if (!encoder)
        return NULL;

    encoder->param = *param;

    return encoder;
}

void video_encoder_close(struct video_encoder *encoder)
{
    assert(encoder->param.cb->close_encoder);
    encoder->param.cb->close_encoder(encoder);
}

void video_encoder_set_next_keyframe(struct video_encoder *encoder)
{
    assert(encoder->param.cb->set_next_keyframe);
    return encoder->param.cb->set_next_keyframe(encoder);
};

int video_encoder_write_frame(struct video_encoder *encoder, struct video_frame *frame)
{
    assert(encoder->param.cb->write_frame);
    return encoder->param.cb->write_frame(encoder, frame);
}

int video_encoder_get_packet(struct video_encoder *encoder, struct media_packet **pkt)
{
    assert(encoder->param.cb->get_pkt);
    return encoder->param.cb->get_pkt(encoder, pkt);
}

void video_encoder_init_muxing_param(struct video_encoder *encoder, struct media_muxing_video_param *param)
{
    assert(encoder->param.cb->init_muxing_param);
    return encoder->param.cb->init_muxing_param(encoder, param);
}


