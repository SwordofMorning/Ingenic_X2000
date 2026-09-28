#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libhardware2/rsa.h>

#define CMD_RSA_PREPARE_KEY _IOWR('r', 0, void *)
#define CMD_RSA_DO_CRYPT    _IOWR('r', 1, void *)

#define RSA_MAX_LEN             4096
#define RSA_MAX_PADDING_LEN     3072
#define RSA_PKCS1_PADDING_SIZE  11

static inline void rsa_err(const char *err_msg)
{
    fprintf(stderr, "RSA: failed to %s, %s\n", err_msg, strerror(errno));
}

static inline int rsa_get_key_bytes(int mode)
{
    switch (mode)
    {
    case RSA_1024:
        return 128;
    case RSA_2048:
        return 256;
    default:
        fprintf(stderr, "rsa: get key bytes failed! mode: %d\n", mode);
        break;
    }
    return -1;
}

static inline void rsa_char_to_int(unsigned char *src, int *dst, int keyl)
{
    int i;
    for (i = 0; i < keyl; i++) {
        dst[i]  = ((uint32_t)(src)[0] << 24) & 0xff000000;
        dst[i] |= ((uint32_t)(src)[1] << 16) & 0x00ff0000;
        dst[i] |= ((uint32_t)(src)[2] << 8)  & 0x0000ff00;
        dst[i] |= ((uint32_t)(src)[3] << 0)  & 0x000000ff;
        src += 4;
    }
}

static inline void rsa_int_to_char(int *src, unsigned char *dst, int keyl)
{
    int i;
    for (i = 0; i < keyl; i++) {
        dst[0] = (src[i] & 0xff000000) >> 24;
        dst[1] = (src[i] & 0x00ff0000) >> 16;
        dst[2] = (src[i] & 0x0000ff00) >> 8;
        dst[3] = (src[i] & 0x000000ff) >> 0;
        dst += 4;
    }
}

static inline int rsa_padding_add_PKCS1_type_2(unsigned char *to, int keyB,
                unsigned char *from, int flen)
{
    int i, j;
    unsigned char a = 1;
    unsigned char *p, *q = to;

    if (keyB <= 0 || flen <= 0)
        return -1;

    unsigned int n;
    while (flen > 0) {
        if (flen > (keyB - RSA_PKCS1_PADDING_SIZE))
            n = keyB - RSA_PKCS1_PADDING_SIZE;
        else
            n = flen;

        p = q;
        *(p++) = 0;
        *(p++) = 2;
        j = keyB - 3 - n;
        for (i = 0; i < j; i++) {
            if (*p == 0) {
                do {
                    *p = a++;
                } while (*p == 0);
            }
            p++;
        }

        *(p++) = 0;
        memmove(p, from, n);
        q += keyB;
        from += n;
        flen -= n;
    }

    return q - to;
}

static inline int rsa_padding_check_PKCS1_type_2(unsigned char *to, int keyB,
                unsigned char *from, int flen)
{
    unsigned char *p, *q = to;

    if (keyB <= 0 || flen <= 0)
        return -1;

    if (flen <= RSA_PKCS1_PADDING_SIZE) {
        fprintf(stderr, "rsa: check padding err\n");
        return -1;
    }

    unsigned int n, N;
    while (flen > 0) {
        p = from;
        if (p[0] != 0 || p[1] != 0x2)
            return -1;

        p += RSA_PKCS1_PADDING_SIZE - 1;
        while (*(p++));

        if (flen > keyB)
            n = keyB;
        else
            n = flen;

        N = n - (p - from);
        memmove(q, p, N);
        q += N;
        from += n;
        flen -= n;
    }

    return q - to;
}

/**
 * @brief 设置密钥长度模式(0(1024),1(2048))，根据密钥选择将key内的密钥e或d和n写到寄存器内, 长度可为128或256字节
 * @param fd rsa设备句柄
 * @param key rsa_key结构体, 存放密钥选择, 密钥长度mode，密钥e, 密钥d, 密钥n
 * @return 成功返回0, 失败返回负数
 */
static int rsa_prepare_key(int fd, struct rsa_key *key)
{
    int ret = ioctl(fd, CMD_RSA_PREPARE_KEY, (unsigned long)key);
    if (ret < 0)
        rsa_err("prepare key");

    return ret;
}

