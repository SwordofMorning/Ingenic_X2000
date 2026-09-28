#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#include <libhardware2/jpege_encode.h>
#include <libhardware2/rmem.h>

#define PAGE_SIZE 4096
#define alignment_up(a, size) ((a + size - 1) & (~(size - 1)))

struct jpege_in_info {
    unsigned int src_paddr;
    unsigned int dst_paddr;
    unsigned int dst_bufsize;
    unsigned short frame_width;
    unsigned short frame_height;
    short qa;
    jpege_pixel_fmt pix_fmt;
    char subsamp;
    unsigned short rstm_request;
};

struct jpege_out_info {
    unsigned int imagesize;
};

struct jpege_info {
    struct jpege_in_info in;
    struct jpege_out_info out;
};

struct jpege_encoder {
    int fd;
    int rmem_fd;
    void *src_buf_vaddr;
    void *dst_buf_vaddr;
    unsigned long src_buf_paddr;
    unsigned long dst_buf_paddr;
    unsigned int src_size;
    unsigned int dst_size;
    char subsample;
    struct jpege_info start;
};

enum JPEGE_SUBSAMPLE{
    no_subsampling = 1,
    horizontal_subsampling,  //进行16x8 采样，uv分量数据减少为1/2
    quad_subsampling,        //进行16x16采样，uv分量数据减少为1/4
};

struct jpege_padding_info {
    int width;
    int height;
    int fill_num_per_line;
    jpege_pixel_fmt in_fmt;
    int infile_line_offset;
    int infile_line_stride;
    int srcbuf_line_stride;
};

#define JZ_JPEGE_START          _IO('J',100)

struct jpege_encoder *jpege_encoder_open(void)
{
    struct jpege_encoder *encoder = (struct jpege_encoder *)malloc(sizeof(struct jpege_encoder));
    if (!encoder) {
        fprintf(stderr, "encoder memory allocation failed: %s\n", strerror(errno));
        return NULL;
    }

    memset(encoder, 0, sizeof(*encoder));

    encoder->fd = open("/dev/jpegenc", O_RDWR);
    if (encoder->fd < 0) {
        fprintf(stderr, "Open /dev/jpegenc failed: %s\n", strerror(errno));
        goto err_open_enc;
    }

    encoder->rmem_fd = rmem_open();
    if (encoder->rmem_fd < 0) {
        fprintf(stderr, "encoder open rmem failed: %s\n", strerror(errno));
        goto err_open_rmem;
    }

    return encoder;

err_open_rmem:
    close(encoder->fd);
err_open_enc:
    free(encoder);
    return NULL;
}

void jpege_encoder_close(struct jpege_encoder *encoder)
{
    if (!encoder) {
        fprintf(stderr, "Attempted to close a NULL encoder.\n");
        return;
    }

    if (encoder->src_buf_paddr)
        rmem_free(encoder->rmem_fd, encoder->src_buf_vaddr, encoder->src_buf_paddr, encoder->src_size);

    rmem_close(encoder->rmem_fd);

    close(encoder->fd);
    free(encoder);
}

static int jpege_encoder_alloc_src_buff(struct jpege_encoder *encoder, int size)
{
    if (size <= 0) {
        fprintf(stderr, "encoder can't alloc src buff due to invalid size!\n");
        return -1;
    }

    if (encoder->src_buf_vaddr && encoder->src_size >= size)
        return 0;

    if (encoder->src_buf_vaddr)
        rmem_free(encoder->rmem_fd, encoder->src_buf_vaddr, encoder->src_buf_paddr, encoder->src_size);

    encoder->src_size = size;
    encoder->src_buf_vaddr = rmem_alloc(encoder->rmem_fd, &encoder->src_buf_paddr, size);

    return 0;
}

static int jpege_encoder_alloc_dst_buff(struct jpege_encoder *encoder, int size)
{
    if (size <= 0) {
        fprintf(stderr, "encoder can't alloc dst buff due to invalid size!\n");
        return -1;
    }

    encoder->dst_size = size;
    encoder->dst_buf_vaddr = rmem_alloc(encoder->rmem_fd, &encoder->dst_buf_paddr, size);

    return 0;
}

static void jpege_encoder_set_subsample(struct jpege_encoder *encoder, struct jpege_encoder_config *input)
{
    if (input->in_fmt == JPEGE_PIX_FMT_NV12 || input->in_fmt == JPEGE_PIX_FMT_NV21)
        encoder->subsample = quad_subsampling;
    else if (input->in_fmt == JPEGE_PIX_FMT_YUYV)
        encoder->subsample = horizontal_subsampling;
    else
        encoder->subsample = no_subsampling;
}

