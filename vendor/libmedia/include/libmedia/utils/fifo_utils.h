#ifndef _FIFO_UTILS_H_
#define _FIFO_UTILS_H_

#include <sys/types.h>
#include <unistd.h>

struct fifo;

struct fifo *create_process_open_fifo(char *args[], pid_t *pid_p);

int close_fifo_wait_process(struct fifo *fifo, pid_t pid);

int fifo_create(const char *path0, const char *path1);

int fifo_create_pid(pid_t pid);

int fifo_create_current_pid(void);

int fifo_delete(const char *path0, const char *path1);

int fifo_delete_pid(pid_t pid);

int fifo_delete_current_pid(void);

struct fifo *fifo_open(const char *path0, const char *path1, int timeout);

struct fifo *fifo_open_pid(pid_t pid, int path0_for_write, int timeout);

struct fifo *fifo_open_current_pid(int path0_for_write, int timeout);

void fifo_close(struct fifo *fifo);

int fifo_read(struct fifo *fifo, void *buf, int size, int timeout);

int fifo_write(struct fifo *fifo, void *buf, int size, int timeout);

int fifo_write_pkt(struct fifo *fifo, void *data, int size, int timeout);

int fifo_write_pkt2(struct fifo *fifo, int timeout, const char *__restrict fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));

int fifo_read_pkt(struct fifo *fifo, void *data, int size, int timeout);

#endif /* _FIFO_UTILS_H_ */
