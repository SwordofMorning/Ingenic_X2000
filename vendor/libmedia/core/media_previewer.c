#include <stdlib.h>
#include <string.h>
#include <libmedia/media_previewer.h>

static void *video_play_thread_func(void *data)
{
    struct media_previewer *previewer = data;
    struct video_player *player = previewer->player;

    while (1) {
        sem_wait(&previewer->video_frame_ok);
        if (previewer->is_stop)
            break;
        video_player_display(player);
        sem_post(&previewer->video_display_done);
    }

    return NULL;
}

struct media_previewer *media_previewer_open(struct media_previewer_param *param)
{
    struct media_previewer *previewer = malloc(sizeof(*previewer));
    assert(previewer);

    memset(previewer, 0, sizeof(*previewer));

    if (param->audio_reader_param) {
        previewer->audio = audio_reader_open(param->audio_reader_param);
        if (!previewer->audio) {
            fprintf(stderr, "media_previewer: failed to open audio\n");
            goto free_previewer;
        }
    }

    if (param->video_reader_param) {
        previewer->video = video_reader_open(param->video_reader_param);
        if (!previewer->video) {
            fprintf(stderr, "media_previewer: failed to open video\n");
            goto close_audio;
        }
    }

    if (param->video_player_param) {
        if (param->player_rotater_param)
            param->video_player_param->rotater_param = param->player_rotater_param;

        previewer->player = video_player_open(param->video_player_param);
        if (!previewer->player) {
            fprintf(stderr, "media_previewer: failed to open player\n");
            goto close_video;
        }

        int ret = sem_init(&previewer->video_frame_ok, 0, 0);
        assert(!ret);
        ret = sem_init(&previewer->video_display_done, 0, 1);
        assert(!ret);

        ret = pthread_create(&previewer->video_display_thread,
                NULL, video_play_thread_func, previewer);
        assert(!ret);
    }

    previewer->param = *param;
    if (param->player_rotater_param)
        previewer->player_rotater = previewer->player->rotater;

    return previewer;

close_video:
    if (previewer->video)
        video_reader_close(previewer->video);
close_audio:
    if (previewer->audio)
        audio_reader_close(previewer->audio);
free_previewer:
    free(previewer);
    return NULL;
}

void media_previewer_close(struct media_previewer *previewer)
{
    if (previewer->player) {
        previewer->is_stop = 1;
        sem_post(&previewer->video_frame_ok);
        pthread_join(previewer->video_display_thread, NULL);
        video_player_close(previewer->player, 0);
        sem_destroy(&previewer->video_display_done);
        sem_destroy(&previewer->video_frame_ok);
    }

    if (previewer->video)
        video_reader_close(previewer->video);

    if (previewer->audio)
        audio_reader_close(previewer->audio);

    free(previewer);
}

static int wait_display(struct media_previewer *previewer, int timeout_ms)
{
    if (!previewer->player)
        return -1;
    if (!previewer->video)
        return -1;

    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    tp.tv_nsec += timeout_ms*1000*1000;
    if (tp.tv_nsec >= (1000*1000*1000)) {
        tp.tv_sec += tp.tv_nsec / (1000*1000*1000);
        tp.tv_nsec = tp.tv_nsec % (1000*1000*1000);
    }

    int ret;
    if (timeout_ms == -1)
        ret = sem_wait(&previewer->video_display_done);
    else
        ret = sem_timedwait(&previewer->video_display_done, &tp);
    if (ret)
        return 1;

    return 0;
}

int media_previewer_display_video_frame(struct media_previewer *previewer, struct video_frame *frame, int timeout_ms)
{
    int ret = wait_display(previewer, timeout_ms);
    if (ret)
        return ret;

    video_player_set_media_frame(previewer->player, frame);

    sem_post(&previewer->video_frame_ok);

    return 0;
}