#include <stdio.h>
#include <assert.h>

#include <libmedia/media_demuxing.h>

struct media_demuxing *demuxing_open(struct media_demuxing_param *param)
{
    assert(param->cb);
    assert(param->cb->open_demuxing);

    struct media_demuxing *demuxing = param->cb->open_demuxing(param);
    if (!demuxing)
        return NULL;

    demuxing->param = *param;

    return demuxing;
}

void demuxing_close(struct media_demuxing *demuxing)
{
    assert(demuxing->param.cb->close_demuxing);

    demuxing->param.cb->close_demuxing(demuxing);
}

int demuxing_get_video_param(struct media_demuxing *demuxing, struct video_decoder_param *param)
{
    assert(demuxing->param.cb->get_video_param);
    return demuxing->param.cb->get_video_param(demuxing, param);
}

int demuxing_get_audio_param(struct media_demuxing *demuxing, struct audio_decoder_param *param)
{
    assert(demuxing->param.cb->get_audio_param);
    return demuxing->param.cb->get_audio_param(demuxing, param);
}

int demuxing_one_pkt(struct media_demuxing *demuxing, struct media_packet **pkt)
{
    assert(demuxing->param.cb->demuxing_packet);
    return demuxing->param.cb->demuxing_packet(demuxing, pkt);
}

uint64_t demuxing_get_pts(struct media_demuxing *demuxing, struct media_packet *pkt)
{
    assert(demuxing->param.cb->get_pts);
    return demuxing->param.cb->get_pts(demuxing, pkt);
}

uint64_t demuxing_get_duration(struct media_demuxing *demuxing)
{
    assert(demuxing->param.cb->get_duration);
    return demuxing->param.cb->get_duration(demuxing);
}

int demuxing_get_metadata(struct media_demuxing *demuxing, struct media_metadata *metadata)
{
    assert(demuxing->param.cb->get_metadata);
    if (metadata == NULL || metadata->key == NULL) {
        fprintf(stderr, "querying metadata cant be null\n");
        return -1;
    }
    return demuxing->param.cb->get_metadata(demuxing, metadata);
}

int demuxing_seek_forward(struct media_demuxing *demuxing, int64_t abs_us)
{
    assert(demuxing->param.cb->seek_forward);
    return demuxing->param.cb->seek_forward(demuxing, abs_us);
}

int demuxing_seek_backward(struct media_demuxing *demuxing, int64_t abs_us)
{
    assert(demuxing->param.cb->seek_backward);
    return demuxing->param.cb->seek_backward(demuxing, abs_us);
}