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
#include <libhardware2/v4l2_h264_encode.h>

// struct v4l2_h264_encoder_config {
//     const char *video_path;
//     unsigned int width;
//     unsigned int height;
//     unsigned int line_length;
//     unsigned int input_fmt;
//     unsigned int gop_size;
//     unsigned int bitrate;
// };

struct v4l2_h264_encoder {
    int fd;
    int rmem_fd;
    void *out_mem;
    int out_size;
    void *y;
    unsigned long y_phys;
    void *uv;
    int key_frame;
    unsigned long uv_phys;
    struct v4l2_h264_encoder_config c;
    int is_key;
};

#define ALIGN(d,a) (((d)+((a)-1))/(a)*(a))

#define pr_err(msg) \
    fprintf(stderr, "error: %s: %s\n", msg, strerror(errno))

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


static int v4l2_set_h264_ctrl(int fd, struct v4l2_h264_encoder_config *config)
{
	struct v4l2_ext_controls ctrls = { 0 };
	struct v4l2_ext_control ctrl[12] = { 0 };

	/* set ctrls */
	ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
	ctrls.controls = ctrl;
	ctrls.count = 12;
	/* set ctrl*/
	ctrl[0].value = config->bitrate;
	ctrl[0].id = V4L2_CID_MPEG_VIDEO_BITRATE;

	ctrl[1].value = config->gop_size;
	ctrl[1].id = V4L2_CID_MPEG_VIDEO_GOP_SIZE;

	ctrl[2].value = 30;
	ctrl[2].id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;

	ctrl[3].value = 30;
	ctrl[3].id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;

	ctrl[4].value = 40;
	ctrl[4].id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP;

	ctrl[5].value = 10;
	ctrl[5].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;

	ctrl[6].value = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME;
	ctrl[6].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;

	ctrl[7].value = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN;
	ctrl[7].id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;

	ctrl[8].value = V4L2_MPEG_VIDEO_H264_LEVEL_3_0;
	ctrl[8].id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;

	ctrl[9].value = 1;
	ctrl[9].id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;

	ctrl[10].value = 0;
	ctrl[10].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;

	ctrl[11].value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	ctrl[11].id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;

	return ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls);
}

#define FMT_C(f) \
    ((char *)&(f))[0],((char *)&(f))[1],((char *)&(f))[2],((char *)&(f))[3]

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
        break;
    case V4L2_PIX_FMT_H264:
        f->fmt.pix.width = width;
        f->fmt.pix.height = height;
        f->fmt.pix.pixelformat = format;
        f->fmt.pix.sizeimage = width*height*3/2;
        break;
    default: {
        fprintf(stderr, "not support this format: %c%c%c%c\n", FMT_C(format));
        errno = EINVAL;
        return -1;
    }
    }

    return 0;
}

static int v4l2_set_format(int fd, int type, unsigned format, int width, int height)
{
    int ret;
    struct v4l2_format f;

    ret = init_v4l2_formt_t(&f, type, format, width, height);
    if (ret)
        return -1;

    ret = ioctl(fd, VIDIOC_S_FMT, &f);
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

static int v4l2_mmap_buffer(int fd, int type, int format, int index, unsigned char **mem, unsigned int *mem_size)
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

static int v4l2_queue_buffer(
    int fd, int type, int memory_type, void **mem, int *size, int count, int flag)
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
    buffer.flags = flag;

    if (count == 1) {
        buffer.bytesused = size[0];
        goto queue_buffer;
    }

    int i;
    for (i = 0; i < count; i++) {
        buffer.m.planes[i].bytesused = size[i];
        buffer.m.planes[i].data_offset = 0;
        buffer.m.planes[i].length = size[i];
        if (memory_type == V4L2_MEMORY_USERPTR)
            buffer.m.planes[i].m.userptr = (unsigned long)mem[i];
    }

queue_buffer:
    ret = ioctl(fd, VIDIOC_QBUF, &buffer);
    if (ret) {
        fprintf(stderr, "failed to qbuf: %d %d %p %d\n",
             type, memory_type, mem?mem[0]:NULL, size?size[0]:-1);
    }

    return ret;
}


static int v4l2_dequeue_buffer(
    int fd, int type, int memory_type, int *size, int count, int *key_flags)
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

    if (key_flags)
        *key_flags = (bool)(buffer.flags & V4L2_BUF_FLAG_KEYFRAME);

    int i;
    for (i = 0; i < count; i++) {
        if (size)
            size[i] = planes[i].bytesused;
    }

    return 0;
}

static void *alloc_phys_buffer(
    struct v4l2_h264_encoder *encoder, unsigned long *phys, int size)
{
    if (encoder->rmem_fd < 0) {
        fprintf(stderr, "error: rmem not open: can't alloc phys buffer\n");
        return NULL;
    }

