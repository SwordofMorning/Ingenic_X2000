#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/media.h>
#include <linux/videodev2.h>
#include <libhardware2/v4l2_jpeg_decode.h>
#include <libhardware2/rmem.h>

struct v4l2_jpeg_decoder {
    int fd;
    void *in_mem;
    int in_size;
    void *y_mem;
    void *uv_mem;
    struct v4l2_jpeg_decoder_config config;
};

#define pr_err(msg) \
    fprintf(stderr, "error: %s: %s\n", msg, strerror(errno))

#define FMT_C(f) \
    ((char *)&(f))[0],((char *)&(f))[1],((char *)&(f))[2],((char *)&(f))[3]

static int v4l2_test_capabilities(int fd, unsigned int caps)
{
    struct v4l2_capability capability;

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

static int type_is_mplane(unsigned int type)
{
    switch (type) {
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
            return 1;

        default:
            return 0;
    }
}

static void setup_format(struct v4l2_format *v4l2_format, unsigned int type,
            unsigned int width, unsigned int height, unsigned int pixelformat)
{
    unsigned int sizeimage;

    memset(v4l2_format, 0, sizeof(*v4l2_format));
    v4l2_format->type = type;

    if (pixelformat == V4L2_PIX_FMT_JPEG)
        /* NV12 NV21 */
        sizeimage = width * height * 3 / 2;
    else
        sizeimage = 0;

    if (type_is_mplane(type)) {
        v4l2_format->fmt.pix_mp.width = width;
        v4l2_format->fmt.pix_mp.height = height;
        v4l2_format->fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
        v4l2_format->fmt.pix_mp.pixelformat = pixelformat;
    } else {
        v4l2_format->fmt.pix.width = width;
        v4l2_format->fmt.pix.height = height;
        v4l2_format->fmt.pix.sizeimage = sizeimage;
        v4l2_format->fmt.pix.pixelformat = pixelformat;
    }
}

static int v4l2_set_format(int fd, int type, unsigned format, int width, int height)
{
    int ret;
    struct v4l2_format v4l2_format;

    setup_format(&v4l2_format, type, width, height, format);

    ret = ioctl(fd, VIDIOC_S_FMT, &v4l2_format);
    if (ret < 0) {
        fprintf(stderr, "failed to set format: %c%c%c%c\n", FMT_C(format));
        return ret;
    }

    return 0;
}

static int v4l2_create_buffer(int fd, int type, int memory_type, int count)
{
    struct v4l2_create_buffers buffers;

    memset(&buffers, 0, sizeof(buffers));
    buffers.format.type = type;
    buffers.memory = memory_type;
    buffers.count = count;

    int ret = ioctl(fd, VIDIOC_G_FMT, &buffers.format);
    if (ret < 0) {
        fprintf(stderr, "failed to get fmt for type: %d\n", type);
        return -1;
    }

    ret = ioctl(fd, VIDIOC_CREATE_BUFS, &buffers);
    if (ret < 0) {
        fprintf(stderr, "failed to create buffer for type: %d\n", type);
        return -1;
    }

    return 0;
}

static int v4l2_format_planes_count(unsigned int format)
{
    switch (format) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            return 2;
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_JPEG:
            return 1;
        default:
            assert(0);
    }
    return 0;
}

static int v4l2_mmap_buffer
    (int fd, int type, int format, int index, unsigned char **mem, unsigned int *mem_size)
{
    int count = v4l2_format_planes_count(format);
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
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
        offset = buffer.m.planes[i].m.mem_offset;
        length = buffer.m.planes[i].length;
        mem_size[i] = length;

        mem[i] = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        if (mem[i] == (void*)0xffffffff) {
            fprintf(stderr, "failed to mmap: %x %d\n", offset, length);
            return -1;
        }
    }

    return 0;
}

static int v4l2_queue_buffer(
    int fd, int type, int memory_type, void **mem, int *size, int count)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;
    int ret;

    memset(&planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = memory_type;
    buffer.index = 0;
    buffer.length = count;
    buffer.m.planes = planes;

    int i;
    for (i = 0; i < count; i++) {
        buffer.m.planes[i].bytesused = size[i];
    }

    ret = ioctl(fd, VIDIOC_QBUF, &buffer);
    if (ret) {
        fprintf(stderr, "failed to qbuf: %d %d %p %d",
            type, memory_type, mem?mem[0]:NULL, size?size[0]:-1);
    }

    return ret;
}

static int v4l2_dequeue_buffer(
    int fd, int type, int memory_type, int count)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;

    memset(&planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = memory_type;
    buffer.length = count;
    buffer.m.planes = planes;

    int ret = ioctl(fd, VIDIOC_DQBUF, &buffer);
    if (ret)
        return ret;

    return 0;
}

