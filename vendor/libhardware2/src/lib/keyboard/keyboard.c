#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <linux/input.h>
#include <dirent.h>
#include <math.h>
#include <assert.h>
#include <poll.h>
#include <time.h>

#include <libhardware2/keyboard.h>

#define INPUT_DEV_PATH  "/dev/input"

struct key_handle {
    int dev_num;
    int fds[];
};

uint64_t get_time_ms(struct timeval *time)
{
    gettimeofday(time, NULL);
    return (uint64_t)(time->tv_sec * 1000 + time->tv_usec / 1000);
}

int poll_key(struct pollfd *key, struct key_handle *handle, int timeout)
{
    int i, poll_ret;
    int cnt = handle->dev_num;

    poll_ret = poll(key, cnt, timeout);
    if (poll_ret == -1)
        return -1;
    else if (poll_ret == 0)
        return 0;

    for (i = 0; i < cnt; i++) {
        if (key[i].revents == POLLIN)
            return i + 1;
    }

    return 0;
}

int read_key_event(long handle, struct key_event *key_event, int timeout)
{
    int i, ret, buf_cnt;
    uint64_t before = 0;
    struct input_event event;
    struct timeval clock;
    struct key_handle *key = (struct key_handle *)handle;
    int cnt = key->dev_num;
    struct pollfd poll_fd[cnt];

    for (i = 0; i < cnt; i++) {
        poll_fd[i].fd = key->fds[i];
        poll_fd[i].events = POLLIN;
    }

    while (1) {
        before = get_time_ms(&clock);

        buf_cnt = poll_key(poll_fd, key, timeout);
        if (buf_cnt < 0) {
            fprintf(stderr, "keyboard: Poll error: %d!\n", buf_cnt);
            return buf_cnt;
        } else if (buf_cnt == 0)
            return buf_cnt;

        ret = read(key->fds[buf_cnt - 1], &event, sizeof(event));
        if (ret < 0) {
            fprintf(stderr, "%s: read key_event error, %s(%d)!\n",
                    __func__, strerror(errno), -errno);

            return ret;
        }

        if (event.type == EV_KEY) {
            key_event->key_type = event.code;
            key_event->is_press = event.value;
            return 1;
        }

        if (timeout != -1) {
            timeout = timeout - (get_time_ms(&clock) - before);
            if (timeout < 0)
                return 0;
        }
    }
}

static int is_key_device(const struct dirent *dir)
{
    return strncmp("event", dir->d_name, 5) == 0;
}

long keys_open(void)
{
    struct dirent **namelist;
    int i, ndev, fd;
    int cnt = 0;

    ndev = scandir(INPUT_DEV_PATH, &namelist, is_key_device, NULL);
    if (ndev < 0) {
        fprintf(stderr, "failed to find event deivce: %s\n", strerror(errno));
        return -1;
    } else if (ndev == 0) {
        fprintf(stderr, "could not find event deivce: %s\n", strerror(errno));
        return -1;
    }

    int fd_buf[ndev];

    for (i = 0; i < ndev; i++) {
        char fname[300];
        unsigned char mask[EV_MAX / 8 + 1];

        sprintf(fname, "%s/%s", INPUT_DEV_PATH, namelist[i]->d_name);
        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            printf("failed to open %s, %s\n", fname, strerror(errno));
            continue;
        }

        ioctl(fd, EVIOCGBIT(0, sizeof(mask)), mask);
        if (mask[EV_KEY / 8] & (1 << (EV_KEY % 8))) {
            fd_buf[cnt++] = fd;
        } else
            close(fd);
    }

    for (i = 0; i < ndev; i++)
        free(namelist[i]);

    free(namelist);

    if (cnt == 0) {
        fprintf(stderr, "The number of key event devices is zero!\n");
        return -1;
    }

    struct key_handle *handle = malloc(sizeof(struct key_handle) + sizeof(int) * cnt);
    if (!handle) {
        fprintf(stderr, "keyboard: fail to malloc key_handle %s\n", strerror(errno));
        goto close_fds;
    }

    handle->dev_num = cnt;
    for (i = 0; i < cnt; i++)
        handle->fds[i] = fd_buf[i];

    return (long)handle;

close_fds:
    for (i = 0; i < cnt; i++)
        close(fd_buf[i]);
    return -1;
}

void keys_close(long handle)
{
    int i;
    struct key_handle *key = (struct key_handle *)handle;

    for (i = 0; i < key->dev_num; i++)
        close(key->fds[i]);

    free(key);
}
