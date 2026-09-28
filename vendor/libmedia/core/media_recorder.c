#include <libmedia/media_recorder.h>
#include <unistd.h>

struct media_recorder {
    struct media_muxing *muxing;
    struct media_muxing_param muxing_param;
    struct media_previewer *previewer;

    struct audio_encoder_param *audio_encoder_param;
    struct video_encoder_param *video_encoder_param;
    struct audio_encoder *audio_encoder;
    struct video_encoder *video_encoder;
    int is_first;
};


static int encode_video(struct media_muxing *muxing, struct video_encoder *encoder, struct video_frame *frame)
{
    int ret;
    struct media_packet *pkt = NULL;

    ret = video_encoder_write_frame(encoder, frame);
    if (ret < 0)
        return -1;

    while(1) {
        ret = video_encoder_get_packet(encoder, &pkt);
        if (ret != 0)
            break;


        media_muxing_one_pkt(muxing, pkt);
        media_packet_put(pkt);
    }

    return ret;
}

static int encode_audio(struct media_muxing *muxing, struct audio_encoder *encoder, struct audio_frame *frame)
{
    int ret;
    struct media_packet *pkt = NULL;

    ret = audio_encoder_write_frame(encoder, frame);
    if (ret < 0)
        return -1;

    while(1) {
        ret = audio_encoder_get_packet(encoder, &pkt);
        if (ret != 0)
            break;

        media_muxing_one_pkt(muxing, pkt);
        media_packet_put(pkt);
    }

    if (ret == -MEDIA_EAGAIN)
        ret = 0;

    return ret;
}

static int open_encoder_muxing(struct media_recorder *recorder, const char *output_file)
{

    struct media_previewer *previewer = recorder->previewer;
    struct audio_encoder *audio_encoder = NULL;
    struct video_encoder *video_encoder = NULL;

    if (recorder->audio_encoder_param && previewer->audio) {
        audio_encoder = audio_encoder_open(recorder->audio_encoder_param);
        if(!audio_encoder) {
            fprintf(stderr, "media recorder: failed to open audio encoder\n");
            return -1;
        }
    }

    if (recorder->video_encoder_param && previewer->video) {
        video_encoder = video_encoder_open(recorder->video_encoder_param);
        if(!video_encoder) {
            fprintf(stderr, "media recorder: failed to open video encoder\n");
            goto video_err;
        }
    }

    if (video_encoder) {
        recorder->muxing_param.video_param.enable = 1;
        video_encoder_init_muxing_param(video_encoder, &recorder->muxing_param.video_param);
    }

    if (audio_encoder) {
        recorder->muxing_param.audio_param.enable = 1;
        audio_encoder_init_muxing_param(audio_encoder, &recorder->muxing_param.audio_param);
    }

    recorder->muxing_param.output_file = output_file;
    recorder->muxing = media_muxing_open(&recorder->muxing_param);
    if (!recorder->muxing) {
        fprintf(stderr, "media recorder: failed to open muxing\n");
        goto muxing_err;
    }

    recorder->audio_encoder = audio_encoder;
    recorder->video_encoder = video_encoder;
    recorder->is_first = 1;

    return 0;

muxing_err:
    if(video_encoder)
        video_encoder_close(video_encoder);

video_err:
    if(audio_encoder)
        audio_encoder_close(audio_encoder);

    return -1;
}

static void close_encoder_muxing(struct media_recorder *recorder)
{
    if (recorder->audio_encoder) {
        audio_encoder_close(recorder->audio_encoder);
        recorder->audio_encoder = NULL;
    }

    if (recorder->video_encoder) {
        video_encoder_close(recorder->video_encoder);
        recorder->video_encoder = NULL;
    }

    if (recorder->muxing) {
        media_muxing_close(recorder->muxing);
        recorder->muxing = NULL;
    }
}

struct media_recorder *media_recorder_open(struct media_recorder_param *param)
{
    struct media_previewer *previewer = NULL;

    if (!param->previewer_param) {
        fprintf(stderr, "media recorder: previewer param not set\n");
        return NULL;
    }

    if (!param->muxing_param) {
        fprintf(stderr, "media recorder: muxing param not set\n");
        return NULL;
    }

    previewer = media_previewer_open(param->previewer_param);
    if (!previewer) {
        fprintf(stderr, "media recorder: failed to open previewer\n");
        return NULL;
    }

    struct media_recorder *recorder = malloc(sizeof(*recorder));
    assert(recorder);

    recorder->audio_encoder = NULL;
    recorder->video_encoder = NULL;
    recorder->audio_encoder_param = param->audio_encoder_param;
    recorder->video_encoder_param = param->video_encoder_param;
    recorder->previewer = previewer;

    recorder->muxing_param = *param->muxing_param;
    recorder->muxing = NULL;

    return recorder;
}

int media_recorder_preview_one_frame(struct media_recorder *recorder)
{
    int ret = 1;
    struct media_previewer *previewer = recorder->previewer;

    struct video_frame *video_frame = NULL;
    struct audio_frame *audio_frame = NULL;

    if (previewer->video) {
        ret = video_reader_read_frame(previewer->video, &video_frame);
        if (ret < 0)
            return ret;
        if (ret == 0) {
            if (previewer->player)
                media_previewer_display_video_frame(previewer, video_frame, 0);

            video_frame_put(video_frame);
        }
    }

    if (previewer->audio) {
        ret = audio_reader_read_frame(previewer->audio, &audio_frame);
        if (ret < 0)
            return ret;

        audio_frame_put(audio_frame);
    }

    if (!previewer->audio && ret == 1)
        usleep(10*1000);

    return 0;
}

int media_recorder_previewer_and_encode_one_frame(struct media_recorder *recorder)
{
    int ret = 1;
    struct media_previewer *previewer = recorder->previewer;

    struct video_frame *video_frame = NULL;
    struct audio_frame *audio_frame = NULL;

    if (!recorder->muxing)
        return -1;

    while (previewer->audio && recorder->audio_encoder) {
        if (!recorder->is_first && audio_reader_avail_audio(previewer->audio) < 1024)
            break;

        ret = audio_reader_read_frame(previewer->audio, &audio_frame);
        if (ret < 0)
            return ret;

        ret = encode_audio(recorder->muxing, recorder->audio_encoder, audio_frame);
        audio_frame_put(audio_frame);

        if (ret < 0)
            return -1;

        recorder->is_first = 0;
    }

    if (previewer->video && recorder->video_encoder) {
        ret = video_reader_read_frame(previewer->video, &video_frame);
        if (ret < 0)
            return ret;
        if (ret == 0) {
            if (previewer->player)
                media_previewer_display_video_frame(previewer, video_frame, 0);

            ret = encode_video(recorder->muxing, recorder->video_encoder, video_frame);
            video_frame_put(video_frame);

            if (ret < 0)
                return -1;
        }
    }

    if (!recorder->audio_encoder && ret == 1)
        usleep(10*1000);

    return 0;
}

void media_recorder_close(struct media_recorder *recorder)
{
    close_encoder_muxing(recorder);
    media_previewer_close(recorder->previewer);
    free(recorder);
}

int media_recorder_start(struct media_recorder *recorder, const char *output_file)
{
    if (recorder->muxing) {
        fprintf(stderr, "media recorder: recorder already started\n");
        return 0;
    }

    return open_encoder_muxing(recorder, output_file);
}

void media_recorder_stop(struct media_recorder *recorder)
{
    close_encoder_muxing(recorder);
}



