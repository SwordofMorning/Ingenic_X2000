#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <libhardware2/fb.h>

#include <libmedia/play/fb_video_player.h>
#include <pthread.h>
#include <libmedia/media_alloter.h>

struct fb_video_player {
    struct video_player player;
    struct fb_video_player_param fb_param;

    struct video_display_config disp_cfg;
    int fb_fd;
    struct fb_device_info fb_info;

    int is_enabled;
    int cfg_is_update;
    struct lcdc_layer cfg;
    enum lcdc_layer_order layer_order;

    pthread_mutex_t lock;

    struct video_frame *new_frame;
    struct video_frame *old_frame;

    struct media_alloter *alloter;
};

static int open_fb(struct fb_video_player *player)
{
    struct fb_video_player_param *fb_param = &player->fb_param;
    int fb_fd = fb_open(fb_param->fb_device, &player->fb_info);
    if (fb_fd < 0) {
        fprintf(stderr, "failed to open fb: %s\n", fb_param->fb_device);
        return -1;
    }

    unsigned int fb_size = player->fb_info.frame_size * player->fb_info.frame_nums;
    void *fb_mem = player->fb_info.mapped_mem;
    unsigned long fb_mem_phys = player->fb_info.fix.smem_start;

    struct media_alloter *alloter = media_alloter_open(fb_mem, fb_mem_phys, fb_size);
    if (!alloter) {
        fprintf(stderr, "fb video player: failed to opne alloter\n");
        fb_close(fb_fd, &player->fb_info);
        return -1;
    }

    player->fb_fd = fb_fd;
    player->disp_cfg.mode = OUTPUT_ADAPT;
    player->disp_cfg.xres = player->fb_info.xres;
    player->disp_cfg.yres = player->fb_info.yres;
    player->disp_cfg.xpos = 0;
    player->disp_cfg.ypos = 0;

    player->new_frame = NULL;
    player->old_frame = NULL;
    player->cfg_is_update = 0;
    player->layer_order = fb_param->layer_order;

    player->alloter = alloter;

    player->player.width = player->fb_info.xres;
    player->player.height = player->fb_info.yres;

    return 0;
}

static struct video_player *fb_video_player_open(struct video_player_param *param)
{
    struct fb_video_player *player = malloc(sizeof(*player));
    assert(player);
    memset(player, 0, sizeof(*player));

    struct fb_video_player_param *fb_param = (void *)param;
    player->fb_param = *fb_param;
    if (!fb_param->fb_device || !access(fb_param->fb_device, F_OK)) {
        if (open_fb(player)) {
            free(player);
            return NULL;
        }
    }

    player->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    return &player->player;
}

int fb_video_player_set_display_config(struct video_player *player, struct video_display_config *disp_cfg)
{

    struct fb_video_player *fb_player = (void *)player;
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

    return 0;
}


#include <libutils2/boot_time.h>

static void fb_show_frame_rate(void)
{
    static double old = 0;
    static int count = 0;

    double t = boot_time_secs();
    if (old == 0)
        old = t;

    count++;

    if (t-old >= 1.0) {
        printf("refresh rate: %.2f\n", count/(t-old));
        old = t;
        count = 0;
    }
}

int fb_video_player_display(struct video_player *player)
{
    int ret;
    struct fb_video_player *fb_player = (void *)player;

    if (!fb_player->cfg_is_update)
        return 0;

    if (fb_player->is_enabled == 0) {
        ret = fb_enable(fb_player->fb_fd);
        if (ret < 0) {
            fprintf(stderr, "frame_player: failed to enable fb\n");
            return -1;
        }

        fb_player->is_enabled = 1;
    }

    ret = 0;

    struct video_frame *new_frame = NULL;

    if (fb_pan_display_enable_user_cfg(fb_player->fb_fd)) {
        fprintf(stderr, "frame_player: fb_pan_display_enable_user_cfg error\n");
        return -1;
    }

    pthread_mutex_lock(&fb_player->lock);

    if (fb_pan_display_set_user_cfg(fb_player->fb_fd, &fb_player->cfg)) {
        fprintf(stderr, "frame_player: fb_pan_display_set_user_cfg error\n");
        pthread_mutex_unlock(&fb_player->lock);
        return -1;
    }

    fb_player->cfg_is_update = 0;
    new_frame = fb_player->new_frame;
    fb_player->new_frame = NULL;

    pthread_mutex_unlock(&fb_player->lock);

    if (fb_pan_display(fb_player->fb_fd, &fb_player->fb_info, 0)) {
        fprintf(stderr, "frame_player: fb_pan_display error\n");
        return -1;
    }

    if (0)
        fb_show_frame_rate();

    pthread_mutex_lock(&fb_player->lock);

    video_frame_put(fb_player->old_frame);
    fb_player->old_frame = new_frame;

    pthread_mutex_unlock(&fb_player->lock);

    return ret;
}

