#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <libhardware2/camera.h>
#include <libmedia/read/vic_video_reader.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

struct frames_data {
    void *mem;
    void *vic_mem;
    unsigned long phys_mem;
    int size;
};

struct vic_video_reader {
    struct video_reader reader;

    int fd;
    struct camera_info info;

    struct video_frame *frames;
    struct frames_data *pdata;
    int is_alloc;
};

static void bayer16_to_nv12(void *_dst, void *_src, int xres, int yres, int line_length)
{
    int len = line_length * yres;
    unsigned char *src = _src + 1;
    unsigned char *dst = _dst;

    int i = len / 32;
    while (i--) {
        unsigned char s0 = src[0*2];
        unsigned char s1 = src[1*2];
        unsigned char s2 = src[2*2];
        unsigned char s3 = src[3*2];
        unsigned char s4 = src[4*2];
        unsigned char s5 = src[5*2];
        unsigned char s6 = src[6*2];
        unsigned char s7 = src[7*2];
        unsigned char s8 = src[8*2];
        unsigned char s9 = src[9*2];
        unsigned char s10 = src[10*2];
        unsigned char s11 = src[11*2];
        unsigned char s12 = src[12*2];
        unsigned char s13 = src[13*2];
        unsigned char s14 = src[14*2];
        unsigned char s15 = src[15*2];

        dst[0] = s0;
        dst[1] = s1;
        dst[2] = s2;
        dst[3] = s3;
        dst[4] = s4;
        dst[5] = s5;
        dst[6] = s6;
        dst[7] = s7;
        dst[8] = s8;
        dst[9] = s9;
        dst[10] = s10;
        dst[11] = s11;
        dst[12] = s12;
        dst[13] = s13;
        dst[14] = s14;
        dst[15] = s15;

        src += 32;
        dst += 16;
    }

    memset(dst, 0x80, len/4);
}

static void y8_to_nv12(void *_dst, void *_src, int xres, int yres)
{
    unsigned char *src = _src;
    unsigned char *dst = _dst;
    int w = xres;
    int h = yres;

    int i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            *dst++ = *src++;
        }
    }

    memset(dst, 0x80, xres * (yres / 2));
}

static void alloc_frames_data(struct vic_video_reader *vic)
{
    int i;
    int nums = vic->info.frame_nums;
    int size = vic->info.width * vic->info.height * 3 / 2;
    struct frames_data *pdata = vic->pdata;

    for (i = 0; i < nums; i++) {
        pdata[i].mem = media_alloter_alloc(NULL, &pdata[i].phys_mem, size);
        pdata[i].size = size;
    }
}

static void free_frames_data(struct vic_video_reader *vic)
{
    int i;
    int nums = vic->info.frame_nums;
    struct frames_data *pdata = vic->pdata;

    for (i = 0; i < nums; i++)
        media_alloter_free(NULL, pdata[i].mem, pdata[i].phys_mem, pdata[i].size);
}

#define FMT_C(f) \
    ((char *)f)[0],((char *)f)[1],((char *)f)[2],((char *)f)[3]

static struct video_reader *vic_video_reader_open(struct video_reader_param *param)
{
    int ret;

    struct vic_video_reader_param *p = (void *)param;
    assert(p);
    assert(p->vic_device);

    struct vic_video_reader *vic = malloc(sizeof(*vic));
    assert(vic);

    int fd = camera_open(&vic->info, p->vic_device);
    if (fd < 0) {
        fprintf(stderr, "vic_video_reader: failed to open vic device\n");
        goto free_vic;
    }

    if (!camera_fmt_is_16BIT(vic->info.data_fmt) && !camera_fmt_is_8BIT(vic->info.data_fmt)) {
        fprintf(stderr, "vic_video_reader: not support this camera fmt %c%c%c%c\n", FMT_C(vic->info.data_fmt));
        goto close_vic;
    }

    ret = camera_power_on(fd);
    if (ret) {
        fprintf(stderr, "vic_video_reader: failed to power on vic\n");
        goto close_vic;
    }

