#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/rmem.h>

static void usage(char *prg_name)
{
    fprintf(stderr, "usage\n");
    fprintf(stderr, "    %s <extra_mem> <extra_size>\n", prg_name);
    exit(-1);
}

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    int size;
    unsigned long mem;

    if (argc != 3)
        usage(argv[0]);

    mem = atol(argv[1]);
    size = atoi(argv[2]);
    fprintf(stdout, "rmem_extra: mem %ld, size %d\n", mem, size);

    fd = rmem_open();
    if (fd < 0) {
        fprintf(stderr, "rmem_extra: open rmem device fail\n");
        return -1;
    }

    ret = rmem_add_extra_mem(fd, mem, size);
    if (fd < 0) {
        fprintf(stderr, "rmem_extra: extra mem fail %d\n", ret);
    }

    rmem_close(fd);

    return ret;
}