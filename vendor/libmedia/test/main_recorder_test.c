#include <libmedia/media_recorder.h>
#include <libmedia/muxing/ffmpeg_muxing.h>

#include <linux/input.h>
#include <libhardware2/keyboard.h>

// #define VIC

#ifdef VIC
#include "../profiles/recorder_vic_alsa_ffmpeg_h264_profile.c"
#else
#include "../profiles/recorder_isp_alsa_ffmpeg_h264_profile.c"
#endif

int width;
int height;

int main(int argc, char *argv[])
{
    if(argc != 4)
        return -1;

    width = atoi(argv[1]);
    height = atoi(argv[2]);

    struct media_recorder_param *recorder_param;

#ifdef VIC
    /*vic recorder 录制的大小取决于摄像头输出的大小*/
    width = 1920;
    height = 1080;
    recorder_param = recorder_vic_alsa_ffmpeg_h264_create_param(width, height, 48000, 2);
#else
    recorder_param = recorder_isp_alsa_ffmpeg_h264_create_param(width, height, 48000, 2);
#endif

    struct media_recorder *recorder = media_recorder_open(recorder_param);
    if (!recorder)
        goto free_param;

    int ret;

    long key_handle = keys_open();
    if (key_handle == -1) {
        fprintf(stderr, "failed to open key device\n");
        ret = -1;
        goto close_recorder;
    }

    struct key_event event;

    while(1) {
        while (1) {
            ret = read_key_event(key_handle, &event, 0);
            if (ret == 1) {
                if (event.key_type == KEY_HOME && event.is_press)
                    break;
                if (event.key_type == KEY_UP && event.is_press)
                    goto out;
            }
            media_recorder_preview_one_frame(recorder);
        }

        ret = media_recorder_start(recorder, argv[3]);
        if (ret < 0) {
            fprintf(stderr, "failed to start recorder\n");
            continue;
        }

        while (1) {
            ret = read_key_event(key_handle, &event, 0);
            if (ret == 1) {
                if (event.key_type == KEY_HOME && event.is_press)
                    break;
            }

            ret = media_recorder_previewer_and_encode_one_frame(recorder);
            if (ret < 0)
                goto out;
        }

        media_recorder_stop(recorder);
    }

out:
    keys_close(key_handle);
close_recorder:
    media_recorder_close(recorder);
free_param:
#ifdef VIC
    recorder_vic_alsa_ffmpeg_h264_free_param(recorder_param);
#else
    recorder_isp_alsa_ffmpeg_h264_free_param(recorder_param);
#endif

    return 0;
}