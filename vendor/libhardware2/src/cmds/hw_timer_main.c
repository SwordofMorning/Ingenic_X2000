#include <stdio.h>
#include <string.h>
#include <libhardware2/hw_timer.h>
#include <stdlib.h>

static char *command;
static void usage(int status)
{
    printf("\n\nUsage1:%s start <us>\n", command);
    printf("Example1:\n");
    printf("\t%s start 1000000\n\n", command);
    printf("Usage2:%s stop\n\n", command);
    printf("Usage3:%s wait\n\n", command);
    printf("Usage4:%s [-h/--help]\n", command);
    printf("Example4:\n");
    printf("\t%s --help\n", command);

    exit(status);
}

int main(int argc, char **argv)
{
    int ret;
    unsigned long us;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if (strcmp(argv[1], "start") == 0) {
        if (argc != 3)
            usage(-1);

        ret = sscanf(argv[2], "%lu", &us);
        if (ret != 1 || us == 0)
            usage(-1);

        int fd = hw_timer_open("watchdog1");

        ret = hw_timer_start(fd, us);

        hw_timer_close(fd);

        return ret;
    }

    if (strcmp(argv[1], "stop") == 0) {
        if (argc != 2)
            usage(-1);

        int fd = hw_timer_open("watchdog1");

        ret = hw_timer_stop(fd);

        hw_timer_close(fd);

        return ret;
    }

    if (strcmp(argv[1], "wait") == 0) {
        if (argc != 2)
            usage(-1);

        int fd = hw_timer_open("watchdog1");

        ret = hw_timer_wait(fd);
        if (!ret)
            fprintf(stderr, "wait for timer finished\n");

        hw_timer_close(fd);

        return ret;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        usage((argc != 2) ? -1: 0);

    usage(-1);
    return -1;
}