#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include <libhardware2/jpegd_decode.h>
#include <libhardware2/rmem.h>

#define PAGE_SIZE 4096
#define alignment_up(a, size) ((a + size - 1) & (~ (size - 1)))
#define floor_2(x) ((x >> 1) << 1)           //向下2对齐
#define max(a, b)       ((a) > (b)) ? (a) : (b)

struct jpegd_param {
    unsigned int file_size;
    unsigned int src_paddr;
    unsigned int dst_paddr;
    jpegd_pixel_fmt out_fmt;
};

struct jpegd_out_info {
    unsigned int yuv_type;
    unsigned short width;
    unsigned short height;
    unsigned int size;
};

struct jpegd_info {
    struct jpegd_param in;
    struct jpegd_out_info out;
};

struct jpegd_decoder {
    int fd;
    int rmem_fd;
    void *src_buf_vaddr;
    void *dst_buf_vaddr;
    unsigned long src_buf_paddr;
    unsigned long dst_buf_paddr;
    unsigned int src_size;
    unsigned int dst_size;
    struct jpegd_info start;
    int use_phy_output;
};

/* jpg文件压缩时的采样方式(会影响解压出来图片的宽高比) */
enum chrome_subsample {
    no_subsampling,
    horizontal_subsampling,
    vertical_subsampling,
    quad_subsampling,
    other_subsampling,
};

#define JZ_JPEGD_START          _IO('J', 100)
#define JZ_JPEGD_GET_RES_STA    _IO('J', 107)

struct jpegd_decoder *jpegd_decoder_open(void)
{
    struct jpegd_decoder *decoder = (struct jpegd_decoder *)malloc(sizeof(struct jpegd_decoder));
    if (!decoder) {
        fprintf(stderr, "decoder memory allocation failed: %s\n", strerror(errno));
        return NULL;
    }

    memset(decoder, 0, sizeof(*decoder));

    decoder->fd = open("/dev/jpegdec", O_RDWR);
    if (decoder->fd < 0) {
        fprintf(stderr, "Open /dev/jpegdec failed: %s\n", strerror(errno));
        goto err_open_dec;
    }

    decoder->rmem_fd = rmem_open();
    if (decoder->rmem_fd < 0) {
        fprintf(stderr, "decoder open rmem failed: %s\n", strerror(errno));
        goto err_open_rmem;
    }

    return decoder;

err_open_rmem:
    close(decoder->fd);
err_open_dec:
    free(decoder);
    return NULL;
}

void jpegd_decoder_close(struct jpegd_decoder *decoder)
{
    if (!decoder) {
        fprintf(stderr, "Attempted to close a NULL decoder.\n");
        return;
    }

    if (decoder->src_buf_paddr)
        rmem_free(decoder->rmem_fd, decoder->src_buf_vaddr, decoder->src_buf_paddr, decoder->src_size);

    rmem_close(decoder->rmem_fd);

    close(decoder->fd);
    free(decoder);
}

static int jpegd_decoder_alloc_src_buff(struct jpegd_decoder *decoder, int size)
{
    if (size <= 0) {
        fprintf(stderr, "decoder can't alloc src buff due to invalid size!\n");
        return -1;
    }

    if (decoder->src_buf_vaddr && decoder->src_size >= size)
        return 0;

    if (decoder->src_buf_vaddr)
        rmem_free(decoder->rmem_fd, decoder->src_buf_vaddr, decoder->src_buf_paddr, decoder->src_size);

    decoder->src_size = size;
    decoder->src_buf_vaddr = rmem_alloc(decoder->rmem_fd, &decoder->src_buf_paddr, size);

    return 0;
}

static int jpegd_decoder_alloc_dst_buff(struct jpegd_decoder *decoder, int size)
{
    if (size <= 0) {
        fprintf(stderr, "decoder can't alloc dst buff due to invalid size!\n");
        return -1;
    }

    decoder->dst_size = size;
    decoder->dst_buf_vaddr = rmem_alloc(decoder->rmem_fd, &decoder->dst_buf_paddr, size);

    return 0;
}

static int get_output_size(struct jpegd_decoder_config *input) {
    int width = alignment_up(input->width, 16);
    int height = alignment_up(input->height, 16);
    int output_size = 0;

    switch (input->out_fmt) {
        case JPEGD_PIX_FMT_NV12:
        case JPEGD_PIX_FMT_NV21:
            output_size = width * height * 3 / 2;
            break;
        case JPEGD_PIX_FMT_YUYV:
            output_size = width * height * 2;
            break;
        case JPEGD_PIX_FMT_YUV444:
        case JPEGD_PIX_FMT_RGB_888:
            output_size = width * height * 3;
            break;
        case JPEGD_PIX_FMT_BGRA_8888:
            output_size = width * height * 4;
            break;
        default:
            fprintf(stderr, "decoder doesn't support this out format.\n");
            break;
    }

    return output_size;
}

