#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <linux/input.h>
#include <dirent.h>
#include <libhardware2/keyboard.h>

static char *command;
static int usage(int status)
{
    printf("Usage1:%s\n", command);
    printf("Example1:\n");
    printf("\t%s\n\n", command);
    printf("Usage2:%s wait_key_press <key_code>\n", command);
    printf("Example2:\n");
    printf("\t%s wait_key_press 102\n\n", command);
    printf("Usage3:%s wait_key_release <key_code>\n", command);
    printf("Example3:\n");
    printf("\t%s wait_key_release 102\n\n", command);
    printf("Usage4:%s [-h/--help]\n", command);
    printf("Example4:\n");
    printf("\t%s --help\n", command);
    printf("Usage5:If you want to set the blocking time, add: <timeout=time(time > 0  unit: ms)> at the end of command\n");
    printf("Example5:\n");
    printf("\t%s timeout=3000\n\n", command);
    exit(status);
}

int main(int argc, char **argv)
{
    int ret, code;
    long fds;
    int timeout = -1;
    int timeout_index = 1;
    int wait_key_press = 0;
    int wait_key_release = 0;
    command = argv[0];
    struct key_event event;

    while (1) {
        if (argc == 1)
            break;

        if (argc == 2) {
            if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
                usage((argc != 2) ? -1: 0);
        }

        if (!strcmp(argv[1], "wait_key_press")) {
            wait_key_press = 1;
            if (argc < 3)
                usage(-1);

            ret = sscanf(argv[2], "%d", &code);
            if (ret != 1 || code < 0)
                usage(-1);

            if (argc == 3)
                break;

            timeout_index = 3;
        }

        if (!strcmp(argv[1], "wait_key_release")) {
            wait_key_release = 1;
            if (argc < 3)
                usage(-1);

            ret = sscanf(argv[2], "%d", &code);
            if (ret != 1 || code < 0)
                usage(-1);

            if (argc == 3)
                break;
            timeout_index = 3;
        }

        if (!strncmp(argv[timeout_index], "timeout=", 8)) {
            if (argc != timeout_index + 1)
                usage(-1);

            ret = sscanf(argv[timeout_index] + 8, "%d", &timeout);
            if (ret != 1 || timeout < 0)
                usage(-1);

            break;
        }

        usage(-1);
    }

    fds = keys_open();
    if (fds < 0)
        return -1;

    if (wait_key_press) {
        while ((ret = read_key_event(fds, &event, timeout)) > 0) {
            if (event.key_type == code && event.is_press) {
                printf("Key %d %s\n", event.key_type, "press");
                keys_close(fds);
                return 0;
            }
        }

        keys_close(fds);
        return -1;
    }

    if (wait_key_release) {
        while ((ret = read_key_event(fds, &event, timeout)) > 0) {
            if (event.key_type == code && !event.is_press) {
                printf("Key %d %s\n", event.key_type, "release");
                keys_close(fds);

                return 0;
            }
        }

        keys_close(fds);
        return -1;
    }

    while ((ret = read_key_event(fds, &event, timeout)) > 0)
            printf("Key %d %s\n", event.key_type, event.is_press ? "press" : "release");

    keys_close(fds);
    return -1;
}
