#include <libmedia/video_frame.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN_DOWN(X, n) ((X) - ((X)%(n)))

void convert_bgr_to_nv12(void *src, int src_linesize, void *Y, int Y_linesize,
                         void *UV, int UV_linesize, int width, int height);

void convert_nv12_to_bgra(void *y, int y_linesize, void *uv, int uv_linesize,
                          void *dst, int dst_linesize, int width, int height);

static void copy_raw_bytes(void *dst, int dst_linesize, void *src, int src_linesize, int height, int width)
{
    if (dst == src && dst_linesize == src_linesize)
        return;

    if (dst_linesize == src_linesize && width == dst_linesize) {
        memcpy(dst, src, width*height);
        return;
    }

    int i;
    for (i = 0; i < height; i++) {
        memcpy(dst, src, width);
        src += src_linesize;
        dst += dst_linesize;
    }
}

static void fill_zero_nv12(void *dst, int line_size, int width, int height)
{
    if (width == line_size) {
        memset(dst, 0x80, width*height/2);
        return;
    }

    int i;
    for (i = 0; i < height / 2; i++) {
        memset(dst, 0x80, width);
        dst += line_size;
    }
}

static int bgra_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                        uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    convert_bgr_to_nv12(s_data[0], s_linesize[0],
                        d_data[0], d_linesize[0],
                        d_data[1], d_linesize[1], width, height);

    return 0;
}

static int nv12_to_bgra(uint8_t **s_data, uint32_t *s_linesize,
                        uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    convert_nv12_to_bgra(s_data[0], s_linesize[0],
                         s_data[1], s_linesize[1],
                         d_data[0], d_linesize[0], width, height);

    return 0;
}

static int y8_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                      uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    void *src = s_data[0];
    int src_linesize = s_linesize[0];

    copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    dst = d_data[1];
    dst_linesize = d_linesize[1];

    fill_zero_nv12(dst, dst_linesize, width, height);

    return 0;
}

static int y16_to_y8(uint8_t **s_data, uint32_t *s_linesize,
                     uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    int i,j;

    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    uint8_t *src = s_data[0];
    int src_linesize = s_linesize[0];

    for (i = 0; i < height; i++) {
        for (j = 0; j < ALIGN_DOWN(width, 8); j+=8) {
            unsigned char s0 = src[j*2+0*2];
            unsigned char s1 = src[j*2+1*2];
            unsigned char s2 = src[j*2+2*2];
            unsigned char s3 = src[j*2+3*2];
            unsigned char s4 = src[j*2+4*2];
            unsigned char s5 = src[j*2+5*2];
            unsigned char s6 = src[j*2+6*2];
            unsigned char s7 = src[j*2+7*2];
            dst[j+0] = s0;
            dst[j+1] = s1;
            dst[j+2] = s2;
            dst[j+3] = s3;
            dst[j+4] = s4;
            dst[j+5] = s5;
            dst[j+6] = s6;
            dst[j+7] = s7;
        }
        for (; j < width; j++) {
            dst[j] = src[j*2];
        }
        src += src_linesize;
        dst += dst_linesize;
    }

    return 0;
}

static int y16_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                       uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    y16_to_y8(s_data, s_linesize, d_data, d_linesize, width, height);

    void *dst = d_data[1];
    int dst_linesize = d_linesize[1];

    fill_zero_nv12(dst, dst_linesize, width, height);

    return 0;
}


static int yuv420p_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                           uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    int i,j;

    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    void *src = s_data[0];
    int src_linesize = s_linesize[0];

    copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    dst = d_data[1];
    dst_linesize = d_linesize[1];

    uint8_t *u = s_data[1];
    uint8_t *v = s_data[2];
    src_linesize = s_linesize[1];

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width/2, 4); j+=4) {
            uint8_t a0 = u[j+0];
            uint8_t a1 = u[j+1];
            uint8_t a2 = u[j+2];
            uint8_t a3 = u[j+3];
            uint8_t b0 = v[j+0];
            uint8_t b1 = v[j+1];
            uint8_t b2 = v[j+2];
            uint8_t b3 = v[j+3];

            dst[j*2+0] = a0;
            dst[j*2+1] = b0;
            dst[j*2+2] = a1;
            dst[j*2+3] = b1;
            dst[j*2+4] = a2;
            dst[j*2+5] = b2;
            dst[j*2+6] = a3;
            dst[j*2+7] = b3;
        }
        for (; j < width/2; j++) {
            dst[j*2+0] = u[j];
            dst[j*2+1] = v[j];
        }
        v += src_linesize;
        u += src_linesize;
        dst += dst_linesize;
    }

    return 0;
}

