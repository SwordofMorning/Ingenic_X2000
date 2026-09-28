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
#include <libhardware2/v4l2_camera.h>

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


#define FMT_C(f) \
    ((char *)&(f))[0],((char *)&(f))[1],((char *)&(f))[2],((char *)&(f))[3]

static int v4l2_format_planes_count(unsigned int format)
{
    switch (format) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        return 2;
    default:
        return 1;
    }
    return 0;
}

static int is_planes(int type)
{
    return type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
           type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
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
    case V4L2_PIX_FMT_MJPEG:
        f->fmt.pix.width = width;
        f->fmt.pix.height = height;
        f->fmt.pix.pixelformat = format;
        f->fmt.pix.sizeimage = width*height*3/2;
        break;
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YYUV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
        f->fmt.pix.width = width;
        f->fmt.pix.height = height;
        f->fmt.pix.pixelformat = format;
        f->fmt.pix.sizeimage = width*height*2;
        break;
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
        f->fmt.pix.width = width;
        f->fmt.pix.height = height;
        f->fmt.pix.pixelformat = format;
        f->fmt.pix.sizeimage = width*height;
        break;

    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_Y16:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SBGGR16:
    // 上层没有这几个定义,我们自己定义的
    case V4L2_PIX_FMT_SGBRG16:
    case V4L2_PIX_FMT_SGRBG16:
    case V4L2_PIX_FMT_SRGGB16:
        f->fmt.pix.width = width;
        f->fmt.pix.height = height;
        f->fmt.pix.pixelformat = format;
        f->fmt.pix.sizeimage = width*height*2;
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

static int v4l2_mmap_buffer(
    int fd, int type, int format, int index, unsigned char **mem, int *mem_size)
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
    if (is_planes(type)) {
        buffer.length = count;
        buffer.m.planes = planes;
    }

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
    if (ret < 0) {
        fprintf(stderr, "failed to query buffer\n");
        return -1;
    }

    int i;
    for (i = 0; i < count; i++) {
        unsigned int offset, length;
        if (is_planes(type)) {
            offset = buffer.m.planes[i].m.mem_offset;
            length = buffer.m.planes[i].length;
        } else {
            offset = buffer.m.offset;
            length = buffer.length;
        }

        mem[i] = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        if (mem[i] == MAP_FAILED) {
            fprintf(stderr, "failed to mmap: %x %d\n", offset, length);
            return -1;
        }
        mem_size[i] = length;
    }

    return 0;
}

