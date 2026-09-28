#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <libisp/isp.h>
#include <libmedia/video_reader.h>
#include <libmedia/read/isp_video_reader.h>
#include <libmedia/video_frame.h>
#include <fcntl.h>
#include <unistd.h>

struct isp_video_reader {
    struct video_reader reader;

    int fd;
    struct camera_info info;

    struct video_frame *frames;
};

static struct video_reader *isp_video_reader_open(struct video_reader_param *param)
{
    int ret;

    struct isp_video_reader_param *p = (void *)param;
    assert(p);
    assert(p->isp_device);

    struct frame_image_format *isp_param = &p->isp_params;

    if (isp_param->pixel_format != CAMERA_PIX_FMT_NV12) {
        fprintf(stderr, "isp_video_reader: isp fmt must be CAMERA_PIX_FMT_NV12\n");
        return NULL;
    }

    struct isp_video_reader *isp = malloc(sizeof(*isp));
    assert(isp);


    int isp_fd = isp_open(p->isp_device);
    if (isp_fd < 0) {
        fprintf(stderr, "isp_video_reader: failed to open isp device\n");
        goto free_isp;
    }

    ret = isp_set_format(isp_fd, isp_param);
    if (ret < 0) {
        fprintf(stderr, "isp_video_reader: failed to set isp format\n");
        goto close_isp;
    }

    ret = isp_requset_buffer(isp_fd, isp_param);
    if (ret < 0) {
        fprintf(stderr, "isp_video_reader: failed to request isp buffer\n");
        goto close_isp;
    }

    ret = isp_get_info(isp_fd, &isp->info);
    if (ret < 0) {
        fprintf(stderr, "isp_video_reader: failed to get isp info\n");
        goto free_isp_buffer;
    }

    ret = isp_mmap(isp_fd, &isp->info);
    if (ret < 0) {
        fprintf(stderr, "isp_video_reader: failed to mmap isp\n");
        goto free_isp_buffer;
    }

    ret = isp_power_on(isp_fd);
    if (ret) {
        fprintf(stderr, "isp_video_reader: failed to power on isp\n");
        goto free_isp_buffer;
    }

    ret = isp_stream_on(isp_fd);
    if (ret) {
        fprintf(stderr, "isp_video_reader: failed to stream on isp\n");
        goto power_off_isp;
    }

    isp->fd = isp_fd;

    struct video_frame *frames = malloc((isp->info.frame_nums + 1) * sizeof(*frames));
    assert(frames);

    isp->frames = frames;

    return &isp->reader;
power_off_isp:
    isp_power_off(isp_fd);
free_isp_buffer:
    isp_free_buffer(isp_fd);
close_isp:
    isp_close(isp_fd, &isp->info);
free_isp:
    free(isp);
    return NULL;
}

static void isp_video_reader_close(struct video_reader *reader)
{
    struct isp_video_reader *isp = (void *)reader;

    isp_stream_off(isp->fd);
    isp_power_off(isp->fd);
    isp_free_buffer(isp->fd);
    isp_close(isp->fd, &isp->info);

    free(isp->frames);
    free(isp);
}

static void isp_video_reader_put_video(void *handle, struct video_frame *frame)
{
    struct isp_video_reader *isp = handle;

    isp_put_frame(isp->fd, frame->data[0]);
}

static struct video_frame *isp_init_media_frame(struct isp_video_reader *isp, void *mem)
{
    struct camera_info *info = &isp->info;

    int index = (mem - isp->info.mapped_mem) / isp->info.frame_size;
    unsigned long phy_data = info->phys_mem + index * info->frame_align_size;

    int align = line_size_to_align_bytes(info->line_length, info->width);
    video_frame_init(&isp->frames[index], info->width, info->height, VIDEO_nv12, align,
                          mem, phy_data, isp, isp_video_reader_put_video);

    return &isp->frames[index];

}

static int isp_video_reader_read_video(struct video_reader *reader, struct video_frame **frame)
{
    struct isp_video_reader *isp = (void *)reader;

    void *mem = isp_get_frame(isp->fd);
    if (!mem)
        return 1;

    *frame = isp_init_media_frame(isp, mem);

    video_frame_get(*frame);

    return 0;
}

static int isp_video_reader_drop_video(struct video_reader *reader)
{
    struct isp_video_reader *isp = (void *)reader;

    return isp_drop_frames(isp->fd, 10);
}

struct video_reader_cb isp_video_reader_cb = {
    .open_reader = isp_video_reader_open,
    .close_reader = isp_video_reader_close,
    .read_video = isp_video_reader_read_video,
    .drop_video = isp_video_reader_drop_video,
};

void isp_video_reader_init_param(
    struct isp_video_reader_param *isp_param)
{
    struct video_reader_param *param = &isp_param->param;

    param->width = isp_param->isp_params.width;
    param->height = isp_param->isp_params.height;
    param->pixel_fmt = VIDEO_nv12;
    param->ignore_video_encoder_linesize = 1;
    param->rotater_param = NULL;
    param->cb = &isp_video_reader_cb;
}

void isp_video_reader_init_default_param(struct isp_video_reader_param *param, int width, int height)
{
    if (!access("/dev/mscaler1-ch0", F_OK))
        param->isp_device =  "/dev/mscaler1-ch0";
    else
        param->isp_device =  "/dev/mscaler0-ch0";

    param->isp_params.width = width;
    param->isp_params.height = height;
    param->isp_params.scaler.enable = 1;
    param->isp_params.scaler.width = width;
    param->isp_params.scaler.height = height;
    param->isp_params.crop.enable = 0;
    param->isp_params.frame_nums = 3;
    param->isp_params.pixel_format = CAMERA_PIX_FMT_NV12;

    isp_video_reader_init_param(param);
}