#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libutils2/nalu_buf.h>
#include <libutils2/h264_bs.h>

struct nalu *nalu_create(void *data, int size)
{
    struct nalu *nalu = malloc(sizeof(*nalu)+size);
    if (!nalu) {
        printf("nalu: failed to create nalu: %d\n", size);
        return NULL;
    }

    nalu->data = (void *)&nalu[1];
    nalu->size = size;
    memcpy(nalu->data, data, size);
    return nalu;
}

void nalu_delete(struct nalu *nalu)
{
    free(nalu);
}

void nalu_print(struct nalu *nalu)
{
    printf("%s: %x, %d\n", nalu->data[2] == 0x00 ? "0x00000001" : "0x000001",
                nalu->data[4], nalu->size);
}

#define MAX_NULA_CNT 16

struct nalu_buf {
    unsigned char *data;
    int size;
    int off;
    int buf_size;
    int zero_cnt;
    int is_nalu;

    struct nalu *array[MAX_NULA_CNT];
    int nalu_cnt;
    int is_keyframe;
    int is_frame;
};

struct nalu_buf *nalu_buf_init(int buf_size)
{
    struct nalu_buf *buf = malloc(sizeof(*buf)+buf_size+4);
    if (!buf) {
        printf("failed to alloc nalu buffer: %d\n", buf_size);
        return NULL;
    }

    memset(buf, 0, sizeof(*buf));
    buf->data = (unsigned char *)&buf[1] + 4;
    buf->buf_size = buf_size;
    buf->data[-1] = 0xff;
    buf->data[-2] = 0xff;
    buf->data[-3] = 0xff;
    buf->data[-4] = 0xff;

    return buf;
}

static void delete_nalus(struct nalu_buf *buf);

void nalu_buf_deinit(struct nalu_buf *buf)
{
    buf->data = NULL;
    delete_nalus(buf);
    free(buf);
}

static inline int is_0x000001(unsigned char *d)
{
    return d[0]==0x01 && d[-1]==0x00 && d[-2]==0x00;
}

static inline int is_0x00000300(unsigned char *d)
{
    return d[0]==0x00 && d[-1]==0x03 && d[-2]==0x00 && d[-3]==0x00;
}

static inline void set_nalu_start_code(unsigned char *d, int zero_cnt)
{
    memset(d, 0, zero_cnt);
    d[zero_cnt] = 0x01;
}

struct nalu *nalu_buf_write2(struct nalu_buf *buf, unsigned char *data, int len, int *len_p)
{
    unsigned char *s = data;
    unsigned char *d = buf->data+buf->off;

    if (!data || !len) {
        struct nalu *nalu = NULL;
        if (buf->is_nalu) {
            int size = buf->off;
            nalu = nalu_create(buf->data, size);
            buf->off = 0;
            buf->is_nalu = 0;
        }
        return nalu;
    }

    int i;
    for (i = 0; i < len; i++, d++) {
        if (d-buf->data >= buf->buf_size) {
            printf("nalu: drop invalid data: %d\n", buf->buf_size);
            memmove(buf->data, d-3, 4);
            d = buf->data;
            buf->is_nalu = 0;
        }

        d[0] = s[i];
        if (!is_0x000001(d))
            continue;

        int sz = 2;
        if (d[-3] == 0x00 && !is_0x00000300(&d[-3]))
            sz = 3;

        if (!buf->is_nalu) {
            buf->is_nalu = 1;
            set_nalu_start_code(buf->data, sz);
            d = buf->data + sz;
            continue;
        }

        int size = d-buf->data-sz;
        struct nalu *nalu = nalu_create(buf->data, size);

        set_nalu_start_code(buf->data, sz);
        buf->off = sz + 1;
        *len_p = i+1;
        return nalu;
    }

    *len_p = len;
    buf->off = d - buf->data;

    return NULL;
}

struct nalu_frame *nalu_frame_create(int size)
{
    struct nalu_frame *frame = malloc(sizeof(*frame)+size);
    if (!frame) {
        printf("nalu: failed to create frame: %d\n", size);
        return NULL;
    }
    frame->data = (void *)&frame[1];
    frame->size = size;
    frame->is_keyframe = 0;
    frame->nalu_cnt = 0;
    return frame;
}

