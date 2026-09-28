#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <driver/console.h>
#include <sys/stat.h>

void _exit(int exit_status)
{
    while (1) ;
}

int _kill(int pid, int sig)
{
    errno = ENOSYS;
    return -1;
}

int _getpid(void)
{
    return 1;
}

#include <stdio.h>

int _open(const char *name, int flags, int mode)
{
    printf("open: %s\n", name);
    errno = ENOSYS;
    return -1;
}

ssize_t _write(int file, const void *ptr, size_t len)
{
    if (!(file == STDOUT_FILENO || file == STDERR_FILENO)) {
        errno = ENOSYS;
        return -1;
    }

    const char *bptr = ptr;
    for (size_t i = 0; i < len; ++i)
        console_put_char(bptr[i]);

    return len;
}

ssize_t _read(int file, void *ptr, size_t len)
{
    if (file != STDIN_FILENO) {
        errno = ENOSYS;
        return -1;    
    }
    
    char *bptr = ptr;
    int i;
    for (i = 0; i < len; i++)
        bptr[i] = console_get_char();

    return len;
}


off_t _lseek(int file, off_t ptr, int dir)
{
    errno = ENOSYS;
    return -1;
}

int _close(int file)
{
    errno = ENOSYS;
    return -1;
}

int _fstat(int fd, struct stat *buf)
{
  buf->st_mode = S_IFCHR;	/* Always pretend to be a tty */
  buf->st_blksize = 0;

  return 0;
}

int _isatty(int file)
{
    return file == STDOUT_FILENO ||
            file == STDIN_FILENO ||
            file == STDERR_FILENO;
}
