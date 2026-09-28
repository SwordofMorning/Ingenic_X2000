#include <stdio.h>
#include <assert.h>

#include <libmedia/video_player.h>
#include <libmedia/video_scaler.h>
#include <libhardware2/fb.h>


struct video_player *video_player_open(struct video_player_param *param)
{
    assert(param->cb);
    assert(param->cb->open_player);

    struct video_rotater *rotater = NULL;
    struct video_player *player;

    if (param->rotater_param) {
        rotater = video_rotater_open(param->rotater_param);
        if (!rotater)
            return NULL;
    }

    player = param->cb->open_player(param);
    if (!player) {
        if (rotater)
            video_rotater_close(rotater);

        return NULL;
    }

    player->rotater = rotater;
    player->param = *param;

    if (param->disp_cfg)
        video_player_set_display_config(player, param->disp_cfg);

    return player;
}

void video_player_close(struct video_player *player, int is_disable)
{
    assert(player->param.cb->close_player);

    if (player->rotater)
        video_rotater_close(player->rotater);

    player->param.cb->close_player(player, is_disable);
}

int video_player_set_media_frame(struct video_player *player, struct video_frame *frame)
{
    int ret;

    if (!player->param.cb->set_media_frame) {
        fprintf(stderr, "video_player:not support set media frame\n");
        return -1;
    }

    if (player->rotater) {
        struct video_frame *convert_frame = NULL;
        video_rotater_convert_video_frame(player->rotater, frame, &convert_frame);
        ret = player->param.cb->set_media_frame(player, convert_frame);
        video_frame_put(convert_frame);
        return ret;
    }

    return player->param.cb->set_media_frame(player, frame);
}

int video_player_set_display_config(struct video_player *player, struct video_display_config *disp_cfg)
{
    if (!player->param.cb->set_display_config) {
        fprintf(stderr,"video_player: not support set display mode\n");
        return -1;
    }

    return player->param.cb->set_display_config(player, disp_cfg);
}
int video_player_display(struct video_player *player)
{
    assert(player->param.cb->display);
    return player->param.cb->display(player);
}