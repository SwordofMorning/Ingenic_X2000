#include <libmedia/utils/ffmpeg_utils.h>


int ffmpeg_fmt_to_audio_fmt(enum AVSampleFormat format)
{
    switch (format) {
        case AV_SAMPLE_FMT_S16:     return AUDIO_s16le;
        case AV_SAMPLE_FMT_S32:     return AUDIO_s32le;
        case AV_SAMPLE_FMT_FLT:     return AUDIO_flt;
        case AV_SAMPLE_FMT_FLTP:     return AUDIO_fltp;
        case AV_SAMPLE_FMT_S16P:     return AUDIO_s16lep;
        case AV_SAMPLE_FMT_S32P:     return AUDIO_s32lep;
        default:
            fprintf(stderr, "ffmpeg utils: not support this fmt: %d\n",format);
            return -1;
    }
}

int audio_fmt_to_ffmpeg_fmt(enum audio_frame_format format)
{
    switch (format) {
        case AUDIO_s16le:     return AV_SAMPLE_FMT_S16;
        case AUDIO_s32le:     return AV_SAMPLE_FMT_S32;
        case AUDIO_flt:     return AV_SAMPLE_FMT_FLT;
        case AUDIO_fltp:     return AV_SAMPLE_FMT_FLTP;
        case AUDIO_s16lep:     return AV_SAMPLE_FMT_S16P;
        case AUDIO_s32lep:     return AV_SAMPLE_FMT_S32P;
        default:
            fprintf(stderr, "ffmpeg utils: not support this fmt: %d\n",format);
            return -1;
    }
}

int ffmpeg_fmt_to_video_fmt(enum AVPixelFormat format)
{
    switch (format) {
        case AV_PIX_FMT_NV12: return VIDEO_nv12;
        case AV_PIX_FMT_YUV420P: return VIDEO_yuv420p;
        case AV_PIX_FMT_YUVA420P: return VIDEO_yuva420p;
        case AV_PIX_FMT_YUV422P: return VIDEO_yuv422p;
        case AV_PIX_FMT_YUV444P: return VIDEO_yuv444p;
        case AV_PIX_FMT_YUVJ420P: return VIDEO_yuvj420p;
        case AV_PIX_FMT_YUVJ422P: return VIDEO_yuvj422p;
        case AV_PIX_FMT_YUVJ444P: return VIDEO_yuvj444p;
        case AV_PIX_FMT_BGRA: return VIDEO_bgra;
        default:
            fprintf(stderr, "ffmpeg utils: not support this video format: %d\n", format);
            return -1;
    }
}

int video_fmt_to_ffmpeg_fmt(enum video_frame_format format)
{
    switch (format) {
        case VIDEO_nv12: return AV_PIX_FMT_NV12;
        case VIDEO_yuv420p: return AV_PIX_FMT_YUV420P;
        case VIDEO_yuva420p: return AV_PIX_FMT_YUVA420P;
        case VIDEO_yuv422p: return AV_PIX_FMT_YUV422P;
        case VIDEO_yuv444p: return AV_PIX_FMT_YUV444P;
        case VIDEO_yuvj420p: return AV_PIX_FMT_YUVJ420P;
        case VIDEO_yuvj422p: return AV_PIX_FMT_YUVJ422P;
        case VIDEO_yuvj444p: return AV_PIX_FMT_YUVJ444P;
        case VIDEO_bgra: return AV_PIX_FMT_BGRA;
        default:
            fprintf(stderr, "ffmpeg utils: not support this video format: %d\n", format);
            return -1;
    }
}

void media_pkt_to_ffmpeg_pkt(struct media_packet *pkt, AVPacket *avpkt)
{
    avpkt->data = pkt->data;
    avpkt->size = pkt->size;
    avpkt->duration = pkt->duration;
    avpkt->pos = pkt->pos;
    avpkt->pts = pkt->pts;
}

void ffmpeg_pkt_to_media_pkt(AVPacket *avpkt, struct media_packet *pkt)
{
    pkt->data = avpkt->data;
    pkt->size = avpkt->size;
    pkt->duration = avpkt->duration;
    pkt->pos = avpkt->pos;
    pkt->pts = avpkt->pts;
}
