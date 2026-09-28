#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <assert.h>

#include <linux/media.h>
#include <linux/videodev2.h>
#include <libhardware2/rmem.h>
#include <libhardware2/v4l2_h264_decode.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define SOURCE_SIZE_MAX        (1024 * 1024)
#define ALIGN(d,a) (((d)+((a)-1))/(a)*(a))

struct V4L2_buf_info {
    enum v4l2_buf_type buf_type;
    enum v4l2_memory mem_type;
    unsigned int format;

    void *h264_mem;
    unsigned int h264_size;

    void *out_mem[2];
    unsigned int out_size[2];
};

struct v4l2_h264_decoder {
    int fd;
    int rmem_fd;

    struct V4L2_buf_info src_buf_info;
    struct V4L2_buf_info dst_buf_info;

    int y_size;
    int uv_size;
    struct v4l2_h264_decoder_config c;
};



#define pr_err(msg) \
    fprintf(stderr, "error: %s: %s\n", msg, strerror(errno))

#define FMT_C(f) \
    ((char *)&(f))[0],((char *)&(f))[1],((char *)&(f))[2],((char *)&(f))[3]

static int v4l2_test_capabilities(int fd, unsigned int caps)
{
    struct v4l2_capability capability;

    memset(&capability, 0, sizeof(capability));

    int ret = ioctl(fd, VIDIOC_QUERYCAP, &capability);
    if (ret < 0)
        return -1;

    unsigned int cap;
    if (capability.capabilities & V4L2_CAP_DEVICE_CAPS)
        cap = capability.device_caps;
    else
        cap = capability.capabilities;

    return (cap & caps) == caps ? 0 : 1;
}

static int v4l2_format_planes_count(unsigned int format)
{
    switch (format) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        return 2;
    case V4L2_PIX_FMT_H264:
        return 1;
    default:
        assert(0);
    }
    return 0;
}

static int init_v4l2_formt_t(
    struct v4l2_format *f, int type, unsigned int format, int width, int height)
{
    memset(f, 0, sizeof(*f));
    f->type = type;

    switch (format) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            f->fmt.pix_mp.width = width;
            f->fmt.pix_mp.height = height;
            f->fmt.pix_mp.pixelformat = format;
            f->fmt.pix_mp.plane_fmt[0].sizeimage = 0;
            break;
        case V4L2_PIX_FMT_H264:
            f->fmt.pix_mp.width = width;
            f->fmt.pix_mp.height = height;
            f->fmt.pix_mp.pixelformat = format;
            f->fmt.pix_mp.plane_fmt[0].sizeimage = SOURCE_SIZE_MAX;
            break;
        default: {
            fprintf(stderr, "not support this format: %c%c%c%c\n", FMT_C(format));
            errno = EINVAL;
            return -1;
        }
    }

    return 0;
}

