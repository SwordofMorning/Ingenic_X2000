#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <libmedia/media_player.h>
#include <libutils2/message_queue.h>
#include <libutils2/boot_time.h>

#define PKT_NUM 4

struct pkt_msg {
    struct message msg;
    struct media_packet *pkt;
};

enum {
    AUDIO,
    VIDEO,
};

enum {
    PLAYER_START,
    PLAYER_STOP,
    PLAYER_PAUSE,
    PLAYER_RESUME,
    PLAYER_SEEK,
};

enum {
    SEEK_CLEAR,
    SEEK_FORWARD,
    SEEK_BACKWARD,
};

struct media_player {
    struct message_queue *usable_msgs;
    struct message_queue *resume_msgs;
    struct message_queue *seek_msgs;

    struct pkt_msg pkts[PKT_NUM];
    struct pkt_msg resume_pkt;
    struct pkt_msg seek_pkt;

    struct media_demuxing *demuxing;

    int64_t frame_pts_us;
    int64_t packet_pts_us;
    pthread_mutex_t lock;
    volatile int is_quit;
    volatile int status;
    volatile int is_end;
    volatile int is_enable_video;
    volatile int is_enable_audio;
    volatile int is_seeking;
    volatile int is_recalc_start_time;
    volatile uint64_t seek_abs;
    volatile uint8_t seek_dir;

    struct {
        struct audio_decoder *decoder;
        struct audio_resampler *resampler;
        struct audio_player *player;
        pthread_t play_thread;
        struct message_queue *msgs;
        volatile int is_empty;
    } audio;

    struct {
        struct video_decoder *decoder;
        struct video_player *player;
        pthread_t decode_thread;
        pthread_t play_thread;
        sem_t decode_done;
        sem_t display_done;
        struct message_queue *msgs;
    } video;
};

static int thread_set_name(const char *name)
{
    return prctl(PR_SET_NAME, name);
}

static void *audio_play_thread_func(void *data)
{
    struct media_player *player = data;

    thread_set_name("audio play");

    while (1) {
        int ret;

        struct pkt_msg *msg = (void *)message_queue_receive(player->audio.msgs);
        struct media_packet *pkt = msg->pkt;

        player->audio.is_empty = 1;

        if (msg->msg.type == PLAYER_STOP)
            break;

        ret = audio_decoder_send_pkt(player->audio.decoder, pkt);
        if (ret < 0) {
            fprintf(stderr, "media_player: decoder send pkt err, ret = %d\n", ret);
            goto send_to_usable_msg;
        }

        while (ret >= 0) {
            struct audio_frame *frame = NULL;
            struct audio_frame *read_frame = NULL;

            ret = audio_decoder_get_frame(player->audio.decoder, &read_frame);
            if (ret == -MEDIA_EAGAIN || ret == -MEDIA_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "media_player: decoder get frame err, ret = %d\n", ret);
                break;
            }

            ret = audio_resampler_convert(player->audio.resampler, read_frame, &frame);
            audio_frame_put(read_frame);
            if (ret == -MEDIA_EAGAIN || ret == -MEDIA_EOF)
                break;

            audio_player_display_audio(player->audio.player, frame);
            audio_frame_put(frame);
        }

send_to_usable_msg:
        message_queue_send(player->usable_msgs, &msg->msg);
        if (message_queue_is_empty(player->audio.msgs))
            player->audio.is_empty = 1;

        media_packet_put(pkt);
    }

    while (1) {
        struct pkt_msg *msg = (void *)message_queue_receive_timeout(player->audio.msgs, 0);
        if (!msg)
            break;
        message_queue_send(player->usable_msgs, &msg->msg);
    }

    return NULL;
}