static int v4l2_munmap_buffer(int fd, int format, unsigned char **mem, int *mem_size)
{
    int planes = v4l2_format_planes_count(format);

    int i;
    for (i = 0; i < planes; i++) {
        int ret = munmap(mem[i], mem_size[i]);
        if (ret)
            fprintf(stderr, "failed to munmap: %p %d\n", mem[i], mem_size[i]);
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
    int fd, int type, int memory_type, int index, unsigned char **mem, int *size, int count, int flag)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;
    int ret;

    memset(&planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = memory_type;
    buffer.index = index;
    buffer.flags = flag;
    if (is_planes(type)) {
        buffer.length = count;
        buffer.m.planes = planes;
    }

    if (count == 1) {
        buffer.bytesused = size[0];
        if (memory_type == V4L2_MEMORY_USERPTR)
            buffer.m.userptr = (unsigned long)mem[0];
    } else {
        int i;
        for (i = 0; i < count; i++) {
            buffer.m.planes[i].bytesused = size[i];
            buffer.m.planes[i].data_offset = 0;
            buffer.m.planes[i].length = size[i];
            if (memory_type == V4L2_MEMORY_USERPTR)
                buffer.m.planes[i].m.userptr = (unsigned long)mem[i];
        }
    }

    ret = ioctl(fd, VIDIOC_QBUF, &buffer);
    if (ret) {
        fprintf(stderr, "failed to qbuf: %d %d %p %d\n",
             type, memory_type, mem?mem[0]:NULL, size?size[0]:-1);
    }

    return ret;
}

static int v4l2_dequeue_buffer(
    int fd, int type, int memory_type, int *index, int *size, int count)
{
    struct v4l2_plane planes[count];
    struct v4l2_buffer buffer;

    memset(&planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = memory_type;
    if (is_planes(type)) {
        buffer.length = count;
        buffer.m.planes = planes;
    }

    int ret = ioctl(fd, VIDIOC_DQBUF, &buffer);
    if (ret)
        return ret;

    if (is_planes(type)) {
        int i;
        for (i = 0; i < count; i++) {
            if (size)
                size[i] = planes[i].bytesused;
        }
    } else {
        if (size)
            size[0] = buffer.bytesused;
    }

    if (index)
        *index = buffer.index;

    return 0;
}

static int v4l2_wait_dequeue_buffer(int fd, int time_msecs)
{
    struct timeval timeout;
    timeout.tv_sec = time_msecs/1000;
    timeout.tv_usec = (time_msecs%1000)*1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    int ret = select(fd + 1, &fds, 0, 0, time_msecs == -1 ? NULL : &timeout);
    if (ret == -1)
        return -1;
    if (ret == 0)
        return 0;
    return 1;
}

static int do_find_fmt(
    int fd, int type, unsigned int *format, int *width, int *height)
{
    int k = 0;
    while (1) {
        struct v4l2_fmtdesc desc;
        memset(&desc, 0, sizeof(desc));
        desc.index = k++;
        desc.type = type;

        int ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
        if (ret)
            break;

        if (*format && *format != desc.pixelformat)
            continue;

        int j = 0;
        while (1) {
            struct v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(frmsize));
            frmsize.pixel_format = desc.pixelformat;
            frmsize.index = j;
            frmsize.type = V4L2_FRMSIZE_TYPE_DISCRETE;
            ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
            if (ret)
                break;

            // printf("%d: %c%c%c%c %d %d \n", j, FMT_C(frmsize.pixel_format),
            //         frmsize.discrete.width, frmsize.discrete.height);
            j++;

            if (*width && *width != frmsize.discrete.width)
                continue;
            if (*height && *height != frmsize.discrete.height)
                continue;

            *width = frmsize.discrete.width;
            *height = frmsize.discrete.height;
            *format = desc.pixelformat;
            return 0;
        }
    }

    return -1;
}

static int v4l2_find_capture_format(
    int fd, int *type, unsigned int *format, int *width, int *height)
{
    int ret, i;

    if (*type)
        return do_find_fmt(fd, *type, format, width, height);

    int types[] = {
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    };

    for (i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        ret = do_find_fmt(fd, types[i], format, width, height);
        if (ret == 0) {
            *type = types[i];
            return 0;
        }
    }

    fprintf(stderr, "failed to find capture format: %c%c%c%c %d %d\n",
            FMT_C(*format), *width, *height);

    return -1;
}

struct v4l2_camera {
    int fd;
    struct v4l2_camera_format fmt;
    struct v4l2_camera_buffer *buf;
    int planes;
    int buf_cnt;
};

int v4l2_camera_enum_formats(
    struct v4l2_camera *camera, struct v4l2_camera_format *fmts, int count)
{
    int fd = camera->fd;
    int types[] = {
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    };

    int i, cnt = 0;
    for (i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        int k = 0;
        while (1) {
            struct v4l2_fmtdesc desc;
            memset(&desc, 0, sizeof(desc));
            desc.index = k++;
            desc.type = types[i];

            int ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
            if (ret)
                break;

            int j = 0;
            while (1) {
                struct v4l2_frmsizeenum frmsize;
                memset(&frmsize, 0, sizeof(frmsize));
                frmsize.pixel_format = desc.pixelformat;
                frmsize.index = j;
                frmsize.type = V4L2_FRMSIZE_TYPE_DISCRETE;
                ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
                if (ret)
                    break;

                j++;

                if (cnt < count) {
                    fmts[cnt].type = desc.type;
                    fmts[cnt].format = desc.pixelformat;
                    fmts[cnt].width = frmsize.discrete.width;
                    fmts[cnt].height = frmsize.discrete.height;
                }

                if (++cnt >= count)
                    return cnt;
            }
        }
    }

    return cnt;
}


struct v4l2_camera *v4l2_camera_open(const char *path)
{
    int fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "failed to open: %s\n", path);
        return NULL;
    }

    int ret = v4l2_test_capabilities(fd, V4L2_CAP_VIDEO_CAPTURE);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "check capture", strerror(errno));
        goto close_fd;
    }

    ret = v4l2_test_capabilities(fd, V4L2_CAP_STREAMING);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "check streaming", strerror(errno));
        goto close_fd;
    }

    struct v4l2_camera *camera = malloc(sizeof(*camera));
    if (!camera) {
        fprintf(stderr, "failed to malloc camera: %s\n", strerror(errno));
        goto close_fd;
    }

    memset(camera, 0, sizeof(*camera));
    camera->fd = fd;

    return camera;

close_fd:
    close(fd);
    return NULL;
}

