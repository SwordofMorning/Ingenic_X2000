#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>
#include <libhardware2/nemc.h>


enum {
    cmd_set_timing,
    cmd_get_timing,
    cmd_write,
    cmd_read,
};

static char *command;

static void usage(int status)
{
    printf("Usage1:%s set_timing <dev_path> tas=<> taw=<> tbp=<> tah<> strv=<>\n", command);
    printf("  Example:\n");
    printf("    %s set_timing /dev/nemc0/ tas=0x6 taw=0xe tbp=0x6 tah=0xe strv=0xf\n", command);
    printf("Usage2:%s get_timing <dev_path>\n", command);
    printf("  Example:\n");
    printf("    %s get_timing /dev/nemc0\n", command);
    printf("Usage3:%s write dev_path data... \n", command);
    printf("  Example:\n");
    printf("    %s write /dev/nemc0 0x01 0x02 0x03 0x04\n", command);
    printf("Usage4:%s read dev_path <bytes>\n", command);
    printf("  Example:\n");
    printf("    %s read /dev/nemc0 4\n", command);

    exit(status);
}

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static int data_process(char *data)
{
    int ret;
    int temp;

    ret = sscanf(data, "%x", &temp);
    if (ret != 1)
        return -1;


    return temp;
}


int main(int argc, char *argv[])
{
    int cmd = -1;

    command = argv[0];

    if (argc < 3)
        usage(-1);

    if (!strcmp(argv[1], "set_timing")) {
        if (argc != 8)
            usage(-1);

        cmd = cmd_set_timing;
    }

    if (!strcmp(argv[1], "get_timing")) {
        if(argc != 3)
            usage(-1);

        cmd = cmd_get_timing;
    }

    if (!strcmp(argv[1], "write")) {
        if (argc < 4)
            usage(-1);

        if (argc - 3 > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", argc - 3);

        cmd = cmd_write;
    }

    if (!strcmp(argv[1], "read")) {
        if (argc != 4)
            usage(-1);

        cmd = cmd_read;
    }

    if (cmd < 0) {
        fprintf(stderr, "not support this cmd: %s\n", argv[1]);
        usage(-1);
    }


    void *mem = NULL;
    int fd = nemc_open(argv[2], &mem);
    if(fd < 0) {
        fprintf(stderr, "%s: failed to open dev : %s\n", command, argv[2]);
        return -1;
    }

    int ret;
    int i;
    struct nemc_timing timing;
    unsigned char *data = mem;

    if (cmd == cmd_set_timing) {

        for (i = 3; i < argc; i++) {
            if (parse_uint(argv[i], "tas=", &timing.tas, 16))
                continue;

            if (parse_uint(argv[i], "tah=", &timing.tah, 16))
                continue;

            if (parse_uint(argv[i], "taw=", &timing.taw, 16))
                continue;

            if (parse_uint(argv[i], "tbp=", &timing.tbp, 16))
                continue;

            if (parse_uint(argv[i], "strv=", &timing.strv, 16))
                continue;

            fprintf(stderr, "error: not support this arg: %s\n", argv[i]);
            goto close_fd;
        }

        nemc_set_timing(fd, &timing);

    }

    if (cmd == cmd_get_timing) {
        nemc_get_timeing(fd, &timing);
        printf("timing->tas = 0x%x, taw = 0x%x, tbp = 0x%x, tah = 0x%x, strv = 0x%x\n",\
                timing.tas, timing.taw, timing.tbp, timing.tah, timing.strv);
    }

    if (cmd == cmd_read) {
        int bytes = data_process(argv[3]);

        if (bytes < 0) {
            fprintf(stderr, "%s: bytes format err %s\n", command, argv[3]);
            goto close_fd;
        }

        for (i = 0; i < bytes; i++)
            printf("%x ", data[i]);

        printf("\n");
    }


    if (cmd == cmd_write) {
        for (i = 0; i < argc - 3; i++) {
            ret = data_process(argv[i + 3]);
            if(ret < 0 || ret > 0xff) {
                fprintf(stderr, "data format error\n");
                goto close_fd;
            }
            data[i] = ret;
        }
    }

close_fd:
    nemc_close(fd, mem);

    return 0;

}