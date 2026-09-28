#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int dtrng_get_random_number(unsigned int *value)
{
    int handle, ret;

    handle = open("/dev/jz_dtrng", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "DTRNG:dtrng open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = read(handle, value, 4);
    if (ret < 0) {
        fprintf(stderr, "DTRNG:dtrng get random number failed: %s\n", strerror(errno));
        close(handle);
        return -1;
    }

    close(handle);

    return 0;
}