static int v4l2_set_format(struct v4l2_h264_decoder *decoder, struct V4L2_buf_info *buf_info)
{
    int ret;
    struct v4l2_format f;

    ret = init_v4l2_formt_t(&f, buf_info->buf_type, buf_info->format, decoder->c.width, decoder->c.height);
    if (ret)
        return -1;

    ret = ioctl(decoder->fd, VIDIOC_S_FMT, &f);
    if (ret < 0) {
        fprintf(stderr, "failed to set format: %c%c%c%c\n", FMT_C(buf_info->format));
        return ret;
    }
    if (buf_info->buf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        decoder->c.linesize = f.fmt.pix_mp.plane_fmt[0].bytesperline;

    return 0;
}

static int v4l2_create_buffer(int fd, struct V4L2_buf_info *buf_info, int count)
{
    struct v4l2_create_buffers buffers;

    memset(&buffers, 0, sizeof(buffers));
    buffers.format.type = buf_info->buf_type;
    buffers.memory = buf_info->mem_type;
    buffers.count = count;

    int ret = ioctl(fd, VIDIOC_G_FMT, &buffers.format);
    if (ret < 0) {
        fprintf(stderr, "failed to get fmt for type: %d\n", buf_info->buf_type);
        return -1;
    }

    ret = ioctl(fd, VIDIOC_CREATE_BUFS, &buffers);
    if (ret < 0) {
        fprintf(stderr, "failed to create buffer for type: %d\n", buf_info->buf_type);
        return -1;
    }

    return 0;
}

static int v4l2_mmap_buffer(int fd, struct V4L2_buf_info *buf_info, int index)
{
    int count = v4l2_format_planes_count(buf_info->format);
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = buf_info->buf_type;
    buffer.memory = buf_info->mem_type;
    buffer.index = index;
    buffer.length = count;
    buffer.m.planes = planes;

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
    if (ret < 0) {
        fprintf(stderr, "failed to query buffer\n");
        return -1;
    }

    int i;
    for (i = 0; i < count; i++) {
        unsigned int offset, length;
        void *mem = NULL;

        offset = buffer.m.planes[i].m.mem_offset;
        length = buffer.m.planes[i].length;
        mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        if (!mem) {
            fprintf(stderr, "failed to mmap: %x %d\n", offset, length);
            return -1;
        }

        if (buf_info->buf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            buf_info->h264_mem = mem;
            buf_info->h264_size = length;
        } else {
            buf_info->out_mem[i] = mem;
            buf_info->out_size[i] = length;
        }
    }

    return 0;
}

static int v4l2_queue_buffer(
    int fd, struct V4L2_buf_info *buf_info, void **mem, int *size, int count)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = buf_info->buf_type;
    buffer.memory = buf_info->mem_type;
    buffer.index = 0;
    buffer.length = count;
    buffer.m.planes = planes;

    int i;
    for (i = 0; i < count; i++) {
        buffer.m.planes[i].bytesused = size[i];

        /* USERPTR */
        buffer.m.planes[i].data_offset = 0;
        buffer.m.planes[i].length = size[i];
        if (buf_info->mem_type == V4L2_MEMORY_USERPTR)
            buffer.m.planes[i].m.userptr = (unsigned long)mem[i];
    }

    ret = ioctl(fd, VIDIOC_QBUF, &buffer);
    if (ret) {
        fprintf(stderr, "failed to queue buf: mem=%p size=%d\n", mem?mem[0]:NULL, size?size[0]:-1);
    }

    return ret;
}

static int v4l2_dequeue_buffer(
    int fd, struct V4L2_buf_info *buf_info, int count)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;

    memset(&planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = buf_info->buf_type;
    buffer.memory = buf_info->mem_type;
    buffer.length = count;
    buffer.m.planes = planes;

    int ret = ioctl(fd, VIDIOC_DQBUF, &buffer);
    if (ret)
        return ret;

    return 0;
}

static int v4l2_stream_on(int fd, struct V4L2_buf_info *buf_info)
{
    return ioctl(fd, VIDIOC_STREAMON, &buf_info->buf_type);
}

