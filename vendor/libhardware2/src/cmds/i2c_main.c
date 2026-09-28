/*
 *  Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 *  Ingenic library hardware version2 Test Command
 *
 */
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <libhardware2/i2c.h>

typedef int(*cmd_func_t)(char *command, int argc, char **argv);

enum {
    FORMAT_UINT_8,
    FORMAT_UINT_16,
    FORMAT_UINT_32,
};

struct operation {
    char *string_key;
    cmd_func_t oprtation_func;

};

static void cmd_usage(char *command)
{
    printf("Usage:\n");

    printf("\t%s <operation>    <bus num> [parameters ...]\n", command);
    printf("\t%s detect         <bus num>\n", command);
    printf("\t%s <read>         <bus num> <dev_addr> <size>\n", command);
    printf("\t%s <write>        <bus num> <dev_addr> <data0> [data1] ...\n", command);
    printf("\t%s <read_reg>     <bus num> <dev_addr> <reg_addr> <size>\n", command);
    printf("\t%s <write_reg>    <bus num> <dev_addr> <reg_addr> <data0> [data1] ...\n", command);
    printf("\t%s <read_reg_16>  <bus num> <dev_addr> <reg_addr_16> <size>\n", command);
    printf("\t%s <write_reg_16> <bus num> <dev_addr> <reg_addr_16> <data0> [data1] ...\n", command);

    printf("Example:\n");
    printf("\t%s --help\n", command);
    printf("\t%s -h\n",     command);
    printf("\t%s detect       2\n", command);
    printf("\t%s read         2 0x58 8\n", command);
    printf("\t%s write        2 0x58 0xaa 0xbb\n", command);
    printf("\t%s read_reg     2 0x58 0x00 8\n", command);
    printf("\t%s write_reg    2 0x58 0x00 0xaa 0xbb\n", command);
    printf("\t%s read_reg_16  0 0x10 0x3010 2\n", command);
    printf("\t%s write_reg_16 0 0x10 0x3010 0x55 0xaa\n", command);
}

static void dump_buffer_content(unsigned char *buffer, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        if ( (i != 0) && (i % 16 == 0)) {
            printf("\n");
        }
        printf("%02x:", buffer[i]);
    }
    printf("\n");
}


static int format_converts(char *string, void *value, int type)
{
    char *endptr;
    int val;
    int ret = 0;

    val = strtol(string, &endptr, 0);

    if (errno) {
        fprintf(stderr, "strtol error %d\n", -errno);
        return -errno;
    }

    if (endptr == string) {
        fprintf(stderr, "No digits were found\n");
        return -EINVAL;
    }

    switch (type) {
    case FORMAT_UINT_8:
        if (val >=0 && val <= 0xFF) {
            *(uint8_t *)value = val;
        } else {
            ret = -EINVAL;
            fprintf(stderr, "0x%x out of rang[0 ~ 0xFF]\n", val);
        }
        break;

    case FORMAT_UINT_16:
        if (val >=0 && val <= 0xFFFF) {
            *(uint16_t *)value = val;
        } else {
            ret = -EINVAL;
            fprintf(stderr, "0x%x out of rang[0 ~ 0xFFFF]\n", val);
        }
        break;

    case FORMAT_UINT_32:
    default:
        *(uint32_t *)value = val;
        break;
    }

    return ret;
}

static int cmd_func_help(char *command, int argc, char **argv)
{
    cmd_usage(command);

    return 0;
}

static int cmd_func_i2c_detect(char *command, int argc, char **argv)
{
    int ret = 0;
    uint8_t bus_num = 0;

    /*
     * 参数检查
     */
    if (argc < 1) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    int start_addr = 0x00;
    int end_addr = 0x80;
    int i, j;
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
        ret = fd;
        goto err_open_fd;
    }

    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (i = 0; i < 0x80; i += 16) {
        printf("%02x: ", i);
        for (j = 0; j < 16; j++) {
            fflush(stdout);

            /* skip unwanted address */
            if (i+j < start_addr || i+j > end_addr) {
                printf("   ");
                continue;
            }

            ret = i2c_detect(fd, i+j);
            if (ret == 1) {
                /* Address Busy */
                printf("UU ");
            } else if (ret < 0) {
                /* Address NACK  */
                printf("-- ");
            } else {
                /* Address ACK */
                printf("%02x ", i+j);
            }
        }
        printf("\n");
    }

    ret = 0;
    close(fd);

