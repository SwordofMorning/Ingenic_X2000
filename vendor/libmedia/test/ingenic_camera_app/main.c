#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include <libhardware2/keyboard.h>

#include <libmedia/utils/fifo_utils.h>
#include <libmedia/utils/thread_utils.h>
#include <libmedia/utils/pkt_parse.h>

#include <unistd.h>
#include <poll.h>

void register_linux_signal_hanler(const char *app_name);

static struct tm *get_datetime(void)
{
    time_t rawtime;

    time(&rawtime);

    return localtime(&rawtime);
}

static inline int get_last_pos(int cur_pos, int limit)
{
    int pos = cur_pos + 1;
    if (pos >= limit)
        pos = 0;

    return pos;
}

static inline int get_next_pos(int cur_pos, int limit)
{
    int pos = cur_pos - 1;
    if (pos < 0)
        pos = limit - 1;

    return pos;
}

struct dir_table {
    char name[64];
};

static int read_dir(DIR *dir, struct dir_table *dir_table, int count, int type)
{
    int file_cnt = 0;

    while (1) {
        struct dirent *tmp_node = readdir(dir);
        if (!tmp_node)
            break;

        int is_found = 0;

        if (file_cnt >= count) {
            fprintf(stderr, "WARNNING: file count is larger than %d, err: %s\n", count, strerror(errno));
            break;
        }

        if (type) {
            if (strstr(tmp_node->d_name, ".mp4"))
                is_found = 1;
        } else {
            if (strstr(tmp_node->d_name, ".jpg") || strstr(tmp_node->d_name, ".jpeg"))
                is_found = 1;
        }

        if (is_found) {
            sprintf(dir_table[file_cnt].name, "%s", tmp_node->d_name);
            file_cnt++;
        }
    }

    return file_cnt;
}

#include "./recorder.c"
#include "./photo.c"
#include "./pic_viewer.c"
#include "./vid_player.c"

int main(int argc, char *argv[])
{
    register_linux_signal_hanler("xxx");

    if (argv[1]) {
        if (!strcmp(argv[1], "record"))
            return main_app_record(argc, argv);

        if (!strcmp(argv[1], "photo"))
            return main_app_photo(argc, argv);

        if (!strcmp(argv[1], "pic_viewer"))
            return main_app_picture_viewer(argc, argv);

        if (!strcmp(argv[1], "play"))
            return main_app_video_play(argc, argv);
    }

    long key_handle = keys_open();
    if (key_handle == -1) {
        fprintf(stderr, "failed to open key device\n");
        return -1;
    }

    struct key_event event;
    char buf[1024];

    int is_end = 0;
    int is_quit = 0;

    while (1) {
        char *args[] = {
            argv[0], NULL, NULL, NULL, NULL, NULL, NULL
        };

        int is_menu = 0;

        printf("start\n");
        printf("KEY_MENU => menu\n");
        printf("    KEY_UP      => record\n");
        printf("    KEY_HOME    => photo\n");
        printf("    KEY_LEFT    => play\n");
        printf("    KEY_RIGHT   => pic_viewer\n");
        printf("    KEY_DOWN    => end\n");

        while (1) {
            int ret = read_key_event(key_handle, &event, 0);
            if (ret == 1) {
                if (event.key_type == KEY_MENU && event.is_press) {
                    args[1] = "menu";
                    is_menu = 1;
                    continue;
                }

                if (event.key_type == KEY_HOME && event.is_press && is_menu) {
                    args[1] = "photo";
                    args[2] = argv[2] ? argv[2] : "/tmp/";
                    break;
                }

                if (event.key_type == KEY_LEFT && event.is_press && is_menu) {
                    args[1] = "play";
                    args[2] = argv[2] ? argv[2] : "/tmp/";
                    break;
                }

                if (event.key_type == KEY_UP && event.is_press && is_menu) {
                    args[1] = "record";
                    args[2] = argv[2] ? argv[2] : "/tmp/";
                    for (int i = 3; i < argc; i++) {
                        args[i] = argv[i];
                    }

                    break;
                }

                if (event.key_type == KEY_RIGHT && event.is_press && is_menu) {
                    args[1] = "pic_viewer";
                    args[2] = argv[2] ? argv[2] : "/tmp/";
                    for (int i = 3; i < argc; i++) {
                        args[i] = argv[i];
                    }
                    break;
                }

                if (event.key_type == KEY_DOWN && event.is_press && is_menu) {
                    is_end = 1;
                    break;
                }
            }
        }

        if (is_end)
            break;

        pid_t pid;
        struct fifo *fifo = create_process_open_fifo(args, &pid);
        if (!fifo) {
            printf("create process open fifo err\n");
            goto close_keys;
        }

        while (1) {
            int ret = read_key_event(key_handle, &event, 0);
            if (ret == 1) {
                if (event.is_press) {
                    char pkt[32];
                    sprintf(pkt, "key_type=%d\n", event.key_type);
                    ret = fifo_write_pkt2(fifo, 100, pkt);
                    if (ret < 0)
                        break;
                }
            }

            ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
            if (ret < 0)
                break;

            pkt_parse_int(buf, "is_end", &is_end, 10);
            pkt_parse_int(buf, "is_quit", &is_quit, 10);

            if (is_quit || is_end) {
                is_quit = 0;
                is_end = 0;
                break;
            }
        }

        close_fifo_wait_process(fifo, pid);

        fifo_delete_pid(pid);
    }

close_keys:
    keys_close(key_handle);

    return 0;
}


