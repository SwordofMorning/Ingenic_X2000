#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/jpegd_decode.h>
#include <libhardware2/fb.h>
#include <libhardware2/rmem.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

static struct fb_device_info fb_info;
static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help               : show help info\n");
    fprintf(stderr, "    file_path               : decode file\n");
    fprintf(stderr, "    width                   : input frame width\n");
    fprintf(stderr, "    height                  : input frame height\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    Example: %s test.jpg 1280 720\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

volatile sig_atomic_t interrupted = 0;

void handle_signal(int signum) {
    if (signum == SIGINT) {
        interrupted = 1;
    }
}

int main(int argc, char *argv[])
{
    int ret;
    char *input_mem;
    void *frames_vaddr;
    unsigned long frames_paddr;
    int indexs[2];
    int num = 0;

    prg_name = argv[0];

    if (argc < 4)
        usage(-1);

    if (!strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help"))
        usage(argc == 4 ? 0 : -1);

    const char *src_file_path = argv[1];
    int src_fd = open(src_file_path, O_RDONLY);
    if (src_fd < 0) {
        fprintf(stderr, "Failed to open src file\n");
        exit(-1);
    }
    int filesize = lseek(src_fd, 0, SEEK_END);
    lseek(src_fd, 0, SEEK_SET);

    input_mem = malloc(filesize);
    if (!input_mem) {
        fprintf(stderr, "Failed to alloc input_mem\n");
        goto close_fd;
    }
    read(src_fd, input_mem, filesize);

    int fb_fd = fb_open("/dev/fb0", &fb_info);
    if (fb_fd < 0) {
        fprintf(stderr, "Failed to open fb0\n");
        goto free_input;
    }

    indexs[0] = 0;
    indexs[1] = (fb_info.frame_nums < 2) ? 0 : 1;

    ret = fb_enable(fb_fd);
    if (ret < 0) {
        fprintf(stderr, "Failed to enable fb0\n");
        goto close_fb;
    }

    /* 设置解码参数 */
    struct jpegd_decoder_config config = {
        .file_size = filesize,
        .width = atoi(argv[2]),
        .height = atoi(argv[3]),
        .out_fmt = JPEGD_PIX_FMT_BGRA_8888,
        .input_mem = input_mem
    };

    struct jpegd_decoder *decoder = jpegd_decoder_open();
    if (!decoder) {
        fprintf(stderr, "Unable to open jpeg decoder\n");
        goto disable_fb;
    }

    struct jpegd_decoder_output_data output;

    signal(SIGINT, handle_signal);
    while (!interrupted) {
        frames_vaddr = fb_info.mapped_mem + fb_info.frame_size * num;
        frames_paddr = fb_info.fix.smem_start + fb_info.frame_size * num;
        ret = jpegd_decoder_set_output_mem(decoder, &config, frames_vaddr, frames_paddr, fb_info.frame_size);
        if (ret < 0) {
            fprintf(stderr, "jpeg decoder set output memory failed.\n");
            goto close_decoder;
        }

        ret = jpegd_decoder_get(decoder, &config, &output);
        if (ret < 0) {
            fprintf(stderr, "jpeg decoder acquire failed.\n");
            continue;
        }

        ret = fb_pan_display(fb_fd, &fb_info, num);
        if (ret < 0)
            goto close_decoder;

        jpegd_decoder_put(decoder, &output);

        num = indexs[!num];
    }

close_decoder:
    usleep(30000);
    jpegd_decoder_close(decoder);

disable_fb:
    fb_disable(fb_fd);

close_fb:
    fb_close(fb_fd, &fb_info);

free_input:
    free(input_mem);

close_fd:
    close(src_fd);

    return 0;
}
