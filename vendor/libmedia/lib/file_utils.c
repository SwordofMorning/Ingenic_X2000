#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <stdint.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libmedia/utils/file_utils.h>
#include <libutils2/boot_time.h>

int file_read_data(const char *input_file, void **data_p, long *size_p)
{
    FILE *file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "file_utils: failed to open file: %s (%s)\n",
            input_file, strerror(errno));
        return -1;
    }

    int ret = fseek(file, 0, SEEK_END);
    if (ret < 0) {
        fprintf(stderr, "file_utils: failed to seek file: %s (%s)\n",
            input_file, strerror(errno));
        goto close_file;
    }

    long size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "file_utils: failed to tell file: %s (%s)\n",
            input_file, strerror(errno));
        goto close_file;
    }

    ret = fseek(file, 0, SEEK_SET);
    if (ret < 0) {
        fprintf(stderr, "file_utils: failed to seek again file: %s (%s)\n",
            input_file, strerror(errno));
        goto close_file;
    }

    void *data = malloc(size);
    assert(data);

    void *save_data = data;
    long save_size = size;

    while (size) {
        ret = fread(data, 1, size, file);
        if (ret <= 0) {
            fprintf(stderr, "file_utils: failed to read file: %s (%s) %d\n",
                input_file, strerror(errno), ret);
            goto free_data;
        }
        size -= ret;
        data += ret;
    }

    if (data_p)
        *data_p = save_data;
    if (size_p)
        *size_p = save_size;

    fclose(file);
    return 0;

free_data:
    free(data);
close_file:
    fclose(file);
    return -1;
}

int file_write_data(const char *out_file, void *data, unsigned long data_size)
{
    int ret = 0;
    FILE *file = fopen(out_file, "w");
    if (!file) {
        fprintf(stderr, "file_utils: failed to open file: %s (%s)\n",
            out_file, strerror(errno));
        return -1;
    }

    int N = 1024*1024;

    while (data_size) {
        int size = data_size < N ? data_size : N;
        ret = fwrite(data, 1, size, file);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to write file: %s (%s) %d\n",
                out_file, strerror(errno), size);
            goto close_file;
        }

        data_size -= ret;
        data += ret;
    }

    ret = 0;
    fflush(file);

close_file:
    fclose(file);

    return ret;
}

int read_timeout(int fd, void *buf, int size, int timeout)
{
    int len = 0;

    if (timeout == -1)
        return read(fd, buf, size);

    int flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) < 0) {
        fprintf(stderr, "file_utils: failed to fcntl: %s\n", strerror(errno));
        return -1;
    }

    uint64_t start = boot_time_usecs();

    int detla = timeout;

    while (size > 0) {
        struct pollfd fds;
        fds.events = POLLIN|POLLPRI;
        fds.fd = fd;
        fds.revents = 0;
        int ret = poll(&fds, 1, timeout);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to poll: %s\n", strerror(errno));
            len = len ? len : -1;
            break;
        }

        if (!(fds.revents && fds.events)) {
            if (len)
                fprintf(stderr, "file_utils: read poll is timeout: %d\n", len);
            break;
        }

        ret = read(fd, buf, size);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to read: %s\n", strerror(errno));
            len = len ? len : -1;
            break;
        }

        size -= ret;
        len += ret;
        buf += ret;

        if (size <= 0)
            break;

        uint64_t now = boot_time_usecs();
        detla = (now - start) / 1000;
        if (detla >= timeout) {
            if (len)
                fprintf(stderr, "file_utils: read poll is timeout: %d\n", len);
            break;
        }
        detla = timeout - detla;
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        fprintf(stderr, "file_utils: failed to fcntl: %s\n", strerror(errno));

    return len;
}

int write_timeout(int fd, void *buf, int size, int timeout)
{
    int len = 0;

    if (timeout == -1)
        return write(fd, buf, size);

    int flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) < 0) {
        fprintf(stderr, "file_utils: failed to fcntl: %s\n", strerror(errno));
        return -1;
    }

    uint64_t start = boot_time_usecs();

    int detla = timeout;

    while (size > 0) {
        struct pollfd fds;
        fds.events = POLLOUT;
        fds.fd = fd;
        fds.revents = 0;
        int ret = poll(&fds, 1, timeout);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to poll: %s\n", strerror(errno));
            len = len ? len : -1;
            break;
        }

        if (!(fds.revents && fds.events)) {
            if (len)
                fprintf(stderr, "file_utils: write poll is timeout: %d\n", len);
            break;
        }

        ret = write(fd, buf, size);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to write: %s\n", strerror(errno));
            len = len ? len : -1;
            break;
        }

        size -= ret;
        len += ret;
        buf += ret;

        if (size <= 0)
            break;

        uint64_t now = boot_time_usecs();
        detla = (now - start) / 1000;
        if (detla >= timeout) {
            fprintf(stderr, "file_utils: write poll is timeout: %d\n", len);
            break;
        }
        detla = timeout - detla;
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        fprintf(stderr, "file_utils: failed to fcntl: %s\n", strerror(errno));

    return len;
}