err_open_fd:
    return ret;
}

static int cmd_func_i2c_read(char *command, int argc, char **argv)
{
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint32_t data_len = 0;

    /*
     * 参数检查
     */
    if (argc < 3) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    ret = format_converts(argv[2], &data_len, FORMAT_UINT_32);
    if (ret < 0) {
        fprintf(stderr, "%s data length value(%s) format is unsupport\n", command, argv[2]);
        return -EINVAL;
    }

    /* buffer */
    uint8_t buffer[data_len];

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
        ret = fd;
        goto err_open_fd;
    }

    ret = i2c_read(fd, device_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c read reg failed\n", command);
        goto err_read_reg;
    }

    printf("Dump Recv Buffer:\n");
    dump_buffer_content(buffer, data_len);

err_read_reg:
    i2c_close(fd);
err_open_fd:
    return ret;
}

static int cmd_func_i2c_write(char *command, int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint8_t data_len = 0;

    /*
     * 参数检查
     */
    if (argc < 3) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    /* buffer */
    uint8_t buffer[data_len];

    for (i = 0; i < data_len; i++) {
        ret = format_converts(argv[2+i], &buffer[i], FORMAT_UINT_8);
        if (ret < 0) {
            fprintf(stderr, "%s data value(%s) format is unsupport\n", command, argv[2+i]);
            goto err_data_format;
        }
    }

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
        ret = fd;
        goto err_open_fd;
    }

    printf("Dump Send Buffer:\n");
    dump_buffer_content(buffer, data_len);

    ret = i2c_write(fd, device_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c write reg failed\n", command);
        goto err_write;
    }

err_write:
    i2c_close(fd);
err_open_fd:
err_data_format:
    return ret;
}

static int cmd_func_i2c_read_reg(char *command, int argc, char **argv)
{
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint32_t reg_addr;
    uint32_t data_len = 0;

    /*
     * 参数检查
     */
    if (argc < 4) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    ret = format_converts(argv[2], &reg_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s reg address value(%s) format is unsupport\n", command, argv[2]);
        return -EINVAL;
    }

    ret = format_converts(argv[3], &data_len, FORMAT_UINT_32);
    if (ret < 0) {
        fprintf(stderr, "%s data length value(%s) format is unsupport\n", command, argv[3]);
        return -EINVAL;
    }

    /* buffer */
    uint8_t buffer[data_len];

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
       ret = fd;
       goto err_open_fd;
    }

    ret = i2c_read_reg(fd, device_addr, reg_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c read reg failed\n", command);
        goto err_read_reg;
    }

    printf("Dump Recv Buffer:\n");
    dump_buffer_content(buffer, data_len);

err_read_reg:
    i2c_close(fd);
err_open_fd:
    return ret;
}

static int cmd_func_i2c_write_reg(char *command, int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint32_t reg_addr;

    /*
     * 参数检查
     */
    if (argc < 4) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    ret = format_converts(argv[2], &reg_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s reg address value(%s) format is unsupport\n", command, argv[2]);
        return -EINVAL;
    }

    /* buffer */
    int data_len = argc - 3;
    uint8_t buffer[data_len];

    for (i = 0; i < data_len; i++) {
        ret = format_converts(argv[3+i], &buffer[i], FORMAT_UINT_8);
        if (ret < 0) {
            printf("%s data value(%s) format is unsupport\n", command, argv[3+i]);
            goto err_data_format;
        }

    }

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
        ret =  fd;
        goto err_open_fd;
    }

    printf("Dump Send Buffer:\n");
    dump_buffer_content(buffer, data_len);
    ret = i2c_write_reg(fd, device_addr, reg_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c write reg failed\n", command);
        goto err_write_reg;
    }

err_write_reg:
    i2c_close(fd);
err_open_fd:
err_data_format:
    return ret;
}