static int get_input_size(jpege_pixel_fmt in_fmt, int width, int height)
{
    int input_size = 0;

    switch (in_fmt) {
        case JPEGE_PIX_FMT_NV12:
        case JPEGE_PIX_FMT_NV21:
            input_size = width * height * 3 / 2;
            break;
        case JPEGE_PIX_FMT_YUYV:
            input_size = width * height * 2;
            break;
        case JPEGE_PIX_FMT_YUV444:
        case JPEGE_PIX_FMT_RGB_888:
            input_size = width * height * 3;
            break;
        case JPEGE_PIX_FMT_BGRA_8888:
            input_size = width * height * 4;
            break;
        default:
            fprintf(stderr, "encoder doesn't support this input format.\n");
            break;
    }

    return input_size;
}

static inline int is_phy_mem_4k_aligned(unsigned long phy_mem)
{
    return phy_mem && (phy_mem % PAGE_SIZE) == 0;
}

static struct jpege_padding_info *get_pinfo(struct jpege_encoder_config *input) {
    struct jpege_padding_info *pinfo = (struct jpege_padding_info *)malloc(sizeof(struct jpege_padding_info));
    if (!pinfo) {
        fprintf(stderr, "encoder memory allocation failed: %s\n", strerror(errno));
        return NULL;
    }

    int width_16align = alignment_up(input->width, 16);
    pinfo->fill_num_per_line = width_16align - input->width;
    pinfo->width = input->width;
    pinfo->height = input->height;
    pinfo->in_fmt = input->in_fmt;

    switch (input->in_fmt) {
    case JPEGE_PIX_FMT_NV12:
    case JPEGE_PIX_FMT_NV21:
        pinfo->infile_line_offset = input->width - 1;
        pinfo->infile_line_stride = input->width;
        pinfo->srcbuf_line_stride = width_16align;
        break;
    case JPEGE_PIX_FMT_YUYV:
        pinfo->infile_line_offset = (input->width - 1) * 2;
        pinfo->infile_line_stride = input->width * 2;
        pinfo->srcbuf_line_stride = width_16align * 2;
        break;
    case JPEGE_PIX_FMT_YUV444:
    case JPEGE_PIX_FMT_RGB_888:
        pinfo->infile_line_offset = (input->width - 1) * 3;
        pinfo->infile_line_stride = input->width * 3;
        pinfo->srcbuf_line_stride = width_16align * 3;
        break;
    case JPEGE_PIX_FMT_RGBA_8888:
    case JPEGE_PIX_FMT_BGRA_8888:
        pinfo->infile_line_offset = (input->width - 1) * 4;
        pinfo->infile_line_stride = input->width * 4;
        pinfo->srcbuf_line_stride = width_16align * 4;
        break;
    default:
        fprintf(stderr, "encoder doesn't support this input format.\n");
        break;
    }

    return pinfo;
}

static void copy_pixels(void *src, void *dst, int pix_nums, int pixel_size) {
    int i, j;
    uint8_t *src_uint8;
    uint8_t *dst_uint8 = (uint8_t*)dst;

    for (i = 0; i < pix_nums; i++) {
        src_uint8 = (uint8_t*)src;
        for (j = 0; j < pixel_size; j++) {
            *dst_uint8 = *src_uint8;
            src_uint8++;
            dst_uint8++;
        }
    }
}

/* 填充拷贝:逐行拷贝, 并将行末最后一个像素拷贝填充,直至宽为16对齐 (所有格式均需要) */
static void fillcpy(void *dst, void *src, struct jpege_padding_info *pinfo)
{
    int src_line_offset = pinfo->infile_line_offset;
    int src_line_stride = pinfo->infile_line_stride;
    int dst_line_stride = pinfo->srcbuf_line_stride;

    int pix_nums = pinfo->fill_num_per_line;
    int fmt = pinfo->in_fmt;


    int copy_lines;
    if (fmt == JPEGE_PIX_FMT_NV12 || fmt == JPEGE_PIX_FMT_NV21)
        copy_lines = pinfo->height * 3 / 2;
    else
        copy_lines = pinfo->height;

    for (int i = 0; i < copy_lines; i++) {
        memcpy(dst, src, src_line_stride);

        void *src_ptr = src + src_line_offset;
        void *dst_ptr = dst + src_line_stride;

        if (fmt == JPEGE_PIX_FMT_NV12 || fmt == JPEGE_PIX_FMT_NV21)
            copy_pixels(src_ptr, dst_ptr, pix_nums, 1);
        if (fmt == JPEGE_PIX_FMT_YUYV)
            copy_pixels(src_ptr, dst_ptr, pix_nums, 2);
        if (fmt == JPEGE_PIX_FMT_YUV444 || fmt == JPEGE_PIX_FMT_RGB_888)
            copy_pixels(src_ptr, dst_ptr, pix_nums, 3);
        if (fmt == JPEGE_PIX_FMT_BGRA_8888 || fmt == JPEGE_PIX_FMT_RGBA_8888)
            copy_pixels(src_ptr, dst_ptr, pix_nums, 4);

        src += src_line_stride;
        dst += dst_line_stride;
    }
}

