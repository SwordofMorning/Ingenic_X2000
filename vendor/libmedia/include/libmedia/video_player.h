#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_

#include <libmedia/video_rotater.h>
#include <libmedia/video_frame.h>
#include <libmedia/video_display_mode.h>

struct video_player;
struct video_player_param;

struct video_player_cb {
    struct video_player *(*open_player)(struct video_player_param *param);
    void (*close_player)(struct video_player *player, int is_disable);
    int (*set_media_frame)(struct video_player *player, struct video_frame *frame);
    int (*set_display_config)(struct video_player *player, struct video_display_config *disp_cfg);;
    int (*display)(struct video_player *player);
};

struct video_player_param {
    struct video_player_cb *cb;
    struct video_rotater_param *rotater_param;
    struct video_scaler_param *scaler_param;
    struct video_display_config *disp_cfg;
};

struct video_player {
    struct video_player_param param;
    struct video_rotater *rotater;
    int width;
    int height;
};


struct video_player *video_player_open(struct video_player_param *param);
void video_player_close(struct video_player *player, int is_disable);
int video_player_set_media_frame(struct video_player *player, struct video_frame *frame);
int video_player_set_display_config(struct video_player *player, struct video_display_config *disp_cfg);
int video_player_display(struct video_player *player);

#endif