static int cmd_func_i2c_read_reg_16(char *command, int argc, char **argv)
{
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint32_t reg_addr;
    uint32_t data_len = 0;

    /*
     * 参数检查
     */
    if (argc < 4) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    ret = format_converts(argv[2], &reg_addr, FORMAT_UINT_16);
    if (ret < 0) {
        fprintf(stderr, "%s reg address value(%s) format is unsupport\n", command, argv[2]);
        return -EINVAL;
    }

    ret = format_converts(argv[3], &data_len, FORMAT_UINT_32);
    if (ret < 0) {
        fprintf(stderr, "%s data length value(%s) format is unsupport\n", command, argv[3]);
        return -EINVAL;
    }

    /* buffer */
    uint8_t buffer[data_len];

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
       ret = fd;
       goto err_open_fd;
    }

    ret = i2c_read_reg_16(fd, device_addr, reg_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c read reg failed\n", command);
        goto err_read_reg;
    }

    printf("Dump Recv Buffer:\n");
    dump_buffer_content(buffer, data_len);

err_read_reg:
    i2c_close(fd);
err_open_fd:
    return ret;
}

static int cmd_func_i2c_write_reg_16(char *command, int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    uint8_t bus_num = 0;
    uint8_t device_addr;
    uint32_t reg_addr;

    /*
     * 参数检查
     */
    if (argc < 4) {
        fprintf(stderr, "%s too few arguments\n", command);
        return -EINVAL;
    }

    ret = format_converts(argv[0], &bus_num, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s bus_num(%s) format is unsupport\n", command, argv[0]);
        return -EINVAL;
    }

    ret = format_converts(argv[1], &device_addr, FORMAT_UINT_8);
    if (ret < 0) {
        fprintf(stderr, "%s device address value(%s) format is unsupport\n", command, argv[1]);
        return -EINVAL;
    }

    ret = format_converts(argv[2], &reg_addr, FORMAT_UINT_16);
    if (ret < 0) {
        fprintf(stderr, "%s reg address value(%s) format is unsupport\n", command, argv[2]);
        return -EINVAL;
    }

    /* buffer */
    int data_len = argc - 3;
    uint8_t buffer[data_len];

    for (i = 0; i < data_len; i++) {
        ret = format_converts(argv[3+i], &buffer[i], FORMAT_UINT_8);
        if (ret < 0) {
            printf("%s data value(%s) format is unsupport\n", command, argv[3+i]);
            goto err_data_format;
        }

    }

    /*
     * Handle
     */
    int fd = i2c_open(bus_num);
    if (fd < 0) {
        fprintf(stderr, "%s i2c open failed\n", command);
        ret =  fd;
        goto err_open_fd;
    }

    printf("Dump Send Buffer:\n");
    dump_buffer_content(buffer, data_len);
    ret = i2c_write_reg_16(fd, device_addr, reg_addr, buffer, data_len);
    if (ret < 0) {
        fprintf(stderr, "%s i2c write reg failed\n", command);
        goto err_write_reg;
    }

err_write_reg:
    i2c_close(fd);
err_open_fd:
err_data_format:
    return ret;
}

static struct operation operations [] = {
    {"--help",       cmd_func_help},
    {"-h",           cmd_func_help},
    {"detect",       cmd_func_i2c_detect},
    {"read",         cmd_func_i2c_read},
    {"write",        cmd_func_i2c_write},
    {"read_reg",     cmd_func_i2c_read_reg},
    {"write_reg",    cmd_func_i2c_write_reg},
    {"read_reg_16",  cmd_func_i2c_read_reg_16},
    {"write_reg_16", cmd_func_i2c_write_reg_16},
};


/*
 * Return >=0: found operation
 *         <0: not found operation
 */
static int cmd_operation_check(char *operate)
{
    int num = sizeof(operations) / sizeof(operations[0]);
    int i = 0;

    for (i=0; i < num; i++) {
        if (strcmp(operate, operations[i].string_key) == 0)
            return i;
    }

    return -EPERM;
}

int main(int argc, char **argv)
{
    int function;
    int ret;

    if (argc < 2) {
        cmd_usage(argv[0]);
        return -EPERM;
    }

    function = cmd_operation_check(argv[1]);
    if (function < 0) {
        fprintf(stderr, "%s: operation is not support\n", argv[1]);
        return function;
    }

    cmd_func_t operation_callback = operations[function].oprtation_func;

    ret = operation_callback(argv[0], argc - 2, &argv[2]);
    if (ret < 0) {
        fprintf(stderr, "%s %s failed\n", argv[0], argv[1]);
        return ret;
    }

    return 0;
}