int rsa_crypt(int fd, struct rsa_data *data)
{
    int ret = ioctl(fd, CMD_RSA_DO_CRYPT, (unsigned long)data);
    if (ret < 0)
        rsa_err("crypt");

    return ret;
}

int rsa_encrypt_file(struct rsa_handle *handle, char *in_file, char *out_file)
{
    struct stat st;
    int ret = stat(in_file, &st);
    if (ret < 0) {
        rsa_err("get in_file information");
        return -1;
    }

    int in_fd = open(in_file, O_RDONLY);
    if (in_fd < 0) {
        rsa_err("open in_file");
        return -1;
    }

    int out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (out_fd < 0) {
        rsa_err("open out_file");
        ret = -EINVAL;
        goto close_in_file;
    }

    int n, N;
    int size = st.st_size;
    struct rsa_data data;

    unsigned char *buffer = malloc(RSA_MAX_LEN * 4);
    if (!buffer) {
        rsa_err("malloc buffer");
        goto close_out_file;
    }

    unsigned char *din = buffer;
    unsigned char *dout = buffer + RSA_MAX_LEN;

    data.src = (unsigned int *)(buffer + RSA_MAX_LEN * 2);
    data.dst = (unsigned int *)(buffer + RSA_MAX_LEN * 3);

    while (size > 0) {
        if (size > RSA_MAX_PADDING_LEN)
            n = RSA_MAX_PADDING_LEN;
        else
            n = size;

        N = read(in_fd, dout, n); // dout for tmp
        if (N != n) {
            rsa_err("read in_file");
            ret = -EACCES;
            goto err_malloc;
        }

        n = rsa_padding_add_PKCS1_type_2(din, handle->keyB, dout, N);
        if (n <= 0 || n % 4) {
            rsa_err("add padding");
            goto err_malloc;
        }

        data.len = n / 4;
        rsa_char_to_int(din, data.src, data.len);
        ret = rsa_crypt(handle->fd, &data);
        if (ret < 0) {
            rsa_err("crypt file");
            goto err_malloc;
        }

        rsa_int_to_char(data.dst, dout, data.len);
        write(out_fd, dout, n);
        size -= N;
    }

    ret = 0;

err_malloc:
    free(buffer);
close_out_file:
    close(out_fd);
close_in_file:
    close(in_fd);

    return ret;
}

int rsa_decrypt_file(struct rsa_handle *handle, char *in_file, char *out_file)
{
    struct stat st;
    int ret = stat(in_file, &st);
    if (ret < 0) {
        rsa_err("get in_file information");
        return -1;
    }

    int in_fd = open(in_file, O_RDONLY);
    if (in_fd < 0) {
        rsa_err("open in_file");
        return -1;
    }

    int out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (out_fd < 0) {
        rsa_err("open out_file");
        ret = -EINVAL;
        goto close_in_file;
    }

    int n, N;
    int size = st.st_size;
    struct rsa_data data;

    unsigned char *buffer = malloc(RSA_MAX_LEN * 4);
    if (!buffer) {
        rsa_err("malloc buffer");
        goto close_out_file;
    }

    unsigned char *din = buffer;
    unsigned char *dout = buffer + RSA_MAX_LEN;

    data.src = (unsigned int *)(buffer + RSA_MAX_LEN * 2);
    data.dst = (unsigned int *)(buffer + RSA_MAX_LEN * 3);

    while (size > 0) {
        if (size > RSA_MAX_LEN)
            n = RSA_MAX_LEN;
        else
            n = size;

        N = read(in_fd, din, n);
        if (N != n || N % 4) {
            rsa_err("read in_file");
            ret = -EACCES;
            goto err_malloc;
        }

        data.len = N / 4;
        rsa_char_to_int(din, data.src, data.len);
        ret = rsa_crypt(handle->fd, &data);
        if (ret < 0) {
            rsa_err("crypt file");
            goto err_malloc;
        }

        rsa_int_to_char(data.dst, dout, data.len);
        n = rsa_padding_check_PKCS1_type_2(din, handle->keyB, dout, N); // din for tmp
        if (n <= 0) {
            rsa_err("check padding");
            goto err_malloc;
        }

        write(out_fd, din, n);
        size -= N;
    }

    ret = 0;

err_malloc:
    free(buffer);
close_out_file:
    close(out_fd);
close_in_file:
    close(in_fd);

    return ret;
}

