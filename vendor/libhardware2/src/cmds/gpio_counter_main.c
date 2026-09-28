#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <libhardware2/gpio_counter.h>

static const char *command;


static void usage(int status)
{
    printf("Usage1: \t%s config  <channel_id> <mode_name> [dev_path]\n", command);
    printf("Example:\t%s config 0 pos_gpio0_up_count\n", command);
    printf("Example:\t%s config 0 pos_gpio0_up_count /dev/jz_tcu1\n\n", command);

    printf("Usage2: \t%s enable <channel_id> [dev_path]\n", command);
    printf("Example:\t%s enable 0\n", command);
    printf("Example:\t%s enable 0 /dev/jz_tcu1\n\n", command);

    printf("Usage3: \t%s get_count <channel_id> [dev_path]\n", command);
    printf("Example:\t%s get_count 0\n", command);
    printf("Example:\t%s get_count 0 /dev/jz_tcu1\n\n", command);

    printf("Usage4: \t%s get_capture <channel_id> [dev_path]\n", command);
    printf("Example:\t%s get_capture 0\n", command);
    printf("Example:\t%s get_capture 0 /dev/jz_tcu1\n\n", command);

    printf("Usage5: \t%s disable <channel_id> [dev_path]\n", command);
    printf("Example:\t%s disable 0\n", command);
    printf("Example:\t%s disable 0 /dev/jz_tcu1\n\n", command);

    printf("Usage6: \t%s print_support_mode\n", command);
    printf("Example:\t%s print_support_mode\n", command);

    exit(status);
}

enum {
    cmd_config,
    cmd_enable,
    cmd_disable,
    cmd_suspend,
    cmd_recovery,
    cmd_get_count,
    cmd_get_capture,
    cmd_print_support_mode,
};

int main(int argc, char **argv)
{
    int cmd = -1;
    int ret;
    int count = 0;
    int high_level_time = 0;
    int period_time = 0;
    unsigned int channel_id;
    command = argv[0];
    const char *dev_path = argv[3];

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    if ((strcmp(argv[1], "config") == 0)) {

        if (argc < 4)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel_id);
        if (ret != 1)
            usage(-1);

        cmd = cmd_config;
        dev_path = argv[4];
    }

    if ((strcmp(argv[1], "enable") == 0)) {

        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel_id);
        if (ret != 1)
            usage(-1);

        cmd = cmd_enable;
    }

    if ((strcmp(argv[1], "disable") == 0)) {
        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel_id);
        if (ret != 1)
            usage(-1);

        cmd = cmd_disable;
    }

    if ((strcmp(argv[1], "get_count") == 0)) {
        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel_id);
        if (ret != 1)
            usage(-1);

        cmd = cmd_get_count;
    }

    if ((strcmp(argv[1], "get_capture") == 0)) {
        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel_id);
        if (ret != 1)
            usage(-1);

        cmd = cmd_get_capture;
    }

    if ((strcmp(argv[1], "print_support_mode") == 0)) {
        if (argc < 2)
            usage(-1);

        cmd = cmd_print_support_mode;
    }

    if (cmd == -1)
        usage(-1);

    int fd;
    if (!dev_path)
        fd = gpio_counter_open();
    else
        fd = gpio_counter_open_by_path(dev_path);

    if (fd == -1)
        return -1;

    if (cmd == cmd_config) {
        ret = gpio_counter_config(fd, channel_id, argv[3]);
        goto close_fd;
    }

    if (cmd == cmd_enable) {
        ret = gpio_counter_enable(fd, channel_id);
        goto close_fd;
    }

    if (cmd == cmd_disable) {
        ret = gpio_counter_disable(fd, channel_id);
        goto close_fd;
    }

    if (cmd == cmd_get_count) {
        count = gpio_counter_get_count(fd, channel_id);
        printf("channel:%d  count=%d\n", channel_id, count);
        goto close_fd;
    }

    if (cmd == cmd_get_capture) {
        count = gpio_counter_get_capture(fd, channel_id, &high_level_time, &period_time);
        printf("channel:%d  high_level_time=%d period_time = %d \n", channel_id, high_level_time, period_time);
        goto close_fd;
    }

    if (cmd == cmd_print_support_mode) {
        char **info;
        int num, i;

        info = gpio_counter_get_mode_information(fd, &num);
        if(info == NULL) {
            printf("Get mode information failed!\n");
            return -1;
        }

        printf("    mode name\n");
        for (i = 0; i < num; i++)
            printf("    %s\n", info[i]);

        gpio_counter_free_mode_information(info);
        goto close_fd;
    }

close_fd:
    gpio_counter_close(fd);
    return ret;
}