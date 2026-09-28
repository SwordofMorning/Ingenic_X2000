#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <libhardware2/camera.h>
#include <libhardware2/fb.h>

#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define MAY_UNUSED(n)                   ((n)=(n))

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                        : show help info\n");
    fprintf(stderr, "    power_on device_path             : power on camera\n");
    fprintf(stderr, "    Example: %s  power_on /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    power_off device_path            : power off camera\n");
    fprintf(stderr, "    Example: %s  power_off /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    stream_on device_path            : stream on camera, start capture\n");
    fprintf(stderr, "    Example: %s  stream_on /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    stream_off device_path           : stream off camera, stop capture\n");
    fprintf(stderr, "    Example: %s  stream_off /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    set_fps [fps] device_path        : set camera fps\n");
    fprintf(stderr, "    Example: %s  set_fps 20 /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    get_reg [reg_addr] device_path                 : get camera reg val\n");
    fprintf(stderr, "    Example: %s  get_reg 0x3500 /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    set_reg [reg_addr] [reg_val] device_path       : set camera reg val\n");
    fprintf(stderr, "    Example: %s  set_reg 0x3500 0x10 /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    info device_path                 : show camera info\n");
    fprintf(stderr, "    Example: %s  info /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    get_frame  device_path           : get one frame data to stdout\n");
    fprintf(stderr, "    Example: %s  get_frame /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    get_frame_files [frame_conut] device_path  : get one or several frame(s) to /tmp/fx.raw\n");
    fprintf(stderr, "    Example: %s  get_frame_files 3 /dev/camera\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    drop_all_frames device_path      : drop all captured frames\n");
    fprintf(stderr, "    Example: %s  drop_all_frames /dev/camera\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

static inline uint64_t boot_time_usecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 * 1000 + ts.tv_nsec / 1000;
}

enum {
    cmd_power_on,
    cmd_power_off,
    cmd_stream_on,
    cmd_stream_off,
    cmd_set_fps,
    cmd_get_reg,
    cmd_set_reg,
    cmd_info,
    cmd_get_frame,
    cmd_get_frame_files,
    cmd_drop_all,
};

int save_mem_to_file(const char *path, void *addr, unsigned int size)
{
    unsigned int ret = 0;

    FILE * pic_fp = fopen(path, "wb");
    if (pic_fp < 0) {
        fprintf(stderr, "Error save open %s fail \n", path);
        return -1;
    }

    int write_size = fwrite((void *)addr, 1, size, pic_fp);
    if (write_size != size) {
        fprintf(stderr, "Error save write %s fail \n", path);
        return -1;
    }

    fclose(pic_fp);
    fflush(stdout);
    printf("save frame to %s\n", path);
    MAY_UNUSED(ret);

    return 0;
}

static struct camera_info info;