static int rsa_get_key_size(int *size)
{
    int ret = 0;
    switch (*size)
    {
    case 0x81:
        ret = 1;
    case 0x80:
        *size = 0x80;
        break;
    case 0x101:
        ret = 1;
    case 0x100:
        *size = 0x100;
        break;

    default:
        *size = 0;
        break;
    }

    return ret;
}

static inline int rsa_get_key_n_or_d(unsigned int *key, unsigned char *ptr, int keyB)
{
    unsigned char *p = ptr;
    if (p[0] != 0x2)
        return -1;

    int size = 0;
    if (p[1] == 0x81) {
        size = p[2] & 0x000000ff;
        p += 3;
    } else if (p[1] == 0x82) {
        size = p[3] & 0x000000ff;
        size |= (p[2] << 8) & 0x0000ff00;
        p += 4;
    } else {
        return -1;
    }

    if (rsa_get_key_size(&size))
        p++;

    if (size != keyB)
        return -1;

    rsa_char_to_int(p, key, keyB / 4);
    p += keyB;

    return p - ptr;
}

static inline int rsa_get_key_e(unsigned int *key, unsigned char *p, int keyB)
{
    if (p[0] != 0x2 || p[1] != 0x3 || p[2] != 0x1 || p[3] != 0x0 || p[4] != 0x1)
        return -1;

    memset(key, 0, keyB);
    key[keyB / 4 - 1] = 0x10001;

    return 0;
}

static int rsa_analysis_key(struct rsa_key *key, unsigned char *pem, int keyB)
{
    if (pem[0] != 0x30 || pem[1] != 0x82 || pem[4] != 0x2 || pem[5] != 0x1 || pem[6] != 0x0)
        return -1;

    unsigned char *p = &pem[7];

    int offset = rsa_get_key_n_or_d(key->n, p, keyB);
    if (offset < 0) {
        rsa_err("get n");
        return -1;
    }

    p += offset;
    if (rsa_get_key_e(key->e, p, keyB)) {
        rsa_err("get e");
        return -1;
    }

    p += 5;
    offset = rsa_get_key_n_or_d(key->d, p, keyB);
    if (offset < 0){
        rsa_err("get d");
        return -1;
    }

    return 0;
}

int rsa_read_key(struct rsa_key *key, char *file, int keyB)
{
    struct stat st;
    int ret = stat(file, &st);
    if (ret < 0) {
        rsa_err("get file information");
        return -1;
    }

    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        rsa_err("open key_file");
        return -1;
    }

    unsigned char *pem = malloc(st.st_size);
    if (pem == NULL) {
        rsa_err("malloc keybuf");
        ret = -1;
        goto err;
    }

    ret = read(fd, pem, st.st_size);
    if (ret != st.st_size) {
        rsa_err("read key data");
        ret = -1;
        goto err_mem;
    }

    if (rsa_analysis_key(key, pem, keyB)) {
        rsa_err("analysis key");
        ret = -1;
        goto err_mem;
    }

    ret = 0;
err_mem:
    free(pem);
err:
    close(fd);
    return ret;
}

struct rsa_handle *rsa_open(struct rsa_key *key, char *key_file)
{
    struct rsa_handle *handle = malloc(sizeof(struct rsa_handle));
    if (handle == NULL) {
        rsa_err("malloc handle");
        return NULL;
    }

    memset(handle, 0, sizeof(struct rsa_handle));

    handle->fd = open("/dev/jz_rsa", O_RDWR);
    if (handle->fd < 0) {
        rsa_err("open dev");
        goto err;
    }

    if (key == NULL || key_file == NULL) {
        rsa_err("get key_file");
        goto err_open;
    }

    handle->mode = key->mode;
    handle->keyB = rsa_get_key_bytes(handle->mode);

    if (rsa_read_key(key, key_file, handle->keyB)) {
        rsa_err("read key");
        goto err_open;
    }

    int ret = rsa_prepare_key(handle->fd, key);
    if (ret < 0) {
        rsa_err("set key");
        goto err_open;
    }

    return handle;
err_open:
    close(handle->fd);
err:
    free(handle);
    return NULL;
}

void rsa_close(struct rsa_handle *handle)
{
    close(handle->fd);
    free(handle);
}