static int yuv422p_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                           uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    int i,j;

    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    void *src = s_data[0];
    int src_linesize = s_linesize[0];

    copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    dst_linesize = d_linesize[1];

    src_linesize = s_linesize[1];
    uint8_t *u0 = s_data[1];
    uint8_t *u1 = s_data[1] + src_linesize;
    uint8_t *uv = d_data[1];

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width/2, 4); j+=4) {
            uint8_t a0 = u0[j+0];
            uint8_t a1 = u0[j+1];
            uint8_t a2 = u0[j+2];
            uint8_t a3 = u0[j+3];
            uint8_t b0 = u1[j+0];
            uint8_t b1 = u1[j+1];
            uint8_t b2 = u1[j+2];
            uint8_t b3 = u1[j+3];

            uv[j*2+0] = ((unsigned short)a0 + b0)/2;
            uv[j*2+2] = ((unsigned short)a1 + b1)/2;
            uv[j*2+4] = ((unsigned short)a2 + b2)/2;
            uv[j*2+6] = ((unsigned short)a3 + b3)/2;
        }
        for (; j < width/2; j++)
            uv[j*2] = ((unsigned short)u0[j] + u1[j])/2;

        u0 += src_linesize;
        u1 += src_linesize;
        uv += dst_linesize;
    }

    src_linesize = s_linesize[2];
    uint8_t *v0 = s_data[2];
    uint8_t *v1 = s_data[2] + src_linesize;
    uv = d_data[1] + 1;

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width/2, 4); j+=4) {
            uint8_t a0 = v0[j+0];
            uint8_t a1 = v0[j+1];
            uint8_t a2 = v0[j+2];
            uint8_t a3 = v0[j+3];
            uint8_t b0 = v1[j+0];
            uint8_t b1 = v1[j+1];
            uint8_t b2 = v1[j+2];
            uint8_t b3 = v1[j+3];

            uv[j*2+0] = ((unsigned short)a0 + b0)/2;
            uv[j*2+2] = ((unsigned short)a1 + b1)/2;
            uv[j*2+4] = ((unsigned short)a2 + b2)/2;
            uv[j*2+6] = ((unsigned short)a3 + b3)/2;
        }
        for (; j < width/2; j++)
            uv[j*2] = ((unsigned short)v0[j] + v1[j])/2;

        v0 += src_linesize;
        v1 += src_linesize;
        uv += dst_linesize;
    }

    return 0;
}

static int yuv444p_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                           uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    int i,j;

    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    void *src = s_data[0];
    int src_linesize = s_linesize[0];

    copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    dst_linesize = d_linesize[1];

    src_linesize = s_linesize[1];
    uint8_t *u0 = s_data[1];
    uint8_t *u1 = s_data[1] + src_linesize;
    uint8_t *uv = d_data[1];

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width, 4); j+=4) {
            uint8_t a0 = u0[j+0];
            uint8_t a1 = u0[j+1];
            uint8_t a2 = u0[j+2];
            uint8_t a3 = u0[j+3];
            uint8_t b0 = u1[j+0];
            uint8_t b1 = u1[j+1];
            uint8_t b2 = u1[j+2];
            uint8_t b3 = u1[j+3];

            uv[j+0] = ((unsigned short)a0 + a1 + b0 + b1)/2;
            uv[j+2] = ((unsigned short)a2 + a3 + b2 + b3)/2;
        }
        for (; j < width; j+=2) {
            uint8_t a0 = u0[j+0];
            uint8_t a1 = u0[j+1];
            uint8_t b0 = u1[j+0];
            uint8_t b1 = u1[j+1];

            uv[j+0] = ((unsigned short)a0 + a1 + b0 + b1)/2;
        }

        u0 += src_linesize;
        u1 += src_linesize;
        uv += dst_linesize;
    }

    src_linesize = s_linesize[1];
    uint8_t *v0 = s_data[2];
    uint8_t *v1 = s_data[2] + src_linesize;
    uv = d_data[1] + 1;

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width, 4); j+=4) {
            uint8_t a0 = v0[j+0];
            uint8_t a1 = v0[j+1];
            uint8_t a2 = v0[j+2];
            uint8_t a3 = v0[j+3];
            uint8_t b0 = v1[j+0];
            uint8_t b1 = v1[j+1];
            uint8_t b2 = v1[j+2];
            uint8_t b3 = v1[j+3];

            uv[j+0] = ((unsigned short)a0 + a1 + b0 + b1)/2;
            uv[j+2] = ((unsigned short)a2 + a3 + b2 + b3)/2;
        }
        for (; j < width; j+=2) {
            uint8_t a0 = v0[j+0];
            uint8_t a1 = v0[j+1];
            uint8_t b0 = v1[j+0];
            uint8_t b1 = v1[j+1];

            uv[j+0] = ((unsigned short)a0 + a1 + b0 + b1)/2;
        }

        v0 += src_linesize;
        v1 += src_linesize;
        uv += dst_linesize;
    }

    return 0;
}

