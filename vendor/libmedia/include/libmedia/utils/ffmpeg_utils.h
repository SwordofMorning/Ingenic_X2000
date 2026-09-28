#ifndef __FFMPEG_UTILS_H__
#define __FFMPEG_UTILS_H__


#include <libmedia/video_frame.h>
#include <libmedia/audio_frame.h>
#include <libmedia/media_packet.h>
#include <libavformat/avformat.h>

int ffmpeg_fmt_to_audio_fmt(enum AVSampleFormat format);
int audio_fmt_to_ffmpeg_fmt(enum audio_frame_format format);
int ffmpeg_fmt_to_video_fmt(enum AVPixelFormat format);
int video_fmt_to_ffmpeg_fmt(enum video_frame_format format);
void media_pkt_to_ffmpeg_pkt(struct media_packet *pkt, AVPacket *avpkt);
void ffmpeg_pkt_to_media_pkt(AVPacket *avpkt, struct media_packet *pkt);

#endif