#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/jpege_encode.h>
#include <libhardware2/rmem.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help               : show help info\n");
    fprintf(stderr, "    file_path               : encode file\n");
    fprintf(stderr, "    width                   : input frame width\n");
    fprintf(stderr, "    height                  : input frame height\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    Example: %s picture 1280 720\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}


int main(int argc, char *argv[])
{
    int ret;
    char *input_mem;
    const char *dst_file_path = "/usr/data/test.jpg";

    prg_name = argv[0];

    if (argc < 4)
        usage(-1);

    if (!strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help"))
        usage(argc == 4 ? 0 : -1);

    const char *src_file_path = argv[1];
    int src_fd = open(src_file_path, O_RDONLY);
    if (src_fd == -1) {
        perror("Failed to open src file");
        return -1;
    }

    int filesize = lseek(src_fd, 0, SEEK_END);
    lseek(src_fd, 0, SEEK_SET);

    input_mem = malloc(filesize);
    if (!input_mem) {
        fprintf(stderr, "Failed to alloc input_mem\n");
        goto close_src_fd;
    }
    read(src_fd, input_mem, filesize);


    struct jpege_encoder *encoder;
    encoder = jpege_encoder_open();
    if (!encoder) {
        fprintf(stderr, "Unable to open jpeg encoder\n");
        goto free_input;
    }

    /* 设置编码参数 */
    struct jpege_encoder_config config;
    config.file_size = filesize;
    config.qa = 40;
    config.width = atoi(argv[2]);
    config.height = atoi(argv[3]);
    config.in_fmt = JPEGE_PIX_FMT_NV12;
    config.input_mem = input_mem;
    /* 获取编码后的output */
    struct jpege_encoder_output_data output;
    ret = jpege_encoder_get(encoder, &config, &output);
    if (ret < 0) {
        fprintf(stderr, "jpeg encoder acquire failed.\n");
        goto close_encoder;
    }

    int dst_fd = open(dst_file_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (dst_fd == -1) {
        fprintf(stderr, "Failed to open dst file");
        goto close_encoder;
    }
    write(dst_fd, output.mem, output.image_size);

    /* 释放output */
    jpege_encoder_put(encoder, &output);

    close(dst_fd);

close_encoder:
    jpege_encoder_close(encoder);

free_input:
    free(input_mem);

close_src_fd:
    close(src_fd);

    return 0;
}
