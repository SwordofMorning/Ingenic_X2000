#include <libhardware2/v4l2_camera.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        exit(-1);
    }

    *value = v;
    return 1;
}

static const char *parse_str(const char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}

const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help      : show help info\n");
    fprintf(stderr, "    show_fmts      : show the supported fmts\n");
    fprintf(stderr, "    width=            camera picture width, default from camera\n");
    fprintf(stderr, "    height=           camera picture height, default from camera\n");
    fprintf(stderr, "    fmt=              camera picture format, YUYV, MJPG... default from camera\n");
    fprintf(stderr, "    type=             camera picture ioctl type, default from camera\n");
    fprintf(stderr, "    output=           path to save camera picture, default /tmp/output\n");
    fprintf(stderr, "    frames=           frames to capture camera picture, default 30\n");
    fprintf(stderr, "-> %s /dev/video4 show_fmts # show the fmts of /dev/video4 \n", prg_name);
    fprintf(stderr, "-> %s /dev/video4  # show the fmts of /dev/video4 \n", prg_name);
    exit(status);
}

int main(int argc, char *argv[])
{
    int frames = 30;
    int show_fmts = 0;
    int ret = 0;
    struct v4l2_camera_format format = {0};
    const char *save_path = "/tmp/ouput";

    prg_name = argv[0];

    if (argc < 2) {
        fprintf(stderr, "too few args\n");
        usage(-1);
    }

    int i;
    for (i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            usage(0);
        if (!strcmp(argv[i], "show_fmts")) {
            show_fmts = 1;
            continue;
        }
        if (parse_uint(argv[i], "width=", &format.width, 10))
            continue;
        if (parse_uint(argv[i], "height=", &format.height, 10))
            continue;
        if (parse_uint(argv[i], "type=", &format.type, 10))
            continue;
        if (parse_uint(argv[i], "frames=", &frames, 10))
            continue;
        const char *str = parse_str(argv[i], "fmt=");
        if (str) {
            if (strlen(str) != 4) {
                fprintf(stderr, "error: fmt len must equal to 4: %s\n", str);
                return -1;
            }
            format.format = v4l2_fourcc(str[0], str[1], str[2], str[3]);
            continue;
        }
        str = parse_str(argv[i], "output=");
        if (str) {
            save_path = str;
            continue;
        }

        fprintf(stderr, "error: not support this arg: %s\n", argv[i]);
        return -1;
    }

    struct v4l2_camera *camera = v4l2_camera_open(argv[1]);
    if (!camera)
        return -1;

    if (show_fmts) {
        struct v4l2_camera_format fmts[32];
        int n = v4l2_camera_enum_formats(camera, fmts, 32);
        if (!n) {
            fprintf(stderr, "failed to enum fmts\n");
            ret = -1;
        } else {
            for (i = 0; i < n; i++)
                v4l2_camera_dump_format(&fmts[i]);
        }
        goto close_camera;
    }

    ret = v4l2_camera_detect_format(camera, &format);
    if (ret)
        goto close_camera;
    
    v4l2_camera_dump_format(&format);

    ret = v4l2_camera_create_buffer(camera, &format, 3);
    if (ret)
        goto close_camera;

    ret = v4l2_camera_stream_on(camera);
    if (ret)
        goto close_camera;

    i = 0;

    while (1) {
        struct v4l2_camera_buffer buf;
        ret = v4l2_camera_dequeue_buffer(camera, &buf, 1000);
        if (ret)
            goto close_camera;

        printf("index: %d %d\n", buf.index, buf.size[0]);

        if (strlen(save_path)) {
            char path[128];

            sprintf(path, "%s_%d.pic", save_path, i);

            FILE *file = fopen(path, "w");
            if (!file) {
                fprintf(stderr, "failed to [%s]: %s %s\n", "create file", path, strerror(errno));
                break;
            }

            int j;
            for (j = 0; buf.data[j] != NULL; j++) {
                ret = fwrite(buf.data[j], 1, buf.size[j], file);
                if (ret != buf.size[j]) {
                    fprintf(stderr, "failed to write: %s %d %s\n", path, ret, strerror(errno));
                    goto close_camera;
                }
            }

            fclose(file);
        }

        ret = v4l2_camera_queue_buffer(camera, &buf);
        if (ret)
            goto close_camera;

        if (i++ >= frames)
            break;
    }

close_camera:
    v4l2_camera_close(camera);
    return ret;
}
