#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libhardware2/aes.h>

#define CMD_aes_get_key         _IOWR('a', 0, void *)
#define CMD_aes_transfer_data   _IOWR('a', 1, void *)

/* must align with 16 bytes */
#define AES_transfer_len    4096
#define AES_ALIGN_LEN       16
#define AES_ALIGN(d,a)      (((d)+((a)-1))/(a)*(a))

static inline void aes_err(const char *err_msg)
{
    fprintf(stderr, "AES: failed to %s, %s\n", err_msg, strerror(errno));
}

struct aes_handle {
    int fd;                     /* aes设备句柄 */
    unsigned int key[8];        /* 转化完成的密钥 */
    struct aes_config config;   /* aes配置(密钥长度,编解码模式,用户密钥,初始化向量(cbc模式使用)) */
};

/**
 * @brief 根据config内的密钥长度keyl将用户输入的密钥转换成int类型保存在key内, 用户密钥最长可为32字节
 * @param fd aes设备句柄
 * @param config aes配置结构体, 存放密钥长度,编解码模式,用户密钥,初始化向量(cbc模式使用)
 * @param key 转化完的密钥存放地址
 * @return 成功返回0, 失败返回负数
 */
int aes_get_key(int fd, struct aes_config *config, unsigned int *key)
{
    int ret;

    unsigned long array[4] = {
        (unsigned long)config->ukey,
        (unsigned long)key,
        (unsigned long)config->keyl,
        (unsigned long)config->endian
    };

    ret = ioctl(fd, CMD_aes_get_key, array);
    if (ret < 0)
        aes_err("get key");

    return ret;
}

int aes_encrypt(struct aes_handle *handle, unsigned char *src, unsigned char *dst, unsigned int len)
{
    unsigned long array[6] = {
        (unsigned long)&handle->config,
        (unsigned long)ENCRYPTION,
        (unsigned long)handle->key,
        (unsigned long)src,
        (unsigned long)dst,
        (unsigned long)len
    };

    int ret = ioctl(handle->fd, CMD_aes_transfer_data, array);
    if (ret < 0)
        aes_err("encrypt");

    return ret;
}

int aes_decrypt(struct aes_handle *handle, unsigned char *src, unsigned char *dst, unsigned int len)
{
    unsigned long array[6] = {
        (unsigned long)&handle->config,
        (unsigned long)DECRYPTION,
        (unsigned long)handle->key,
        (unsigned long)src,
        (unsigned long)dst,
        (unsigned long)len
    };

    int ret = ioctl(handle->fd, CMD_aes_transfer_data, array);
    if (ret < 0)
        aes_err("decrypt");

    return ret;
}

int aes_encrypt_file(struct aes_handle *handle, unsigned char *in_file, unsigned char *out_file)
{
    int ret = -1;
    struct stat st;

    ret = stat(in_file, &st);
    if (ret < 0) {
        aes_err("get in_file_size");
        return ret;
    }

    int in_filefd = open(in_file, O_RDONLY);
    if (in_filefd < 0) {
        aes_err("open in_file");
        return -1;
    }

    int out_filefd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (out_filefd < 0) {
        aes_err("open out_file");
        goto close_in_file;
    }

    int len = 0;
    int file_size = st.st_size;
    unsigned char *src = malloc(AES_transfer_len);
    if (!src) {
        aes_err("malloc src");
        ret = -ENOMEM;
        goto close_out_file;
    }

    unsigned char *dst = malloc(AES_transfer_len);
    if (!dst) {
        aes_err("malloc dst");
        ret = -ENOMEM;
        goto free_src;
    }

    while (file_size) {
        len = read(in_filefd, src, AES_transfer_len);
        if (len > 0)
            file_size -= len;
        else {
            aes_err("read in_file");
            break;
        }

        ret = aes_encrypt(handle, src, dst, len);
        if (ret < 0) {
            aes_err("encrypt file");
            break;
        }

        write(out_filefd, dst, AES_ALIGN(len, AES_ALIGN_LEN));
    }

    free(dst);
free_src:
    free(src);
close_out_file:
    close(out_filefd);
close_in_file:
    close(in_filefd);

    return ret;
}

int aes_decrypt_file(struct aes_handle *handle, unsigned char *in_file, unsigned char *out_file)
{
    int ret = -1;
    struct stat st;

    ret = stat(in_file, &st);
    if (ret < 0) {
        aes_err("get in_file_size");
        return ret;
    }

    int in_filefd = open(in_file, O_RDONLY);
    if (in_filefd < 0) {
        aes_err("open in_file");
        return -1;
    }

    int out_filefd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (out_filefd < 0) {
        aes_err("open out_file");
        goto close_in_file;
    }

    int len = 0;
    int file_size = st.st_size;
    unsigned char *src = malloc(AES_transfer_len);
    if (!src) {
        aes_err("malloc src");
        ret = -ENOMEM;
        goto close_out_file;
    }

    unsigned char *dst = malloc(AES_transfer_len);
    if (!dst) {
        aes_err("malloc dst");
        ret = -ENOMEM;
        goto free_src;
    }

    while (file_size) {
        len = read(in_filefd, src, AES_transfer_len);
        if (len > 0)
            file_size -= len;
        else {
            aes_err("read in_file");
            break;
        }

        ret = aes_decrypt(handle, src, dst, len);
        if (ret < 0) {
            aes_err("decrypt file");
            break;
        }

        write(out_filefd, dst, AES_ALIGN(len, AES_ALIGN_LEN));
    }

    free(dst);
free_src:
    free(src);
close_out_file:
    close(out_filefd);
close_in_file:
    close(in_filefd);

    return ret;
}

struct aes_handle *aes_open(struct aes_config *config)
{
    struct aes_handle *handle = malloc(sizeof(struct aes_handle));
    if (!handle) {
        aes_err("malloc handle");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aes_handle));

    handle->fd = open("/dev/jz_aes", O_RDWR);
    if (handle->fd < 0) {
        aes_err("open dev");
        goto err;
    }

    int ret = aes_get_key(handle->fd, config, handle->key);
    if (ret < 0) {
        aes_err("get key");
        close(handle->fd);
        goto err;
    }

    memcpy(&handle->config, config, sizeof(struct aes_config));

    return handle;
err:
    free(handle);
    return NULL;
}

void aes_close(struct aes_handle *handle)
{
    close(handle->fd);
    free(handle);
}
