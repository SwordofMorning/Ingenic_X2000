#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libhardware2/dtrng.h>


static char *command;
static void usage(int status)
{
    printf("Usage: \t%s <operation>\n", command);
    printf("Example1:\n");
    printf("\t%s get_random_number\n", command);
    printf("Usage2:%s [-h/--help]\n", command);
    printf("Example2:\n");
    printf("\t%s --help\n", command);

    exit(status);
}

int main(int argc, char **argv)
{
    int ret;
    unsigned int value;

    command = argv[0];

    if (argc != 2)
        usage(-1);

    if (strcmp(argv[1], "get_random_number") == 0) {

        ret = dtrng_get_random_number(&value);
        if (ret == -1) {
            fprintf(stderr, "DTRNG:dtrng read data failure!\n");
            return ret;
        }

        printf("%u\n", value);

        return 0;
    }

    usage(-1);

    return -1;
}
