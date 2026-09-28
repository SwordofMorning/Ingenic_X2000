#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <libhardware2/fb.h>
#include <libmedia/play/fb_simple_video_player.h>
#include <pthread.h>
#include <libmedia/video_scaler.h>

struct fb_simple_video_player{
    struct video_player player;

    struct video_display_config disp_cfg;
    int fb_fd;
    struct fb_device_info fb_info;
    void *fb_mem;

    struct video_frame *convert_frame;
    int is_enabled;
    int display_cnt;
    int scale_is_enable;
};


struct video_player *fb_simple_video_player_open(struct video_player_param *param)
{
    struct fb_simple_video_player_param *fb_param = (void *)param;
    const char *fb_path = fb_param->fb_device;

    struct fb_simple_video_player *player = malloc(sizeof(*player));
    assert(player);

    memset(player, 0, sizeof(*player));

    int fb_fd = fb_open(fb_path, &player->fb_info);
    if (fb_fd < 0) {
        fprintf(stderr, "failed to open fb: %s\n", fb_path);
        free(player);
        return NULL;
    }

    player->fb_fd = fb_fd;
    player->fb_mem = player->fb_info.mapped_mem;

    memset(player->fb_mem, 0, player->fb_info.frame_nums * player->fb_info.frame_size);

    player->disp_cfg.mode = OUTPUT_ADAPT;
    player->disp_cfg.xres = player->fb_info.xres;
    player->disp_cfg.yres = player->fb_info.yres;
    player->disp_cfg.xpos = 0;
    player->disp_cfg.ypos = 0;

    if (param->scaler_param)
        player->scale_is_enable = 1;
    else
        player->disp_cfg.mode = -1;

    player->display_cnt = -1;

    return &player->player;
}

void fb_simple_video_player_close(struct video_player *player, int is_disable)
{
    struct fb_simple_video_player *fb_player = (void *)player;

    if (fb_player->is_enabled) {
        if (is_disable)
            fb_disable(fb_player->fb_fd);
    }

    if (fb_player->convert_frame)
        video_frame_put(fb_player->convert_frame);

    fb_close(fb_player->fb_fd, &fb_player->fb_info);

    free(fb_player);
}

int fb_simple_video_player_set_display_config(struct video_player *player, struct video_display_config *disp_cfg)
{
    struct fb_simple_video_player *fb_player = (void *)player;

    if (disp_cfg->xres <= 0 || disp_cfg->yres <= 0) {
        fprintf(stderr, "frame_player: display xres or yres must large than 0\n");
        return -1;
    }

    if (disp_cfg->xpos < 0 || disp_cfg->ypos < 0) {
        fprintf(stderr, "frame_player: display xres or yres can not less than 0\n");
        return -1;
    }

    if (disp_cfg->xres % 2 != 0 || disp_cfg->yres % 2 != 0) {
        fprintf(stderr, "frame_player: xres yres must align 2\n");
        return -1;
    }

    if ((disp_cfg->xres + disp_cfg->xpos > fb_player->fb_info.xres ||
        disp_cfg->yres + disp_cfg->ypos > fb_player->fb_info.yres) ) {
        fprintf(stderr, "frame_player: display area must less than fb area\n");
        return -1;
    }

    fb_player->disp_cfg = *disp_cfg;

    if (!fb_player->scale_is_enable)
        fb_player->disp_cfg.mode = -1;

    return 0;
}

static void *get_display_mem(struct fb_simple_video_player *player, int x_offset, int y_offset)
{
    int frame_nums = player->fb_info.frame_nums;
    int frame_size = player->fb_info.frame_size;
    int linesize = player->fb_info.line_length;
    int index = (player->display_cnt + 1) % frame_nums;

    return player->fb_mem + index * frame_size + x_offset * 4 + y_offset * linesize;
}

int fb_simple_video_player_set_media_frame(struct video_player *player, struct video_frame *frame)
{
    struct fb_simple_video_player *fb_player = (void *)player;
    struct video_display_config *disp_cfg = &fb_player->disp_cfg;

    if (frame->format != VIDEO_bgra && frame->format != VIDEO_nv12) {
        fprintf(stderr, "fb_simple_video_player: not support this format: %s\n", video_fmt_name(frame->format));
        return -1;
    }

    struct video_frame *disp_frame = NULL;
    struct video_display_data frame_data = {0};
    struct video_scaler_param *scale_param = fb_player->player.param.scaler_param;

    disp_frame = video_display_get_frame(frame, scale_param, disp_cfg, &frame_data);

    uint8_t *display_mem = get_display_mem(fb_player, frame_data.x_offset, frame_data.y_offset);

    video_frame_data_copy(disp_frame->format, disp_frame->data, disp_frame->linesize,
                          VIDEO_bgra, &display_mem, &fb_player->fb_info.line_length,
                          disp_frame->width, disp_frame->height);

    video_frame_put(disp_frame);

    return 0;
}

int fb_simple_video_player_display(struct video_player *player)
{
    struct fb_simple_video_player *fb_player = (void *)player;
    int ret;
    if (fb_player->is_enabled == 0) {
        ret = fb_enable(fb_player->fb_fd);
        if (ret < 0) {
            fprintf(stderr, "frame_player: failed to enable fb\n");
            return -1;
        }

        fb_player->is_enabled = 1;
    }

    fb_player->display_cnt++;
    int frame_nums = fb_player->fb_info.frame_nums;
    int display_index = fb_player->display_cnt % frame_nums;

    if (fb_pan_display(fb_player->fb_fd, &fb_player->fb_info, display_index)) {
        fprintf(stderr, "frame_player: fb_pan_display error\n");
        return -1;
    }

    return 0;
}

struct video_player_cb fb_simple_video_player_cb = {
    .open_player = fb_simple_video_player_open,
    .close_player = fb_simple_video_player_close,
    .set_display_config = fb_simple_video_player_set_display_config,
    .set_media_frame = fb_simple_video_player_set_media_frame,
    .display = fb_simple_video_player_display,
};

void fb_simple_video_player_init_param(struct fb_simple_video_player_param *fb_param)
{
    struct video_player_param *param = &fb_param->param;

    param->cb = &fb_simple_video_player_cb;
}

void fb_simple_video_player_init_default_param(struct fb_simple_video_player_param *param)
{
    memset(param, 0, sizeof(*param));
    param->fb_device = "/dev/fb0";
    fb_simple_video_player_init_param(param);
}