static void v4l2_stream_off(int fd, struct V4L2_buf_info *buf_info)
{
    int ret = ioctl(fd, VIDIOC_STREAMOFF, &buf_info->buf_type);

    if (ret < 0) {
        if (buf_info->buf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
            pr_err("stream off src\n");
        else
            pr_err("stream off dst\n");
    }
}

static int h264_decoder_init(struct v4l2_h264_decoder *decoder, struct v4l2_h264_decoder_config *config)
{
    decoder->c = *config;

    decoder->fd = open(config->video_path, O_RDWR | O_NONBLOCK, 0);
    if (decoder->fd < 0) {
        pr_err("open");
        free(decoder);
        return -1;
    }

    int ret = v4l2_test_capabilities(decoder->fd, V4L2_CAP_STREAMING);
    if (ret) {
        pr_err("test capture");
        close(decoder->fd);
        return -1;
    }

    return 0;
}

static int h264_decoder_setup(struct v4l2_h264_decoder *decoder, unsigned int dst_format,  enum v4l2_memory dst_mem_type)
{
    int ret;
    decoder->src_buf_info.format   = V4L2_PIX_FMT_H264;
    decoder->src_buf_info.mem_type = V4L2_MEMORY_MMAP;
    decoder->src_buf_info.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    decoder->dst_buf_info.format   = dst_format;
    decoder->dst_buf_info.mem_type = dst_mem_type;
    decoder->dst_buf_info.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    ret = v4l2_set_format(decoder, &decoder->src_buf_info);
    if (ret) {
        pr_err("set input fmt");
        return -1;
    }

    ret = v4l2_set_format(decoder, &decoder->dst_buf_info);
    if (ret) {
        pr_err("set output fmt");
        return -1;
    }

    ret = v4l2_create_buffer(decoder->fd, &decoder->src_buf_info, 1);
    if (ret) {
        pr_err("create source buffer\n");
        return -1;
    }

    ret = v4l2_create_buffer(decoder->fd, &decoder->dst_buf_info, 1);
    if (ret) {
        pr_err("create dst buffer\n");
        return -1;
    }


    ret = v4l2_mmap_buffer(decoder->fd, &decoder->src_buf_info, 0);
    if (ret) {
        pr_err("mmap src buffer\n");
        return -1;
    }

    if (decoder->dst_buf_info.mem_type == V4L2_MEMORY_MMAP) {
        ret = v4l2_mmap_buffer(decoder->fd, &decoder->dst_buf_info, 0);
        if (ret) {
            pr_err("mmap dst buffer\n");
            return -1;
        }
    }

    ret = v4l2_stream_on(decoder->fd, &decoder->src_buf_info);
    if (ret < 0) {
        pr_err("stream on src\n");
        return -1;
    }

    ret = v4l2_stream_on(decoder->fd, &decoder->dst_buf_info);
    if (ret < 0) {
        pr_err("stream on dst\n");
        return -1;
    }


    return 0;
}

static int wait_for_decode(struct v4l2_h264_decoder *decoder)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(decoder->fd, &fds);
    struct timeval tv = { 0, 300000 };
    //有数据可读时就返回 (queue 和 dequeue需整个过程上锁)
    int ret = select(decoder->fd+1, &fds, NULL, NULL, &tv);
    if (ret < 0) {
        pr_err("select result");
        return -1;
    }
    if (ret == 0) {
        pr_err("select timeout");
        return -1;
    }

    return 0;
}

int v4l2_h264_decoder_work(struct v4l2_h264_decoder *decoder, void *input_mem, unsigned int input_size, void **output_mem)
{
    memcpy(decoder->src_buf_info.h264_mem, input_mem, input_size);

    int out_size[2] = {decoder->y_size, decoder->uv_size};

    int ret = v4l2_queue_buffer(decoder->fd, &decoder->src_buf_info, &decoder->src_buf_info.h264_mem, &input_size, 1);
    if (ret) {
        fprintf(stderr, "This a src buffer queue failure\n");
        return -1;
    }

    ret = v4l2_queue_buffer(decoder->fd, &decoder->dst_buf_info, NULL, out_size, 2);
    if (ret) {
        fprintf(stderr, "This a dst buffer queue failure\n");
        return -1;
    }

    ret = wait_for_decode(decoder);
    if (ret < 0)
        return -1;

    output_mem[0] = decoder->dst_buf_info.out_mem[0];
    output_mem[1] = decoder->dst_buf_info.out_mem[1];

    return 0;
}

int v4l2_h264_decoder_work_release(struct v4l2_h264_decoder *decoder)
{
    int ret;

    ret = v4l2_dequeue_buffer(decoder->fd, &decoder->src_buf_info, 1);
    if (ret) {
        fprintf(stderr, "This a src buffer de-queue failure\n");
        return -1;
    }

    ret = v4l2_dequeue_buffer(decoder->fd, &decoder->dst_buf_info, 2);
    if (ret) {
        fprintf(stderr, "This a dst buffer de-queue failure\n");
        return -1;
    }

    return 0;
}

struct v4l2_h264_decoder *v4l2_h264_decoder_open(struct v4l2_h264_decoder_config *config)
{
    int ret;
    struct v4l2_h264_decoder *decoder = malloc(sizeof(*decoder));
    if(!decoder) {
        pr_err("malloc decoder");
        return NULL;
    }
    memset(decoder, 0, sizeof(*decoder));

    ret = h264_decoder_init(decoder, config);
    if (ret < 0)
        return NULL;

