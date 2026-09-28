#ifndef __FB_VIDEO_PLAYER__
#define __FB_VIDEO_PLAYER__

#include <libmedia/video_player.h>
#include <libmedia/video_frame.h>
#include <libhardware2/fb.h>

struct fb_video_player_param {
    struct video_player_param param;
    const char *fb_device;
    enum lcdc_layer_order layer_order;
};


void fb_video_player_init_param(struct fb_video_player_param *fb_param);

void fb_video_player_init_default_param(struct fb_video_player_param *param);

#endif