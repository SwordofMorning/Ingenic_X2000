#ifndef __HW_JPEGD_DECODER_H__
#define __HW_JPEGD_DECODER_H__

#include <libmedia/video_decoder.h>
#include <libhardware2/jpegd_decode.h>

struct hw_jpegd_decoder_param {
    struct video_decoder_param param;

    struct jpegd_decoder_config config;
};

void hw_jpegd_decoder_init_param(struct hw_jpegd_decoder_param *decoder_param);

void hw_jpegd_decoder_init_default_param(
    struct hw_jpegd_decoder_param *decoder_param, int width, int height);

#endif /*__HW_JPEGD_DECODER_H__*/