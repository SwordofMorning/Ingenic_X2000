#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>
#include <time.h>
#include <semaphore.h>

#include <libutils2/os.h>

int thread_set_name(const char *name)
{
    return prctl(PR_SET_NAME, name);
}

pid_t process_create(char *args[])
{
    pid_t pid = vfork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork: %s %s\n", args[0], strerror(errno));
        return pid;
    }

    if (pid) {
        return pid;
    } else {
        execvp(args[0], args);
        fprintf(stderr, "failed to execute: %s, %s\n", args[0], strerror(errno));
        exit(-1);
    }
}

static int keep_cmd_exist = 0;

pid_t utils_gettid(void)
{
    return syscall(SYS_gettid);
}

void utils_system_keep_cmd_file(int enable)
{
    keep_cmd_exist = enable;
}

static void get_cmd_exe_file_name(char *path, int len)
{
    time_t t;
    struct tm tm;
    t = time(NULL);
    localtime_r(&t, &tm);

    static volatile atomic_uint count;

    snprintf(path, len, "/tmp/cmd.[%d.%d].%d.%02d:%02d-%02d:%02d:%02d.sh",
             getpid(), utils_gettid(), atomic_fetch_add_explicit(&count, 1, memory_order_seq_cst),
             tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static int create_cmd_exe_file(char *path, const char *cmd)
{
    int fd = open(path, O_WRONLY|O_CREAT, 777);
    if (fd < 0) {
        fprintf(stderr, "utils_system: failed to create %s %s\n", path, strerror(errno));
        return -1;
    }

    const char header[] = "#!/bin/sh\n";
    int len = sizeof(header)-1;
    int ret = write(fd, header, len);
    if (ret != len) {
        fprintf(stderr, "utils_system: failed to write %s %s %d\n", path, strerror(errno), ret);
        close(fd);
        return -1;
    }

    len = strlen(cmd);
    ret = write(fd, cmd, len);
    if (ret != len) {
        fprintf(stderr, "utils_system: failed to write %s %s %d\n", path, strerror(errno), ret);
        close(fd);
        return -1;
    }

    const char reter0[] = "\n\nm_ret=$?\n";
    len = sizeof(reter0)-1;
    ret = write(fd, reter0, len);
    if (ret != len) {
        fprintf(stderr, "utils_system: failed to write %s %s %d\n", path, strerror(errno), ret);
        close(fd);
        return -1;
    }

    if (!keep_cmd_exist) {
        const char *tail = "rm $0\n";
        len = strlen(tail);
        ret = write(fd, tail, len);
        if (ret != len) {
            fprintf(stderr, "utils_system: failed to write %s %s %d\n", path, strerror(errno), ret);
            close(fd);
            return -1;
        }
    }

    const char reter1[] = "exit $m_ret\n";
    len = sizeof(reter1)-1;
    ret = write(fd, reter1, len);
    if (ret != len) {
        fprintf(stderr, "utils_system: failed to write %s %s %d\n", path, strerror(errno), ret);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int utils_system(const char *cmd)
{
    char path[256];

    get_cmd_exe_file_name(path, sizeof(path));

    int ret = create_cmd_exe_file(path, cmd);
    if (ret)
        return -1;

    char *args[2] = {
        path, NULL,
    };

    pid_t pid = process_create(args);
    if (pid < 0) {
        fprintf(stderr, "utils_system: failed to execute %s %s\n", path, strerror(errno));
        return -1;
    }

    int status = -1;
    waitpid(pid, &status, 0);

    return status;
}

int sem_wait_until(sem_t *sem, struct timespec *tp)
{
    if (tp->tv_nsec < 0)
        return sem_wait(sem);
    return sem_timedwait(sem, tp);
}

int sem_wait_timeout(sem_t *sem, int wait_usecs)
{
    if (!wait_usecs)
        return sem_trywait(sem);

    struct timespec tp;
    timespec_add_current_usecs(&tp, wait_usecs);

    return sem_wait_until(sem, &tp);
}

int pthread_cond_wait_until(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec *tp)
{
    if (tp->tv_nsec < 0)
        return pthread_cond_wait(cond, mutex);
    return pthread_cond_timedwait(cond, mutex, tp);
}

int pthread_cond_wait_timeout(pthread_cond_t *cond, pthread_mutex_t *mutex, int wait_usecs)
{
    struct timespec tp;
    timespec_add_current_usecs(&tp, wait_usecs);

    return pthread_cond_wait_until(cond, mutex, &tp);
}

void timespec_add_usecs(struct timespec *tp, int usecs)
{
    if (usecs < 0)
        tp->tv_nsec = -1;
    else {
        int delta = usecs % (1000*1000);
        tp->tv_sec += usecs / (1000*1000);
        tp->tv_nsec += delta*1000;
        if (tp->tv_nsec >= (1000*1000*1000)) {
            tp->tv_sec += 1;
            tp->tv_nsec -= (1000*1000*1000);
        }
    }
}

void timespec_add_current_usecs(struct timespec *tp, int usecs)
{
    clock_gettime(CLOCK_REALTIME, tp);
    timespec_add_usecs(tp, usecs);
}
