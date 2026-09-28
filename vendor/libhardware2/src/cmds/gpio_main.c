#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/gpio.h>

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                : show help info\n");
    fprintf(stderr, "    set_func gpio funcs...   : set gpio func\n");
    fprintf(stderr, "    get_func gpio            : get gpio func\n");
    fprintf(stderr, "    get_value gpio           : get gpio input value\n");
    fprintf(stderr, "    set_value gpio   1/0     : set gpio output value\n");
    fprintf(stderr, "    set_strength gpio value  : set gpio drive strength value\n");
    fprintf(stderr, "    get_strength gpio        : get gpio drive strength value\n");
    fprintf(stderr, "    set_schmitt gpio value   : set gpio schmitt trigger value\n");
    fprintf(stderr, "    get_schmitt gpio         : get gpio schmitt trigger value\n");
    fprintf(stderr, "    set_slew_rate gpio value : set gpio slew rate value\n");
    fprintf(stderr, "    get_slew_rate gpio       : get gpio slew rate value\n");
    fprintf(stderr, "about gpio:\n");
    fprintf(stderr, "    like the PB18, PA16,,,,\n");
    fprintf(stderr, "about func and attr:\n");

    int fd = gpio_open();
    if (fd < 0) {
        fprintf(stderr, "can't open device, no more help info\n");
        exit(-1);
    }

    char buf[2048];
    gpio_get_help(fd, buf, sizeof(buf));
    fprintf(stderr, "%s\n", buf);

    gpio_close(fd);

    exit(status);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    prg_name = argv[0];

    if (argc < 2)
        usage(-1);

    if (!strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1);

    int fd = gpio_open();
    if (fd < 0)
        exit(-1);

    if (!strcmp(argv[1], "set_func")) {
        if (argc < 4)
            usage(-1);
        ret = gpio_set_func(fd, argv[2], &argv[3], argc - 3);
        goto close_fd;
    }

    if (!strcmp(argv[1], "get_func")) {
        if (argc != 3)
            usage(-1);
        char buf[256];
        ret = gpio_get_func(fd, argv[2], buf, sizeof(buf));
        if (!ret)
            printf("%s\n", buf);
        goto close_fd;
    }

    if (!strcmp(argv[1], "set_strength")) {
        if (argc != 4)
            usage(-1);
        ret = gpio_set_strength(fd, argv[2], atoi(argv[3]));
        goto close_fd;
    }

    if (!strcmp(argv[1], "get_strength")) {
        if (argc != 3)
            usage(-1);

        ret = gpio_get_strength(fd, argv[2]);
        if (ret >= 0)
            printf("%d\n", ret);
        goto close_fd;
    }

    if (!strcmp(argv[1], "set_schmitt")) {
        if (argc != 4)
            usage(-1);
        ret = gpio_set_schmitt(fd, argv[2], atoi(argv[3]));
        goto close_fd;
    }

    if (!strcmp(argv[1], "get_schmitt")) {
        if (argc != 3)
            usage(-1);

        ret = gpio_get_schmitt(fd, argv[2]);
        if (ret >= 0)
            printf("%d\n", ret);
        goto close_fd;
    }

    if (!strcmp(argv[1], "set_slew_rate")) {
        if (argc != 4)
            usage(-1);
        ret = gpio_set_slew_rate(fd, argv[2], atoi(argv[3]));
        goto close_fd;
    }

    if (!strcmp(argv[1], "get_slew_rate")) {
        if (argc != 3)
            usage(-1);

        ret = gpio_get_slew_rate(fd, argv[2]);
        if (ret >= 0)
            printf("%d\n", ret);
        goto close_fd;
    }

    if (!strcmp(argv[1], "get_value")) {
        if (argc != 3)
            usage(-1);
        ret = gpio_get_value(fd, argv[2]);
        if (ret >= 0)
            printf("%d\n", ret);
        goto close_fd;
    }

    if (!strcmp(argv[1], "set_value")) {
        if (argc != 4)
            usage(-1);
        if (strcmp(argv[3], "0") && strcmp(argv[3], "1"))
            usage(-1);
        ret = gpio_set_value(fd, argv[2], *argv[3] == '1');
        goto close_fd;
    }

    ret = -1;
    fprintf(stderr, "gpio: not support this cmd: %s\n", argv[1]);

close_fd:
    gpio_close(fd);

    return 0;
}