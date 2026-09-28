#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>

#include <libhardware2/fb_layer_mixer.h>

#define CMD_SET_INPUT_LAYER           _IOWR('M', 82, struct fb_layer_mixer_layer *)
#define CMD_SET_OUTPUT_FRAME          _IOWR('M', 83, struct fb_layer_mixer_output_cfg *)
#define CMD_WORK_OUT                  _IO('M', 84)
#define CMD_DELETE_MIXER              _IO('M', 85)

struct fb_layer_mixer_layer {
    struct lcdc_layer cfg;
    int layer_id;
};

static inline void fb_layer_mixer_err(const char *err_msg)
{
    fprintf(stderr, "fb_layer_mixer: failed to %s, %s\n", err_msg, strerror(errno));
}


int fb_layer_mixer_create(struct fb_layer_mixer_output_cfg *cfg)
{
    int ret;
    int fd = open("/dev/fb_layer_mixer", O_RDWR);
    if (fd < 0) {
        fb_layer_mixer_err("open device");
        return fd;
    }

    ret = ioctl(fd, CMD_SET_OUTPUT_FRAME, cfg);
    if(ret < 0) {
        fb_layer_mixer_err("create mixer");
        close(fd);
        return ret;
    }

    return fd;
}

int fb_layer_mixer_set_output_cfg(int fd, struct fb_layer_mixer_output_cfg *cfg)
{
    int ret;
    ret = ioctl(fd, CMD_SET_OUTPUT_FRAME, cfg);
    if (ret < 0) {
        fb_layer_mixer_err("set output cfg");
        return ret;
    }

    return ret;
}

void fb_layer_mixer_delete(int fd)
{
    close(fd);
}

int fb_layer_mixer_set_input_layer(int fd, int layer_id, struct lcdc_layer *layer)
{
    int ret;
    struct fb_layer_mixer_layer layer_data = {
        .cfg = *layer,
        .layer_id = layer_id,
    };

    ret = ioctl(fd, CMD_SET_INPUT_LAYER, &layer_data);
    if (ret < 0) {
        fb_layer_mixer_err("config layer");
        return ret;
    }

    return 0;
}


int fb_layer_mixer_work_out_one_frame(int fd)
{
    int ret;

    ret = ioctl(fd, CMD_WORK_OUT);
    if (ret < 0) {
        fb_layer_mixer_err("work out");
        return ret;
    }

    return 0;
}