    ret = h264_decoder_setup(decoder, config->output_fmt, V4L2_MEMORY_MMAP);
    if (ret < 0)
        goto decoder_release;

    config->linesize = decoder->c.linesize;
    config->colunmsize = config->height;

    decoder->y_size = config->linesize * config->colunmsize;
    decoder->uv_size = config->linesize * config->colunmsize / 2;

    return decoder;

decoder_release:
    close(decoder->fd);
    free(decoder);

    return NULL;
}

int v4l2_h264_decoder_close(struct v4l2_h264_decoder *decoder)
{
    v4l2_stream_off(decoder->fd, &decoder->src_buf_info);
    v4l2_stream_off(decoder->fd, &decoder->dst_buf_info);

    munmap(decoder->src_buf_info.h264_mem, decoder->src_buf_info.h264_size);
    munmap(decoder->dst_buf_info.out_mem[0], decoder->dst_buf_info.out_size[0]);
    munmap(decoder->dst_buf_info.out_mem[1], decoder->dst_buf_info.out_size[1]);

    close(decoder->fd);
    free(decoder);

    return 0;
}



int v4l2_h264_decoder_direct_work(struct v4l2_h264_decoder *decoder, void *input_mem, unsigned int input_size, void *y_mem, void *uv_mem)
{
    int ret;
    memcpy(decoder->src_buf_info.h264_mem, input_mem, input_size);

    void *out_mem[2] = {y_mem, uv_mem};                                                //指定输出地址
    int out_size[2] = {decoder->y_size, decoder->uv_size};

    int page_size = getpagesize();

    if ((unsigned long)y_mem %  page_size) {
        fprintf(stderr, "v4l2 h264 decode: failed to decode direct work, y_mem must align pagesize\n");
        return -1;
    }

    if ((unsigned long)uv_mem %  page_size) {
        fprintf(stderr, "v4l2 h264 decode: failed to decode direct work, uv_mem must align pagesize\n");
        return -1;
    }


    ret = v4l2_queue_buffer(decoder->fd, &decoder->src_buf_info, &decoder->src_buf_info.h264_mem, &input_size, 1);
    if (ret) {
        fprintf(stderr, "This a src buffer queue failure\n");
        return -1;
    }

    ret = v4l2_queue_buffer(decoder->fd, &decoder->dst_buf_info, out_mem, out_size, 2);//设置out_mem
    if (ret) {
        fprintf(stderr, "This a dst buffer queue failure\n");
        return -1;
    }

    ret = wait_for_decode(decoder);
    if (ret < 0)
        return -1;

    ret = v4l2_h264_decoder_work_release(decoder);
    if (ret < 0)
        return -1;

    return 0;
}

struct v4l2_h264_decoder *v4l2_h264_decoder_direct_open(struct v4l2_h264_decoder_config *config)
{
    int ret;
    struct v4l2_h264_decoder *decoder = malloc(sizeof(*decoder));
    if(!decoder) {
        pr_err("malloc decoder");
        return NULL;
    }
    memset(decoder, 0, sizeof(*decoder));

    ret = h264_decoder_init(decoder, config);
    if (ret < 0)
        goto err_decoder_init;

    ret = h264_decoder_setup(decoder, config->output_fmt, V4L2_MEMORY_USERPTR);
    if (ret < 0)
        goto err_decoder_setup;

    config->linesize = decoder->c.linesize;
    config->colunmsize = ALIGN(config->height, 16);//userptr模式高需要16对齐

    decoder->y_size = config->linesize * config->colunmsize;
    decoder->uv_size = config->linesize * config->colunmsize / 2;

    return decoder;

err_decoder_setup:
    close(decoder->fd);

err_decoder_init:
    free(decoder);

    return NULL;
}

int v4l2_h264_decoder_direct_close(struct v4l2_h264_decoder *decoder)
{
    v4l2_stream_off(decoder->fd, &decoder->src_buf_info);
    v4l2_stream_off(decoder->fd, &decoder->dst_buf_info);

    munmap(decoder->src_buf_info.h264_mem, decoder->src_buf_info.h264_size);

    close(decoder->fd);
    free(decoder);

    return 0;
}