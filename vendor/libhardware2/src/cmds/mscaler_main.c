#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libhardware2/mscaler.h>
#include <assert.h>

static const char *prg_name;

#define ALIGN(d,a) (((d)+((a)-1))/(a)*(a))

static void usage(int status)
{
    fprintf(stderr,
             "\n %s usage\n"
             "    -h, --help                  show help info\n"
             "    convert src args...  dst args...  convert frame\n"
             "    src:\n"
             "        file=                         frame path, default stdin\n"
             "        size=                         frame size, default 0, width, height, format and stride can get size\n"
             "        fmt=                          frame format(only support NV12/NV21)\n"
             "        width=                        frame width\n"
             "        height=                       frame height\n"
             "        stride=                       frame stride, default 0, width, height, format and size can get stride\n"
             "    dst:\n"
             "        file=                         frame path, default stdout\n"
             "        fmt=                          frame format\n"
             "        width=                        frame width\n"
             "        height=                       frame height\n"
             "        stride=                       frame stride, default 0, width, height, format and size can get stride\n"
             "    example:%s convert src file=stdin fmt=NV12 width=1920 heigth=1080 dst file=stdout fmt=BGRA8888 width=480 heigth=854\n"
             "\n"
             "    show_format                 show support format\n"
             "\n", prg_name, prg_name);
    exit(status);
}

static void show_dst_format(void)
{
    fprintf(stderr,
            "\nShow: %s dst_format\n"
            "NV12\n"
            "NV21\n"
            "BGRA8888\n"
            "GBRA8888\n"
            "RBGA8888\n"
            "BRGA8888\n"
            "GRBA8888\n"
            "RGBA8888\n"
            "ABGR8888\n"
            "AGBR8888\n"
            "ARBG8888\n"
            "ABRG8888\n"
            "AGRB8888\n"
            "ARGB8888\n"
            "BGR565\n"
            "GBR565\n"
            "RBG565\n"
            "BRG565\n"
            "GRB565\n"
            "RGB565\n"
             "\n", prg_name);
}

static void show_src_format(void)
{
    fprintf(stderr,
            "\nShow: %s src_format\n"
            "NV12\n"
            "NV21\n"
             "\n", prg_name);
}

static int get_default_stride(int width, enum mscaler_fmt fmt)
{
    switch (fmt) {
        case MSCALER_FORMAT_NV12:
        case MSCALER_FORMAT_NV21:
            return width;
        case MSCALER_FORMAT_BGRA_8888:
        case MSCALER_FORMAT_GBRA_8888:
        case MSCALER_FORMAT_RBGA_8888:
        case MSCALER_FORMAT_BRGA_8888:
        case MSCALER_FORMAT_GRBA_8888:
        case MSCALER_FORMAT_RGBA_8888:
        case MSCALER_FORMAT_ABGR_8888:
        case MSCALER_FORMAT_AGBR_8888:
        case MSCALER_FORMAT_ARBG_8888:
        case MSCALER_FORMAT_ABRG_8888:
        case MSCALER_FORMAT_AGRB_8888:
        case MSCALER_FORMAT_ARGB_8888:
            return width * 4;
        case MSCALER_FORMAT_BGR_565:
        case MSCALER_FORMAT_GBR_565:
        case MSCALER_FORMAT_RBG_565:
        case MSCALER_FORMAT_BRG_565:
        case MSCALER_FORMAT_GRB_565:
        case MSCALER_FORMAT_RGB_565:
            return width * 2;
    }
    return 0;
}

static inline int stride_align(int stride, struct mscaler_device_info *ms_info)
{
    return ALIGN(stride, ms_info->stride_align);
}

static inline int frame_align(int frame_size, struct mscaler_device_info *ms_info)
{
    return ALIGN(frame_size, ms_info->frame_align);
}

