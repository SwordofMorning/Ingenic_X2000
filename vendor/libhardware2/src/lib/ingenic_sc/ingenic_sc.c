#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <linux/input.h>
#include <assert.h>
#include <libhardware2/ingenic_sc.h>

#define CMD_sc_setup    _IOWR('s', 0, void *)
#define CMD_sc_decrypt  _IOWR('s', 1, void *)

static inline void sc_err(const char *err_msg)
{
    fprintf(stderr, "sc: failed to %s, %s\n", err_msg, strerror(errno));
}

int sc_setup(int fd, unsigned char *head)
{
    unsigned int len = -1;

    unsigned long array[2] = {
        (unsigned long)head,
        (unsigned long)&len
    };

    int ret = ioctl(fd, CMD_sc_setup, array);
    if (ret) {
        sc_err("setup");
        return ret;
    }

    return len;
}

int sc_decrypt(int fd, unsigned char *src, unsigned char *dst, int size)
{
    unsigned long array[3] = {
        (unsigned long)src,
        (unsigned long)dst,
        (unsigned long)size
    };

    int ret = ioctl(fd, CMD_sc_decrypt, array);
    if (ret)
        sc_err("decrypt");

    return ret;
}

int sc_decrypt_file(int fd, unsigned char *src_file, unsigned char *dst_file)
{
    int ret = -1;
    int src_fd = open(src_file, O_RDONLY);
    if (src_fd < 0) {
        sc_err("open src_file");
        return -1;
    }

    int dst_fd = open(dst_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (dst_fd < 0) {
        sc_err("open dst_file");
        goto close_src_file;
    }

    struct stat st;
    ret = stat(src_file, &st);
    if (ret < 0 || st.st_size <= SC_SIGNATURE_SIZE) {
        sc_err("stat src_file or src_file do not be encrypted");
        goto close_dst_file;
    }

    unsigned char keybuf[SC_SIGNATURE_SIZE];
    unsigned char src[SC_MAX_SIZE_PERROUND];
    unsigned char dst[SC_MAX_SIZE_PERROUND];

    int total_size = -1;
    if (read(src_fd, keybuf, SC_SIGNATURE_SIZE) != SC_SIGNATURE_SIZE) {
        sc_err("read src_file_signature");
        goto close_dst_file;
    }
    total_size = sc_setup(fd, keybuf);
    if (total_size < 0 || total_size > st.st_size - SC_SIGNATURE_SIZE) {
        sc_err("setup_keys or get real_data_size");
        goto close_dst_file;
    }

    int len = 0;
    while (total_size) {
        len = total_size > SC_MAX_SIZE_PERROUND ? SC_MAX_SIZE_PERROUND : total_size;
        if (read(src_fd, src, len) <= 0) {
            sc_err("read src_file");
            goto close_dst_file;
        }

        ret = sc_decrypt(fd, src, dst, len);
        if (ret) {
            sc_err("decrypt");
            goto close_dst_file;
        }

        if (write(dst_fd, dst, len) != len) {
            sc_err("write dst_file");
            goto close_dst_file;
        }

        total_size -= len;
    }

close_dst_file:
    close(dst_fd);
close_src_file:
    close(src_fd);

    return ret;
}

int sc_open(void)
{
    int fd;

    fd = open("/dev/sc", O_RDWR);
    if (fd < 0)
        sc_err("open dev");

    return fd;
}

void sc_close(int fd)
{
    close(fd);
}