int main(int argc, char *argv[])
{
    int ret = 0;
    int cmd;
    int fps_s = 0;
    int frame_count = 0;
    struct sensor_dbg_register reg = {0};

    prg_name = argv[0];

    while (1) {
        if (argc < 3)
            usage(-1);

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "--help"))
            usage(argc >= 3 ? 0 : -1);

        if (!strcmp(argv[1], "power_on")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_power_on;
            break;
        }

        if (!strcmp(argv[1], "power_off")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_power_off;
            break;
        }

        if (!strcmp(argv[1], "stream_on")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_stream_on;
            break;
        }

        if (!strcmp(argv[1], "stream_off")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_stream_off;
            break;
        }

        if (!strcmp(argv[1], "set_fps")) {
            if (argc != 4)
                usage(-1);
            cmd = cmd_set_fps;
            fps_s = atoi(argv[2]);
            break;
        }

        if (!strcmp(argv[1], "get_reg")) {
            if (argc != 4)
                usage(-1);
            cmd = cmd_get_reg;
            char *p = 0;
            reg.reg = strtoul(argv[2], &p, 0);
            break;
        }

        if (!strcmp(argv[1], "set_reg")) {
            if (argc != 5)
                usage(-1);
            cmd = cmd_set_reg;
            char *p = 0;
            reg.reg = strtoul(argv[2], &p, 0);
            reg.val = strtoul(argv[3], &p, 0);
            break;
        }

        if (!strcmp(argv[1], "info")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_info;
            break;
        }

        if (!strcmp(argv[1], "get_frame")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_get_frame;
            break;
        }

        if (!strcmp(argv[1], "get_frame_files")) {
            if (argc != 4)
                usage(-1);
            cmd = cmd_get_frame_files;
            frame_count = atoi(argv[2]);
            break;
        }

        if (!strcmp(argv[1], "drop_all_frames")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_drop_all;
            break;
        }

        fprintf(stderr, "error: not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    int fd = -1;
    if (cmd == cmd_set_reg)
        fd = camera_open(&info, argv[4]);
    else if (cmd == cmd_set_fps || cmd == cmd_get_reg || cmd == cmd_get_frame_files)
        fd = camera_open(&info, argv[3]);
    else
        fd = camera_open(&info, argv[2]);

    if (fd == -1)
        return -1;

    if (cmd == cmd_power_on) {
        ret = camera_power_on(fd);
        goto close_fd;
    }

    if (cmd == cmd_power_off) {
        ret = camera_power_off(fd);
        goto close_fd;
    }

    if (cmd == cmd_stream_on) {
        ret = camera_stream_on(fd);
        goto close_fd;
    }

    if (cmd == cmd_stream_off) {
        ret = camera_stream_off(fd);
        goto close_fd;
    }

    if (cmd == cmd_set_fps) {
        ret = camera_set_fps(fd, fps_s, 1);
        goto close_fd;
    }

    if (cmd == cmd_get_reg) {
        ret = camera_get_sensor_reg(fd, &reg);
        printf("get sensor reg 0x%llx, val=0x%llx\n", reg.reg, reg.val);
        goto close_fd;
    }

    if (cmd == cmd_set_reg) {
        printf("set sensor reg 0x%llx, val=0x%llx\n", reg.reg, reg.val);
        ret = camera_set_sensor_reg(fd, &reg);
        goto close_fd;
    }

    if (cmd == cmd_info) {
        char fmt_a = (char)(info.data_fmt >> 0);
        char fmt_b = (char)(info.data_fmt >> 8);
        char fmt_c = (char)(info.data_fmt >> 16);
        char fmt_d = (char)(info.data_fmt >> 24);
        printf("name: %s\n", info.name);
        printf("width: %d\n", info.width);
        printf("height: %d\n", info.height);
        printf("fps: %d\n", info.fps);
        printf("data_fmt: %C%C%C%C\n", fmt_a, fmt_b, fmt_c, fmt_d);
        printf("line_length: %d\n", info.line_length);
        printf("frame_size: %d\n", info.frame_size);
        printf("frame_nums: %d\n", info.frame_nums);
        printf("phys_mem: %08lx\n", info.phys_mem);
        printf("mapped_mem: %p\n", info.mapped_mem);
        goto close_fd;
    }

    if (cmd == cmd_get_frame) {
        void *mem = camera_wait_frame(fd);
        if (mem)
            fwrite(mem, 1, info.frame_size, stdout);
        goto close_fd;
    }

    if (cmd == cmd_get_frame_files && frame_count > 0) {
        int i = 0;
        char f_name[32];
        while (i < frame_count) {
            void *mem = camera_wait_frame(fd);
            if (mem) {
                snprintf(f_name, 31, "/tmp/f%d.raw", i);
                ret = save_mem_to_file(f_name, mem, info.frame_size);
                camera_put_frame(fd, mem);
                if (ret)
                    break;
            }
            i++;
        }

        goto close_fd;
    }

    if (cmd == cmd_drop_all) {
        ret = camera_drop_frames(fd, info.frame_nums);
        goto close_fd;
    }

close_fd:
    camera_close(fd, &info);
    return ret;
}
