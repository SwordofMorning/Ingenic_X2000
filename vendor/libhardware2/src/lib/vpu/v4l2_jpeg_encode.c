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
#include <libhardware2/v4l2_jpeg_encode.h>
#include <linux/v4l2-controls.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// #define DBG_MODE
#ifdef DBG_MODE
#define debug_pr(fmt, args...) printf(fmt, ##args);
#else
#define debug_pr(fmt, args...)
#endif

struct v4l2_frame_info {
    unsigned int width;
    unsigned int height;
    unsigned int line_length;
};

struct format_description {
    char *description;
    unsigned int v4l2_format;
    unsigned int v4l2_buffers_count;
    bool v4l2_mplane;
    unsigned int planes_count;
};

struct video_buffer {
    /*source*/
    void *source_map[VIDEO_MAX_PLANES];
    unsigned int source_map_lengths[VIDEO_MAX_PLANES];
    void *source_data[VIDEO_MAX_PLANES];
    unsigned int source_sizes[VIDEO_MAX_PLANES];
    unsigned int source_planes_count;
    unsigned int source_buffers_count;
    unsigned int source_line_length;

    /*dst*/
    void *destination_map[VIDEO_MAX_PLANES];
    unsigned int destination_map_lengths[VIDEO_MAX_PLANES];
    void *destination_data[VIDEO_MAX_PLANES];
    unsigned int destination_sizes[VIDEO_MAX_PLANES];
    unsigned int destination_planes_count;
    unsigned int destination_buffers_count;

    unsigned int output_type;
    unsigned int capture_type;
};

struct v4l2_jpeg_encoder {
    int fd;
    struct video_buffer buffer;
    struct v4l2_frame_info info;
};

/* VPU支持的编码格式 */
static struct format_description formats[] = {
    {
        .description        = "NV12",
        .v4l2_format        = V4L2_PIX_FMT_NV12,
        .v4l2_buffers_count = 2,
        .v4l2_mplane        = true,
        .planes_count       = 2,
    },
    {
        .description        = "NV21",
        .v4l2_format        = V4L2_PIX_FMT_NV21,
        .v4l2_buffers_count = 2,
        .v4l2_mplane        = true,
        .planes_count       = 2,
    },
};

static bool type_is_mplane(unsigned int type)
{
    switch (type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        return true;

    default:
        return false;
    }
}

static bool type_is_output(unsigned int type)
{
    switch (type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        return true;

    default:
        return false;
    }
}

/* 选择视频输入功能 */
static int query_capabilities(int video_fd, unsigned int *capabilities)
{
    struct v4l2_capability capability;
    int ret;

    memset(&capability, 0, sizeof(capability));

    ret = ioctl(video_fd, VIDIOC_QUERYCAP, &capability);
    if (ret < 0)
        return -1;

    if (capabilities != NULL) {
        if ((capability.capabilities & V4L2_CAP_DEVICE_CAPS) != 0)
            *capabilities = capability.device_caps;
        else
            *capabilities = capability.capabilities;
    }

    return 0;
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

static int v4l2_set_format(int video_fd, unsigned int type, struct v4l2_frame_info *info,
                unsigned int pixelformat)
{
    struct v4l2_format v4l2_format;
    unsigned int width = info->width;
    unsigned int height = info->height;
    int ret;

    setup_format(&v4l2_format, type, width, height, pixelformat);

    ret = ioctl(video_fd, VIDIOC_S_FMT, &v4l2_format);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to set format for type %d: %s\n", type, strerror(errno));
        return -1;
    }

    return 0;
}

static int create_buffers(int video_fd, unsigned int *line_length,unsigned int type,
              unsigned int buffers_count, unsigned int *index_base)
{
    struct v4l2_create_buffers buffers;
    int ret;

    memset(&buffers, 0, sizeof(buffers));
    buffers.format.type = type;
    buffers.memory = V4L2_MEMORY_MMAP;
    buffers.count = buffers_count;

    ret = ioctl(video_fd, VIDIOC_G_FMT, &buffers.format);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to get format for type %d: %s\n",
            type, strerror(errno));
        return -1;
    }

    /* 获取 source buffer line_length */
    if (type_is_output(type))
        *line_length = buffers.format.fmt.pix_mp.plane_fmt[0].bytesperline;

    ret = ioctl(video_fd, VIDIOC_CREATE_BUFS, &buffers);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to create buffer for type %d: %s\n",
            type, strerror(errno));
        return -1;
    }

    if (index_base != NULL)
        *index_base = buffers.index;

    return 0;
}

