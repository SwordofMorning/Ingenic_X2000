#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

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
