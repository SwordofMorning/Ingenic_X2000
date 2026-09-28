#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <libhardware2/gpio_counter.h>

#define GPIO_COUNTER_GET_MODE_NUM                       _IOWR('t', 194, unsigned long *)
#define GPIO_COUNTER_GET_MODE_NAME                      _IOWR('t', 195, unsigned long *)
#define GPIO_COUNTER_CONFIG                             _IOWR('t', 196, unsigned long *)
#define GPIO_COUNTER_ENABLE                             _IOWR('t', 197, unsigned int)
#define GPIO_COUNTER_DISABLE                            _IOWR('t', 198, unsigned int)
#define GPIO_COUNTER_GET_COUNT                          _IOWR('t', 199, unsigned long *)
#define GPIO_COUNTER_GET_CAPTURE                        _IOWR('t', 200, unsigned long *)

int gpio_counter_open(void)
{
    int fd = open("/dev/jz_tcu", O_RDWR);
    if (fd < 0)
        fprintf(stderr, "gpio_counter:open /dev/jz_tcu failed: %s\n", strerror(errno));

    return fd;
}

int gpio_counter_open_by_path(const char *dev_path)
{
    int fd = open(dev_path, O_RDWR);
    if (fd < 0)
        fprintf(stderr, "gpio_counter:open %s failed: %s\n", dev_path, strerror(errno));

    return fd;
}

int gpio_counter_close(int fd)
{
    return close(fd);
}

int gpio_counter_config(int fd, int channel_id, char *mode_name)
{
    int ret;
    assert(mode_name);

    unsigned long argv[] = {
        channel_id,  (unsigned long)mode_name
    };

    ret = ioctl(fd, GPIO_COUNTER_CONFIG, argv);
    if (ret < 0)
        fprintf(stderr, "gpio_counter:gpio_counter config failed %s!\n", strerror(errno));

    return ret;
}

int gpio_counter_enable(int fd, unsigned int channel_id)
{
    int ret;

    ret = ioctl(fd, GPIO_COUNTER_ENABLE, channel_id);
    if (ret < 0)
        fprintf(stderr, "gpio_counter:gpio_counter enable failed %s!\n", strerror(errno));

    return ret;
}

int gpio_counter_disable(int fd, unsigned int channel_id)
{
    int ret;

    ret = ioctl(fd, GPIO_COUNTER_DISABLE, channel_id);
    if (ret < 0)
        fprintf(stderr, "gpio_counter:gpio_counter disable failed %s!\n", strerror(errno));

    return ret;
}

int gpio_counter_get_count(int fd, int channel_id)
{
    int ret;
    int count = 0;

    unsigned long argv[] = {
        channel_id, (unsigned long)&count
    };

    ret = ioctl(fd, GPIO_COUNTER_GET_COUNT, argv);
    if (ret < 0) {
        fprintf(stderr, "gpio_counter:gpio_counter get channel count failed %s!\n", strerror(errno));
        return 0;
    }

    return count;
}

int gpio_counter_get_capture(int fd, int channel_id, int *high_level_time, int *period_time)
{
    int ret;

    unsigned long argv[] = {
        channel_id, (unsigned long)high_level_time, (unsigned long)period_time
    };

    ret = ioctl(fd, GPIO_COUNTER_GET_CAPTURE, argv);
    if (ret < 0) {
        fprintf(stderr, "gpio_counter:gpio_counter get channel count failed %s!\n", strerror(errno));
        return ret;
    }

    return 0;
}

char **gpio_counter_get_mode_information(int fd, int *num)
{
    int ret, i;
    int mode_num = 0;
    int size = 0;

    unsigned long argv[] = {
        (unsigned long)&mode_num,  (unsigned long)&size
    };

    ret = ioctl(fd, GPIO_COUNTER_GET_MODE_NUM, argv);
    *num = mode_num;

    char *info = malloc((mode_num + 1) * sizeof(char *) + mode_num * size);
    char **array = (char **)(info + mode_num * size);

    ret = ioctl(fd, GPIO_COUNTER_GET_MODE_NAME, info);
    if (ret < 0) {
        fprintf(stderr, "gpio_counter:get mode information failed!! %s\n", strerror(errno));
        goto out;
    }

    for (i = 0; i < mode_num; i++)
        array[i] = info + i * size;

    close(fd);
    return array;

out:
    close(fd);
    if(info)
        free(info);

    return NULL;
}

void gpio_counter_free_mode_information(char **array)
{
    if (array[0])
        free(array[0]);

    return;
}