static void *video_decode_thread_func(void *data)
{
    struct media_player *player = data;
    int is_first_frame = 1;
    int64_t start;

    thread_set_name("video decode");

    while (1) {
        int ret;
        struct video_frame *frame = NULL;

        struct pkt_msg *msg = (void *)message_queue_receive(player->video.msgs);
        struct media_packet *pkt = msg->pkt;

        if (msg->msg.type == PLAYER_STOP)
            break;

        if (msg->msg.type == PLAYER_RESUME) {
            message_queue_send(player->resume_msgs, &msg->msg);
            player->is_recalc_start_time = 1;
            goto put_media_packet;
        }

        ret = video_decoder_send_pkt(player->video.decoder, pkt);
        if (ret < 0) {
            fprintf(stderr, "media_player: decoder send pkt err, ret = %d.\n", ret);
            goto send_msg;
        }

        while (ret >= 0) {
            if (frame)
                video_frame_put(frame);

            ret = video_decoder_get_frame(player->video.decoder, &frame);
            if (ret == -MEDIA_EAGAIN || ret == -MEDIA_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "media_player: decoder get frame err, ret = %d.\n", ret);
                break;
            }
            player->frame_pts_us = frame->timestamp_us;

            int time_out = 30*1000*1000;

            struct timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);
            tp.tv_nsec += time_out;
            if (tp.tv_nsec >= (1000*1000*1000)) {
                tp.tv_sec += tp.tv_nsec / (1000*1000*1000);
                tp.tv_nsec = tp.tv_nsec % (1000*1000*1000);
            }

            ret = sem_timedwait(&player->video.display_done, &tp);
            if (ret) {
                // printf("drop: %f\n", (double)demuxing_get_pts(player->demuxing, pkt) / 1000000);
                continue;
            }

            // 由于 B 帧的存在,有audio的时候,也不能用audio来触发video的显示了,
            // video 的显示还是得跟着时间戳来,否则会一会卡(遇到B帧,等P帧)一会快的(遇到P帧之后立马解码多帧),
            int64_t time_us = player->frame_pts_us;
            if (time_us < 0)
                time_us = 0;

            if (is_first_frame) {
                start = boot_time_usecs();
                is_first_frame = 0;
            }

            if (player->is_seeking && player->packet_pts_us != time_us) {
                sem_post(&player->video.display_done);
                continue;
            }

            if (player->is_recalc_start_time) {
                start = boot_time_usecs() - time_us;
                player->is_recalc_start_time = 0;
                player->is_seeking = 0;
            }

            video_player_set_media_frame(player->video.player, frame);

            int64_t now = boot_time_usecs();
 
            if (now - start > time_us) {
                int64_t diff = now - start - time_us;
                if (diff > 30 * 1000) {
                    // printf("drop: %.3f %.3f\n", diff/1000.0, time_us/1000.0);
                    sem_post(&player->video.display_done);
                    continue;
                }
            }

            if (time_us >= now - start) {
                time_us = time_us - (now - start);
                if (time_us > 5*1000)
                    usleep(time_us-5*1000);
            }

            sem_post(&player->video.decode_done);
        }

        if (frame)
            video_frame_put(frame);

send_msg:
        message_queue_send(player->usable_msgs, &msg->msg);

put_media_packet:
        media_packet_put(pkt);

    }

    sem_post(&player->video.decode_done);

    while (1) {
        struct pkt_msg *msg = (void *)message_queue_receive_timeout(player->video.msgs, 0);
        if (!msg)
            break;
        message_queue_send(player->usable_msgs, &msg->msg);
    }

    return NULL;
}

static void *video_play_thread_func(void *data)
{
    struct media_player *player = data;

    thread_set_name("video display");

    while (1) {
        sem_wait(&player->video.decode_done);

        if (player->is_quit)
            break;

        video_player_display(player->video.player);

        sem_post(&player->video.display_done);
    }

    return NULL;
}

struct media_player *media_player_open(struct media_player_param *param)
{
    int ret;
    struct media_player *player = malloc(sizeof(*player));
    assert(player);

    memset(player, 0, sizeof(*player));

    struct media_demuxing *demux = demuxing_open(param->demuxing_param);
    if (!demux) {
        fprintf(stderr, "media_player: demuxing open err.\n");
        free(player);
        return NULL;
    }

    player->demuxing = demux;
    player->is_seeking = 0;
    player->is_recalc_start_time = 0;

