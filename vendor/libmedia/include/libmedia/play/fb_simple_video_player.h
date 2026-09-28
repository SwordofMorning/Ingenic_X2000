#ifndef __FB_SIMPLE_VIDEO_PLAYER__
#define __FB_SIMPLE_VIDEO_PLAYER__

#include <libmedia/video_player.h>
#include <libmedia/video_frame.h>

struct fb_simple_video_player_param {
    struct video_player_param param;
    const char *fb_device;
};


void fb_simple_video_player_init_param(struct fb_simple_video_player_param *fb_param);

void fb_simple_video_player_init_default_param(struct fb_simple_video_player_param *param);

#endif