static int query_buffer(int video_fd, unsigned int type, unsigned int index,
            unsigned int *lengths, unsigned int *offsets,
            unsigned int buffers_count)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    unsigned int i;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    ret = ioctl(video_fd, VIDIOC_QUERYBUF, &buffer);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to query buffer: %s\n",
            strerror(errno));
        return -1;
    }

    if (type_is_mplane(type)) {
        if (lengths != NULL)
            for (i = 0; i < buffer.length; i++)
                lengths[i] = buffer.m.planes[i].length;

        if (offsets != NULL)
            for (i = 0; i < buffer.length; i++)
                offsets[i] = buffer.m.planes[i].m.mem_offset;
    } else {
        if (lengths != NULL)
            lengths[0] = buffer.length;

        if (offsets != NULL)
            offsets[0] = buffer.m.offset;
    }

    return 0;
}

static int queue_buffer(int video_fd, unsigned int type,
            uint64_t ts, unsigned int index, unsigned int* sizes,
            unsigned int buffers_count)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    unsigned int i;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    for (i = 0; i < buffers_count; i++)
        if (type_is_mplane(type))
            buffer.m.planes[i].bytesused = sizes[i];
        else
            buffer.bytesused = sizes[0];

    buffer.timestamp.tv_usec = ts / 1000;
    buffer.timestamp.tv_sec = ts / 1000000000ULL;

    ret = ioctl(video_fd, VIDIOC_QBUF, &buffer);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to queue buffer: %s\n",
            strerror(errno));
        return -1;
    }

    return 0;
}

static int dequeue_buffer(int video_fd, unsigned int type,
              unsigned int *index, unsigned int buffers_count,
              bool *error, unsigned int *bytesused)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    int ret;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    /* 对于dequeue_buffer 只需要指定 type */
    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    ret = ioctl(video_fd, VIDIOC_DQBUF, &buffer);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to dequeue buffer: %s, type: %d\n",
            strerror(errno), type);
        return -1;
    }

    if (bytesused)
        *bytesused = buffer.m.planes[0].bytesused;

    *index = buffer.index;

    if (error != NULL)
        *error = !!(buffer.flags & V4L2_BUF_FLAG_ERROR);

    return 0;
}

static int v4l2_set_stream(int video_fd, unsigned int type, bool enable)
{
    enum v4l2_buf_type buf_type = type;
    int ret;

    ret = ioctl(video_fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF,
           &buf_type);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to %s stream: %s\n",
            enable ? "enable" : "disable", strerror(errno));
        return -1;
    }

    return 0;
}

static int load_data_to_buffer(void *src, struct video_buffer *buffer, unsigned int width,
                    unsigned int height, unsigned int src_line_length)
{
    void **dst_buf = buffer->source_data;
    assert(buffer->source_planes_count == 2);

    int i;
    void *dst = dst_buf[0];
    int dst_line_length = buffer->source_line_length;

    for (i = 0; i < height; i++) {
        memcpy(dst, src, width);
        dst += dst_line_length;
        src += src_line_length;
    }

    dst = dst_buf[1];
    for (i = 0; i < height / 2; i++) {
        memcpy(dst , src, width);
        dst += dst_line_length;
        src += src_line_length;
    }

    return 0;
}

/* 检查设备是否支持请求的功能 */
static bool v4l2_capabilities_check(int video_fd, unsigned int capabilities_required)
{
    unsigned int capabilities;
    int ret;

    ret = query_capabilities(video_fd, &capabilities);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to query video capabilities: %s\n",
            strerror(errno));
        return false;
    }

    if ((capabilities & capabilities_required) != capabilities_required)
        return false;

    return true;
}

/* 检查V4L2是否支持该输入格式 */
static struct format_description *v4l2_format_check(int intput_fmt)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(formats); i++) {
        if (intput_fmt == formats[i].v4l2_format)
            return &formats[i];
    }
    return NULL;
}