    player->usable_msgs = message_queue_create(0, 0);
    assert(player->usable_msgs);

    player->resume_msgs = message_queue_create(0, 0);
    assert(player->resume_msgs);

    player->seek_msgs = message_queue_create(0, 0);
    assert(player->seek_msgs);

    message_queue_send(player->resume_msgs, &player->resume_pkt.msg);
    message_queue_send(player->seek_msgs, &player->seek_pkt.msg);

    int i;
    for (i = 0; i < PKT_NUM; i++) {
        memset(&player->pkts[i], 0, sizeof(player->pkts[i]));
        message_queue_send(player->usable_msgs, &player->pkts[i].msg);
    }

    if (!demuxing_get_video_param(demux, param->video_decoder_param)) {
        struct video_decoder *decoder = video_decoder_open(param->video_decoder_param);
        assert(decoder);

        struct video_player *v_player = video_player_open(param->video_player_param);
        assert(v_player);

        player->video.decoder = decoder;
        player->video.player = v_player;

        player->video.msgs = message_queue_create(0, 0);
        assert(player->video.msgs);

        ret = sem_init(&player->video.decode_done, 0, 0);
        assert(!ret);
        ret = sem_init(&player->video.display_done, 0, 1);
        assert(!ret);

        ret = pthread_create(&player->video.decode_thread, NULL, video_decode_thread_func, player);
        assert(!ret);
        ret = pthread_create(&player->video.play_thread, NULL, video_play_thread_func, player);
        assert(!ret);

        player->is_enable_video = 1;
    }

    if (!demuxing_get_audio_param(demux, param->audio_decoder_param)) {
        struct audio_decoder *decoder = audio_decoder_open(param->audio_decoder_param);
        assert(decoder);

        param->audio_resampler_param->src_format = param->audio_decoder_param->fmt;
        param->audio_resampler_param->src_channels = param->audio_decoder_param->channels;
        param->audio_resampler_param->src_rate = param->audio_decoder_param->rate;

        struct audio_resampler *resampler = audio_resampler_create(param->audio_resampler_param);
        assert(resampler);

        struct audio_player *a_player = audio_player_open(param->audio_player_param);
        assert(a_player);

        player->audio.decoder = decoder;
        player->audio.resampler = resampler;
        player->audio.player = a_player;

        player->audio.msgs = message_queue_create(0, 0);
        assert(player->audio.msgs);

        ret = pthread_create(&player->audio.play_thread, NULL, audio_play_thread_func, player);
        assert(!ret);

        player->is_enable_audio = 1;
    }

    return player;
}

int media_player_do_play_one_frame(struct media_player *player, struct pkt_msg *msg)
{
    struct media_packet *pkt = NULL;

    if(player->seek_dir != SEEK_CLEAR) {
        player->is_recalc_start_time = 1;
        player->is_seeking = 1;
    }
    if (player->seek_dir == SEEK_FORWARD)
        demuxing_seek_forward(player->demuxing, player->seek_abs);
    else if (player->seek_dir == SEEK_BACKWARD)
        demuxing_seek_backward(player->demuxing, player->seek_abs);
    player->seek_dir = SEEK_CLEAR;

    int ret = demuxing_one_pkt(player->demuxing, &pkt);
    if (ret < 0) {
        if (ret == -MEDIA_EOF) {
            printf("is eof\n");
            player->is_end = 1;
        }
        message_queue_send(player->usable_msgs, &msg->msg);
        return ret;
    }

    msg->pkt = pkt;

    if (!player->is_enable_video)
        player->packet_pts_us = demuxing_get_pts(player->demuxing, pkt);

    if (pkt->type >= VIDEO_pkt_h264 && player->is_enable_video) {
        message_queue_send(player->video.msgs, &msg->msg);
        player->packet_pts_us = demuxing_get_pts(player->demuxing, pkt);
    } else if (pkt->type == AUDIO_pkt_aac && player->is_enable_audio)
        message_queue_send(player->audio.msgs, &msg->msg);
    else {
        media_packet_put(pkt);
        message_queue_send(player->usable_msgs, &msg->msg);
    }

    return 0;
}