    ret = camera_stream_on(fd);
    if (ret) {
        fprintf(stderr, "vic_video_reader: failed to stream on vic\n");
        goto power_off_vic;
    }

    struct video_frame *frames = malloc(vic->info.frame_nums * sizeof(*frames));
    assert(frames);

    struct frames_data *pdata = malloc(vic->info.frame_nums * sizeof(*pdata));
    assert(pdata);

    param->width = vic->info.width;
    param->height = vic->info.height;

    vic->fd = fd;
    vic->frames = frames;
    vic->pdata = pdata;
    vic->is_alloc = 0;

    if (camera_fmt_is_8BIT(vic->info.data_fmt)) {
        alloc_frames_data(vic);
        vic->is_alloc = 1;
    }


    return &vic->reader;

power_off_vic:
    camera_power_off(fd);
close_vic:
    camera_close(fd, &vic->info);
free_vic:
    free(vic);
    return NULL;
}

static void vic_video_reader_close(struct video_reader *reader)
{
    struct vic_video_reader *vic = (void *)reader;
    int fd = vic->fd;

    if (vic->is_alloc)
        free_frames_data(vic);

    camera_stream_off(fd);
    camera_power_off(fd);
    camera_close(fd, &vic->info);

    free(vic->pdata);
    free(vic->frames);
    free(vic);
}

static void vic_video_reader_put_video(void *handle, struct video_frame *frame)
{
    struct vic_video_reader *vic = handle;

    struct frames_data *pdata = frame->pdata;
    camera_put_frame(vic->fd, pdata->vic_mem);
}


static void vic_init_media_frame(struct vic_video_reader *vic, struct video_frame *frame, struct frames_data *pdata)
{
    struct camera_info *info = &vic->info;

    int align = line_size_to_align_bytes(info->line_length, info->width);
    video_frame_init(frame, info->width, info->height, VIDEO_nv12, align,
                          pdata->mem, pdata->phys_mem, vic, vic_video_reader_put_video);

    frame->pdata = pdata;

    video_frame_get(frame);

}

static int vic_video_reader_read_video(struct video_reader *reader, struct video_frame **frame)
{
    struct vic_video_reader *vic = (void *)reader;
    struct camera_info *info = &vic->info;

    void *mem = camera_get_frame(vic->fd);
    if (!mem)
        return 1;

    int index = (mem - info->mapped_mem) / info->frame_size;

    struct video_frame *vic_frame = &vic->frames[index];
    struct frames_data *pdata = &vic->pdata[index];

    if (camera_fmt_is_8BIT(vic->info.data_fmt)) {
        y8_to_nv12(pdata->mem, mem, info->width, info->height);
        pdata->vic_mem = mem;
    } else {
        bayer16_to_nv12(mem, mem, vic->info.width, vic->info.height, vic->info.line_length);
        pdata->mem = mem;
        pdata->phys_mem = info->phys_mem + index * info->frame_align_size;
        pdata->vic_mem = mem;
    }

    vic_init_media_frame(vic, vic_frame, pdata);

    *frame = vic_frame;

    return 0;
}

static int vic_video_reader_drop_video(struct video_reader *reader)
{
    struct vic_video_reader *vic = (void *)reader;

    return camera_drop_frames(vic->fd, 10);
}

struct video_reader_cb vic_video_reader_cb = {
    .open_reader = vic_video_reader_open,
    .close_reader = vic_video_reader_close,
    .read_video = vic_video_reader_read_video,
    .drop_video = vic_video_reader_drop_video,
};

void vic_video_reader_init_param(
    struct vic_video_reader_param *vic_param)
{
    struct video_reader_param *param = &vic_param->param;

    param->pixel_fmt = VIDEO_nv12;
    param->ignore_video_encoder_linesize = 1;
    param->cb = &vic_video_reader_cb;
}

void vic_video_reader_init_default_param(struct vic_video_reader_param *param)
{
    memset(param, 0, sizeof(*param));

    if (!access("/dev/vic1", F_OK))
        param->vic_device =  "/dev/vic1";
    else
        param->vic_device =  "/dev/vic0";

    vic_video_reader_init_param(param);
}