static inline int is_phy_mem_4k_aligned(unsigned long phy_mem)
{
    return phy_mem && (phy_mem % PAGE_SIZE) == 0;
}

static int jpegd_get_info(struct jpegd_decoder *decoder, struct jpegd_decoder_output_data *output)
{
    int width = decoder->start.out.width;
    int height = decoder->start.out.height;
    int subsample = decoder->start.out.yuv_type;
    int other_subsample;
    int err = ioctl(decoder->fd, JZ_JPEGD_GET_RES_STA, &other_subsample);
    if (err < 0) {
        fprintf(stderr, "JPEG get reserved status failed: %s\n", strerror(errno));
        return err;
    }

    int w_ratio;
    int h_ratio;
    switch (subsample) {
    case no_subsampling:
        width = alignment_up(width, 8);
        height = alignment_up(height, 8);
        break;
    case horizontal_subsampling:
        width = alignment_up(width, 16);
        height = alignment_up(height, 8);
        break;
    case vertical_subsampling:
        width = alignment_up(width, 8);
        height = alignment_up(height, 16);
        break;
    case quad_subsampling:
        width = alignment_up(width, 16);
        height = alignment_up(height, 16);
        break;
    case other_subsampling:
        w_ratio = (((other_subsample & 0xc000) >> 14) + 1) * 8;
        h_ratio = (((other_subsample & 0x3000) >> 12) + 1) * 8;
        width = alignment_up(width, w_ratio);
        height = alignment_up(height, h_ratio);
        break;
    default:
        break;
    }

    output->image_width = width;
    output->image_height = height;
    output->image_size = decoder->start.out.size;

    return 0;
}

static void copy_by_line(char *s, char *d, int s_lineSize, int d_lineSize, int num)
{
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < d_lineSize; j++) {
            d[j] = s[j];
        }
        s += s_lineSize;
        d += d_lineSize;
    }
}

/* cpu进行裁切偏移 */
static void jpegd_decoder_crop(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, struct jpegd_decoder_output_data *output)
{
    int width = output->image_width;       //偏移前的宽
    int height = output->image_height;     //偏移前的高
    int w = input->width;                  //偏移后的宽
    int h = input->height;                 //偏移后的高
    int fmt = decoder->start.in.out_fmt;

    int bytes_per_line_src = 0;
    int bytes_per_line_dst = 0;
    int size = 0;

    switch (fmt) {
    case JPEGD_PIX_FMT_NV12:
    case JPEGD_PIX_FMT_NV21:
        h = floor_2(h);//向下取偶(y分量行数必须为偶数)
        bytes_per_line_src = width;
        bytes_per_line_dst = w;
        size = w * h * 3 / 2;
        break;

    case JPEGD_PIX_FMT_YUYV:
        bytes_per_line_src = width * 2;
        bytes_per_line_dst = w * 2;
        size = w * h * 2;
        break;
    case JPEGD_PIX_FMT_YUV444:
    case JPEGD_PIX_FMT_RGB_888:
        bytes_per_line_src = width * 3;
        bytes_per_line_dst = w * 3;
        size = w * h * 3;
        break;
    case JPEGD_PIX_FMT_BGRA_8888:
        bytes_per_line_src = width * 4;
        bytes_per_line_dst = w * 4;
        size = w * h * 4;
        break;
    default:
        fprintf(stderr, "decoder doesn't support this out format.\n");
        break;
    }

    char *s = decoder->dst_buf_vaddr;
    char *d = output->mem;

    copy_by_line(s, d, bytes_per_line_src, bytes_per_line_dst, h);

    if (fmt == JPEGD_PIX_FMT_NV12 || fmt == JPEGD_PIX_FMT_NV21) {
        char *src_uv = s + height * bytes_per_line_src;
        char *dst_uv = d + h * bytes_per_line_dst;
        int uv_line_nums = h / 2;
        copy_by_line(src_uv, dst_uv, bytes_per_line_src, bytes_per_line_dst, uv_line_nums);
    }

    output->image_width = w;
    output->image_height = h;
    output->image_size = size;
}