int media_player_play_one_frame(struct media_player *player)
{
    int ret = 0;

    pthread_mutex_lock(&player->lock);

    struct pkt_msg *msg = (void *)message_queue_receive(player->usable_msgs);

    if (msg->msg.type == PLAYER_PAUSE) {
        player->status = PLAYER_PAUSE;
        message_queue_send(player->resume_msgs, &msg->msg);
        usleep(20 * 1000);
        goto unlock;
    }

    if (player->status == PLAYER_PAUSE) {
        if (msg->msg.type == PLAYER_RESUME) {
            player->status = PLAYER_START;
            if (player->video.decoder)
                message_queue_send(player->video.msgs, &msg->msg);
            else
                message_queue_send(player->resume_msgs, &msg->msg);
        } else {
            message_queue_send(player->usable_msgs, &msg->msg);
            usleep(20 * 1000);
        }
        goto unlock;
    }

    ret = media_player_do_play_one_frame(player, msg);

unlock:
    pthread_mutex_unlock(&player->lock);

    return ret;
}

int media_player_is_end(struct media_player *player)
{
    return player->is_end;
}

void media_player_close(struct media_player *player)
{
    struct pkt_msg msg[2];
    msg[0].msg.type = PLAYER_STOP;
    msg[1].msg.type = PLAYER_STOP;

    player->is_quit = 1;

    pthread_mutex_destroy(&player->lock);

    if (player->is_enable_audio) {
        message_queue_send(player->audio.msgs, &msg[0].msg);
        pthread_join(player->audio.play_thread, NULL);
        message_queue_delete(player->audio.msgs);
        audio_player_close(player->audio.player);
        audio_resampler_delete(player->audio.resampler);
        audio_decoder_close(player->audio.decoder);
    }

    if (player->is_enable_video) {
        message_queue_send(player->video.msgs, &msg[1].msg);
        pthread_join(player->video.decode_thread, NULL);
        pthread_join(player->video.play_thread, NULL);
        sem_destroy(&player->video.decode_done);
        sem_destroy(&player->video.display_done);
        message_queue_delete(player->video.msgs);
        video_player_close(player->video.player, 0);
        video_decoder_close(player->video.decoder);
    }

    message_queue_delete(player->usable_msgs);

    message_queue_delete(player->resume_msgs);

    message_queue_delete(player->seek_msgs);

    demuxing_close(player->demuxing);

    free(player);
}

int64_t media_player_current_time(struct media_player *player)
{
    if (player->frame_pts_us > 0)
        return player->frame_pts_us;
    return player->packet_pts_us;
}

int media_player_pause(struct media_player *player)
{
    assert(player);

    struct pkt_msg *msg = (void *)message_queue_receive(player->resume_msgs);

    msg->msg.type = PLAYER_PAUSE;

    message_queue_send(player->usable_msgs, &msg->msg);

    return 0;
}

int media_player_resume(struct media_player *player)
{
    assert(player);

    struct pkt_msg *msg = (void *)message_queue_receive(player->resume_msgs);

    msg->msg.type = PLAYER_RESUME;

    message_queue_send(player->usable_msgs, &msg->msg);

    return 0;
}

int media_player_get_metadata(struct media_player *player, struct media_metadata *metadata)
{
    assert(player);

    return demuxing_get_metadata(player->demuxing, metadata);
}

int media_player_seek_forward(struct media_player *player, uint64_t abs_us)
{
    assert(player);

    pthread_mutex_lock(&player->lock);
    player->seek_dir = SEEK_FORWARD;
    player->seek_abs = abs_us;
    pthread_mutex_unlock(&player->lock);

    return 0;
}

int media_player_seek_backward(struct media_player *player, uint64_t abs_us)
{
    assert(player);

    pthread_mutex_lock(&player->lock);
    player->seek_dir = SEEK_BACKWARD;
    player->seek_abs = abs_us;
    pthread_mutex_unlock(&player->lock);

    return 0;
}

