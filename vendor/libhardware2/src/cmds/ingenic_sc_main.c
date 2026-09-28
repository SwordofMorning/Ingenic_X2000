#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <linux/input.h>
#include <libhardware2/ingenic_sc.h>

static char *prg_name;
static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help  : show help info\n");
    fprintf(stderr, "    src=       : encrypted secure src_data_filepath, must be set and cannot be same with dst_file_name\n");
    fprintf(stderr, "    dst=       : decrypt dst_data_filepath, default \"src_file_name\" + _dst\n");
    fprintf(stderr, "    Example: %s src=/usr/data/secure_file\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    prg_name = argv[0];
    unsigned char *src_file = NULL;
    unsigned char *dst_file = NULL;
    unsigned char is_malloc = 0;

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    int i;
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "src=", strlen("src=")) == 0) {
            src_file = argv[i] + strlen("src=");
            continue;
        }

        if (strncmp(argv[i], "dst=", strlen("dst=")) == 0) {
            dst_file = argv[i] + strlen("dst=");
            continue;
        }

        fprintf(stderr, "error: not support this arg: %s\n", argv[i]);
        exit(-1);
    }

    if (src_file == NULL) {
        fprintf(stderr, "sc: src must be set!\n");
        return -1;
    }

    /* just for set dst_file_name */
    if (dst_file == NULL) {
        dst_file = malloc(strlen(src_file) + 4 + 1);
        memset(dst_file, 0, strlen(src_file) + 4 + 1);
        sprintf(dst_file, "%s_dst", src_file);
        is_malloc = 1;
    }

    if (strlen(src_file) == strlen(dst_file) && strncmp(src_file, dst_file, strlen(src_file)) == 0) {
        fprintf(stderr, "sc: dst_file_name cannot be same with src_file_name!\n");
        return -1;
    }

    fprintf(stderr, "src_file    : %s\n", src_file);
    fprintf(stderr, "dst_file    : %s\n", dst_file);

    int fd = sc_open();
    if (fd < 0) {
        fprintf(stderr, "sc: open dev filed!\n");
        goto err;
    }

    ret = sc_decrypt_file(fd, src_file, dst_file);
    if (ret < 0)
        fprintf(stderr, "sc: failed to decrypt file!\n");

err:
    if (is_malloc)
        free(dst_file);

    sc_close(fd);

    return ret;
}