int v4l2_camera_detect_format(struct v4l2_camera *camera, struct v4l2_camera_format *fmt)
{
    int ret = v4l2_find_capture_format(
        camera->fd, &fmt->type, &fmt->format, &fmt->width, &fmt->height);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "find fmt", strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_camera_create_buffer(
    struct v4l2_camera *camera, struct v4l2_camera_format *fmt, int buf_cnt)
{
    int ret = v4l2_set_format(
        camera->fd, fmt->type, fmt->format, fmt->width, fmt->height);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "set fmt", strerror(errno));
        return -1;
    }

    ret = v4l2_create_buffer(camera->fd, fmt->type, V4L2_MEMORY_MMAP, buf_cnt);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "create buffer", strerror(errno));
        return -1;
    }

    struct v4l2_camera_buffer *buf = malloc(buf_cnt * sizeof(*buf));
    if (!buf) {
        fprintf(stderr, "failed to malloc v4l2_camera_buffer: %s", strerror(errno));
        return -1;
    }
    memset(buf, 0, sizeof(*buf) * buf_cnt);

    int planes = v4l2_format_planes_count(fmt->format);

    int i;
    for (i = 0; i < buf_cnt; i++) {
        ret = v4l2_mmap_buffer(
            camera->fd, fmt->type, fmt->format, i, buf[i].data, buf[i].size);
        if (ret) {
            fprintf(stderr, "failed to [%s]: %s\n", "mmap buffer", strerror(errno));
            goto err_unmap_buffer;
        }
        buf[i].index = i;
    }

    for (i = 0; i < buf_cnt; i++) {
        ret = v4l2_queue_buffer(
            camera->fd, fmt->type, V4L2_MEMORY_MMAP, i, buf[i].data, buf[i].size, planes, 0);
        if (ret) {
            fprintf(stderr, "failed to [%s]: %s\n", "queue buffer", strerror(errno));
            goto err_dqueue_buffer;
        }
    }

    camera->buf = buf;
    camera->fmt = *fmt;
    camera->planes = planes;
    camera->buf_cnt = buf_cnt;

    return 0;

err_dqueue_buffer:
    while (i--) {
        v4l2_dequeue_buffer(
            camera->fd, camera->fmt.type, V4L2_MEMORY_MMAP, &camera->buf[i - 1].index, camera->buf[i - 1].size, camera->planes);
    }
    i = buf_cnt;
err_unmap_buffer:
    while (i--) {
        v4l2_munmap_buffer(
            camera->fd, camera->fmt.format, camera->buf[i - 1].data, camera->buf[i - 1].size);
    }
    free(buf);
    return -1;
}

int v4l2_camera_stream_on(struct v4l2_camera *camera)
{
    int ret = v4l2_stream_on(camera->fd, camera->fmt.type);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "stream on", strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_camera_stream_off(struct v4l2_camera *camera)
{
    int ret = v4l2_stream_off(camera->fd, camera->fmt.type);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "stream off", strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_camera_dequeue_buffer(
    struct v4l2_camera *camera, struct v4l2_camera_buffer *buf, int timeout_ms)
{
    int ret = v4l2_wait_dequeue_buffer(camera->fd, timeout_ms);
    if (ret < 0) {
        fprintf(stderr, "failed to [%s]: %s\n", "wait buffer", strerror(errno));
        return -1;
    }
    if (ret == 0) {
        return 1;
    }

    ret = v4l2_dequeue_buffer(
        camera->fd, camera->fmt.type, V4L2_MEMORY_MMAP,
         &buf->index, buf->size, camera->planes);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "dequeue buffer", strerror(errno));
        return -1;
    }

    memcpy(buf->data, camera->buf[buf->index].data, sizeof(buf->data));

    return 0;
}

int v4l2_camera_queue_buffer(
    struct v4l2_camera *camera, struct v4l2_camera_buffer *buf)
{
    struct v4l2_camera_buffer *b = &camera->buf[buf->index];

    int ret = v4l2_queue_buffer(
        camera->fd, camera->fmt.type, V4L2_MEMORY_MMAP,
         buf->index, b->data, b->size, camera->planes, 0);
    if (ret) {
        fprintf(stderr, "failed to [%s]: %s\n", "queue buffer", strerror(errno));
        return -1;
    }

    return 0;
}

void v4l2_camera_close(struct v4l2_camera *camera)
{
    int i;
    for (i = 0; i < camera->buf_cnt; i++) {
        v4l2_munmap_buffer(
            camera->fd, camera->fmt.format, camera->buf[i].data, camera->buf[i].size);
    }

    if (camera->buf)
        free(camera->buf);

    close(camera->fd);
    free(camera);
}

void v4l2_camera_dump_format(struct v4l2_camera_format *fmt)
{
    fprintf(stderr, "type:%d fmt:%c%c%c%c size:%dx%d\n",
        fmt->type, FMT_C(fmt->format), fmt->width, fmt->height);
}
