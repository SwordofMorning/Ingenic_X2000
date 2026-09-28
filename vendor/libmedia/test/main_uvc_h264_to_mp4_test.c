#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libmedia/video_frame.h>
#include <libmedia/uvc_demuxing.h>

#include <libmedia/audio_frame.h>
#include <libmedia/audio_encoder.h>
#include <libmedia/read/alsa_audio_reader.h>
#include <libmedia/encode/ffmpeg_audio_encoder.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>

#include <libmedia/media_packet.h>
#include <libmedia/media_muxing.h>
#include <libmedia/muxing/ffmpeg_muxing.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <signal.h>

static struct uvc_demuxing_param uvc_demuxing_param = {
    // .uvc_format.width = width,
    // .uvc_format.height = height,
    .uvc_format.format = V4L2_PIX_FMT_H264,
    // .uvc_device = uvc_device,
};

static struct media_muxing *video_muxing;
static unsigned int running;

static void *uvc_thread_func(void *data)
{
    int ret;
    struct media_demuxing *uvc_demuxing;
    struct media_packet *pkt;

    uvc_demuxing_init_param(&uvc_demuxing_param.param);
    uvc_demuxing = demuxing_open(&uvc_demuxing_param.param);
    if (!uvc_demuxing) {
        fprintf(stderr, "failed to open video uvc_demuxing\n");
        return NULL;
    }

    while (running) {
        ret = demuxing_one_pkt(uvc_demuxing, &pkt);
        if (ret) {
            fprintf(stderr, "failed to uvc_demuxing one pkt \n");
            break;
        }

        media_muxing_one_pkt(video_muxing, pkt);
        media_packet_put(pkt);
    }

    demuxing_close(uvc_demuxing);
    return NULL;
}

static void *audio_thread_func(void *data)
{
    int ret;
    struct audio_reader *audio_reader;
    static struct audio_encoder *audio_encoder;
    struct audio_frame *audio_frame;
    struct media_packet *pkt;

    struct audio_resampler_param resampler_param = {
        .src_channels = 1,
        .src_format = AUDIO_s16le,
        .src_rate = 16000,
        .dst_channels = 1,
        .dst_format = AUDIO_fltp,
        .dst_rate = 16000,
    };
    ffmpeg_audio_resampler_init_param(&resampler_param);

    struct alsa_audio_reader_param alsa_reader_param = {
        .alsa_capture_device = "plughw:0,0",
        .alsa_params.rate = 16000,
        .alsa_params.format = SND_PCM_FORMAT_S16_LE,
        .alsa_params.channels = 1,
        .param.resampler_param = &resampler_param, /*音频重采样*/
        .param.samples = 1024,  /*一帧包含的采样数，根据需求来，ffmpeg aac 编一帧 需要1024个采样*/
    };
    alsa_audio_reader_init_param(&alsa_reader_param);

    struct ffmpeg_audio_encoder_param audio_encoder_param = {
        .encoder_name = "aac",
        .bit_rate = 64000,
        .sample_rate = 16000,
        .channels = 1,
        .sample_fmt = AV_SAMPLE_FMT_FLTP, /*ffmpeg 提供的 aac 编码必须得是 浮点 planer(多通道的音频数据分开存储)格式的*/
    };
    ffmpeg_audio_encoder_init_param(&audio_encoder_param);

    audio_reader = audio_reader_open(&alsa_reader_param.param);
    if (!audio_reader) {
        fprintf(stderr, "failed to open audio_reader\n");
        return NULL;
    }

    audio_encoder = audio_encoder_open(&audio_encoder_param.param);
    if (!audio_encoder) {
        audio_reader_close(audio_reader);
        fprintf(stderr, "failed to open audio_encoder\n");
        return NULL;
    }

    while (running) {
        ret = audio_reader_read_frame(audio_reader, &audio_frame);
        if (ret) {
            fprintf(stderr, "failed to audio_reader read frame\n");
            break;
        }

        ret = audio_encoder_write_frame(audio_encoder, audio_frame);
        audio_frame_put(audio_frame);
        if (ret) {
            fprintf(stderr, "failed to audio_encoder write frame\n");
            break;
        }

        ret = audio_encoder_get_packet(audio_encoder, &pkt);
        if (ret == -MEDIA_EAGAIN) {
            continue;
        } else if (ret < 0) {
            fprintf(stderr, "failed to audio_encoder get packet\n");
            break;
        }

        media_muxing_one_pkt(video_muxing, pkt);
        media_packet_put(pkt);
    }

    audio_encoder_close(audio_encoder);
    audio_reader_close(audio_reader);

    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    pthread_t uvc_thread;
    pthread_t audio_thread;

    if (argc < 5) {
        printf("usage: %s width(uvc video) height(uvc video) device(uvc device) file timeout\n", argv[0]);
        return -1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    char *uvc_device = argv[3];
    char *file = argv[4];
    int timeout;
    if (argc > 5)
        timeout = atoi(argv[5]);
    else
        timeout = -1;

    uvc_demuxing_param.uvc_format.width = width;
    uvc_demuxing_param.uvc_format.height = height;
    uvc_demuxing_param.uvc_device = uvc_device;

    struct media_muxing_param muxing_param = {
        .output_file = file,

        .video_param.enable = 1,
        .video_param.bit_rate = 2000*1000,
        .video_param.fmt = VIDEO_nv12,
        .video_param.type = VIDEO_pkt_h264,
        .video_param.width = width,
        .video_param.height = height,
        .video_param.framerate = 15,
        .video_param.gop_size = 15,

        .audio_param.enable = 1,
        .audio_param.bit_rate = 64000,
        .audio_param.sample_rate = 16000,
        .audio_param.channels = 1,
        .audio_param.fmt = AUDIO_s16le,
        .audio_param.type = AUDIO_pkt_aac,
    };
    ffmpeg_muxing_init_param(&muxing_param);

    video_muxing = media_muxing_open(&muxing_param);
    if (!video_muxing) {
        fprintf(stderr, "failed to open video_muxing\n");
        return -1;
    }

    running = 1;

    ret = pthread_create(&uvc_thread, NULL, uvc_thread_func, NULL);
    assert(!ret);

    ret = pthread_create(&audio_thread, NULL, audio_thread_func, NULL);
    assert(!ret);

    if (timeout > 0) {
        sleep(timeout);
        running = 0;
    }

    pthread_join(uvc_thread, NULL);
    pthread_join(audio_thread, NULL);

    media_muxing_close(video_muxing);
    return 0;
}