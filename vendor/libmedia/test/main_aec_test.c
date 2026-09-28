// 多线程使用样例
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include <libmedia/filter/alsa_speex_aec_card.h>

static volatile int run = 1;

static void *capture_thread_func(void *data)
{
    struct aec_card *card = data;
    int ret;

    char *dst_file = "/tmp/aec.pcm";
    FILE *dst = fopen(dst_file, "w");
    if (!dst) {
        fprintf(stderr, "open file %s err\n", "/tmp/aec.pcm");
        run = 0;
        return NULL;
    }
    struct audio_frame *aec_frame;
    while (run) {
        ret = aec_card_read_frame(card, &aec_frame);
        if (!ret) {
            ret = fwrite(aec_frame->data[0], aec_frame->total_size, 1, dst);
            audio_frame_put(aec_frame);
        }
    }

    fclose(dst);
    return NULL;
}

static void *playback_thread_func(void *data)
{
    struct aec_card *card = data;
    int ret;

    char *src_path = "/usr/data/48000_rhythm.pcm";
    FILE *src = fopen(src_path, "r");
    if (!src) {
        fprintf(stderr, "open file %s err\n", src_path);
        run = 0;
        return NULL;
    }

    struct audio_frame *playback_frame = audio_frame_alloc_with_buffer(48000, 1024, 1, AUDIO_s16le);
    while (run) {
        ret = fread(playback_frame->data[0], playback_frame->total_size, 1, src);
        if (ret < 0 || feof(src)) {
            run = 0;
            break;
        }
        aec_card_display_frame(card, playback_frame);
    }
    audio_frame_put(playback_frame);

    fclose(src);
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    struct alsa_speex_aec_card_param param = {
        .alsa_capture_device = "hw:0,0",
        .alsa_playback_device = "hw:0,0",
        // .alsa_capture_ctl_device = "hw:0",
        // .alsa_playback_ctl_device = "hw:0",
        // .alsa_capture_ctl_name = "Mic Volume",
        // .alsa_playback_ctl_name = "Master Playback Volume",

        .alsa_params.channels = 1,
        .alsa_params.rate = 48000,
        .alsa_params.format = SND_PCM_FORMAT_S16_LE,

        // .speex_params.capture_align_ms = 100,
        .speex_params.filter_count = 5,
        // .speex_params.mic_volume_m = 26,
        // .speex_params.mic_volume_p = 24,
        // .speex_params.spk_volume = 24,
    };
    struct aec_card *card = aec_card_open(&param);
    assert(card);

    pthread_t capture_thread;
    pthread_t playback_thread;

    ret = pthread_create(&capture_thread, NULL, capture_thread_func, card);
    assert(!ret);

    ret = pthread_create(&playback_thread, NULL, playback_thread_func, card);
    assert(!ret);

    pthread_join(playback_thread, NULL);
    pthread_join(capture_thread, NULL);

    aec_card_close(card);

    return 0;
}

// // 单线程使用样例
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>
// #include <unistd.h>

// #include <libmedia/filter/alsa_speex_aec_card.h>

// int main(int argc, char *argv[])
// {
//     struct alsa_speex_aec_card_param param;
//     alsa_aec_card_init_default_param(&param, 1, 48000);
//     struct aec_card *card = aec_card_open(&param);
//     assert(card);

//     FILE *src = fopen("/usr/data/48000.pcm", "r");
//     FILE *dst = fopen("/tmp/aec.pcm", "w");

//     int run = 500;

//     struct audio_frame *src_frame = audio_frame_alloc_with_buffer(48000, 480, 1, AUDIO_s16le);
//     struct audio_frame *dst_frame;
//     while(run) {
//         int count = fread(src_frame->data[0], src_frame->total_size, 1, src);
//         if (!count) {
//             run = 0;
//             break;
//         }
//         aec_card_display_frame(card, src_frame);

//         run --;

//         while (aec_card_avail_capture(card) > 0) {
//             aec_card_read_frame(card, &dst_frame);
//             fwrite(dst_frame->data[0], dst_frame->total_size, 1, dst);
//             audio_frame_put(dst_frame);
//         }
//     }
//     audio_frame_put(src_frame);
//     while (aec_card_avail_capture(card) > 0) {
//         aec_card_read_frame(card, &dst_frame);
//         fwrite(dst_frame->data[0], dst_frame->total_size, 1, dst);
//         audio_frame_put(dst_frame);
//     }

//     aec_card_close(card);

//     fclose(src);

//     return 0;
// }