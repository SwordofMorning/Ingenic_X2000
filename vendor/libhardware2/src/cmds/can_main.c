#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <libhardware2/can.h>

static char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "Usage1:-h/--help                                 : show help info\n");
    fprintf(stderr, "Usage2:set_rate <dev_path> [rate]                : set rate of can\n");
    fprintf(stderr, "       [rate] transfer rate(dec, 50000/100000/125000/250000/500000/1000000), default 500000\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s set_rate /dev/can0 500000\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage3:set_filter <dev_path> <args...>           : enable can acceptance filter, default accept all id\n");
    fprintf(stderr, "       num=   filter num(0-3), must be set\n");
    fprintf(stderr, "       id=    filter id(hex, standard:0-7FF, extended:0-1FFFFFFF), default or set FFFFFFFF to accept all id\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s set_filter /dev/can0 num=0 id=16F\n", prg_name);
    fprintf(stderr, "    %s set_filter /dev/can0 num=1\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage4:put_filter <dev_path> <args...>           : disable can acceptance filter\n");
    fprintf(stderr, "       num=   filter num(0-3), must be set\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s put_filter /dev/can0 num=0\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage4:get_filter <dev_path>                     : dump can acceptance filter\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s get_filter /dev/can0\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage5:enable <dev_path>                         : enable can\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s enable /dev/can0\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage6:write <dev_path> <id>#[data...] [args...] : write frame\n");
    fprintf(stderr, "       id=    frame id(hex, standard:0-7FF, extended:0-1FFFFFFF), must be set\n");
    fprintf(stderr, "       data=  frame data up to 16 char in length, aligned to 2, must be set when type is data\n");
    fprintf(stderr, "       mode=  the frame mode(standard/extended), default standard\n");
    fprintf(stderr, "       type=  the frame type(data/request), default data\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s write /dev/can0 0A6#11223344556677\n", prg_name);
    fprintf(stderr, "    %s write /dev/can0 3721281# mode=extended type=remote\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage7:dump <dev_path>                           : receive frames and dump frame info\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s dump /dev/can0\n", prg_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage8:disable <dev_path>                        : disable can\n");
    fprintf(stderr, "  Example:\n");
    fprintf(stderr, "    %s disable /dev/can0\n", prg_name);

    exit(status);
}

enum {
    cmd_set_rate,
    cmd_set_filter,
    cmd_put_filter,
    cmd_get_filter,
    cmd_enable,
    cmd_write,
    cmd_dump,
    cmd_disable
};

int c_to_hex(char c)
{
    if((c >= '0') && (c <= '9'))
        return (c - 48);
    else if ((c >= 'a') && (c <='f'))
        return (c - 87);
    else if ((c >= 'A') && (c <='F'))
        return (c - 55);
    else if (c == 0)
        return 0x10;

    fprintf(stderr, "invalid data %c\n", c);
    usage(-1);
    return -1;
}

int str_to_hex_data(char *str, char *data, int base)
{
    int len = strlen(str);
    if (len > base)
        len = base;

    if (len % 2) {
        fprintf(stderr, "len should align to 2\n");
        return -1;
    }

    int i;
    for (i = 0; i < len; i += 2)
        data[i / 2] = ((c_to_hex(str[i]) & 0xF) << 4) | (c_to_hex(str[i+1]) & 0xF);

    return len / 2;
}

unsigned int str_to_hex_id(char *id_buf, int base)
{
    int i, id_len = strlen(id_buf);
    if (id_len > base) {
        id_buf += id_len - base;
        id_len = base;
    }

    unsigned char id_tmp[10] = {0};
    unsigned char high, low;
    for (i = 0; i < id_len; i += 2) {
        high = c_to_hex(id_buf[i]);
        low = c_to_hex(id_buf[i+1]);
        id_tmp[3 - i / 2] = ((high & 0xF) << 4) | (low & 0xF);
        if (low == 0x10) {
            i++;
            break;
        }
    }

    return *(unsigned int*)id_tmp >> (32 - 4 * i);
}

