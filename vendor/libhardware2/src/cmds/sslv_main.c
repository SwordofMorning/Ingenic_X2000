#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <linux/input.h>
#include <libhardware2/sslv.h>

static char *command;
static void usage(int status)
{
    printf("Usage1:%s info dev_path\n", command);
    printf("  Example:\n");
    printf("    %s info /dev/sslv0\n", command);
    printf("Usage2:%s set dev_path args...\n", command);
    printf("  Example:\n");
    printf("    %s set /dev/sslv0 mode=0x3 bits=8\n", command);
    printf("Usage3:%s enable dev_path\n", command);
    printf("  Example:\n");
    printf("    %s enable /dev/sslv0\n", command);
    printf("Usage4:%s receive dev_path num\n", command);
    printf("  Example:\n");
    printf("    %s receive /dev/sslv0 5\n", command);
    printf("Usage5:%s send dev_path data...\n", command);
    printf("  Example:\n");
    printf("    %s send /dev/sslv0 0x01 0x02 0x03\n", command);
    printf("Usage6:%s disable dev_path\n", command);
    printf("  Example:\n");
    printf("    %s disable /dev/sslv0\n", command);

    exit(status);
}

enum {
    cmd_enable,
    cmd_disable,
    cmd_send,
    cmd_receive,
    cmd_get_info,
    cmd_set_config,
};

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str + len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

int main(int argc, char **argv)
{
    int i;
    int cmd = -1;
    int fd = -1;
    int ret = 0;
    command = argv[0];

    int len;
    unsigned char *rx_buf = NULL;
    unsigned char *tx_buf = NULL;

    if (argc < 3)
        usage(-1);

    if (!strcmp(argv[1], "set")) {
        if (argc < 3)
            usage(-1);

        cmd = cmd_set_config;
        goto start;
    }

    if (!strcmp(argv[1], "info")) {
        if (argc != 3)
            usage(-1);

        cmd = cmd_get_info;
        goto start;
    }

    if (!strcmp(argv[1], "enable")) {
        if (argc != 3)
            usage(-1);

        cmd = cmd_enable;
        goto start;
    }

    if (!strcmp(argv[1], "disable")) {
        if (argc != 3)
            usage(-1);

        cmd = cmd_disable;
        goto start;
    }

    if (!strcmp(argv[1], "receive")) {
        if (argc != 4)
            usage(-1);

        ret = sscanf(argv[3], "%d", &len);
        if (ret != 1 || len < 0)
            usage(-1);

        if (len > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", len);

        cmd = cmd_receive;
        goto start;
    }

    if (!strcmp(argv[1], "send")) {
        if (argc < 4)
            usage(-1);

        if (argc - 3 > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", argc - 3);

        cmd = cmd_send;
        goto start;
    }

    if (cmd < 0) {
        fprintf(stderr, "no support %s cmd\n", argv[1]);
        usage(-1);
    }

start:
    fd = sslv_open(argv[2]);
    if (fd < 0)
        usage(-1);

    if (cmd == cmd_enable) {
        ret = sslv_enable(fd);
        goto end;
    }

    if (cmd == cmd_disable) {
        ret = sslv_disable(fd);
        goto end;
    }

    if (cmd == cmd_get_info) {
        struct sslv_config_data info;

        sslv_get_info(fd, &info);
        printf("id: %d, mode: %d, bits: %d.\n", info.id,
                    (info.sslv_pha | (info.sslv_pol << 1)),
                    info.bits_per_word);

        goto end;
    }

    if (cmd == cmd_set_config) {
        int mode;
        int bits;

        for (i = 3; i < argc; i++) {
            if (parse_uint(argv[i], "mode=", &mode, 16)) {
                ret = sslv_set_mode(fd, mode);
                if (ret < 0)
                    goto end;
                continue;
            }

            if (parse_uint(argv[i], "bits=", &bits, 10)) {
                ret = sslv_set_bits(fd, bits);
                if (ret < 0)
                    goto end;
                continue;
            }
            fprintf(stderr, "error: not support this arg: %s\n", argv[i]);
            ret = -1;
            goto end;
        }
    }

    if (cmd == cmd_send) {
        tx_buf = malloc(argc - 3);
        if (tx_buf == NULL) {
            fprintf(stderr, "malloc error\n");
            goto close_fd;
        }

        for (i = 0; i < argc - 3; i++) {
            int temp;
            ret = sscanf(argv[i + 3], "%x", &temp);
            if (ret != 1 || temp > 0xff) {
                fprintf(stderr, "data format error\n");
                goto close_fd;
            }
            tx_buf[i] = temp;
        }

        ret = sslv_send(fd, tx_buf, argc - 3, 1);
        if (ret < 0)
            goto close_fd;

        fprintf(stderr, "send %d bytes succeed!\n", ret);
    }

    if (cmd == cmd_receive) {
        rx_buf = malloc(len);
        if (rx_buf == NULL) {
            fprintf(stderr, "malloc error\n");
            goto close_fd;
        }

        ret = sslv_receive(fd, rx_buf, len);
        if (ret < 0)
            goto close_fd;

        fprintf(stderr, "receive %d bytes succeed!\n", ret);
        for (i = 0; i < ret; i++)
            printf("receive[%d]: %x\n", i, rx_buf[i]);
    }

close_fd:
    if (rx_buf)
        free(rx_buf);
    if (tx_buf)
        free(tx_buf);
end:
    sslv_close(fd);
    return ret;
}