int v4l2_jpeg_encoder_set_quality(struct v4l2_jpeg_encoder *encoder, struct v4l2_jpeg_encoder_config *config)
{
    int ret = 0;
    int video_fd = encoder->fd;
    struct v4l2_ext_controls ctrls = {0};
    struct v4l2_ext_control ctrl[1] = {0};

    ctrls.ctrl_class = V4L2_CTRL_CLASS_JPEG;
    ctrls.controls = ctrl;
    ctrls.count = 1;

    ctrl[0].value = config->quality;
    ctrl[0].id = V4L2_CID_JPEG_COMPRESSION_QUALITY;

    ret = ioctl(video_fd, VIDIOC_S_EXT_CTRLS, &ctrls);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to set quality: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int v4l2_jpeg_encoder_get_quality(struct v4l2_jpeg_encoder *encoder)
{
    struct v4l2_ext_controls ctrls = {0};
    struct v4l2_ext_control ctrl[1] = {0};
    int video_fd = encoder->fd;

    ctrls.ctrl_class = V4L2_CTRL_CLASS_JPEG;
    ctrls.controls = ctrl;
    ctrls.count = 1;

    ctrl[0].id = V4L2_CID_JPEG_COMPRESSION_QUALITY;

    int ret = ioctl(video_fd, VIDIOC_G_EXT_CTRLS, &ctrls);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] get quality error: %s\n", strerror(errno));
        return -1;
    }

    return ctrl[0].value;
}

struct v4l2_jpeg_encoder *v4l2_jpeg_encoder_open(struct v4l2_jpeg_encoder_config *config)
{
    unsigned int source_map_offsets[VIDEO_MAX_PLANES];
    unsigned int destination_map_offsets[VIDEO_MAX_PLANES];
    unsigned int source_planes_count;
    unsigned int destination_planes_count;

    unsigned int video_buffer_count = 1;
    unsigned int video_buffer_index = 0;
    unsigned int output_fmt = V4L2_PIX_FMT_JPEG;

    unsigned int j;
    int ret;

    struct v4l2_jpeg_encoder *encoder;
    struct video_buffer *video_buffer;
    struct v4l2_frame_info *info;
    struct format_description *format;
    int video_fd;

    encoder = malloc(sizeof(struct v4l2_jpeg_encoder));
    video_buffer = &encoder->buffer;
    info = &encoder->info;
    info->width = config->width;
    info->height = config->height;
    info->line_length = config->line_length;

    video_fd = open(config->video_path, O_RDWR, 0);
    if (video_fd < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to open %s : %s \n", config->video_path, strerror(errno));
        goto free_encoder;
    }
    encoder->fd = video_fd;

    /* 检查是否支持 streaming I/O 功能 */
    ret = v4l2_capabilities_check(video_fd, V4L2_CAP_STREAMING);
    if (!ret) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Missing required driver streaming capability: %s\n", strerror(errno));
        goto close_video;
    }

    format = v4l2_format_check(config->input_fmt);
    if (!format)
        goto close_video;

    if (format->v4l2_mplane) {
        video_buffer->output_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        video_buffer->capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    } else {
        video_buffer->output_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        video_buffer->capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }

    ret = v4l2_set_format(video_fd, video_buffer->output_type, info, config->input_fmt);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unsupport input format: %s\n", strerror(errno));
        goto close_video;
    }

    ret = v4l2_set_format(video_fd, video_buffer->capture_type, info, output_fmt);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unsupport output format: %s\n", strerror(errno));
        goto close_video;
    }

    // destination_planes_count = format.planes_count;
    source_planes_count = format->planes_count;
    destination_planes_count = 1;

     /*********** Output Buffer ***********/
    ret = create_buffers(video_fd, &video_buffer->source_line_length, video_buffer->output_type, video_buffer_count, NULL);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to create source buffers: %s\n", strerror(errno));
        goto close_video;
    }

    ret = query_buffer(video_fd, video_buffer->output_type, video_buffer_index,
                video_buffer->source_map_lengths, source_map_offsets, source_planes_count);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to request source buffer: %s\n", strerror(errno));
        goto close_video;
    }

    for (j = 0; j < format->v4l2_buffers_count; j++) {
        video_buffer->source_map[j] = mmap(NULL,
                video_buffer->source_map_lengths[j],
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                video_fd,
                source_map_offsets[j]);

        if (video_buffer->source_map[j] == MAP_FAILED) {
            fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to map source buffer: %s\n", strerror(errno));
            goto close_video;
        }

        video_buffer->source_data[j] = video_buffer->source_map[j];
        video_buffer->source_sizes[j] = video_buffer->source_map_lengths[j];
    }

    video_buffer->source_planes_count = source_planes_count;
    video_buffer->source_buffers_count = video_buffer->source_planes_count;

    /*********** Capture Buffer ***********/
    ret = create_buffers(video_fd, &video_buffer->source_line_length, video_buffer->capture_type, video_buffer_count, NULL);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to create destination buffers: %s\n", strerror(errno));
        goto close_video;
    }

    ret = query_buffer(video_fd, video_buffer->capture_type, video_buffer_index, video_buffer->destination_map_lengths,
                destination_map_offsets, destination_planes_count);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to request destination buffer: %s\n", strerror(errno));
        goto close_video;
    }

    video_buffer->destination_map[0] = mmap(NULL,
                    video_buffer->destination_map_lengths[0],
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    video_fd,
                    destination_map_offsets[0]);
    if (video_buffer->destination_map[0] == MAP_FAILED) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to map destination buffer: %s\n", strerror(errno));
        goto close_video;
    }

    video_buffer->destination_data[0] = video_buffer->destination_map[0];
    video_buffer->destination_sizes[0] = video_buffer->destination_map_lengths[0];

    video_buffer->destination_planes_count = destination_planes_count;
    video_buffer->destination_buffers_count = destination_planes_count;

    ret = v4l2_set_stream(video_fd, video_buffer->output_type, true);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to enable source stream: %s\n", strerror(errno));
        goto unmmap_buffers;
    }

    ret = v4l2_set_stream(video_fd, video_buffer->capture_type, true);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to enable destination stream: %s\n", strerror(errno));
        goto unmmap_buffers;
    }

    ret = v4l2_jpeg_encoder_set_quality(encoder, config);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 H264 ENCODER] Unable to set gop_size/bitrate\n");
        goto unmmap_buffers;
    }

    return encoder;

