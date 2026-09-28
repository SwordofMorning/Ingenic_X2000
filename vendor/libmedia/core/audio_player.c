#include <libmedia/audio_player.h>

struct audio_player *audio_player_open(struct audio_player_param *param)
{
    assert(param->cb);
    assert(param->cb->open_player);

    struct audio_player *player = param->cb->open_player(param);
    if (!player)
        return NULL;

    player->param = *param;

    return player;
}

int audio_player_display_audio(struct audio_player *player, struct audio_frame *frame)
{
    assert(player->param.cb->display_audio);

    return player->param.cb->display_audio(player, frame);
}

void audio_player_close(struct audio_player *player)
{
    assert(player->param.cb->close_player);

    return player->param.cb->close_player(player);
}