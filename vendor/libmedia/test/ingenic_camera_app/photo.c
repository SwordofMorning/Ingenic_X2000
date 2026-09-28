#include <libmedia/utils/ffmpeg_utils.h>
#include <libmedia/media_muxing.h>
#include <libmedia/encode/ffmpeg_video_encoder.h>
#include <libmedia/muxing/ffmpeg_muxing.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

static int ffmpeg_convert_frame(
    struct video_frame **dst, struct video_frame *src, enum video_frame_format fmt, struct SwsContext **sws_ctx)
{
    int ret;

    struct SwsContext *sws = sws_ctx ? *sws_ctx : NULL;

    AVFrame *av_frame = av_frame_alloc();
    av_frame->width = src->width;
    av_frame->height = src->height;
    av_frame->format = video_fmt_to_ffmpeg_fmt(fmt);

    if (!av_frame->data[0]) {
        ret = av_frame_get_buffer(av_frame, 0);
        if (ret < 0) {
            fprintf(stderr, "ffmpeg_utils: failed to allocate frame data: %s\n", av_err2str(ret));
            return -1;
        }
    }

    if (!sws) {
        sws = sws_getContext(src->width, src->height, video_fmt_to_ffmpeg_fmt(src->format),
            av_frame->width, av_frame->height, video_fmt_to_ffmpeg_fmt(fmt), SWS_POINT, NULL, NULL, NULL);
        if (!sws) {
            fprintf(stderr, "ffmpeg_utils: failed to allocate frame data: %s\n", av_err2str(ret));
            return -1;
        }
    }

    sws_scale(sws,
        (const uint8_t * const*)src->data, (const int *)src->linesize, 0, src->height,
        av_frame->data, av_frame->linesize);

    struct video_frame* frame = video_frame_alloc();
    frame->format = fmt;
    frame->width = src->width;
    frame->height = src->height;

    int i;

    for (i = 0; i < 3; i++) {
        if (av_frame->data[i]) {
            int size = av_frame->height * av_frame->linesize[i];
            if (i > 0)
                size /= 2;

            frame->data[i] = malloc(size);
            frame->size[i] = size;
            frame->linesize[i] = av_frame->linesize[i];
            void *src = av_frame->data[i];
            void *dst = frame->data[i];
            memcpy(dst, src, size);
        }
    }

    *dst = frame;

    av_frame_free(&av_frame);

    if (sws_ctx && !*sws_ctx)
        *sws_ctx = sws;
    if (!sws_ctx)
        sws_freeContext(sws);

    return 0;
}

static int frame_to_jpg(struct video_frame *src_frame, const char *output_file)
{
    assert(src_frame);

    struct video_frame *dst_frame = NULL;
    struct SwsContext *sws_ctx = NULL;

    struct ffmpeg_video_encoder_param ffmpeg_param = {0};

    if (src_frame->format == VIDEO_nv12) {
        ffmpeg_convert_frame(&dst_frame, src_frame, VIDEO_yuv420p, &sws_ctx);
        ffmpeg_param.pix_fmt = AV_PIX_FMT_YUVJ420P;
    } else {
        dst_frame = src_frame;
        ffmpeg_param.pix_fmt = video_fmt_to_ffmpeg_fmt(dst_frame->format);
    }

    ffmpeg_param.width = dst_frame->width;
    ffmpeg_param.height = dst_frame->height;
    ffmpeg_param.encoder_name = "mjpeg";
    ffmpeg_param.framerate = 30;
    ffmpeg_video_encoder_init_param(&ffmpeg_param);

    struct video_encoder *encoder = video_encoder_open(&ffmpeg_param.param);
    assert(encoder);

    struct media_muxing_param muxing_param = {0};
    muxing_param.video_param.enable = 1;
    muxing_param.output_file = output_file;

    video_encoder_init_muxing_param(encoder, &muxing_param.video_param);

    ffmpeg_muxing_init_param(&muxing_param);
    struct media_muxing *muxing = media_muxing_open(&muxing_param);
    assert(muxing);

    struct media_packet *pkt = NULL;

    int ret = video_encoder_write_frame(encoder, dst_frame);

    while (1) {
        ret = video_encoder_get_packet(encoder, &pkt);
        if (ret != 0)
            break;

        media_muxing_one_pkt(muxing, pkt);
        media_packet_put(pkt);
    }

    media_muxing_close(muxing);
    video_encoder_close(encoder);
    return 0;
}

static void save_picture(struct video_frame *frame, const char *dir)
{
    char name[32];
    struct tm *t = get_datetime();

    if (!dir) {
        fprintf(stderr, "No such file or directory %s\n", dir);
        return;
    }

    sprintf(name, "%s/IMG_%02d%02d%02d%02d%02d%02d.jpg",
                                    dir,
                                    t->tm_year + 1900,
                                    t->tm_mon + 1,
                                    t->tm_mday,
                                    t->tm_hour,
                                    t->tm_min,
                                    t->tm_sec);
    frame_to_jpg(frame, name);
}

int main_app_photo(int argc, char *argv[])
{
    int ret = fifo_create_current_pid();
    if (ret < 0)
        return -1;

    struct fifo *fifo = fifo_open_current_pid(1, 0);
    if (!fifo)
        goto end;

    struct fb_video_player_param fb_param;
    struct isp_video_reader_param isp_param;

    fb_video_player_init_default_param(&fb_param);
    isp_video_reader_init_default_param(&isp_param, 1920, 1080);

    struct media_previewer_param previewer_param = {
        .video_player_param = &fb_param.param,
        .player_rotater_param = NULL,
        .video_reader_param = &isp_param.param,
    };

    struct media_previewer *previewer = media_previewer_open(&previewer_param);
    if(!previewer)
        goto end;

    printf("===============[take a photo]==================>\n");
    printf("    KEY_DOWN => quit\n");
    printf("    KEY_LEFT => take a photo\n");
    printf("=============================================>\n");

    char buf[2048] = {0};

    while (1) {
        int cmd_quit = 0;
        int cmd_take_photo = 0;

        while (1) {
            ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
            if (ret < 0)
                break;

            int key_type = 0;

            if (buf[0] != '\0') {
                pkt_parse_int(buf, "key_type", &key_type, 10);

                if (key_type == KEY_DOWN)
                    cmd_quit = 1;

                if (key_type == KEY_LEFT)
                    cmd_take_photo = 1;
            }

            if (cmd_quit)
                break;

            struct video_frame *frame = NULL;

            ret = video_reader_read_frame(previewer->video, &frame);
            if (ret < 0)
                break;

            if (ret == 1)
                continue;

            media_previewer_display_video_frame(previewer, frame, 0);

            if (cmd_take_photo)
                save_picture(frame, argv[2]);

            video_frame_put(frame);
            cmd_take_photo = 0;
        }

        if (cmd_quit)
            break;
    }

    media_previewer_close(previewer);

    fifo_write_pkt2(fifo, 1000, "is_quit=1\n");

    printf("take photo end\n");

    return 0;

end:
    fifo_write_pkt2(fifo, 1000, "is_quit=1\n");
    return -1;
}