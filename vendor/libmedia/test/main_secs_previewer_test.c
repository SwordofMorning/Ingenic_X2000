#include <libmedia/read/isp_video_reader.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/rotate/hw_video_rotater.h>

#include <libmedia/media_previewer.h>
#include <pthread.h>
#include <unistd.h>

#include <libmedia/font/stb_font.h>
#include <libmedia/utils/file_utils.h>

#include <sys/time.h>
#include <libutils2/boot_time.h>

static char *font_name = "/YaHeiConsolas.ttf";

void *font_display_thread_func(void *data)
{
    struct video_display_config fb_display_config = {
        .xpos = 0,
        .ypos = 500,
        .xres = 600,
        .yres = 500,
    };

    struct fb_video_player_param fb_param = {
        .param.disp_cfg = &fb_display_config,
        .layer_order = lcdc_layer_1,
        .fb_device = "/dev/fb1",
    };

    fb_video_player_init_param(&fb_param);

    struct video_player *player = video_player_open(&fb_param.param);
    
    struct video_frame *frame = video_frame_alloc_init(600, 500, VIDEO_bgra, 0);

    struct stb_font_param param;
    param.param.path = font_name;

    stb_font_init_param(&param);

    struct font *font = font_open(&param.param);
    if (!font) {
        fprintf(stderr, "failed to open font\n");
        return NULL;
    }

    font_set_pixels_height(font, 240);

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        char buf[128];
        sprintf(buf, "%02d:%03d\n", (int)(tv.tv_sec%60), (int)(tv.tv_usec/1000));
        memset(frame->data[0], 0x00, frame->total_size);
        font_draw_line(font, buf, NULL, 0xffff0000, frame, 0, 0, NULL, NULL);
        video_player_set_media_frame(player, frame);
        video_player_display(player);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;

    if (argc >= 2)
        font_name = argv[1];

    struct video_rotater_param rotater_param = {
        .rotate_angle = rotate_0,
        .hflip = 1,
        .vflip = 0,
    };

    struct video_display_config fb_display_config = {
        .xpos = 0,
        .ypos = 0,
        .xres = 600,
        .yres = 500,
    };

    struct fb_video_player_param fb_param = {
        .param.disp_cfg = &fb_display_config,
        .layer_order = lcdc_layer_0,
        .fb_device = "/dev/fb0",
    };

    struct isp_video_reader_param isp_param = {
        .isp_device = "/dev/mscaler1-ch0",
        .isp_params = {
            .pixel_format       = CAMERA_PIX_FMT_NV12,
            .frame_nums         = 4,
            .width              = 640,
            .height             = 360,
            .scaler.enable      = 1,
            .scaler.width       = 640,
            .scaler.height      = 360,
            .crop.enable        = 0,
        },
    };

    fb_video_player_init_param(&fb_param);
    isp_video_reader_init_param(&isp_param);
    hw_video_rotater_init_param(&rotater_param);

    struct media_previewer_param previewer_param = {
        .audio_reader_param = NULL,
        .video_player_param = &fb_param.param,
        .player_rotater_param = NULL, // &rotater_param.param
        .video_reader_param = &isp_param.param,
    };

    pthread_t thread;
    pthread_create(&thread, NULL, font_display_thread_func, NULL);

    struct media_previewer *previewer = media_previewer_open(&previewer_param);
    if(!previewer)
        return -1;

    int cnt = 500;
    struct video_frame *frame = NULL;

    double old = boot_time_secs();
    while(1) {
        ret = video_reader_read_frame(previewer->video, &frame);
        if (ret < 0)
            break;
        if (ret == 1) {
            usleep(5*1000);
            continue;
        }

        double now = boot_time_secs();
        if (now-old >= 0.04)
            printf("---> %f\n", now-old);
        old = now;

        cnt--;
        media_previewer_display_video_frame(previewer, frame, 0);

        video_frame_put(frame);
    }

    media_previewer_close(previewer);

    return 0;
}