    return rmem_alloc(encoder->rmem_fd, phys, size);
}

static int check_alloc_y_mem(struct v4l2_h264_encoder *encoder)
{
    if (encoder->y)
        return 0;

    int page_size = getpagesize();
    int size = ALIGN(encoder->c.width*encoder->c.height, page_size);
    encoder->y = alloc_phys_buffer(encoder, &encoder->y_phys, size);

    return encoder->y ? 0 : -1;
}

static int check_alloc_uv_mem(struct v4l2_h264_encoder *encoder)
{
    if (encoder->uv)
        return 0;

    int page_size = getpagesize();
    int size = ALIGN(encoder->c.width*encoder->c.height/2, page_size);
    encoder->uv = alloc_phys_buffer(encoder, &encoder->uv_phys, size);

    return encoder->uv ? 0 : -1;
}

static void *do_encode_work(
    struct v4l2_h264_encoder *encoder, void *y, void *uv, unsigned int *output_size)
{
    int y_size = encoder->c.height*encoder->c.width;

    void *mem[2] = {y, uv};
    int size[2] = {y_size, y_size/2};

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    int memory_type = V4L2_MEMORY_USERPTR;
    int ret = v4l2_queue_buffer(encoder->fd, type, memory_type, mem, size, 2, encoder->key_frame);
    if (ret) {
        pr_err("queue src buffer");
        return NULL;
    }

    encoder->key_frame = 0;