static int parse_id_data(const char *str, int *id, char *buf, int base)
{
    char tmp[30] = {0};
    memcpy(tmp, str, strlen(str));

    char *data = strchr(tmp, '#');
    if (data == NULL)
        usage(-1);

    data[0] = 0;

    *id = str_to_hex_id(tmp, 8);

    return str_to_hex_data(&data[1], buf, base);
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

static int parse_ustr(const char *src, const char *prefix,
                    unsigned char *dest, int base)
{
    int len = strlen(prefix);

    if (strncmp(src, prefix, len) || ((strlen(src) - len) >= base))
        return 0;

    memmove(dest, src + len, base);

    return 1;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

int main(int argc, char **argv)
{
    int cmd = -1;
    prg_name = argv[0];

    if (argc < 2)
        usage(-1);

    while (1) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "set_rate")) {
            if (argc < 3)
                usage(-1);
            cmd = cmd_set_rate;
            break;
        }

        if (!strcmp(argv[1], "set_filter")) {
            if (argc < 4)
                usage(-1);
            cmd = cmd_set_filter;
            break;
        }

        if (!strcmp(argv[1], "put_filter")) {
            if (argc != 4)
                usage(-1);
            cmd = cmd_put_filter;
            break;
        }

        if (!strcmp(argv[1], "get_filter")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_get_filter;
            break;
        }

        if (!strcmp(argv[1], "enable")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_enable;
            break;
        }

        if (!strcmp(argv[1], "write")) {
            if (argc < 4)
                usage(-1);
            cmd = cmd_write;
            break;
        }

        if (!strcmp(argv[1], "dump")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_dump;
            break;
        }

        if (!strcmp(argv[1], "disable")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_disable;
            break;
        }

        fprintf(stderr, "[err] not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    int ret = 0, i;
    int fd = can_open(argv[2]);
    if (fd < 0)
        return -1;

    if (cmd == cmd_set_rate) {
        int rate = 500000;
        for (i = 3; i < argc; i++) {
            if (parse_uint(argv[i], "", &rate, 10))
                continue;
            error_arg(argv[i]);
        }
        ret = can_set_rate(fd, rate);
        goto close_fd;
    }

    if (cmd == cmd_set_filter) {
        int num, id = CAN_ALL_ACCEPT_AFID;
        char tmp[10] = {0};
        for (i = 3; i < argc; i++) {
            if (parse_uint(argv[i], "num=", &num, 10))
                continue;
            if (parse_ustr(argv[i], "id=", tmp, 10)) {
                id = str_to_hex_id(tmp, 8);
                continue;
            }
            error_arg(argv[i]);
        }
        ret = can_set_filter(fd, num, id);
        goto close_fd;
    }

    if (cmd == cmd_put_filter) {
        int num;
        if (!parse_uint(argv[3], "num=", &num, 10))
            error_arg(argv[3]);
        ret = can_put_filter(fd, num);
        goto close_fd;
    }

    if (cmd == cmd_get_filter) {
        ret = can_get_filter(fd);
        goto close_fd;
    }

    if (cmd == cmd_enable) {
        ret = can_enable(fd);
        goto close_fd;
    }

    if (cmd == cmd_write) {
        unsigned char tmp[10];
        struct can_frame_cfg cfg;
        cfg.mode = CAN_STANDARD;
        cfg.type = CAN_REGULAR_DATA;
        for (i = 4; i < argc; i++) {
            if (parse_ustr(argv[i], "mode=", tmp, 10)) {
                if (strcmp(tmp, "standard") == 0)
                    cfg.mode = CAN_STANDARD;
                else if (strcmp(tmp, "extended") == 0)
                    cfg.mode = CAN_EXTENDED;
                else {
                    fprintf(stderr, "[err] mode should be standard/extended\n");
                    goto close_fd;
                }
                continue;
            }
            if (parse_ustr(argv[i], "type=", tmp, 10)) {
                if (strcmp(tmp, "data") == 0)
                    cfg.type = CAN_REGULAR_DATA;
                else if (strcmp(tmp, "request") == 0)
                    cfg.type = CAN_REMOTE_REQUEST;
                else {
                    fprintf(stderr, "[err] type should be data/request\n");
                    goto close_fd;
                }
                continue;
            }
            error_arg(argv[i]);
        }

        cfg.len = parse_id_data(argv[3], &cfg.frm_id, cfg.data, 16);
        if (cfg.len <= 0 && cfg.type == CAN_REGULAR_DATA) {
            fprintf(stderr, "[err] failed to get data\n");
            goto close_fd;
        }
        ret = can_write(fd, &cfg);
        goto close_fd;
    }

    if (cmd == cmd_dump) {
        ret = can_dump(fd);
        goto close_fd;
    }

    if (cmd == cmd_disable) {
        ret = can_disable(fd);
        goto close_fd;
    }

close_fd:
    can_close(fd);
    return ret;
}
