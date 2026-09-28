#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <libhardware2/spi.h>

char *command;

static void usage(int status)
{
    printf("Usage1:%s add_dev <busnum> <cs_gpio>\n", command);
    printf("  Example:\n");
    printf("    %s add_dev 0 pc16\n", command);
    printf("Usage2:%s info dev_path\n", command);
    printf("  Example:\n");
    printf("    %s info /dev/spidev0.0\n", command);
    printf("Usage3:%s set dev_path args... \n", command);
    printf("  Example:\n");
    printf("    %s set /dev/spidev0.0 mode=0x03 speed=500000 bits=8 lsb=0\n", command);
    printf("Usage4:%s transfer dev_path data...\n", command);
    printf("  Example:\n");
    printf("    %s transfer /dev/spidev0.0 0x00 0x01\n", command);
    printf("Usage5:%s read dev_path data...\n", command);
    printf("  Example:\n");
    printf("    %s read /dev/spidev0.0 5\n", command);
    printf("Usage6:%s write data...\n", command);
    printf("  Example:\n");
    printf("    %s write /dev/spidev0.0 0x01 0x02 0x03\n", command);
    printf("Usage7:%s del_dev dev_path\n", command);
    printf("  Example:\n");
    printf("    %s del_dev /dev/spidev0.0\n", command);

    exit(status);
}
enum {
    cmd_get_info,
    cmd_set_config,
    cmd_transfer,
    cmd_add_dev,
    cmd_del_dev,
    cmd_spi_read,
    cmd_spi_write,
};

static int data_process(char *data)
{
    int ret;
    int temp;

    ret = sscanf(data, "%x", &temp);
    if (ret != 1)
        return -1;


    return temp;
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

int main(int argc, char **argv)
{
    int cmd = -1;
    int fd = -1;
    int busnum;
    int ret = 0;
    int i;
    command = argv[0];

    int len;
    char *rx_buf = NULL;
    char *tx_buf = NULL;
    struct spi_ioc_transfer spi_msg;

    if (argc < 3)
        usage(-1);

    if (!strcmp(argv[1], "info")) {
        if (argc != 3)
            usage(-1);

        cmd = cmd_get_info;
    }

    if (!strcmp(argv[1], "set")) {
        if (argc < 4)
            usage(-1);

        cmd = cmd_set_config;
    }

    if (!strcmp(argv[1], "transfer")) {
        if (argc < 4)
            usage(-1);

        if (argc - 3 > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", argc - 3);

        cmd = cmd_transfer;
    }

    if (!strcmp(argv[1], "read")) {
        if (argc != 4)
            usage(-1);

        ret = sscanf(argv[3], "%d", &len);
        if (ret != 1 || len < 0)
            usage(-1);

        if (len > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", len);

        cmd = cmd_spi_read;
    }

    if (!strcmp(argv[1], "write")) {
        if (argc < 4)
            usage(-1);

        if (argc - 3 > 64)
            fprintf(stderr, "data_len: %d must less than 64.\n", argc - 3);

        cmd = cmd_spi_write;
    }

    if (!strcmp(argv[1], "add_dev")) {
        if (argc != 4)
            usage(-1);

        cmd = cmd_add_dev;
    }

    if (!strcmp(argv[1], "del_dev")) {
        if (argc != 3)
            usage(-1);

        cmd = cmd_del_dev;
    }

    if (cmd < 0) {
        fprintf(stderr, "no support %s cmd\n", argv[1]);
        usage(-1);
    }

    if (cmd != cmd_add_dev && cmd != cmd_del_dev) {
        fd = spi_open(argv[2]);
        if (fd < 0)
            usage(-1);
    }

    if (cmd == cmd_get_info) {
        struct spi_info spi;

        spi_get_info(fd, &spi);

        printf("mode speed bits lsb\n");
        printf("%02x %d %d %d\n", spi.spi_mode, spi.spi_speed, spi.spi_bits, spi.spi_lsb);
    }

    if (cmd == cmd_set_config) {
        int mode;
        int speed;
        int bits;
        int lsb;

        for (i = 3; i < argc; i++) {
            if (parse_uint(argv[i], "mode=", &mode, 16)) {
                ret = spi_set_mode(fd, mode);
                if (ret < 0)
                    goto close_fd;
                continue;
            }

            if (parse_uint(argv[i], "speed=", &speed, 10)) {
                ret = spi_set_speed(fd, speed);
                if (ret < 0)
                    goto close_fd;
                continue;
            }

            if (parse_uint(argv[i], "bits=", &bits, 10)) {
                ret = spi_set_bits(fd, bits);
                if (ret < 0)
                    goto close_fd;
                continue;
            }

            if (parse_uint(argv[i], "lsb=", &lsb, 10)) {
                ret = spi_set_lsb(fd, lsb);
                if(ret < 0)
                    goto close_fd;
                continue;
            }
            fprintf(stderr, "error: not support this arg: %s\n", argv[i]);
            ret = -1;
            goto close_fd;
        }
    }

    if (cmd == cmd_transfer || cmd == cmd_spi_write) {
        tx_buf = malloc(argc - 3);
        if (tx_buf == NULL) {
            fprintf(stderr, "malloc error\n");
            goto close_fd;
        }

        for (i = 0; i < argc - 3; i++) {
            ret = data_process(argv[i + 3]);
            if(ret < 0 || ret > 0xff) {
                fprintf(stderr, "data format error\n");
                goto close_fd;
            }
            tx_buf[i] = ret;
        }

        if (cmd == cmd_transfer) {
            rx_buf = malloc(argc - 3);
            if (rx_buf == NULL) {
                fprintf(stderr, "malloc error\n");
                goto close_fd;
            }

            memset(&spi_msg, 0, sizeof(spi_msg));
            spi_msg.tx_buf = (unsigned long long)tx_buf;
            spi_msg.rx_buf = (unsigned long long)rx_buf;
            spi_msg.len = argc - 3;
            spi_msg.bits_per_word = 0;
            spi_msg.speed_hz = 0;

            ret = spi_transfer(fd, &spi_msg, 1);
            if(ret < 0)
                goto close_fd;

            for (i = 0; i < argc - 3; i++)
                printf("receive[%d]: %02x\n", i, rx_buf[i]);
        }

        if (cmd == cmd_spi_write) {
            ret = spi_write(fd, tx_buf, argc - 3);
            if (ret < 0)
                goto close_fd;
        }
    }

    if (cmd == cmd_spi_read) {
        rx_buf = malloc(len);
        if (rx_buf == NULL) {
            fprintf(stderr, "malloc error\n");
            goto close_fd;
        }

        ret = spi_read(fd, rx_buf, len);
        if (ret < 0)
            goto close_fd;

        for (i = 0; i < len; i++)
            printf("receive[%d]: %02x\n", i, rx_buf[i]);
    }

    if (cmd == cmd_add_dev) {
        struct spidev_register_data data;

        ret = sscanf(argv[2], "%d", &busnum);
        if (ret < 0)
            usage(-1);

        data.busnum = busnum;
        data.cs_gpio = argv[3];

        ret = spi_add_device(&data);
        if (ret < 0)
            return ret;

        printf("%s\n", data.spidev_path);

        return 0;
    }

    if (cmd == cmd_del_dev) {

        ret = spi_del_device(argv[2]);
        if (ret < 0)
            return ret;

        return 0;
    }

close_fd:
    if (rx_buf)
        free(rx_buf);
    if (tx_buf)
        free(tx_buf);

    spi_close(fd);
    return ret;

}