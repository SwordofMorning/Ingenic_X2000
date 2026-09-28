#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "libutils2/boot_time.h"

#define USE_PIX_BILINEAR

#define ROTATE_BITS 8
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 16
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 24
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 32
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS


#undef USE_PIX_BILINEAR

#define ROTATE_BITS 8
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 16
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 24
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 32
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

int file_write_data(const char *out_file, void *data, unsigned long data_size)
{
    int ret = 0;
    FILE *file = fopen(out_file, "w");
    if (!file) {
        fprintf(stderr, "file_utils: failed to open file: %s (%s)\n",
            out_file, strerror(errno));
        return -1;
    }

    int N = 1024*1024;

    while (data_size) {
        int size = data_size < N ? data_size : N;
        ret = fwrite(data, 1, size, file);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to write file: %s (%s) %d\n",
                out_file, strerror(errno), size);
            goto close_file;
        }

        data_size -= ret;
        data += ret;
    }

    ret = 0;
    fflush(file);

close_file:
    fclose(file);

    return ret;
}

char *prg_name;

void usage(int status)
{
    printf("usage of %s\n", prg_name);
    printf("    \n");
    printf("    sw=width    set src width, default 1920\n");
    printf("    sh=height   set src height, default 1080\n");
    printf("    sx=x        set src rotate point x, default sw/2\n");
    printf("    sy=y        set src rotate point y, default sh/2\n");
    printf("    \n");
    printf("    dw=width    set dst width, default 1920\n");
    printf("    dh=height   set dst height, default 1080\n");
    printf("    dx=x        set rotate result's center point on dst x, default dw/2\n");
    printf("    dy=y        set rotate result's center point on dst y, default dh/2\n");
    printf("    \n");
    printf("    angle=n     the rotate angle,rotate clockwise\n");
    printf("    bits=N      N=[8,16,32] 8:grey  16:rgb565 24:bgrx888x 32:bgra8888 to define the rotate word size\n");
    printf("    type=n          n=[0,1,2,3,4] specify the rotate alg version, default 2\n");
    printf("    bilinear=1/0    1: use bilinear 0: not use  if use the effect will be better, but slow, default 1\n");
    printf("    border=1/0      1: use bilinear for border 0: not use  default 1\n");
    printf("    count=n     call n times to calculate the rotate alg time \n");
    printf("    save_file=1/0   1: save the roate result  0: not save  the result is raw data, default 1\n");
    printf("    find_fast=1/0   1: find the fast rotate alg  0:not do that, default 0\n");
    printf("    \n");
    printf("    example:\n");
    printf("        # find the fast alg when rotate 1920x200 src img to 1920x1080 dst img (bgra)\n");
    printf("        %s sw=1920 sh=200 dw=1920 dh=1080 bits=32 find_fast=1\n", prg_name);
    printf("        \n");
    printf("        # see the result rotate 30 degree 1920x200 src img to 1920x1080 dst img (bgra)\n");
    printf("        %s sw=1920 sh=200 dw=1920 dh=1080 bits=32 save_file=1 angle=30\n", prg_name);
    printf("        \n");
    printf("        # we can specify the rotate src center to (900,100)\n");
    printf("        %s sw=1000 sh=200 sx=900 sy=100 dw=1920 dh=1080 bits=32 save_file=1 angle=30\n", prg_name);
    printf("        # if slow we can disable bilinear alg \n");
    printf("        %s sw=1000 sh=200 dw=1920 dh=1080 bits=32 bilinear=0 find_fast=1\n", prg_name);
    printf("    \n");

    exit(status);
}

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

static int parse_int(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtol(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static void sort_up_index(int *v, int *index, int len)
{
    int i;
    int j;
    for (j = 0; j < len; j++)
        index[j] = j;

    for (j = 0; j < len-1; j++) {
        int k = j;
        for (i = j+1; i < len; i++) {
            if (v[k] > v[i])
                k = i;
        }
        if (k != j) {
            int tmp = v[j];
            v[j] = v[k];
            v[k] = tmp;
            tmp = index[j];
            index[j] = index[k];
            index[k] = tmp;
        }
    }
}

void clear_to_color(uint8_t *dst, int w, int h, int bits, int color)
{
    int i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            if (bits == 8)
                dst[j*w+i] = color&0xff;
            if (bits == 16)
                ((uint16_t *)dst)[j*w+i] = color&0xffff;
            if (bits == 24)
                ((uint32_t *)dst)[j*w+i] = color;
            if (bits == 32)
                ((uint32_t *)dst)[j*w+i] = color;
        }
    }
}

static inline int to_bytes(int bits)
{
    return bits == 24 ? 4 : bits / 8;
}

typedef void (*rotate_func_t)(
    void *s_, int s_w, int s_h, int s_x, int s_y, int s_linesz,
    void *d_, int d_w, int d_h, int d_x, int d_y, int d_linesz,
    float angle, int border_bilinear,
    int rotate_version, rotate_get_fast_version_t get_fast_version);

