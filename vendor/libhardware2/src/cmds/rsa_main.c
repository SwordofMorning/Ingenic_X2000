#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <libhardware2/rsa.h>

static char *prg_name;
static void usage(int status)
{
    fprintf(stderr, "%s encrypt/decrypt usage\n", prg_name);
    fprintf(stderr, "    -h/--help  : show help info\n");
    fprintf(stderr, "    bits=      : rsa bits which can be 1024/2048, default 1024\n");
    fprintf(stderr, "    key=       : key filepath which should be DER_format_file, must be set\n");
    fprintf(stderr, "    in=        : encrypt/decrypt input data filepath, must be set and differ from out_file\n");
    fprintf(stderr, "    out=       : encrypt/decrypt output data filepath, default \"in_file\" + _out\n");
    fprintf(stderr, "    Example: %s encrypt bits=1024 key=pri.der in=m\n", prg_name);
    fprintf(stderr, "             %s decrypt bits=1024 key=pri.der in=m_out\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

enum {
    cmd_encrypt,
    cmd_decrypt
};

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

static inline int rsa_get_mode(int bits)
{
    switch (bits)
    {
    case 1024:
        return RSA_1024;
    case 2048:
        return RSA_2048;
    default:
        fprintf(stderr, "rsa: get key len failed! bits: %d\n", bits);
        break;
    }
    return -1;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int cmd = -1;
    prg_name = argv[0];

    unsigned int bits = 1024;
    char *key_file = NULL;
    char *in_file = NULL;
    char *out_file = NULL;
    char is_malloc = 0;

    struct rsa_key key;
    key.mode = RSA_1024;

    if (argc < 2)
        usage(-1);

    while (1) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "encrypt")) {
            cmd = cmd_encrypt;
            break;
        }

        if (!strcmp(argv[1], "decrypt")) {
            cmd = cmd_decrypt;
            break;
        }

        fprintf(stderr, "[err] not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    if (argc < 4)
        usage(-1);

    int i;
    for (i = 2; i < argc; i++) {
        if (parse_uint(argv[i], "bits=", &bits, 10)) {
            key.mode = rsa_get_mode(bits);
            if (key.mode == -1) {
                fprintf(stderr, "[err] bits should be 1024/2048\n");
                return -1;
            }
            continue;
        }

        if (strncmp(argv[i], "key=", strlen("key=")) == 0) {
            key_file = argv[i] + strlen("key=");
            continue;
        }

        if (strncmp(argv[i], "in=", strlen("in=")) == 0) {
            in_file = argv[i] + strlen("in=");
            continue;
        }

        if (strncmp(argv[i], "out=", strlen("out=")) == 0) {
            out_file = argv[i] + strlen("out=");
            continue;
        }

        error_arg(argv[i]);
    }

    if (key_file == NULL || in_file == NULL) {
        fprintf(stderr, "[err] key_file & in must be set!\n");
        return -1;
    }

    if (out_file == NULL) {
        out_file = malloc(strlen(in_file) + 4 + 1);
        memset(out_file, 0, strlen(in_file) + 4 + 1);
        sprintf(out_file, "%s_out", in_file);
        is_malloc = 1;
    }

    fprintf(stderr, "bits       : %d\n", bits);
    fprintf(stderr, "key        : %s\n", key_file);
    fprintf(stderr, "in_file    : %s\n", in_file);
    fprintf(stderr, "out_file   : %s\n", out_file);

    key.is_pub_use = cmd == cmd_encrypt;
    struct rsa_handle *handle = rsa_open(&key, key_file);
    if (handle == NULL) {
        fprintf(stderr, "RSA: open dev failed!\n");
        goto err;
    }

    if (key.is_pub_use)
        ret = rsa_encrypt_file(handle, in_file, out_file);
    else
        ret = rsa_decrypt_file(handle, in_file, out_file);

    if (ret < 0)
        fprintf(stderr, "RSA: rsa encrypt/decrypt file failed!\n");

    rsa_close(handle);
err:
    if (is_malloc)
        free(out_file);

    return ret;
}
