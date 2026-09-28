#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <libhardware2/mcu.h>

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);

    printf("Usage1:%s write_firmware <file>\n", prg_name);
    printf("Example:\n");
    printf("\t%s write_firmware /usr/data/libmcu-bare.bin\n", prg_name);
    printf("Usage2:%s bootup\n", prg_name);
    printf("Example:\n");
    printf("\t%s bootup\n", prg_name);
    printf("Usage3:%s write_str <str>\n", prg_name);
    printf("Example:\n");
    printf("\t%s write_str test123456\n", prg_name);
    printf("Usage4:%s write_data <data...>\n", prg_name);
    printf("Example:\n");
    printf("\t%s write_data 0x01 0x02 0x03 0x04\n", prg_name);
    printf("Usage5:%s write_mem <offset> <data...>\n", prg_name);
    printf("Example:\n");
    printf("\t%s write_mem 0x400 0x01 0x02 0x03\n", prg_name);
    printf("Usage6:%s read_str <size>\n", prg_name);
    printf("Example:\n");
    printf("\t%s read_str 128\n", prg_name);
    printf("Usage7:%s read_data <size>\n", prg_name);
    printf("Example:\n");
    printf("\t%s read_data 11\n", prg_name);
    printf("Usage8:%s shutdown\n", prg_name);
    printf("Example:\n");
    printf("\t%s shutdown\n", prg_name);
    printf("Usage9:%s reset\n", prg_name);
    printf("Example:\n");
    printf("\t%s reset\n", prg_name);

    exit(status);
}

enum {
    cmd_shutdown,
    cmd_reset,
    cmd_bootup,
    cmd_write_firmware,
    cmd_write_mem,
    cmd_write_data,
    cmd_write_str,
    cmd_read_data,
    cmd_read_str,
};

static int convert_to_uint(char *string, void *value, int type)
{
    char *endptr;
    unsigned int val;
    int ret = 0;

    val = strtoul(string, &endptr, 0);

    if (errno) {
        fprintf(stderr, "strtol error %d\n", -errno);
        return -errno;
    }

    if (endptr == string) {
        fprintf(stderr, "No digits were found: %s\n", string);
        return -EINVAL;
    }

    if (*endptr != '\0' ) {
        fprintf(stderr, "Not a complete digit num: %s\n", string);
        return -EINVAL;
    }

    switch (type) {
    case 8:
        if (val >=0 && val <= 0xFF) {
            *(uint8_t *)value = val;
        } else {
            ret = -EINVAL;
            fprintf(stderr, "%s out of rang[0 ~ 0xFF]\n", string);
        }
        break;

    case 16:
        if (val >=0 && val <= 0xFFFF) {
            *(uint16_t *)value = val;
        } else {
            ret = -EINVAL;
            fprintf(stderr, "%s out of rang[0 ~ 0xFFFF]\n", string);
        }
        break;

    case 32:
    default:
        *(uint32_t *)value = val;
        break;
    }

    return ret;
}

int main(int argc, char *argv[])
{
    int cmd = -1;
    prg_name = argv[0];

    while (1) {
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "shutdown")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_shutdown;
            break;
        }

        if (!strcmp(argv[1], "reset")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_reset;
            break;
        }

        if (!strcmp(argv[1], "bootup")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_bootup;
            break;
        }

        if (!strcmp(argv[1], "write_firmware")) {
            if (argc != 3)
                usage(-1);
            cmd = cmd_write_firmware;
            break;
        }

        if (!strcmp(argv[1], "write_mem")) {
            if (argc < 4)
                usage(-1);
            cmd = cmd_write_mem;
            break;
        }

        if (!strcmp(argv[1], "write_data")) {
            if (argc < 3)
                usage(-1);
            cmd = cmd_write_data;
            break;
        }

        if (!strcmp(argv[1], "write_str")) {
            if (argc < 3)
                usage(-1);
            cmd = cmd_write_str;
            break;
        }

        if (!strcmp(argv[1], "read_data")) {
            if (argc < 3)
                usage(-1);
            cmd = cmd_read_data;
            break;
        }
        if (!strcmp(argv[1], "read_str")) {
            if (argc < 3)
                usage(-1);
            cmd = cmd_read_str;
            break;
        }

        fprintf(stderr, "error: not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    int ret = 0;
    int mcu_fd = mcu_open();
    if (mcu_fd < 0)
        return -1;

    if (cmd == cmd_shutdown) {
        ret = mcu_shutdown(mcu_fd);
        goto out;
    }

    if (cmd == cmd_reset) {
        ret = mcu_reset(mcu_fd);
        goto out;
    }

    if (cmd == cmd_bootup) {
        ret = mcu_bootup(mcu_fd);
        goto out;
    }

    if (cmd == cmd_write_firmware) {
        if (!strcmp(argv[2], "-"))
            ret = mcu_write_firmware2(mcu_fd, stdin);
        else
            ret = mcu_write_firmware(mcu_fd, argv[2]);
        goto out;
    }

    if (cmd == cmd_write_mem) {
        unsigned int offset;
        ret = convert_to_uint(argv[2], &offset, 32);
        if (ret)
            goto out;

        unsigned char buf[argc - 3];
        int i;
        for (i = 0; i < argc - 3; i++) {
            ret = convert_to_uint(argv[i + 3], &buf[i], 8);
            if (ret)
                goto out;
        }

        ret = mcu_write_mem(mcu_fd, offset, buf, argc - 3);
        goto out;
    }

    if (cmd == cmd_write_data) {
        int len = argc - 2;
        unsigned char buf[len];
        int i;
        for (i = 0; i < len; i++) {
            ret = convert_to_uint(argv[i + 2], &buf[i], 8);
            if (ret)
                goto out;
        }

        ret = mcu_write_data_timeout(mcu_fd, buf, len, 300 * 1000);
        if (ret != len)
            fprintf(stderr, "mcu write error, %d have been written\n", ret);

        goto out;
    }

    if (cmd == cmd_read_str) {

        unsigned char *buf;
        int size;
        ret = convert_to_uint(argv[2], &size, 32);
        if (ret)
            goto out;

        buf = malloc(size);

        ret = mcu_read_str_timeout(mcu_fd, buf, size, 300 * 1000);
        if (ret < 0) {
            fprintf(stderr, "mcu read str err\n");
            free(buf);
            goto out;
        }

        printf("%s\n", buf);

        free(buf);
        goto out;
    }

    if (cmd == cmd_read_data) {
        unsigned char *buf;
        int i;
        int size;

        ret = convert_to_uint(argv[2], &size, 32);
        if (ret)
            goto out;

        buf = malloc(size);

        ret = mcu_read_data_timeout(mcu_fd, buf, size, 300 * 1000);
        if (ret != size) {
            fprintf(stderr, "mcu read error, have read %d\n", ret);
            goto read_err;
        }

        for(i = 0;i < size; i++)
            printf("%02x ", buf[i]);

        printf("\n");

read_err:
        free(buf);
        goto out;
    }

    if (cmd == cmd_write_str) {
        int i;
        for (i = 2; i < argc; i++) {
            int len = strlen(argv[i]) + 1;
            ret = mcu_write_data_timeout(mcu_fd, argv[i], len, 300 * 1000);
            if (ret != len) {
                fprintf(stderr, "mcu write str error, have read %d\n", ret);
                goto out;
            }
        }

        goto out;
    }

out:
    mcu_close(mcu_fd);
    return ret;
}
