#ifndef _LIBUTILS2_OS_H_
#define _LIBUTILS2_OS_H_

#include <sys/types.h>
#include <pthread.h>

/**
 * @brief 设置当前线程的名字
 * @return 0 表示成功
 */
int thread_set_name(const char *name);

/**
 * @brief 创建一个进程(使用vfork实现开销更小)
 * @param args 
 * @return >= 0 表示进程的pid, < 0表示失败
 */
pid_t process_create(char *args[]);

/**
 * @brief 获得当前线程的pid
 * @return >= 0 表示线程的pid, < 0表示失败
 */
pid_t utils_gettid(void);

/**
 * @brief 设置是否保留执行 utils_system 的临时脚本文件,用于debug
 * @param enable 1 保留 0 不保留
 */
void utils_system_keep_cmd_file(int enable);

/**
 * @brief 执行给定的命令,并等待其执行完成(使用vfork实现开销更小)
 * @param cmd 需要被执行的命令
 * @return 0 表示成功, 其它表示命令不成功
 */
int utils_system(const char *cmd);

/**
 * @brief 给定超时时间内,等待信号量
 * @param wait_usecs < 0 时表示不超时, >=0 表示超时时间,单位微秒
 * @return 0 表示成功, 其它表示不成功
 */
int sem_wait_timeout(sem_t *sem, int wait_usecs);

/**
 * @brief 给定超时的绝对时间刻度,等待信号量
 * @param tp  tp->tv_nsec < 0 时表示不超时, 其它表示设定的绝对时间
 * @return 0 表示成功, 其它表示不成功
 */
int sem_wait_until(sem_t *sem, struct timespec *tp);

/**
 * @brief 给定超时时间内,等待pthread_cond
 * @param wait_usecs < 0 时表示不超时, >=0 表示超时时间,单位微秒
 * @return 0 表示成功, 其它表示不成功
 */
int pthread_cond_wait_timeout(pthread_cond_t *cond, pthread_mutex_t *mutex, int wait_usecs);

/**
 * @brief 给定超时的绝对时间刻度,等待pthread_cond
 * @param tp  tp->tv_nsec < 0 时表示不超时, 其它表示设定的绝对时间
 * @return 0 表示成功, 其它表示不成功
 */
int pthread_cond_wait_until(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec *tp);

/**
 * @brief 计算timespec加上微秒
 * @param tp 起始时间,并且返回相加的结果
 * @param usecs 需要加上的时间,单位微秒, < 0 时表示无限期, 此时 tp->tv_nesc = -1
 */
void timespec_add_usecs(struct timespec *tp, int usecs);

/**
 * @brief 计算当前时间加上微秒
 * clock_gettime(CLOCK_REALTIME, tp) 获取当前时间
 * @param tp 返回相加的结果
 * @param usecs 需要加上的时间,单位微秒, < 0 时表示无限期, 此时 tp->tv_nesc = -1
 */
void timespec_add_current_usecs(struct timespec *tp, int usecs);

#endif /* _LIBUTILS2_OS_H_ */