uint32_t test_rotate(void *s, int s_w, int s_h, int s_x, int s_y, int s_linesz,
    void *d, void *d2, int d_w, int d_h, int d_x, int d_y, int d_linesz,
    float angle, int version, int bits, int bilinear, int border_blinear)
{
    uint64_t start = boot_time_usecs();

    int n_w = d_w, n_h = d_h, n_x = d_x, n_y = d_y;

    rotate_cal_dst_area(s_w, s_h, s_x, s_y, d_w, d_h, d_x, d_y, angle,
        &n_w, &n_h, &n_x, &n_y);

    rotate_func_t rotate_func = NULL;

    if (n_w && n_h) {
        if (bilinear) {
            if (bits == 8)
                rotate_func = rotate_func_uint8_bilinear;
            if (bits == 16)
                rotate_func = rotate_func_uint16_bilinear;
            if (bits == 24)
                rotate_func = rotate_func_uint24_bilinear;
            if (bits == 32)
                rotate_func = rotate_func_uint32_bilinear;
        } else {
            if (bits == 8)
                rotate_func = rotate_func_uint8;
            if (bits == 16)
                rotate_func = rotate_func_uint16;
            if (bits == 24)
                rotate_func = rotate_func_uint24;
            if (bits == 32)
                rotate_func = rotate_func_uint32;
        }
    }

    rotate_func((void*)s, s_w, s_h, s_x, s_y, s_w*to_bytes(bits),
    (void *)d2, n_w, n_h, n_x, n_y, n_w*to_bytes(bits), angle, border_blinear, version, NULL);

    uint64_t end = boot_time_usecs();

    int i;
    int dx0 = d_x - n_x;
    int dy0 = d_y - n_y;
    d += (dy0*d_w+dx0)*to_bytes(bits);
    for (i = 0; i < n_h; i++) {
        memcpy(d, d2, n_w*to_bytes(bits));
        d += d_w*to_bytes(bits);
        d2 += n_w*to_bytes(bits);
    }

    return end-start;
}

int can_print = 0;
void *old_s = NULL;
int old_w, old_h, old_lw;
uint32_t old_x, old_y;

