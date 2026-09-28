#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libhardware2/hash.h>

static int encryption_mode = -1;
static unsigned long hash_getSize(int mode)
{
    switch (mode)
    {
    case MD5:
        return MD5_byte;
        break;
    case SHA1:
        return SHA1_byte;
        break;
    case SHA224:
        return SHA224_byte;
        break;
    case SHA256:
        return SHA256_byte;
        break;
    default:
        return -1;
        break;
    }
}

int hash_init(enum encryption_mode mode)
{
    int ret = 0;
    int fd;

    if (encryption_mode != -1) {
        fprintf(stderr, "HASH:hash_init failed, you haven't deinit yet!\n");
        return -1;
    }

    fd = open("/dev/jz_hash", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "HASH:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, CMD_hash_init, mode);
    if (ret < 0) {
        fprintf(stderr, "HASH:init failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    encryption_mode = mode;

    return fd;
}

int hash_write(int handle, unsigned char *str, int str_size)
{
    unsigned long arg[2];
    arg[0] = (unsigned long)str;
    arg[1] = (unsigned long)str_size;
    int ret = ioctl(handle, CMD_hash_write, arg);
    if (ret < 0) {
        fprintf(stderr, "HASH:write failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int hash_deinit(int handle, unsigned char *rec, unsigned long hash_size)
{
    int ret;
    unsigned long arg[2];
    unsigned long mode_size = hash_getSize(encryption_mode);

    if (hash_size < mode_size) {   //这里要求hash_size 要大于解析得到的字节数
        fprintf(stderr, "HASH:the hash_size is too shorter\n");
        return -1;
    }

    arg[0] = (unsigned long)rec;
    arg[1] = mode_size;
    ret = ioctl(handle, CMD_hash_deinit, arg);
    if (ret < 0) {
        fprintf(stderr, "HASH:deinit failed: %s\n", strerror(errno));
        return -1;
    }

    close(handle);
    encryption_mode = -1;       //与hash_init(int mode)函数形成闭环对应
    return 0;
}