static int jpege_encoder_start(struct jpege_encoder *encoder, struct jpege_encoder_config *input)
{   encoder->start.in.qa = input->qa;
    encoder->start.in.pix_fmt = input->in_fmt;
    encoder->start.in.frame_width = alignment_up(input->width, 16);;
    encoder->start.in.frame_height = input->height;
    if (is_phy_mem_4k_aligned(input->phy_mem))
        encoder->start.in.src_paddr = input->phy_mem;
    else
        encoder->start.in.src_paddr = encoder->src_buf_paddr;
    encoder->start.in.dst_paddr = encoder->dst_buf_paddr;
    encoder->start.in.dst_bufsize = encoder->dst_size;
    encoder->start.in.subsamp = encoder->subsample;

    int err = ioctl(encoder->fd, JZ_JPEGE_START, &encoder->start);
    if (err < 0) {
        fprintf(stderr, "JPEG compression failed: %s\n", strerror(errno));
        return err;
    }

    return 0;
}

static int jpege_encoder_setup_src_buffer(struct jpege_encoder *encoder, struct jpege_encoder_config *input, int src_size)
{
    struct jpege_padding_info* pinfo = get_pinfo(input);
    if (!pinfo)
        return -1;

    int ret = jpege_encoder_alloc_src_buff(encoder, alignment_up(src_size, PAGE_SIZE));
    if (ret < 0)
        return -1;

    if (pinfo->fill_num_per_line)
        fillcpy(encoder->src_buf_vaddr, input->input_mem, pinfo);     //宽为非16对齐时,填充拷贝
    else
        memcpy(encoder->src_buf_vaddr, input->input_mem, src_size);   //宽为16对齐时,直接拷贝

    rmem_cache_sync(encoder->rmem_fd, encoder->src_buf_vaddr, encoder->src_size, rmem_cache_mem_to_dev);

    return 0;
}

static int jpege_encoder_setup_dst_buffer(struct jpege_encoder *encoder, struct jpege_encoder_output_data *output, int dst_size)
{
    int ret = jpege_encoder_alloc_dst_buff(encoder, alignment_up(dst_size, PAGE_SIZE));
    if (ret < 0)
        return -1;

    output->buff_size = encoder->dst_size;
    output->phy_mem = encoder->dst_buf_paddr;
    output->mem = encoder->dst_buf_vaddr;

    return 0;
}

void jpege_encoder_put(struct jpege_encoder *encoder , struct jpege_encoder_output_data *output)
{
    rmem_free(encoder->rmem_fd, output->mem, output->phy_mem, output->buff_size);
}

int jpege_encoder_work(struct jpege_encoder *encoder, struct jpege_encoder_config *input, struct jpege_encoder_output_data *output)
{
    int err = jpege_encoder_start(encoder, input);
    if (err < 0) {
        jpege_encoder_put(encoder, output);
        return err;
    }

    output->image_size = encoder->start.out.imagesize;

    return 0;
}

static int jpege_encoder_setup(struct jpege_encoder *encoder, struct jpege_encoder_config *input, struct jpege_encoder_output_data *output)
{
    int ret;
    int aligned_width = alignment_up(input->width, 16);

    int infile_size = get_input_size(input->in_fmt, input->width, input->height);
    int srcbuf_size = get_input_size(input->in_fmt, aligned_width, input->height);
    int dstbuf_size = srcbuf_size;

    if (infile_size != input->file_size) {
        fprintf(stderr, "Invalid buffer size, file size %d != expected buff_size %d\n", input->file_size, infile_size);
        return -1;
    }

    if (!is_phy_mem_4k_aligned(input->phy_mem)) {
        ret = jpege_encoder_setup_src_buffer(encoder, input, srcbuf_size);
        if (ret < 0)
            return -1;
    }

    ret = jpege_encoder_setup_dst_buffer(encoder, output, dstbuf_size);
    if (ret < 0)
        return -1;

    jpege_encoder_set_subsample(encoder, input);

    return 0;
}

int jpege_encoder_get(struct jpege_encoder *encoder, struct jpege_encoder_config *input, struct jpege_encoder_output_data *output)
{
    int err = jpege_encoder_setup(encoder, input, output);
    if (err < 0)
        return err;

    return jpege_encoder_work(encoder, input, output);
}