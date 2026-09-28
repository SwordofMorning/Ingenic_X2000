#include <stdio.h>
#include <assert.h>

#include <libmedia/media_muxing.h>

struct media_muxing *media_muxing_open(struct media_muxing_param *param)
{
    assert(param->cb);
    assert(param->cb->open_muxing);

    struct media_muxing *muxing = param->cb->open_muxing(param);
    if (!muxing)
        return NULL;

    muxing->param = *param;

    return muxing;
}

void media_muxing_close(struct media_muxing *muxing)
{
    assert(muxing->param.cb->close_muxing);
    muxing->param.cb->close_muxing(muxing);
}

int media_muxing_one_pkt(struct media_muxing *muxing, struct media_packet *pkt)
{
    assert(muxing->param.cb->muxing_packet);
    return muxing->param.cb->muxing_packet(muxing, pkt);
}