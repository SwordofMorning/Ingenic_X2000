#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>
#include <libhardware2/aes.h>

struct aes_handle;
static char *prg_name;
static void usage(int status)
{
    fprintf(stderr, "%s encrypt/decrypt usage\n", prg_name);
    fprintf(stderr, "    -h/--help  : show help info\n");
    fprintf(stderr, "    bits=      : size for key which can be 128/192/256, default 128\n");
    fprintf(stderr, "    mode=      : aes mode which can be ecb/cbc, default ecb\n");
    fprintf(stderr, "    endian=    : data input endian which can be little/big, default big(openssl uses)\n");
    fprintf(stderr, "    ukey=      : user key which should less than 32bytes(string), must be set\n");
    fprintf(stderr, "    iv=        : initial vector for cbc_mode, when cbc_mode must be set and less than 16 bytes\n");
    fprintf(stderr, "    in_file=   : encrypt/decrypt input data filepath, must be set and cannot be same with out_file\n");
    fprintf(stderr, "    out_file=  : encrypt/decrypt output data filepath, default \"in_file\" + _out\n");
    fprintf(stderr, "    Example: %s encrypt ukey=0123456789abcdef in_file=/usr/data/in\n", prg_name);
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

static int parse_ustr(const char *src, const char *prefix,
                    unsigned char *dest, int base)
{
    int len = strlen(prefix);

    if (strncmp(src, prefix, len) || ((strlen(src) - len) >= base))
        return 0;

    memmove(dest, src + len, base);

    return 1;
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

static inline int aes_get_key_len(int bits)
{
    switch (bits)
    {
    case 128:
        return AES128;
    case 192:
        return AES192;
    case 256:
        return AES256;
    default:
        fprintf(stderr, "aes: get key len failed! bits: %d\n", bits);
        break;
    }
    return -1;
}

static inline unsigned int aes_get_key_bits(enum aes_keyl keyl)
{
    switch (keyl)
    {
    case AES128:
        return 128;
    case AES192:
        return 192;
    case AES256:
        return 256;
    default:
        fprintf(stderr, "aes: get key bits failed! keyl: %d\n", keyl);
        break;
    }
    return -1;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int cmd = -1;
    prg_name = argv[0];

    unsigned int bits = 128;
    unsigned char mode_buf[4] = {'e', 'c', 'b'};
    unsigned char endian_buf[8] = {'b', 'i', 'g'};
    unsigned char *in_file = NULL;
    unsigned char *out_file = NULL;
    unsigned char is_malloc = 0;

    struct aes_config config;
    memset(&config, 0, sizeof(struct aes_config));
    config.keyl = AES128;
    config.mode = ECB_MODE;
    config.endian = ENDIAN_BIG;

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

    int i;
    for (i = 2; i < argc; i++) {
        if (parse_uint(argv[i], "bits=", &bits, 10)) {
            config.keyl = aes_get_key_len(bits);
            if (config.keyl == -1) {
                fprintf(stderr, "[err] bits should be 128/192/256\n");
                return -1;
            }
            continue;
        }

        if (parse_ustr(argv[i], "mode=", mode_buf, 4)) {
            if (strcmp(mode_buf, "ecb") == 0)
                config.mode = ECB_MODE;
            else if (strcmp(mode_buf, "cbc") == 0)
                config.mode = CBC_MODE;
            else {
                fprintf(stderr, "[err] mode should be ecb/cbc\n");
                return -1;
            }
            continue;
        }

        if (parse_ustr(argv[i], "endian=", endian_buf, 8)) {
            if (strcmp(endian_buf, "little") == 0)
                config.endian = ENDIAN_LITTLE;
            else if (strcmp(endian_buf, "big") == 0)
                config.endian = ENDIAN_BIG;
            else {
                fprintf(stderr, "[err] endian should be little/big\n");
                return -1;
            }
            continue;
        }

        if (parse_ustr(argv[i], "ukey=", config.ukey, 33))
            continue;

        if (parse_ustr(argv[i], "iv=", config.iv, 17))
            continue;

        if (strncmp(argv[i], "in_file=", strlen("in_file=")) == 0) {
            in_file = argv[i] + strlen("in_file=");
            continue;
        }

        if (strncmp(argv[i], "out_file=", strlen("out_file=")) == 0) {
            out_file = argv[i] + strlen("out_file=");
            continue;
        }

        error_arg(argv[i]);
    }

    if (strlen(config.ukey) == 0 || in_file == NULL) {
        fprintf(stderr, "[err] ukey & in_file must be set!\n");
        return -1;
    }

    if (out_file == NULL) {
        out_file = malloc(strlen(in_file) + 4 + 1);
        memset(out_file, 0, strlen(in_file) + 4 + 1);
        sprintf(out_file, "%s_out", in_file);
        is_malloc = 1;
    }

    if (strlen(config.iv) == 0 && config.mode == CBC_MODE) {
        fprintf(stderr, "[err] when cbc mode iv must be set!\n");
        return -1;
    }

    memset(config.ukey + strlen(config.ukey), 0, sizeof(config.ukey) - strlen(config.ukey));
    memset(config.iv + strlen(config.iv), 0, sizeof(config.iv) - strlen(config.iv));

    while (strlen(config.ukey) > (bits / 8) && config.keyl != AES256) {
        config.keyl++;
        bits = aes_get_key_bits(config.keyl);
        fprintf(stderr, "AES: ukey is too large than bits, change bits to %d\n", bits);
    }

    fprintf(stderr, "bits       : %d\n", bits);
    fprintf(stderr, "mode       : %s\n", mode_buf);
    fprintf(stderr, "endian     : %s\n", endian_buf);
    if (config.mode == CBC_MODE)
        fprintf(stderr, "iv         : %s\n", config.iv);
    fprintf(stderr, "ukey       : %s\n", config.ukey);
    fprintf(stderr, "in_file    : %s\n", in_file);
    fprintf(stderr, "out_file   : %s\n", out_file);

    struct aes_handle *handle = aes_open(&config);
    if (handle == NULL) {
        fprintf(stderr, "AES: open dev failed!\n");
        goto close_aes;
    }

    if (cmd == cmd_encrypt) {
        ret = aes_encrypt_file(handle, in_file, out_file);
        if (ret < 0)
            fprintf(stderr, "AES: aes_encrypt_file failed!\n");
    } else if (cmd == cmd_decrypt) {
        ret = aes_decrypt_file(handle, in_file, out_file);
        if (ret < 0)
            fprintf(stderr, "AES: aes_decrypt_file failed!\n");
    }

close_aes:
    if (is_malloc)
        free(out_file);

    aes_close(handle);

    return ret;
}