static int v4l2_stream_on(int fd, int type)
{
    enum v4l2_buf_type buf_type = type;

    return ioctl(fd, VIDIOC_STREAMON, &buf_type);
}

static int v4l2_stream_off(int fd, int type)
{
    enum v4l2_buf_type buf_type = type;

    return ioctl(fd, VIDIOC_STREAMOFF, &buf_type);
}

struct v4l2_jpeg_decoder *v4l2_jpeg_decoder_open(struct v4l2_jpeg_decoder_config *config)
{
    int fd = open(config->video_path, O_RDWR, 0);
    if (fd < 0) {
        pr_err("open");
        return NULL;
    }

    int ret = v4l2_test_capabilities(fd, V4L2_CAP_STREAMING);
    if (ret) {
        pr_err("test capture");
        goto close_fd;
    }

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = v4l2_set_format(fd, type, V4L2_PIX_FMT_JPEG, config->width, config->height);
    if (ret) {
        pr_err("set input fmt");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_set_format(fd, type, config->output_fmt, config->width, config->height);
    if (ret) {
        pr_err("set output fmt");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = v4l2_create_buffer(fd, type, V4L2_MEMORY_MMAP, 1);
    if (ret) {
        pr_err("create source buffer\n");
        goto close_fd;
    }

    unsigned char *in_mem;
    unsigned int in_size = 0;
    ret = v4l2_mmap_buffer(fd, type, V4L2_PIX_FMT_JPEG, 0, &in_mem, &in_size);
    if (ret) {
        pr_err("mmap buffer\n");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_create_buffer(fd, type, V4L2_MEMORY_MMAP, 1);
    if (ret) {
        pr_err("create dst buffer\n");
        goto close_fd;
    }

    unsigned char *out_mem[2];
    unsigned int out_size[2] = {0};
    ret = v4l2_mmap_buffer(fd, type, config->output_fmt, 0, out_mem, out_size);
    if (ret) {
        pr_err("mmap buffer\n");
        goto close_fd;
    }

    ret = v4l2_stream_on(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (ret) {
        pr_err("stream on source\n");
        goto close_fd;
    }

    ret = v4l2_stream_on(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret) {
        pr_err("stream on dst\n");
        goto close_fd;
    }

    struct v4l2_jpeg_decoder *decoder = malloc(sizeof(*decoder));
    assert(decoder);

    decoder->fd = fd;
    decoder->in_mem = in_mem;
    decoder->in_size = in_size;
    decoder->y_mem = out_mem[0];
    decoder->uv_mem = out_mem[1];
    decoder->config = *config;

    return decoder;
close_fd:
    close(fd);
    return NULL;
}

int v4l2_jpeg_decoder_close(struct v4l2_jpeg_decoder *decoder)
{
    int y_size = decoder->config.height * decoder->config.width;
    int ret = v4l2_stream_off(decoder->fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (ret)
        pr_err("stream off source\n");

    ret = v4l2_stream_off(decoder->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret)
        pr_err("stream off dst\n");

    munmap(decoder->in_mem, decoder->in_size);
    munmap(decoder->y_mem, y_size);
    munmap(decoder->uv_mem, y_size / 2);

    close(decoder->fd);

    free(decoder);

    return 0;
}

int v4l2_jpeg_decoder_work(struct v4l2_jpeg_decoder *decoder, void *input_mem, unsigned int input_size, void **output_mem)
{
    memcpy(decoder->in_mem, input_mem, input_size);

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    int memory_type = V4L2_MEMORY_MMAP;
    /* 理论上input_size传入前后没有改变 */    
    int ret = v4l2_queue_buffer(decoder->fd, type, memory_type, &decoder->in_mem, &input_size, 1);
    if (ret) {
        pr_err("queue src buffer");
        return ret;
    }

    int out_size = 0;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    memory_type = V4L2_MEMORY_MMAP;
    ret = v4l2_queue_buffer(decoder->fd, type, memory_type, NULL, &out_size, 2);
    if (ret) {
        pr_err("queue dst buffer");
        return ret;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(decoder->fd, &fds);
    struct timeval tv = { 0, 300000 };
    ret = select(decoder->fd+1, &fds, NULL, NULL, &tv);
    if (ret < 0) {
        pr_err("select result");
        return ret;
    }
    if (ret == 0) {
        pr_err("select timeout");
        ret = -99;
        return ret;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    memory_type = V4L2_MEMORY_MMAP;
    ret = v4l2_dequeue_buffer(decoder->fd, type, memory_type, 1);
    if (ret) {
        pr_err("dequeue src");
        return ret;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    memory_type = V4L2_MEMORY_MMAP;
    ret = v4l2_dequeue_buffer(decoder->fd, type, memory_type, 2);
    if (ret) {
        pr_err("dequeue dst");
        return ret;
    }

    output_mem[0] = decoder->y_mem;
    output_mem[1] = decoder->uv_mem;

    return 0;
}