static void fb_video_player_close(struct video_player *player, int is_disable)
{
    struct fb_video_player *fb_player = (void *)player;

    if (fb_player->is_enabled) {
        if (is_disable)
            fb_disable(fb_player->fb_fd);
    }

    if (fb_player->old_frame)
        video_frame_put(fb_player->old_frame);

    if (fb_player->new_frame)
        video_frame_put(fb_player->new_frame);

    fb_close(fb_player->fb_fd, &fb_player->fb_info);

    media_alloter_close(fb_player->alloter);

    free(player);
}

static int set_media_frame_phy(struct fb_video_player *fb_player, struct video_frame *frame)
{
    video_frame_get(frame);

    enum video_frame_format video_format = frame->format;

    int video_width = frame->width;
    int video_height = frame->height;

    int linesize = frame->linesize[0];
    int video_size = linesize * video_height;

    enum fb_fmt fb_fmt;
    switch (video_format) {
        case VIDEO_bgra:
            fb_fmt = fb_fmt_ARGB8888;
            break;
        case VIDEO_nv12:
            fb_fmt = fb_fmt_NV12;
            break;
        default:
            fprintf(stderr, "fb video player:not support this format %d\n", video_format);
            video_frame_put(frame);
            return -1;
    }

    if (video_size <= 0) {
        fprintf(stderr, "frame_player: unsupported video res: %d %d\n", video_width, video_height);
        video_frame_put(frame);
        return -1;
    }

    pthread_mutex_lock(&fb_player->lock);

    struct video_frame *disp_frame = NULL;
    struct video_display_data frame_data = {0};

    disp_frame = video_display_get_frame(frame, NULL, &fb_player->disp_cfg, &frame_data);

    struct lcdc_layer *cfg = &fb_player->cfg;
    memset(cfg, 0, sizeof(*cfg));

    cfg->xres = disp_frame->width;
    cfg->yres = disp_frame->height;
    cfg->fb_fmt = fb_fmt;
    cfg->xpos = frame_data.x_offset;
    cfg->ypos = frame_data.y_offset;

    cfg->layer_enable = 1;
    cfg->layer_order = fb_player->layer_order;

    cfg->alpha.enable = 0;
    cfg->alpha.value = 0xff;

    if (fb_fmt < fb_fmt_NV12) {
        cfg->rgb.mem = (void *)disp_frame->phys_data[0];
        cfg->rgb.stride = disp_frame->linesize[0];
    } else {
        cfg->y.mem = (void *)disp_frame->phys_data[0];
        cfg->y.stride = disp_frame->linesize[0];
        cfg->uv.mem = (void *)disp_frame->phys_data[1];
        cfg->uv.stride = disp_frame->linesize[1];
    }

    if (frame_data.scale_enable) {
        cfg->scaling.enable = 1;
        cfg->scaling.xres = frame_data.scale_width;
        cfg->scaling.yres = frame_data.scale_height;
    }

    video_frame_put(disp_frame);

    fb_player->cfg_is_update = 1;

    video_frame_put(fb_player->new_frame);

    fb_player->new_frame = frame;

    pthread_mutex_unlock(&fb_player->lock);


    return 0;
}

int set_media_frame_vir(struct fb_video_player *fb_player, struct video_frame *frame)
{
    int video_width = frame->width;
    int video_height = frame->height;

    enum video_frame_format format;

    switch (frame->format) {
    case VIDEO_bgra:
        format = VIDEO_bgra;
        break;
    case VIDEO_yuv420p:
    case VIDEO_yuva420p:
    case VIDEO_yuv422p:
    case VIDEO_yuv444p:
    case VIDEO_yuvj420p:
    case VIDEO_yuvj422p:
    case VIDEO_yuvj444p:
    case VIDEO_nv12:
        format = VIDEO_nv12;
        break;
    default:
        fprintf(stderr, "fb video player:not support this format %d\n", frame->format);
        return -1;
    }

    struct video_frame *display_frame = media_alloter_alloc_video_frame(fb_player->alloter, video_width,
                                                                        video_height, format, 0);

    if (!display_frame) {
        fprintf(stderr, "fb_video_player: failed to alloc frame by media alloter\n");
        video_frame_put(frame);
        return -1;
    }

    video_frame_copy(frame, display_frame);

    set_media_frame_phy(fb_player, display_frame);

    video_frame_put(display_frame);

    return 0;
}

static int fb_video_player_set_media_frame(struct video_player *player, struct video_frame *frame)
{
    struct fb_video_player *fb_player = (void *)player;

    if (!frame->is_phys)
        return set_media_frame_vir(fb_player, frame);

    return set_media_frame_phy(fb_player, frame);
}

struct video_player_cb fb_video_player_cb = {
    .open_player = fb_video_player_open,
    .close_player = fb_video_player_close,
    .set_display_config = fb_video_player_set_display_config,
    .set_media_frame = fb_video_player_set_media_frame,
    .display = fb_video_player_display,
};

void fb_video_player_init_param(struct fb_video_player_param *fb_param)
{
    struct video_player_param *param = &fb_param->param;

    param->cb = &fb_video_player_cb;
}

void fb_video_player_init_default_param(struct fb_video_player_param *param)
{
    memset(param, 0, sizeof(*param));
    param->fb_device = "/dev/fb0";
    fb_video_player_init_param(param);
}