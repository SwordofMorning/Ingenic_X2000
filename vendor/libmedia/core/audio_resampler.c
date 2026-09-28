#include <libmedia/audio_resampler.h>


struct audio_resampler *audio_resampler_create(struct audio_resampler_param *param)
{
    assert(param->cb);
    assert(param->cb->create_resampler);

    struct audio_resampler *resampler = param->cb->create_resampler(param);
    if(!resampler)
        return NULL;

    resampler->param = *param;

    return resampler;
}

int audio_resampler_convert(struct audio_resampler *resampler, struct audio_frame *frame_in, struct audio_frame **frame_out)
{
    assert(resampler->param.cb->convert_audio);
    return resampler->param.cb->convert_audio(resampler, frame_in, frame_out);
}

void audio_resampler_delete(struct audio_resampler *resampler)
{
    assert(resampler->param.cb->delete_resampler);
    resampler->param.cb->delete_resampler(resampler);
}

