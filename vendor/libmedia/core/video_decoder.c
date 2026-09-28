
#include <stdio.h>
#include <assert.h>

#include <libmedia/video_decoder.h>

struct video_decoder *video_decoder_open(struct video_decoder_param *param)
{
    assert(param->cb);
    assert(param->cb->open_decoder);

    struct video_decoder *decoder = param->cb->open_decoder(param);
    if (!decoder)
        return NULL;

    decoder->param = *param;

    return decoder;
}

void video_decoder_close(struct video_decoder *decoder)
{
    assert(decoder->param.cb->close_decoder);
    decoder->param.cb->close_decoder(decoder);
}

int video_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt)
{
    assert(decoder->param.cb->send_pkt);
    return decoder->param.cb->send_pkt(decoder, pkt);
}

int video_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame)
{
    assert(decoder->param.cb->get_frame);
    return decoder->param.cb->get_frame(decoder, dst_frame);
}