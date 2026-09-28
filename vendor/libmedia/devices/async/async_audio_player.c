#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libmedia/play/async_audio_player.h>

struct async_player {
    struct audio_player play;
    struct audio_player *player;
    int is_opened;
    struct async_audio_player_param param;
};

struct audio_player *async_audio_player_open(struct audio_player_param *param)
{
    struct async_audio_player_param *p = (void *)param;

    struct async_player *player = malloc(sizeof(*player));
    assert(player);

    player->param = *p;
    player->player = NULL;
    player->is_opened = 0;

    return &player->play;
}

void async_audio_player_close(struct audio_player *player)
{
    struct async_player *async = (void *) player;
    if (async->player)
        audio_player_close(async->player);
    free(async);
}

static struct audio_player *wait_init_player(struct async_player *async)
{
    struct async_audio_player_param *p = &async->param;

    if (async->is_opened)
        return async->player;

    if (p->device_node) {
        int count = 100;
        while (access(p->device_node, F_OK)) {
            usleep(10*1000);
            if (count-- == 0) {
                fprintf(stderr, "async player: wait device: %s timeout\n", p->device_node);
                goto out;
            }
        }
    }

    async->player = audio_player_open(p->device_param);
out:
    async->is_opened = 1;
    return async->player;
}

int async_audio_player_display_audio(struct audio_player *player_, struct audio_frame *frame)
{
    struct async_player *async = (void *) player_;
    struct audio_player *player = wait_init_player(async);
    if (!player)
        return -1;
    return audio_player_display_audio(player, frame);
}

struct audio_player_cb async_audio_player_cb = {
    .open_player = async_audio_player_open,
    .close_player = async_audio_player_close,
    .display_audio = async_audio_player_display_audio,
};

void async_audio_player_init_param(
    struct async_audio_player_param *p,
    const char *device_node, struct audio_player_param *device_param)
{
    p->param = *device_param;
    p->param.cb = &async_audio_player_cb;
    p->device_node = device_node;
    p->device_param = device_param;
}
