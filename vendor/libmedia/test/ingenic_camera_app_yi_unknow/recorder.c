#include "./../profiles/recorder_isp_alsa_ffmpeg_h264_profile.c"

int main_app_record(int argc, char *argv[])
{
    struct media_recorder_param *recorder_param;

    int ret = fifo_create_current_pid();
    if (ret < 0)
        return -1;

    struct fifo *fifo = fifo_open_current_pid(1, 0);
    if (!fifo)
        return -1;

    int width = 1920;
    int height = 1072;
    int sample_rate = 48000;
    int channles = 2;

    char *s = argv[2];
    if (!s) {
        //fprintf(stderr, "No such file or directory %s\n", s); //fix compile error, modify by yhy on 041524
        fprintf(stderr, "argv[2] is invalid, No such file or directory!\n");
        return -1;
    }

    if (argc >= 7) {
        if (argv[5])
            sample_rate = atoi(argv[5]);

        if (argv[6])
            channles = atoi(argv[6]);
    }

    if (argc >= 5) {
        if (argv[3])
            width = atoi(argv[3]);

        if (argv[4])
            height = atoi(argv[4]);
    }

    recorder_param = recorder_isp_alsa_ffmpeg_h264_create_param(width, height, sample_rate, channles);

    struct media_recorder *recorder = media_recorder_open(recorder_param);
    if (!recorder)
        goto free_param;

    printf("===============[video record]==================>\n");
    printf("    KEY_DOWN => quit\n");
    printf("    KEY_HOME => start/stop record\n");
    printf("=============================================>\n");

    char buf[2048] = {0};

    int start_record = 0;

    while (1) {
        ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
        if (ret < 0)
            break;

        if (buf[0] != '\0') {
            int cmd_quit = 0;
            int key_type = 0;

            pkt_parse_int(buf, "key_type", &key_type, 10);

            if (key_type == KEY_DOWN)
                cmd_quit = 1;

            if (key_type == KEY_HOME) {
                if (start_record) {
                    media_recorder_stop(recorder);
                } else {
                    char name[64];  //fix compile error, modify by yhy on 041524
                    struct tm *t = get_datetime();
                    sprintf(name, "%s/VID_%02d%02d%02d%02d%02d%02d.mp4",
                                                    s,
                                                    t->tm_year + 1900,
                                                    t->tm_mon + 1,
                                                    t->tm_mday,
                                                    t->tm_hour,
                                                    t->tm_min,
                                                    t->tm_sec);
                    ret = media_recorder_start(recorder, name);
                    if (ret < 0) {
                        fprintf(stderr, "failed to start recorder\n");
                        goto close_record;
                    }
                }
                start_record = !start_record;
            }

            if (cmd_quit)
                break;
        }

        if (start_record)
            ret = media_recorder_previewer_and_encode_one_frame(recorder);
        else
            ret = media_recorder_preview_one_frame(recorder);

        if (ret < 0)
            goto stop_record;
    }

stop_record:
    media_recorder_stop(recorder);
close_record:
    media_recorder_close(recorder);
free_param:
    recorder_isp_alsa_ffmpeg_h264_free_param(recorder_param);

    fifo_write_pkt2(fifo, 1000, "is_quit=1\n");

    printf("record end\n");

    return 0;
}
