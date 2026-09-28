#include <libmedia/audio_decoder.h>

struct audio_decoder *audio_decoder_open(struct audio_decoder_param *param)
{
    assert(param->cb->open_decoder);

    struct audio_decoder *decoder = param->cb->open_decoder(param);
    if (!decoder)
        return NULL;

    decoder->param = *param;

    return decoder;
}

void audio_decoder_close(struct audio_decoder *decoder)
{
    assert(decoder->param.cb->close_decoder);
    decoder->param.cb->close_decoder(decoder);
}

int audio_decoder_send_pkt(struct audio_decoder *decoder, struct media_packet *pkt)
{
    assert(decoder->param.cb->send_pkt);
    return decoder->param.cb->send_pkt(decoder, pkt);
}

int audio_decoder_get_frame(struct audio_decoder *decoder, struct audio_frame **frame)
{
    assert(decoder->param.cb->get_frame);
    return decoder->param.cb->get_frame(decoder, frame);
}