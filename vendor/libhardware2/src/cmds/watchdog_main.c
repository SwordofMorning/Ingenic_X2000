#include <stdio.h>
#include <string.h>
#include <libhardware2/watchdog.h>
#include <stdlib.h>

static char *command;
static void usage(int status)
{
    printf("\n\nUsage1:%s start <ms>\n", command);
    printf("Example1:\n");
    printf("\t%s start 1000\n\n", command);
    printf("Usage2:%s stop\n\n", command);
    printf("Usage3:%s feed\n\n", command);
    printf("Usage4:%s reset\n\n", command);
    printf("Usage5:%s [-h/--help]\n", command);
    printf("Example5:\n");
    printf("\t%s --help\n", command);

    exit(status);
}

int main(int argc, char **argv)
{
    int ret;
    unsigned long ms;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if (strcmp(argv[1], "start") == 0) {
        if (argc != 3)
            usage(-1);

        ret = sscanf(argv[2], "%lu", &ms);
        if (ret != 1 || ms == 0)
            usage(-1);

        ret = watchdog_start(ms);

        return ret;
    }

    if (strcmp(argv[1], "stop") == 0) {
        if (argc != 2)
            usage(-1);

        ret = watchdog_stop();

        return ret;
    }

    if (strcmp(argv[1], "feed") == 0) {
        if (argc != 2)
            usage(-1);

        ret = watchdog_feed();

        return ret;
    }

    if (strcmp(argv[1], "reset") == 0) {
        if (argc != 2)
            usage(-1);

        ret = watchdog_reset();

        return ret;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        usage((argc != 2) ? -1: 0);

    usage(-1);
    return -1;
}