unmmap_buffers:
    for (j = 0; j < video_buffer->source_buffers_count; j++) {
        munmap(video_buffer->source_map[j],
            video_buffer->source_map_lengths[j]);
    }

    munmap(video_buffer->destination_map[0],
                video_buffer->destination_map_lengths[0]);

close_video:
    close(video_fd);

free_encoder:
    free(encoder);

    return NULL;
}

int v4l2_jpeg_encoder_close(struct v4l2_jpeg_encoder *encoder)
{
    int ret = 0;
    unsigned int j;
    struct video_buffer *video_buffer = &encoder->buffer;
    int video_fd = encoder->fd;

    ret = v4l2_set_stream(video_fd, video_buffer->output_type, false);
    if (ret < 0)
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to disable source stream: %s\n", strerror(errno));

    ret = v4l2_set_stream(video_fd, video_buffer->capture_type, false);
    if (ret < 0)
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to disable destination stream: %s\n", strerror(errno));

    for (j = 0; j < video_buffer->source_buffers_count; j++) {
        if (video_buffer->source_map[j] == NULL)
            break;

        munmap(video_buffer->source_map[j],
            video_buffer->source_map_lengths[j]);
    }

    if (video_buffer->destination_map[0] != NULL) {
        munmap(video_buffer->destination_map[0],
            video_buffer->destination_map_lengths[0]);
    }

    close(encoder->fd);

    free(encoder);

    return ret;
}

void *v4l2_jpeg_encoder_work(struct v4l2_jpeg_encoder *encoder, void *input_mem, unsigned int *ouput_size)
{
    bool source_error, destination_error;
    struct video_buffer *video_buffer = &encoder->buffer;
    struct v4l2_frame_info info = encoder->info;
    int video_fd = encoder->fd;
    /* 在该应用中只用到一个buffer */
    unsigned int index = 0;
    int ret;

    load_data_to_buffer(input_mem, video_buffer, info.width, info.height, info.line_length);

    ret = queue_buffer(video_fd, video_buffer->output_type, 0, index, video_buffer->source_sizes,
           video_buffer->source_planes_count);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to queue source buffer: %s\n", strerror(errno));
        return NULL;
    }

    ret = queue_buffer(video_fd, video_buffer->capture_type, 0, index, video_buffer->destination_sizes,
            video_buffer->destination_planes_count);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to queue destination buffer: %s\n", strerror(errno));
        return NULL;
    }

    int out_index = 0;
    ret = dequeue_buffer(video_fd, video_buffer->output_type, &out_index, video_buffer->source_buffers_count,
                &source_error, NULL);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to dequeue source buffer: %s\n", strerror(errno));
        return NULL;
    }

    int cap_index = 0;
    ret = dequeue_buffer(video_fd, video_buffer->capture_type, &cap_index,
                video_buffer->destination_buffers_count,
                &destination_error, ouput_size);
    if (ret < 0) {
        fprintf(stderr, "[V4L2 JPEG ENCODER] Unable to dequeue destination buffer: %s\n", strerror(errno));
        return NULL;
    }

    return video_buffer->destination_data[0];
}