int main(int argc, char *argv[])
{
    int s_w = 1920, s_h = 1080;
    int d_w = 1920, d_h = 1080;
    int s_x, s_y, d_x, d_y;
    int flag = 0;
    int version = 2;
    int color = 0xffffffff;
    int src_bg = 0x0;
    int dst_bg = 0x0;
    int bits = 32;
    int count = 1;
    int angle = 30;
    int save_file = 1;
    int find_fast = 0;
    int bilinear = 1;
    int border_bilinear = 1;

    int i, j, k;

    prg_name = argv[0];

    for (i = 1; i < argc; i++) {
        if (parse_int(argv[i], "sx=", &s_x, 10)) {
            flag |= (1<<0); continue;
        }
        if (parse_int(argv[i], "sy=", &s_y, 10)) {
            flag |= (1<<1); continue;
        }
        if (parse_int(argv[i], "dx=", &d_x, 10)) {
            flag |= (1<<2); continue;
        }
        if (parse_int(argv[i], "dy=", &d_y, 10)) {
            flag |= (1<<3); continue;
        }

        if (parse_uint(argv[i], "sw=", &s_w, 10))
            continue;
        if (parse_uint(argv[i], "sh=", &s_h, 10))
            continue;
        if (parse_uint(argv[i], "dw=", &d_w, 10))
            continue;
        if (parse_uint(argv[i], "dh=", &d_h, 10))
            continue;

        if (parse_uint(argv[i], "type=", &version, 10))
            continue;
        if (parse_uint(argv[i], "bilinear=", &bilinear, 10))
            continue;
        if (parse_uint(argv[i], "border=", &border_bilinear, 10))
            continue;
        if (parse_uint(argv[i], "bits=", &bits, 10))
            continue;
        if (parse_uint(argv[i], "count=", &count, 10))
            continue;
        if (parse_uint(argv[i], "angle=", &angle, 10))
            continue;
        if (parse_uint(argv[i], "save_file=", &save_file, 10))
            continue;
        if (parse_uint(argv[i], "find_fast=", &find_fast, 10))
            continue;

        if (strcmp("-h", argv[i]) || strcmp("--help", argv[i]))
            usage(0);

        fprintf(stderr, "not support this arg: %s\n", argv[i]);
        exit(-1);
    }

    if (!(bits == 8 || bits == 16 || bits == 24 || bits == 32)) {
        fprintf(stderr, "not support this bits: %d\n", bits);
        exit(-1);
    }

    if (!(flag & (1<<0))) s_x = s_w/2;
    if (!(flag & (1<<1))) s_y = s_h/2;
    if (!(flag & (1<<2))) d_x = d_w/2;
    if (!(flag & (1<<3))) d_y = d_h/2;

    uint8_t *src = malloc(s_w*s_h*(to_bytes(bits)));
    assert(src);

    uint8_t *dst = malloc(d_w*d_h*(to_bytes(bits)));
    assert(dst);

    uint8_t *dst2 = malloc(d_w*d_h*(to_bytes(bits)));
    assert(dst2);

    memset(src, 0x11, s_w*s_h*(to_bytes(bits)));
    memset(dst, 0x11, d_w*d_h*(to_bytes(bits)));

    for (j = 0; j < s_h; j++) {
        for (i = 0; i < s_w; i++) {
            if (j%10 < 5 || j > (s_h-3)) {
                if (bits == 8)
                    src[j*s_w+i] = color&0xff;
                if (bits == 16)
                    ((uint16_t *)src)[j*s_w+i] = color&0xffff;
                if (bits == 24)
                    ((uint32_t *)src)[j*s_w+i] = color;
                if (bits == 32)
                    ((uint32_t *)src)[j*s_w+i] = color;
            }
            else {
                if (bits == 8)
                    src[j*s_w+i] = src_bg&0xff;
                if (bits == 16)
                    ((uint16_t *)src)[j*s_w+i] = src_bg&0xffff;
                if (bits == 24)
                    ((uint32_t *)src)[j*s_w+i] = src_bg;
                if (bits == 32)
                    ((uint32_t *)src)[j*s_w+i] = src_bg;
            }
        }
    }

    uint64_t total = 0;

    if (find_fast) {
        int times[362][5];
        printf("rotate%d: find fast: %dx%d(%d,%d) to %dx%d(%d,%d)\n",
        bits, s_w,s_h, s_x,s_y, d_w,d_h, d_x,d_y);

        uint64_t t = 0;
        for (j = 0; j < 5; j++) {
            total = 0;
            printf("creating version:%d time..... please wait\n", j);
            for (i = 0; i < 361; i++) {
                t = 0;
                for (k = 0; k < count; k++) {
                    clear_to_color(dst, d_w, d_h, bits, dst_bg);
                    can_print = 1;
                    t += test_rotate(src, s_w, s_h, s_x, s_y, s_w*to_bytes(bits),
                        dst, dst2, d_w, d_h, d_x, d_y, d_w*to_bytes(bits), i, j, bits,
                         bilinear, border_bilinear);
                    can_print = 0;
                }
                times[i][j] = t/count;
                total += t/count;
                printf("rotate:%d: %03d: %.3f\n", j, i, times[i][j]/1000.0);
            }
            times[361][j] = total/360;
        }

        int counts[5] = {0};
        for (i = 0; i < 362; i++) {
            int v[5], in[5];
            memcpy(v, times[i], 5*sizeof(int));
            sort_up_index(v, in, 5);
            counts[in[0]]++;
            if (i == 361)
                printf("total: v%d is %d%% fast     (%.3f  %.3f  %.3f  %.3f  %.3f)\n",
                 in[0], v[4]*100/v[0], times[i][0]/1000.0, times[i][1]/1000.0,
                 times[i][2]/1000.0, times[i][3]/1000.0, times[i][4]/1000.0);
            else
                printf("[%03d]: v%d is %d%% fast    (%.3f  %.3f  %.3f  %.3f  %.3f)\n",
                i, in[0], v[4]*100/v[0], times[i][0]/1000.0, times[i][1]/1000.0,
                 times[i][2]/1000.0, times[i][3]/1000.0, times[i][4]/1000.0);
        }

        int in[5];
        sort_up_index(counts, in, 5);
        int max_j = in[4];

        printf("static int get_fast_version_%dx%d_to_%dx%d_%dbits%s(int angle)\n{\n",
             s_w, s_h, d_w, d_h, bits, bilinear ? "bilinear" : "");
        printf("switch(angle) {\n");
        for (j = 0; j < 5; j++) {
            int is_print = 0;
            if (max_j == j)
                continue;
            int count = 0;
            for (i = 0; i < 361; i++) {
                int v[5], in[5];
                memcpy(v, times[i], 5*sizeof(int));
                sort_up_index(v, in, 5);
                if (in[0] == j) {
                    if (++count == 8) {
                        count = 0;
                        printf("\n");
                    }
                    printf("case %d: ", i);
                    is_print = 1;
                }
            }
            if (is_print) {
                printf("\n    return %d;\n", j);
            }
        }
        printf("default:\n    return %d;\n}\n}\n", max_j);

        goto free_mem;
    }

    for (i = 0; i < count; i++) {
        clear_to_color(dst, d_w, d_h, bits, dst_bg);

        total += test_rotate(src, s_w, s_h, s_x, s_y, s_w*to_bytes(bits),
            dst, dst2, d_w, d_h, d_x, d_y, d_w*to_bytes(bits), angle, version, bits,
            bilinear, border_bilinear);
    }

    total = total/count;
    printf("rotate%d: angle:%d %dx%d(%d,%d) to %dx%d(%d,%d) time %d.%03d\n",
            bits, angle, s_w,s_h, s_x,s_y, d_w,d_h, d_x,d_y, (int)(total/1000), (int)(total%1000));

    char buf[32];
    sprintf(buf, "%03d.rgb", angle);

    if (save_file)
    file_write_data(buf, dst, d_w*d_h*to_bytes(bits));
free_mem:
    free(src);
    free(dst);

    return 0;
}