static int yuv411p_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                           uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    int i,j;

    uint8_t *dst = d_data[0];
    int dst_linesize = d_linesize[0];

    void *src = s_data[0];
    int src_linesize = s_linesize[0];

    copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    dst_linesize = d_linesize[1];

    src_linesize = s_linesize[1];
    uint8_t *u0 = s_data[1];
    uint8_t *u1 = s_data[1] + src_linesize;
    uint8_t *uv = d_data[1];

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width/4, 4); j+=2) {
            uint8_t a0 = u0[j+0];
            uint8_t a1 = u0[j+1];
            uint8_t b0 = u1[j+0];
            uint8_t b1 = u1[j+1];

            uv[j*4+0] = ((unsigned short)a0 + b0)/2;
            uv[j*4+2] = ((unsigned short)a0 + b0)/2;
            uv[j*4+4] = ((unsigned short)a1 + b1)/2;
            uv[j*4+6] = ((unsigned short)a1 + b1)/2;
        }
        for (; j < width/4; j++) {
            uint8_t a0 = u0[j+0];
            uint8_t b0 = u1[j+0];

            uv[j*4+0] = ((unsigned short)a0 + b0)/2;
            uv[j*4+2] = ((unsigned short)a0 + b0)/2;
        }

        u0 += src_linesize;
        u1 += src_linesize;
        uv += dst_linesize;
    }

    src_linesize = s_linesize[1];
    uint8_t *v0 = s_data[2];
    uint8_t *v1 = s_data[2] + src_linesize;
    uv = d_data[1] + 1;

    for (i = 0; i < height/2; i++) {
        for (j = 0; j < ALIGN_DOWN(width/4, 4); j+=2) {
            uint8_t a0 = v0[j+0];
            uint8_t a1 = v0[j+1];
            uint8_t b0 = v1[j+0];
            uint8_t b1 = v1[j+1];

            uv[j*4+0] = ((unsigned short)a0 + b0)/2;
            uv[j*4+2] = ((unsigned short)a0 + b0)/2;
            uv[j*4+4] = ((unsigned short)a1 + b1)/2;
            uv[j*4+6] = ((unsigned short)a1 + b1)/2;
        }
        for (; j < width/4; j++) {
            uint8_t a0 = v0[j+0];
            uint8_t b0 = v1[j+0];

            uv[j*4+0] = ((unsigned short)a0 + b0)/2;
            uv[j*4+2] = ((unsigned short)a0 + b0)/2;
        }

        v0 += src_linesize;
        v1 += src_linesize;
        uv += dst_linesize;
    }

    return 0;
}

int yuvxxxp_to_y8(uint8_t **s_data, uint32_t *s_linesize,
                  uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    uint8_t **src = s_data;
    uint8_t **dst = d_data;
    uint32_t *src_lsz = s_linesize;
    uint32_t *dst_lsz = d_linesize;

    copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);

    return 0;
}

