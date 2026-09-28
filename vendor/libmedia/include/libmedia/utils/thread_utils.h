#ifndef __THREAD_UTILS_H__
#define __THREAD_UTILS_H__

int thread_set_name(const char *name);

pid_t process_create(char *args[]);

#endif /* __THREAD_UTILS_H__ */
