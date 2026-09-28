#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#include <libmedia/media_alloter.h>
#include <libmedia/media_packet.h>
#include <libmedia/video_frame.h>
#include <libmedia/audio_frame.h>
#include <libmedia/video_player.h>
#include <libmedia/audio_player.h>

#include <libmedia/audio_resampler.h>

#include <libmedia/media_muxing.h>
#include <libmedia/media_demuxing.h>
#include <libmedia/audio_decoder.h>
#include <libmedia/video_decoder.h>

struct media_player_param {
    struct media_demuxing_param *demuxing_param;
    struct video_decoder_param *video_decoder_param;
    struct video_player_param *video_player_param;
    struct audio_decoder_param *audio_decoder_param;
    struct audio_resampler_param *audio_resampler_param;
    struct audio_player_param *audio_player_param;
};

struct media_player *media_player_open(struct media_player_param *param);

void media_player_close(struct media_player *player);

int media_player_play_one_frame(struct media_player *player);

int media_player_is_end(struct media_player *player);

int64_t media_player_current_time(struct media_player *player);

int media_player_resume(struct media_player *player);

int media_player_pause(struct media_player *player);

int media_player_get_metadata(struct media_player *player, struct media_metadata *metadata);

int media_player_seek_forward(struct media_player *player, uint64_t abs_us);

int media_player_seek_backward(struct media_player *player, uint64_t abs_us);

#endif