    size[0] = y_size * 3/2;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    memory_type = V4L2_MEMORY_MMAP;
    ret = v4l2_queue_buffer(encoder->fd, type, memory_type, &encoder->out_mem, size, 1, 0);
    if (ret) {
        pr_err("queue dst buffer");
        return NULL;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(encoder->fd, &fds);
    struct timeval tv = { 0, 300000 };
    ret = select(encoder->fd+1, &fds, NULL, NULL, &tv);
    if (ret < 0) {
        pr_err("select result");
        return NULL;
    }
    if (ret == 0) {
        pr_err("select timeout");
        return NULL;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    memory_type = V4L2_MEMORY_USERPTR;
    ret = v4l2_dequeue_buffer(encoder->fd, type, memory_type, NULL, 2, NULL);
    if (ret) {
        pr_err("dequeue src");
        return NULL;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    memory_type = V4L2_MEMORY_MMAP;
    ret = v4l2_dequeue_buffer(encoder->fd, type, memory_type, (void *)output_size, 1, &encoder->is_key);
    if (ret) {
        pr_err("dequeue dst");
        return NULL;
    }

    return encoder->out_mem;
}

void *v4l2_h264_encoder_work(struct v4l2_h264_encoder *encoder, void *input_mem,
                unsigned int *output_size)
{
    int y_size = encoder->c.height*encoder->c.width;
    void *y_mem = input_mem;
    void *uv_mem = input_mem + y_size;

    return v4l2_h264_encoder_work_separate(encoder, y_mem, uv_mem, output_size);
}

void *v4l2_h264_encoder_work_separate(struct v4l2_h264_encoder *encoder,
                void *y_mem, void *uv_mem, unsigned int *output_size)
{
    if (check_alloc_y_mem(encoder) || check_alloc_uv_mem(encoder)) {
        pr_err("check mem\n");
        return NULL;
    }

    int y_size = encoder->c.height*encoder->c.width;
    memcpy(encoder->y, y_mem, y_size);
    memcpy(encoder->uv, uv_mem, y_size/2);

    return do_encode_work(encoder, encoder->y, encoder->uv, output_size);
}

void *v4l2_h264_encoder_work_phys(struct v4l2_h264_encoder *encoder,
                void *input_mem, unsigned int *output_size)
{
    int y_size = encoder->c.height*encoder->c.width;
    void *y_mem = input_mem;
    void *uv_mem = input_mem + y_size;

    return v4l2_h264_encoder_work_phys_separate(encoder, y_mem, uv_mem, output_size);
}

void *v4l2_h264_encoder_work_phys_separate(struct v4l2_h264_encoder *encoder,
                void *y_mem, void *uv_mem, unsigned int *output_size)
{
    int page_size = getpagesize();

    int y_size = encoder->c.height*encoder->c.width;

    void *y_work_mem = y_mem;
    void *uv_work_mem = uv_mem;

    if ((unsigned long)y_mem % page_size) {
        if (check_alloc_y_mem(encoder)) {
            pr_err("check y mem\n");
            return NULL;
        }
        y_work_mem = encoder->y;
        memcpy(y_work_mem, y_mem, y_size);
    }

    if ((unsigned long)uv_mem % page_size) {
        if (check_alloc_uv_mem(encoder)) {
            pr_err("check uv mem\n");
            return NULL;
        }
        uv_work_mem = encoder->uv;
        memcpy(encoder->uv, uv_mem, y_size/2);
    }

    return do_encode_work(encoder, y_work_mem, uv_work_mem, output_size);
}

/**
 * 与 v4l2_h264_encoder_work 区别:
 * 底层直接使用inputmem的内存作为VPU输入源,内存占用更少.(但要求mem必须是物理上连续的)
 * */
void *v4l2_h264_encoder_work_by_phy_mem(struct v4l2_h264_encoder *encoder,
                void *input_mem, unsigned long phy_mem, unsigned int *output_size)
{
    int page_size = getpagesize();

    int y_size = encoder->c.height*encoder->c.width;
    unsigned long y = phy_mem;
    unsigned long uv = (unsigned long) phy_mem + y_size;

    void *y_mem = input_mem;
    void *uv_mem = input_mem + y_size;

    if (y%page_size) {
        if (check_alloc_y_mem(encoder)) {
            pr_err("check y mem\n");
            return NULL;
        }
        y_mem = encoder->y;
        memcpy(y_mem, input_mem, y_size);
    }

    if (uv%page_size) {
        if (check_alloc_uv_mem(encoder)) {
            pr_err("check uv mem\n");
            return NULL;
        }
        uv_mem = encoder->uv;
        memcpy(uv_mem, input_mem + y_size, y_size/2);
    }

    return do_encode_work(encoder, y_mem, uv_mem, output_size);
}

struct v4l2_h264_encoder *v4l2_h264_encoder_open(struct v4l2_h264_encoder_config *config)
{
    int rmem_fd = rmem_open();
    if (rmem_fd < 0) {
        pr_err("open rmem");
        return NULL;
    }

    int fd = open(config->video_path, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        pr_err("open");
        goto close_fd;
    }

    int ret = v4l2_test_capabilities(fd, V4L2_CAP_STREAMING);
    if (ret) {
        pr_err("test capture");
        goto close_fd;
    }

    ret = v4l2_set_h264_ctrl(fd, config);
    if (ret) {
        pr_err("set h264 ctrl");
        goto close_fd;
    }

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = v4l2_set_format(fd, type, config->input_fmt, config->width, config->height);
    if (ret) {
        pr_err("set input fmt");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_set_format(fd, type, V4L2_PIX_FMT_H264, config->width, config->height);
    if (ret) {
        pr_err("set output fmt");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = v4l2_create_buffer(fd, type, V4L2_MEMORY_USERPTR, 1);
    if (ret) {
        pr_err("create source buffer\n");
        goto close_fd;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_create_buffer(fd, type, V4L2_MEMORY_MMAP, 1);
    if (ret) {
        pr_err("create dst buffer\n");
        goto close_fd;
    }

    unsigned char *mem;
    unsigned int mem_size;
    ret = v4l2_mmap_buffer(fd, type, V4L2_PIX_FMT_H264, 0, &mem, &mem_size);
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

    struct v4l2_h264_encoder *encoder = malloc(sizeof(*encoder));
    assert(encoder);

    encoder->fd = fd;
    encoder->out_mem = mem;
    encoder->out_size = mem_size;
    encoder->rmem_fd = rmem_fd;
    encoder->y = NULL;
    encoder->uv = NULL;
    encoder->c = *config;


    return encoder;
close_fd:
    close(fd);
    rmem_close(rmem_fd);
    return NULL;
}

int v4l2_h264_encoder_close(struct v4l2_h264_encoder *encoder)
{
    int size = encoder->c.height*encoder->c.width;
    int page_size = getpagesize();

    if (encoder->rmem_fd >= 0) {
        if (encoder->y)
            rmem_free(encoder->rmem_fd, encoder->y, encoder->y_phys, ALIGN(size, page_size));
        if (encoder->uv)
            rmem_free(encoder->rmem_fd, encoder->uv, encoder->uv_phys, ALIGN(size/2, page_size));
        rmem_close(encoder->rmem_fd);
    }

    int ret = v4l2_stream_off(encoder->fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (ret)
        pr_err("stream off source\n");

    ret = v4l2_stream_off(encoder->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret)
        pr_err("stream off dst\n");

    munmap(encoder->out_mem, encoder->out_size);

    close(encoder->fd);

    free(encoder);

    return 0;
}

void v4l2_h264_encoder_set_keyframe(struct v4l2_h264_encoder *encoder)
{
    encoder->key_frame = V4L2_BUF_FLAG_KEYFRAME;
}

int v4l2_h264_encoder_get_stream_param(struct v4l2_h264_encoder *encoder, struct v4l2_streamparm *param)
{
    return ioctl(encoder->fd, VIDIOC_G_PARM, param);
}

int v4l2_h264_encoder_get_whether_keyframe(struct v4l2_h264_encoder *encoder)
{
    return encoder->is_key;
}