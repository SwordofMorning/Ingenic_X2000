#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>

#include <libmedia/utils/file_utils.h>
#include <libmedia/utils/fifo_utils.h>
#include <libmedia/utils/thread_utils.h>

#include <libutils2/boot_time.h>

struct fifo {
    int fd[3];
};

int fifo_create(const char *path0, const char *path1)
{
    int ret = mkfifo(path0, 0777);
    if (ret < 0) {
        fprintf(stderr, "fifo: failed to create: %s %s\n", path0, strerror(errno));
        return ret;
    }

    ret = mkfifo(path1, 0777);
    if (ret < 0) {
        fprintf(stderr, "fifo: failed to create: %s %s\n", path1, strerror(errno));
        remove(path0);
        return ret;
    }

    return 0;
}

int fifo_create_pid(pid_t pid)
{
    char path0[256];
    char path1[256];
    sprintf(path0, "/tmp/fifo.%d.0", pid);
    sprintf(path1, "/tmp/fifo.%d.1", pid);
    int ret = fifo_create(path0, path1);
    if (ret < 0)
        return -1;
    return 0;
}

int fifo_create_current_pid()
{
    return fifo_create_pid(getpid());
}

static int fifo_wait(const char *path0, const char *path1, int timeout)
{
    uint64_t start = boot_time_usecs();

    while (access(path0, F_OK) || access(path1, F_OK)) {
        usleep(1000);
        if ((boot_time_usecs()-start)/1000 >= timeout)
            return -1;
    }

    return 0;
}

int fifo_delete(const char *path0, const char *path1)
{
    int ret = remove(path0) + remove(path1);
    if (ret < 0)
        fprintf(stderr, "fifo: failed to delete: %s or %s\n", path0, path1);
    return ret;
}

int fifo_delete_pid(pid_t pid)
{
    char path0[256];
    char path1[256];
    sprintf(path0, "/tmp/fifo.%d.0", pid);
    sprintf(path1, "/tmp/fifo.%d.1", pid);

    return fifo_delete(path0, path1);
}

int fifo_delete_current_pid(void)
{
    return fifo_delete_pid(getpid());
}

struct fifo *fifo_open(const char *path0, const char *path1, int timeout)
{
    if (fifo_wait(path0, path1, timeout)) {
        fprintf(stderr, "fifo: %s or %s not exist\n", path0, path1);
        return NULL;
    }

    int fd[3];

    /* 从path0读数据
     */
    fd[0] = open(path0, O_RDONLY | O_NONBLOCK);
    if (fd[0] < 0) {
        fprintf(stderr, "fifo: failed to open fifo for %s: %s %s\n", "read", path0, strerror(errno));
        return NULL;
    }

    /* 当使用 O_NONBLOCK 时,需要先打开fifo用作读,才能打开fifo用作写
     * 所以这个fd,没有实际用处,只是确保下面的写能够完成
     */
    fd[2] = open(path1, O_RDONLY | O_NONBLOCK);
    if (fd[0] < 0) {
        fprintf(stderr, "fifo: failed to open fifo for %s: %s %s\n", "keep", path1, strerror(errno));
        return NULL;
    }

    /* 往path1写数据
     */
    fd[1] = open(path1, O_WRONLY | O_NONBLOCK);
    if (fd[1] < 0) {
        fprintf(stderr, "fifo: failed to open fifo for %s: %s %s\n", "write", path1, strerror(errno));
        return NULL;
    }

    struct fifo *fifo = malloc(sizeof(*fifo));
    assert(fifo);

    fifo->fd[0] = fd[0];
    fifo->fd[1] = fd[1];
    fifo->fd[2] = fd[2];

    return fifo;
}

struct fifo *fifo_open_pid(pid_t pid, int path0_for_write, int timeout)
{
    char path0[256];
    char path1[256];
    sprintf(path0, "/tmp/fifo.%d.0", pid);
    sprintf(path1, "/tmp/fifo.%d.1", pid);

    if (path0_for_write)
        return fifo_open(path1, path0, timeout);
    else
        return fifo_open(path0, path1, timeout);
}