static int jpegd_decoder_start(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input)
{
    decoder->start.in.file_size = input->file_size;
    decoder->start.in.out_fmt   = input->out_fmt;
    if (is_phy_mem_4k_aligned(input->phy_mem))
        decoder->start.in.src_paddr = input->phy_mem;
    else
        decoder->start.in.src_paddr = decoder->src_buf_paddr;
    decoder->start.in.dst_paddr = decoder->dst_buf_paddr;

    int err = ioctl(decoder->fd, JZ_JPEGD_START, &decoder->start);
    if (err < 0) {
        fprintf(stderr, "JPEG decompression failed: %s\n", strerror(errno));
        return err;
    }

    return 0;
}

static int jpegd_decoder_setup_src_buffer(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, unsigned int src_size)
{
    int alloc_size = max((input->width * input->height), src_size);

    /* 按宽高申请, 尽量避免按mjpeg大小申请时需要反复更新导致rmem产生内存碎片 */
    int ret = jpegd_decoder_alloc_src_buff(decoder, alignment_up(alloc_size, PAGE_SIZE));
    if (ret < 0)
        return -1;

    memcpy(decoder->src_buf_vaddr, input->input_mem, src_size);
    rmem_cache_sync(decoder->rmem_fd, decoder->src_buf_vaddr, decoder->src_size, rmem_cache_mem_to_dev);

    return 0;
}

static int jpegd_decoder_setup_dst_buffer(struct jpegd_decoder *decoder, struct jpegd_decoder_output_data *out, int dec_size)
{
    if (decoder->use_phy_output == 1) {
        out->buff_size = decoder->dst_size;
        out->mem = decoder->dst_buf_vaddr;
        out->phy_mem = decoder->dst_buf_paddr;
    }

    if(decoder->use_phy_output == 0) {
        int ret = jpegd_decoder_alloc_dst_buff(decoder, alignment_up(dec_size, PAGE_SIZE));
        if (ret < 0)
            return -1;

        out->buff_size = decoder->dst_size;
        out->mem = decoder->dst_buf_vaddr;
        out->phy_mem = decoder->dst_buf_paddr;
    }

    return 0;
}

int jpegd_decoder_set_output_mem(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, void *mem, unsigned long phy_mem, unsigned int output_size)
{
    int dec_size = get_output_size(input);
    if (dec_size > output_size) {
        fprintf(stderr, "param output_size isn't big enough for decode.\n");
        return -1;
    }

    if (is_phy_mem_4k_aligned(phy_mem)) {
        decoder->dst_buf_vaddr = mem;
        decoder->dst_buf_paddr = phy_mem;
        decoder->dst_size = output_size;
        decoder->use_phy_output = 1;
        return 0;
    } else if (!mem && !phy_mem && !output_size) {
        decoder->dst_buf_vaddr = NULL;
        decoder->dst_buf_paddr = 0;
        decoder->dst_size = 0;
        decoder->use_phy_output = 0;
        return 0;
    } else {
        fprintf(stderr, "param phy_mem isn't 4k aligned address!\n");
        return -1;
    }
}

void jpegd_decoder_put(struct jpegd_decoder *decoder, struct jpegd_decoder_output_data *output)
{
    if (!decoder->use_phy_output)
        rmem_free(decoder->rmem_fd, output->mem, output->phy_mem, output->buff_size);
}

static int jpegd_decoder_work(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, struct jpegd_decoder_output_data *output)
{
    int err;
    err = jpegd_decoder_start(decoder, input);
    if (err < 0) {
        jpegd_decoder_put(decoder, output);
        return err;
    }

    err = jpegd_get_info(decoder, output);
    if (err < 0) {
        jpegd_decoder_put(decoder, output);
        return err;
    }

    if ((input->width < output->image_width) || (input->height < output->image_height))
        jpegd_decoder_crop(decoder, input, output);

    return 0;
}

static int jpegd_decoder_setup(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, struct jpegd_decoder_output_data *output)
{
    int ret;
    int srcbuf_size = input->file_size;
    int dstbuf_size = get_output_size(input);

    if (!is_phy_mem_4k_aligned(input->phy_mem)) {
        ret = jpegd_decoder_setup_src_buffer(decoder, input, srcbuf_size);
        if (ret < 0)
            return -1;
    }

    ret = jpegd_decoder_setup_dst_buffer(decoder, output, dstbuf_size);
    if (ret < 0)
        return -1;

    return 0;
}

int jpegd_decoder_get(struct jpegd_decoder *decoder, struct jpegd_decoder_config *input, struct jpegd_decoder_output_data *output)
{
    int err = jpegd_decoder_setup(decoder, input, output);
    if (err < 0)
        return err;

    return jpegd_decoder_work(decoder, input, output);
}