#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/pwm_battery.h>

static char *command;
static void usage(int status)
{
    printf("Usage1: \t%s <operation>\n", command);
    printf("Example1:\n");
    printf("\t%s get_voltage\n", command);

    exit(status);
}

int main(int argc, char *argv[])
{
    int fd;
    int voltage;
    command = argv[0];

    if (argc != 2)
        usage(-1);

    if(strcmp(argv[1], "get_voltage"))
        usage(-1);

    fd = pwm_battery_open();
    if (fd < 0)
        return -1;

    voltage = pwm_battery_read(fd);

    printf("battery_voltage = %d\n", voltage);

    pwm_battery_close(fd);
}