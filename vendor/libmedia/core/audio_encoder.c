#include <libmedia/audio_encoder.h>

struct audio_encoder *audio_encoder_open(struct audio_encoder_param *param)
{
    assert(param->cb->open_encoder);

    struct audio_encoder *encoder = param->cb->open_encoder(param);
    if (!encoder)
        return NULL;

    encoder->param = *param;

    return encoder;

}

void audio_encoder_close(struct audio_encoder *encoder)
{
    assert(encoder->param.cb->close_encoder);
    encoder->param.cb->close_encoder(encoder);
}

int audio_encoder_write_frame(struct audio_encoder *encoder, struct audio_frame *frame)
{
    assert(encoder->param.cb->write_frame);
    return encoder->param.cb->write_frame(encoder, frame);
}

int audio_encoder_get_packet(struct audio_encoder *encoder, struct media_packet **pkt)
{
    assert(encoder->param.cb->get_packet);
    return encoder->param.cb->get_packet(encoder, pkt);
}

void audio_encoder_init_muxing_param(struct audio_encoder *encoder, struct media_muxing_audio_param *param)
{
    assert(encoder->param.cb->init_muxing_param);
    encoder->param.cb->init_muxing_param(encoder, param);
}