struct fifo *fifo_open_current_pid(int path0_for_write, int timeout)
{
    return fifo_open_pid(getpid(), path0_for_write, timeout);
}

void fifo_close(struct fifo *fifo)
{
    close(fifo->fd[0]);
    close(fifo->fd[1]);
    close(fifo->fd[2]);
    free(fifo);
}

int fifo_read(struct fifo *fifo, void *buf, int size, int timeout)
{
    int ret = read_timeout(fifo->fd[0], buf, size, timeout);
    if (ret < 0)
        fprintf(stderr, "fifo: failed to read\n");

    return ret;
}

int fifo_write(struct fifo *fifo, void *buf, int size, int timeout)
{
    int ret = write_timeout(fifo->fd[1], buf, size, timeout);
    if (ret < 0)
        fprintf(stderr, "fifo: failed to write\n");

    return ret;
}

#define HEADER_LEN 16
#define MAX_PKT_SIZE 8192

int fifo_write_pkt(struct fifo *fifo, void *data, int size, int timeout)
{
    char header[HEADER_LEN+1];
    sprintf(header, "[fifo %08x]\n", size);

    assert(size >= 0);

    int ret = write_timeout(fifo->fd[1], header, HEADER_LEN, timeout);
    if (ret != HEADER_LEN) {
        fprintf(stderr, "fifo: failed to write header: %d\n", ret);
        return -1;
    }

    if (size == 0)
        return 0;

    ret = write_timeout(fifo->fd[1], data, size, timeout);
    if (ret != size) {
        fprintf(stderr, "fifo: failed to write pkt data: %d\n", ret);
        return -1;
    }

    return 0;
}

int fifo_write_pkt2(struct fifo *fifo, int timeout, const char *__restrict fmt, ...)
{
    char buf[MAX_PKT_SIZE];
    va_list list;
    va_start(list, fmt);
    int len = vsnprintf(buf, MAX_PKT_SIZE, fmt, list);
    va_end(list);

    return fifo_write_pkt(fifo, buf, len+1, timeout);
}

int fifo_read_pkt(struct fifo *fifo, void *data, int size, int timeout)
{
    char header[HEADER_LEN+1];

    int ret = read_timeout(fifo->fd[0], header, HEADER_LEN, timeout);
    if (ret == 0) {
        *(char *)data = '\0';
        return 0;
    }

    if (ret != HEADER_LEN) {
        fprintf(stderr, "fifo: failed to read header: %d\n", ret);
        return ret;
    }

    header[HEADER_LEN] = '\0';
    if (strncmp(header, "[fifo ", 6)) {
        fprintf(stderr, "fifo: header: %s not valid\n", header);
        return -1;
    }

    char *end = NULL;
    unsigned int pkt_size = strtoul(&header[6], &end, 16);
    if (end == NULL || *end != ']') {
        fprintf(stderr, "fifo: header: %s not valid\n", header);
        return -1;
    }

    if (pkt_size == 0)
        return 0;

    if (pkt_size > size) {
        fprintf(stderr, "fifo: pkt size too large: 0x%x 0x%x\n", pkt_size, size);
        assert(0);
    }

    ret = read_timeout(fifo->fd[0], data, pkt_size, timeout);
    if (ret != pkt_size) {
        fprintf(stderr, "fifo: failed to read pkt data: %d\n", ret);
        return -1;
    }

    return 0;
}

struct fifo *create_process_open_fifo(char *args[], pid_t *pid_p)
{
    pid_t pid = process_create(args);
    if (pid < 0)
        return NULL;

    struct fifo *fifo = fifo_open_pid(pid, 0, 300);
    if (!fifo) {
        fprintf(stderr, "failed to open %s %d fifo\n", args[0], pid);
        return NULL;
    }

    if (pid_p)
        *pid_p = pid;

    return fifo;
}

int close_fifo_wait_process(struct fifo *fifo, pid_t pid)
{
    int status;

    fifo_close(fifo);
    return waitpid(pid, &status, 0);
}
