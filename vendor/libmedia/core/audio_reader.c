#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <libmedia/audio_reader.h>

struct audio_reader *audio_reader_open(struct audio_reader_param *param)
{
    assert(param->cb);
    assert(param->cb->open_reader);

    struct audio_resampler *resampler = NULL;

    if (param->resampler_param) {
        resampler = audio_resampler_create(param->resampler_param);
        if (!resampler) {
            fprintf(stderr, "audio reader: failed to create resampler\n");
            return NULL;
        }
    }

    struct audio_reader *reader = param->cb->open_reader(param);
    if (!reader) {
        if (resampler)
            audio_resampler_delete(resampler);
        return NULL;
    }

    reader->resampler = resampler;
    reader->param = *param;

    return reader;
}

void audio_reader_close(struct audio_reader *reader)
{
    assert(reader->param.cb->close_reader);
    if (reader->resampler)
        audio_resampler_delete(reader->resampler);

    reader->param.cb->close_reader(reader);
}

int audio_reader_read_frame(struct audio_reader *reader, struct audio_frame **frame)
{
    assert(reader->param.cb->read_audio);
    int ret;

    struct audio_frame *read_frame = NULL;

    ret = reader->param.cb->read_audio(reader, &read_frame);
    if (ret < 0)
        return ret;

    if (reader->resampler) {
        ret = audio_resampler_convert(reader->resampler, read_frame, frame);
        audio_frame_put(read_frame);
        if (ret == -MEDIA_EAGAIN || ret == -MEDIA_EOF)
            return ret;
        if(ret < 0) {
            fprintf(stderr, "audio reader: failed to convert audio\n");
            return ret;
        }
    } else {
        *frame = read_frame;
    }

    return 0;
}

int audio_reader_drop_audio(struct audio_reader *reader)
{
    assert(reader->param.cb->drop_audio);

    return reader->param.cb->drop_audio(reader);
}

int audio_reader_avail_audio(struct audio_reader *reader)
{
    assert(reader->param.cb->avail_audio);

    return reader->param.cb->avail_audio(reader);
}
