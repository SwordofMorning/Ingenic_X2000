#ifndef _NALU_BUF_H_
#define _NALU_BUF_H_

struct nalu {
    unsigned char *data;
    int size;
};

static inline unsigned int nalu_start_code_size(struct nalu *nalu)
{
    return nalu->data[2] == 0x01 ? 3 : 4;
}

static inline unsigned char nalu_header(struct nalu *nalu)
{
    int n = nalu_start_code_size(nalu);
    return nalu->data[n];
}

static inline unsigned char *nalu_bs_data(struct nalu *nalu)
{
    int n = nalu_start_code_size(nalu) + 1;
    return nalu->data + n;
}

static inline int nalu_bs_data_size(struct nalu *nalu)
{
    int n = nalu_start_code_size(nalu) + 1;
    return nalu->size - n;
}

struct nalu *nalu_create(void *data, int size);

void nalu_delete(struct nalu *nalu);

void nalu_print(struct nalu *nalu);

struct nalu_frame {
    unsigned char *data;
    int size;
    int nalu_cnt;
    int is_keyframe;
};
struct nalu_frame *nalu_frame_create(int size);

void nalu_frame_delete(struct nalu_frame *frame);

void nalu_frame_print(struct nalu_frame *f);

struct nalu_buf;

struct nalu_buf *nalu_buf_init(int buf_size);

void nalu_buf_deinit(struct nalu_buf *buf);

struct nalu_frame *nalu_buf_write(
    struct nalu_buf *buf, unsigned char *data, int len, int *len_p);

struct nalu *nalu_buf_write2(
    struct nalu_buf *buf, unsigned char *data, int len, int *len_p);

#endif /* _NALU_BUF_H_ */