static int write_dst(FILE *fp, void *y_mem, void *uv_mem, int height, int default_stride, int buffer_stride)
{
    assert(fp);
    int i = 0;
    size_t write_size;

    for (i = 0; i < height; i++) {
        write_size = fwrite(y_mem, sizeof(char), default_stride, fp);
        if(write_size != default_stride) {
            fprintf(stderr, "mscaler: fwrite fail, write_size(%d) != default_stride(%d)\n", write_size, default_stride);
            return -1;
        }
        y_mem = y_mem + buffer_stride;
    }

    if (uv_mem) {
        for (i = 0; i < height / 2; i++) {
            write_size = fwrite(uv_mem, sizeof(char), default_stride, fp);
            if(write_size != default_stride) {
                fprintf(stderr, "mscaler: fwrite fail, write_size(%d) != default_stride(%d)\n", write_size, default_stride);
                return -1;
            }
            uv_mem = uv_mem + buffer_stride;
        }
    }
    return 0;
}

static int read_src(FILE *fp, void *y_mem, void *uv_mem, int height, int default_stride, int buffer_stride)
{
    assert(fp);
    int i = 0;
    size_t read_size = 0;

    for (i = 0; i < height; i++) {
        read_size = fread(y_mem, sizeof(char), default_stride, fp);
        if(read_size != default_stride) {
            fprintf(stderr, "mscaler: fread y fail, read_size(%d) != default_stride(%d)\n", read_size, default_stride);
            return -1;
        }
        y_mem = y_mem + buffer_stride;
    }

    for (i = 0; i < height / 2; i++) {
        read_size = fread(uv_mem, sizeof(char), default_stride, fp);
        if(read_size != default_stride) {
            fprintf(stderr, "mscaler: fread uv fail, read_size(%d) != default_stride(%d)\n", read_size, default_stride);
            return -1;
        }
        uv_mem = uv_mem + buffer_stride;
    }

    return 0;
}

static size_t get_file_size(char *path)
{
    int ret;
    int fd;
    struct stat statbuf;

    assert(path);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "mscaler: getFileSize open %s fail!\n", path);
        return -1;
    }

    ret = fstat(fd, &statbuf);
    if (ret < 0) {
        fprintf(stderr, "mscaler: fstat fail, %s\n", path);
        close(fd);
        return -1;
    }
    close(fd);

    return statbuf.st_size;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

static enum mscaler_fmt get_src_format(const char *str)
{
    if (!strcmp(str, "NV12"))
        return  MSCALER_FORMAT_NV12;
    if (!strcmp(str, "NV21"))
        return  MSCALER_FORMAT_NV21;
    return -1;
}

static enum mscaler_fmt get_dst_format(const char *str)
{
    if (!strcmp(str, "NV12"))
        return  MSCALER_FORMAT_NV12;
    if (!strcmp(str, "NV21"))
        return  MSCALER_FORMAT_NV21;
    if (!strcmp(str, "BGRA8888"))
        return  MSCALER_FORMAT_BGRA_8888;
    if (!strcmp(str, "BGRA8888"))
        return  MSCALER_FORMAT_GBRA_8888;
    if (!strcmp(str, "RBGA8888"))
        return  MSCALER_FORMAT_RBGA_8888;
    if (!strcmp(str, "BRGA8888"))
        return  MSCALER_FORMAT_BRGA_8888;
    if (!strcmp(str, "GRBA8888"))
        return  MSCALER_FORMAT_GRBA_8888;
    if (!strcmp(str, "RGBA8888"))
        return  MSCALER_FORMAT_RGBA_8888;
    if (!strcmp(str, "ABGR8888"))
        return  MSCALER_FORMAT_ABGR_8888;
    if (!strcmp(str, "AGBR8888"))
        return  MSCALER_FORMAT_AGBR_8888;
    if (!strcmp(str, "ARBG8888"))
        return  MSCALER_FORMAT_ARBG_8888;
    if (!strcmp(str, "ABRG8888"))
        return  MSCALER_FORMAT_ABRG_8888;
    if (!strcmp(str, "AGRB8888"))
        return  MSCALER_FORMAT_AGRB_8888;
    if (!strcmp(str, "ARGB8888"))
        return  MSCALER_FORMAT_ARGB_8888;
    if (!strcmp(str, "BGR565"))
        return  MSCALER_FORMAT_BGR_565;
    if (!strcmp(str, "GBR565"))
        return  MSCALER_FORMAT_GBR_565;
    if (!strcmp(str, "RBG565"))
        return  MSCALER_FORMAT_RBG_565;
    if (!strcmp(str, "BRG565"))
        return  MSCALER_FORMAT_BRG_565;
    if (!strcmp(str, "GRB565"))
        return  MSCALER_FORMAT_GRB_565;
    if (!strcmp(str, "RGB565"))
        return  MSCALER_FORMAT_RGB_565;
    return -1;
}

