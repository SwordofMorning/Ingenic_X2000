#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libmedia/play/async_video_player.h>

struct async_player {
    struct video_player play;
    struct video_player *player;
    int is_opened;
    struct async_video_player_param param;
};

struct video_player *async_video_player_open(struct video_player_param *param)
{
    struct async_video_player_param *p = (void *)param;

    struct async_player *player = malloc(sizeof(*player));
    assert(player);

    player->param = *p;
    player->player = NULL;
    player->is_opened = 0;

    return &player->play;
}

void async_video_player_close(struct video_player *player, int is_disable)
{
    struct async_player *async = (void *) player;
    if (async->player)
        video_player_close(async->player, is_disable);
    free(async);
}

static struct video_player *wait_init_player(struct async_player *async)
{
    struct async_video_player_param *p = &async->param;

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

    async->player = video_player_open(p->device_param);
out:
    async->is_opened = 1;
    return async->player;
}

int async_video_player_set_display_config(struct video_player *player_, struct video_display_config *disp_cfg)
{
    struct async_player *async = (void *) player_;
    struct video_player *player = wait_init_player(async);
    if (!player)
        return -1;
    return video_player_set_display_config(player, disp_cfg);
}

int async_video_player_set_media_frame(struct video_player *player_, struct video_frame *frame)
{
    struct async_player *async = (void *) player_;
    struct video_player *player = wait_init_player(async);
    if (!player)
        return -1;
    return video_player_set_media_frame(player, frame);
}

int async_video_player_display(struct video_player *player_)
{
    struct async_player *async = (void *) player_;
    struct video_player *player = wait_init_player(async);
    if (!player)
        return -1;
    return video_player_display(player);
}

struct video_player_cb async_video_player_cb = {
    .open_player = async_video_player_open,
    .close_player = async_video_player_close,
    .display = async_video_player_display,
    .set_display_config = async_video_player_set_display_config,
    .set_media_frame = async_video_player_set_media_frame,
};

void async_video_player_init_param(
    struct async_video_player_param *p,
    const char *device_node, struct video_player_param *device_param)
{
    p->param = *device_param;
    p->param.cb = &async_video_player_cb;
    p->device_node = device_node;
    p->device_param = device_param;
}