int raw_frame_to_raw_frame(enum video_frame_format s_format, uint8_t **s_data, uint32_t *s_linesize,
                           enum video_frame_format d_format, uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    uint8_t **src = s_data;
    uint8_t **dst = d_data;
    uint32_t *src_lsz = s_linesize;
    uint32_t *dst_lsz = d_linesize;

    switch (s_format) {
    case VIDEO_nv12:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        copy_raw_bytes(dst[1], dst_lsz[1], src[1], src_lsz[1], height/2, width);
        break;
    case VIDEO_yuyv:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width*2);
        break;
    case VIDEO_yuv420p:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        copy_raw_bytes(dst[1], dst_lsz[1], src[1], src_lsz[1], height/2, width/2);
        copy_raw_bytes(dst[2], dst_lsz[2], src[2], src_lsz[2], height/2, width/2);
        break;
    case VIDEO_yuv422p:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        copy_raw_bytes(dst[1], dst_lsz[1], src[1], src_lsz[1], height, width/2);
        copy_raw_bytes(dst[2], dst_lsz[2], src[2], src_lsz[2], height, width/2);
        break;
    case VIDEO_yuv444p:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        copy_raw_bytes(dst[1], dst_lsz[1], src[1], src_lsz[1], height, width);
        copy_raw_bytes(dst[2], dst_lsz[2], src[2], src_lsz[2], height, width);
    case VIDEO_yuv411p:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        copy_raw_bytes(dst[1], dst_lsz[1], src[1], src_lsz[1], height, width/4);
        copy_raw_bytes(dst[2], dst_lsz[2], src[2], src_lsz[2], height, width/4);
        break;
    case VIDEO_bgra:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width*4);
        break;
    case VIDEO_y8:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width);
        break;
    case VIDEO_y16:
        copy_raw_bytes(dst[0], dst_lsz[0], src[0], src_lsz[0], height, width*2);
        break;
    default:
        fprintf(stderr, "video: can't support this format:%d\n", s_format);
        return -1;
    }

    return 0;
}

static int (*copy_func_list[][VIDEO_nums])(uint8_t **, uint32_t *, uint8_t **, uint32_t *, int, int) = {
    [VIDEO_yuv420p][VIDEO_nv12] = yuv420p_to_nv12,
    [VIDEO_yuva420p][VIDEO_nv12] = yuv420p_to_nv12,
    [VIDEO_yuvj420p][VIDEO_nv12] = yuv420p_to_nv12,
    [VIDEO_yuv422p][VIDEO_nv12] = yuv422p_to_nv12,
    [VIDEO_yuv444p][VIDEO_nv12] = yuv444p_to_nv12,
    [VIDEO_yuv411p][VIDEO_nv12] = yuv411p_to_nv12,
    [VIDEO_yuvj420p][VIDEO_nv12] = yuv420p_to_nv12,
    [VIDEO_yuvj422p][VIDEO_nv12] = yuv422p_to_nv12,
    [VIDEO_yuvj444p][VIDEO_nv12] = yuv444p_to_nv12,
    [VIDEO_y8][VIDEO_nv12] = y8_to_nv12,
    [VIDEO_y16][VIDEO_nv12] = y16_to_nv12,
    [VIDEO_bgra][VIDEO_nv12] = bgra_to_nv12,

    [VIDEO_yuv420p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuva420p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuv422p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuv444p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuv411p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuvj420p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuvj422p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_yuvj444p][VIDEO_y8] = yuvxxxp_to_y8,
    [VIDEO_y16][VIDEO_y8] = y16_to_y8,

    [VIDEO_nv12][VIDEO_bgra] = nv12_to_bgra,
};

int video_frame_copy(struct video_frame *src_frame, struct video_frame *dst_frame)
{
    if (!dst_frame || !dst_frame->data[0]) {
        fprintf(stderr, "dst_frame is null or dst_frame->data[0] is null\n");
        return -1;
    }


    if (src_frame->format == dst_frame->format) {
        return raw_frame_to_raw_frame(src_frame->format, src_frame->data, src_frame->linesize,
                                      dst_frame->format, dst_frame->data, dst_frame->linesize,
                                      src_frame->width, src_frame->height);
    }

    int (*copy_func)(uint8_t **, uint32_t *,
                     uint8_t **, uint32_t *, int, int) = NULL;

        copy_func = copy_func_list[src_frame->format][dst_frame->format];

    if (!copy_func)
        return -1;


    return copy_func(src_frame->data, src_frame->linesize,
                     dst_frame->data, dst_frame->linesize, src_frame->width, src_frame->height);
}

int video_frame_data_copy(enum video_frame_format s_format, uint8_t **s_data, uint32_t *s_linesize,
                          enum video_frame_format d_format, uint8_t **d_data, uint32_t *d_linesize, int width, int height)
{
    if (!d_data[0]) {
        fprintf(stderr, "dst_data[0] is null\n");
        return -1;
    }

    if(s_format == d_format)
        return raw_frame_to_raw_frame(s_format, s_data, s_linesize,
                                      d_format, d_data, d_linesize,
                                      width, height);


    int (*copy_func)(uint8_t **, uint32_t *,
                     uint8_t **, uint32_t *, int, int) = NULL;


    copy_func = copy_func_list[s_format][d_format];

    if (!copy_func)
        return -1;

    return copy_func(s_data, s_linesize,
                     d_data, d_linesize, width, height);
}