void nalu_frame_delete(struct nalu_frame *frame)
{
    free(frame);
}

void nalu_frame_print(struct nalu_frame *f)
{
    printf("%s: %x, %d, %d\n", f->data[2] == 0x00 ? "0x00000001" : "0x000001",
                f->data[4], f->size, f->nalu_cnt);
}

static void reset_frame_status(struct nalu_buf *buf)
{
    buf->is_frame = 0;
    buf->is_keyframe = 0;
    buf->nalu_cnt = 0;
}

static void delete_nalus(struct nalu_buf *buf)
{
    int i;
    for (i = 0; i < buf->nalu_cnt; i++) {
        nalu_delete(buf->array[i]);
    }
    reset_frame_status(buf);
}

static struct nalu_frame *create_nalu_frame(struct nalu_buf *buf)
{
    int i, size = 0;

    for (i = 0; i < buf->nalu_cnt; i++)
        size += buf->array[i]->size;

    struct nalu_frame *frame = nalu_frame_create(size);
    if (!frame) {
        delete_nalus(buf);
        return NULL;
    }

    frame->is_keyframe = buf->is_keyframe;
    frame->nalu_cnt = buf->nalu_cnt;

    int n = 0;
    for (i = 0; i < buf->nalu_cnt; i++) {
        memcpy(frame->data+n, buf->array[i]->data, buf->array[i]->size);
        n += buf->array[i]->size;
        nalu_delete(buf->array[i]);
    }

    reset_frame_status(buf);

    return frame;
}

static struct nalu_frame *check_frame(struct nalu_buf *buf, struct nalu *nalu)
{
    struct nalu_frame *frame = NULL;

    bs_t bs;
    bs_init(&bs, nalu_bs_data(nalu), nalu_bs_data_size(nalu));

    int first_mb_in_slice = -1, slice_type = -1;
    unsigned char type = nalu_header(nalu) & 0x1f;
    if (type == 0x05 || type == 0x01) {
        first_mb_in_slice = bs_read_ue(&bs);
        slice_type = bs_read_ue(&bs);
        if (first_mb_in_slice == 0) {
            if (buf->is_frame)
                frame = create_nalu_frame(buf);

            buf->is_frame = 1;
            buf->is_keyframe = (type == 0x05);
        }
    }

    if (0)
    printf("slice: %x %d %d\n", type, first_mb_in_slice, slice_type);

    return frame;
}

static struct nalu_frame *get_nalu_frame(struct nalu_buf *buf, struct nalu *nalu)
{
    struct nalu_frame *frame = NULL;

    if (!nalu) {
        if (buf->is_frame) {
            frame = create_nalu_frame(buf);
            return frame;
        } else {
            if (buf->nalu_cnt) {
                printf("nalu buf: drop last no frame nuls\n");
                delete_nalus(buf);
            }
            return NULL;
        }
    }

    frame = check_frame(buf, nalu);

    if (buf->nalu_cnt == MAX_NULA_CNT) {
        printf("nalu buf: too many nalu, clear it\n");
        delete_nalus(buf);
    }

    buf->array[buf->nalu_cnt] = nalu;
    buf->nalu_cnt++;

    return frame;
}

struct nalu_frame *nalu_buf_write(
    struct nalu_buf *buf, unsigned char *data, int len, int *len_p)
{
    struct nalu *nalu;
    struct nalu_frame *frame = NULL;

    if (!data || !len) {
        nalu = nalu_buf_write2(buf, data, len, len_p);
        frame = get_nalu_frame(buf, nalu);
        if (!frame && nalu)
            frame = get_nalu_frame(buf, NULL);
        return frame;
    }

    int n;
    for (n = 0; n < len; ) {
        int sz = len - n;
        nalu = nalu_buf_write2(buf, data, sz, &sz);
        n += sz;
        data += sz;
        if (!nalu)
            break;
        frame = get_nalu_frame(buf, nalu);
        if (frame)
            break;
    }

    *len_p = n;

    return frame;
}