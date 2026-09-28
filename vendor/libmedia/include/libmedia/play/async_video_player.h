#ifndef _ASYNC_VIDEO_PLAYER_H_
#define _ASYNC_VIDEO_PLAYER_H_

#include <libmedia/video_player.h>

struct async_video_player_param {
    struct video_player_param param;
    struct video_player_param *device_param;
    const char *device_node;
};

void async_video_player_init_param(
    struct async_video_player_param *param,
    const char *device_node, struct video_player_param *device_param);

#endif /* _ASYNC_VIDEO_PLAYER_H_ */
