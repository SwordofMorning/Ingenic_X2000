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

struct param {
    int value;
    int count;
    int press;
};


static char *command;

static uint64_t boot_time_usecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}


static int usage(int status)
{
    fprintf(stderr, "Usage: get help\n");
    fprintf(stderr, "\t%s\n", command);
    fprintf(stderr, "\t%s -h\n", command);
    fprintf(stderr, "\t%s --help\n", command);
    fprintf(stderr, "Usage: use command\n");
    fprintf(stderr, "\t%s [key_code]\n", command);
    fprintf(stderr, "\t%s [key_code] timeout=[time_value]\n", command);
    fprintf(stderr, "\tkey_code is code which you want to test\n");
    fprintf(stderr, "\ttime_value is time which you want to set up\n");
    fprintf(stderr, "\tif you don't set up time_value default 10 sec\n");
    fprintf(stderr, "Usage: for example\n");
    fprintf(stderr, "\t%s 102\n", command);
    fprintf(stderr, "\t%s 102 103 104\n", command);
    fprintf(stderr, "\t%s 102 timeout=10\n", command);
    exit(status);
}

int main(int argc, char **argv)
{
    int i, ret, timeout_value = 10;
    long fds;
    char *endptr = NULL;
    struct key_event event;
    command = argv[0];
    int key_count = 0;
    struct param param[argc];

    memset(param, 0, sizeof(struct param) * argc);

    if (argc == 1) {
        usage(0);
        return 0;
    }

    for (i = 1; i < argc ; i++) {
        if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "--h") == 0)) {
            usage(0);
            return 0;
        }

        if (!strncmp(argv[i], "timeout=", 8)) {
            timeout_value = (int)strtoul(argv[i] + 8, &endptr, 0);
            if (*endptr != '\0') {
                fprintf(stderr, "The time value is not a value number : %d%s\n", timeout_value, endptr);
                usage(-1);
                return -1;
            }
            continue;
        }

        param[key_count].value = (int)strtoul(argv[i], &endptr, 0);
        if (*endptr != '\0' || param[key_count].value < 0) {
            fprintf(stderr, "The parameter is not a value number : %s\n", argv[i]);
            usage(-1);
            return -1;
        }
        key_count++;
    }

    fds = keys_open();
    if (fds < 0) {
        fprintf(stderr, "keys open is failure\n");
        return -1;
    }

    fprintf(stderr, "the keys which you want to test :");
    for (i = 0; i < key_count; i++) {
        fprintf(stderr, " %d", param[i].value);
    }
    fprintf(stderr, "\n");

    int timeout_ms, count = 0;
    uint64_t current_time;
    uint64_t duration;
    uint64_t timeout_origin = timeout_value * 1000000;
    uint64_t timeout = timeout_origin;
    uint64_t origin_time = boot_time_usecs();

    while (key_count != count) {

        timeout_ms = (int)timeout/1000;
        ret = read_key_event(fds, &event, timeout_ms);
        if (ret < 0) {
            fprintf(stderr, "read key event is failure\n");
            return -1;
        }

        current_time = boot_time_usecs();

        duration = current_time - origin_time;
        if (duration >= timeout_origin) {
            fprintf(stderr, "has been timeout now\n");

            fprintf(stderr, "the keys are not pressed :");
            for (i = 0; i < key_count; i++) {
                if (param[i].count != 2)
                    fprintf(stderr, " %d", param[i].value);
            }
            fprintf(stderr, "\n");
            return -1;
        }

        timeout = timeout_origin - duration;

        for (i = 0; i < key_count; i++) {
            if (event.key_type == param[i].value)
                break;
        }

        if (i == key_count)
            continue;

        if (event.is_press != param[i].press) {
            param[i].press = event.is_press;
            if (param[i].count++ == 1) {
                fprintf(stderr, "code of %d has been press\n", param[i].value);
                count++;
            }
        }
    }

    keys_close(fds);

    return 0;
}