static int parse_int(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtol(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static char *parse_str(char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}

int param_check(struct mscaler_frame *frame)
{
    if (frame->fmt == -1) {
        fprintf(stderr, "mscaler: fmt error\n");
        exit(-1);
    }

    if (frame->xres == -1) {
        fprintf(stderr, "mscaler: width error\n");
        exit(-1);
    }

    if (frame->yres == -1) {
        fprintf(stderr, "mscaler: height error\n");
        exit(-1);
    }
    fprintf(stderr, " pass\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    static int fd;
    int src_size = 0;
    int alloc_size = 0;
    FILE *src_file = NULL;
    FILE *dst_file = NULL;
    char *src_file_path = NULL;
    char *dst_file_path = NULL;

    int src_file_stride = 0;
    int dst_file_stride = 0;
    struct mscaler_frame src;
    struct mscaler_frame dst;
    struct mscaler_device_info ms_info;

    memset(&src, 0, sizeof(struct mscaler_frame));
    memset(&dst, 0, sizeof(struct mscaler_frame));

    src.fmt = -1;
    dst.fmt = -1;
    prg_name = argv[0];

    while (1) {
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "convert")) {
            if (argc < 3 || argc > 14)
                usage(-1);
            int i = 2;
            char *format = NULL;
            struct mscaler_frame *tmp = NULL;

            for (i = 2; i < argc; i++) {
                if (!strcmp(argv[i], "src")) {
                    tmp = &src;
                    continue;
                }

                if (!strcmp(argv[i], "dst")) {
                    tmp = &dst;
                    continue;
                }

                if (parse_int(argv[i], "size=", &src_size, 10))
                    continue;

                if (parse_int(argv[i], "width=", &tmp->xres, 10))
                    continue;

                if (parse_int(argv[i], "height=", &tmp->yres, 10))
                    continue;

                if (parse_int(argv[i], "stride=", &tmp->y.stride, 10)) {
                    if (tmp == &src)
                        src_file_stride = tmp->y.stride;
                    else
                        dst_file_stride = tmp->y.stride;
                    continue;
                }

                if ((parse_str(argv[i], "file="))) {
                    if (tmp == &src)
                        src_file_path = parse_str(argv[i], "file=");
                    else
                        dst_file_path = parse_str(argv[i], "file=");
                    continue;
                }

                if ((format = parse_str(argv[i], "fmt="))) {
                    if (tmp == &src)
                        src.fmt = get_src_format(format);
                    else
                        dst.fmt = get_dst_format(format);
                    continue;
                }

                error_arg(argv[i]);
            }
            break;
        }

        if (!strcmp(argv[1], "show_format")) {
            if (argc != 2)
                usage(-1);

            show_src_format();
            show_dst_format();
            exit(0);
        }

        fprintf(stderr, "error: not support this cmd: %s\n", argv[1]);
        exit(-1);
    }
    fprintf(stderr, "mscaler: src param check");
    param_check(&src);
    fprintf(stderr, "mscaler: dst param check");
    param_check(&dst);

    fd = mscaler_open(&ms_info);
    if (fd < 0)
        return -1;

    src_file = stdin;
    dst_file = stdout;

    if (strcmp(src_file_path, "stdin") != 0) {
        int tmp_size = 0;
        tmp_size = get_file_size(src_file_path);
        if (tmp_size == -1) {
            ret = -1;
            goto close;
        }
        if (src_size) {
            if (tmp_size < src_size) {
                fprintf(stderr, "mscaler: %s file size %d < size %d\n", src_file_path, tmp_size, src_size);
                ret = -1;
                goto close;
            }
        }
        if (src_size == 0)
            src_size = tmp_size;

        src_file = fopen(src_file_path, "rb");
        if (src_file == NULL) {
           fprintf(stderr, "mscaler: fopen %s fail!\n",src_file_path);
           ret = -1;
           goto close;
        }
    }

    if (strcmp(dst_file_path, "stdout") != 0) {
        dst_file = fopen(dst_file_path, "wb");
        if (dst_file == NULL) {
           fprintf(stderr, "mscaler: fopen %s fail!\n",dst_file_path);
           ret = -1;
           goto close;
        }
    }

    if (src_size == 0) {
        if (src_file_stride == 0)
            src_file_stride = get_default_stride(src.xres, src.fmt);
    }

    if (src_file_stride == 0)
        src_file_stride = src_size * 2 / (3 * src.yres);

    if (dst_file_stride == 0)
        dst_file_stride = get_default_stride(dst.xres, dst.fmt);

    src.y.stride = stride_align(src_file_stride, &ms_info);
    src.y.mem_size = frame_align(src.y.stride * src.yres, &ms_info);

    src.uv.stride = src.y.stride;
    src.uv.mem_size = frame_align(src.uv.stride * src.yres / 2, &ms_info);

    dst.y.stride = stride_align(dst_file_stride, &ms_info);
    dst.y.mem_size = frame_align(dst.y.stride * dst.yres, &ms_info);

    if (dst.fmt == MSCALER_FORMAT_NV12 || dst.fmt == MSCALER_FORMAT_NV21) {
        dst.uv.stride = dst.y.stride;
        dst.uv.mem_size = frame_align(dst.uv.stride * dst.yres / 2, &ms_info);
    }

    alloc_size = src.y.mem_size + src.uv.mem_size + dst.y.mem_size + dst.uv.mem_size;
    ret = mscaler_alloc_mem(fd, &ms_info, alloc_size);
    if (ret < 0)
        goto close;

    src.y.mem = ms_info.mapped_mem;
    src.y.phys_addr = ms_info.phys_addr;

    src.uv.mem = src.y.mem + src.y.mem_size;
    src.uv.phys_addr = src.y.phys_addr + src.y.mem_size;

    dst.y.mem = src.uv.mem + src.uv.mem_size;
    dst.y.phys_addr = src.uv.phys_addr + src.uv.mem_size;

    if (dst.fmt == MSCALER_FORMAT_NV12 || dst.fmt == MSCALER_FORMAT_NV21) {
        dst.uv.mem = dst.y.mem + dst.y.mem_size;
        dst.uv.phys_addr = dst.y.phys_addr + dst.y.mem_size;
    }

    ret = read_src(src_file, src.y.mem, src.uv.mem, src.yres, src_file_stride, src.y.stride);
    if(ret < 0) {
        fprintf(stderr, "mscaler: read fail\n");
        if (strcmp(src_file_path, "stdin")) {
            fclose(src_file);
        }
        goto free;
    }

    if (strcmp(src_file_path, "stdin") != 0)
        fclose(src_file);

    fprintf(stderr, "mscaler:src.y.stride  %d\n", src.y.stride);
    fprintf(stderr, "mscaler:src.uv.stride %d\n", src.uv.stride);
    fprintf(stderr, "mscaler:dst.y.stride  %d\n", dst.y.stride);
    fprintf(stderr, "mscaler:dst.uv.stride %d\n", dst.uv.stride);

    ret = mscaler_convert(fd, &src, &dst);
    if (ret < 0) {
        fprintf(stderr, "mscaler: error mscaler_convert ret %d\n",ret);
        goto free;
    }

    ret = write_dst(dst_file, dst.y.mem, dst.uv.mem, dst.yres, dst_file_stride, dst.y.stride);
    if(ret) {
        fprintf(stderr, "mscaler: write fail\n");
        if (strcmp(dst_file_path, "stdout") != 0) {
            fclose(dst_file);
        }
        goto free;
    }

    if (strcmp(dst_file_path, "stdout") != 0)
        fclose(dst_file);

free:
    ret = mscaler_free_mem(fd, &ms_info);
    if (ret < 0)
        fprintf(stderr, "mscaler: mscaler_munmap error\n");
close:
    if (mscaler_close(fd) < 0)
        return -1;
    return ret;
}
