#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpu/tcsm_section.h>
#include <soc/base.h>
#include <soc/gpio.h>
#include <driver/clk.h>
#include <driver/irq.h>
#include <delay.h>
#include <driver/systick.h>
#include <assert.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <errno.h>
#include <cpu/uncache_mem.h>

#include "fb_layer_mixer.h"
#include "dpu_hal.h"

#define State_writeback_start 0
#define State_writeback_end   1

struct lcdc_frame {
    struct framedesc *framedesc;
    struct layerdesc *layers[4];
};

struct fb_layer_mixer_layer {
    struct lcdc_layer cfg;
    int layer_id;
};

struct fb_layer_mixer_info {
    int xres;
    int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    enum fb_fmt format;
};

struct fb_layer_mixer_dev {
    struct lcdc_frame frame;
    struct lcdc_layer layer_cfg[4];
    struct fb_layer_mixer_output_cfg mixer_cfg;
    struct fb_layer_mixer_info info;
    int set_input_count;
};

struct {
    int frame_state;
    int is_enable;
    int fb_is_srdma;
    int fb_is_rot;
} layer_mixer;

static inline int bytes_per_line(int xres, enum fb_fmt fmt)
{
    int len;
    int bytes;

    if(fmt == fb_fmt_ARGB8888 || fmt == fb_fmt_RGB888)
        bytes = 4;
    else
        bytes = 2;

    len = xres * bytes;
    return ALIGN(len, 8);
}

static inline int bytes_per_frame(int line_len, int yres)
{
    unsigned int frame_size = line_len * yres;

    return frame_size;
}

static inline void *m_dma_alloc_coherent(int size)
{
    void *mem = malloc(size);
    assert(mem);

    return mem;
}

static inline void m_dma_free_coherent(void *mem)
{
    free(mem);
}

static void init_lcdc(void)
{
    unsigned long intc = dpu_read(INTC);
    set_bit_field_v(&intc, EOW_MSK, 1);
    dpu_write(INTC, intc);

    dpu_write(CLR_ST, dpu_read(INT_FLAG));

    unsigned long com_cfg = dpu_read(COM_CFG);
    set_bit_field_v(&com_cfg, BURST_LEN_BDMA, 3);
    set_bit_field_v(&com_cfg, BURST_LEN_RDMA, 3);
    set_bit_field_v(&com_cfg, CH_SEL, 0);
    set_bit_field_v(&com_cfg, ROT_SEL, 0);
    dpu_write(COM_CFG, com_cfg);
}

static int wb_format(enum fb_fmt format)
{
    switch (format)
    {
        case fb_fmt_RGB565: return 1;
        case fb_fmt_RGB555: return 2;
        case fb_fmt_RGB888: return 6;
        case fb_fmt_ARGB8888: return 0;
        default:
            panic("writeback not support this format:%d\n", format);
    }
}

static void init_writeback_desc(struct fb_layer_mixer_dev *dev)
{
    struct lcdc_frame *frame = &dev->frame;
    struct framedesc *desc = frame->framedesc;

    desc->FrameCfgAddr = (unsigned long)(desc);

    desc->FrameCtrl = 0;
    set_bit_field_v(&desc->FrameCtrl, f_DirectEn, 0);
    set_bit_field_v(&desc->FrameCtrl, f_WriteBack, 1);
    set_bit_field_v(&desc->FrameCtrl, f_Change2RDMA, 0);
    set_bit_field_v(&desc->FrameCtrl, f_stop, 1);
    set_bit_field_v(&desc->FrameCtrl, f_WB_DitherAuto, 1);
    set_bit_field_v(&desc->FrameCtrl, f_WB_DitherEn, 0);

    desc->Layer0CfgAddr = (unsigned long)(frame->layers[0]);
    desc->Layer1CfgAddr = (unsigned long)(frame->layers[1]);
    desc->Layer2CfgAddr = (unsigned long)(frame->layers[2]);
    desc->Layer3CfgAddr = (unsigned long)(frame->layers[3]);

    set_bit_field_v(&desc->LayerCfgScaleEn, f_layer0order, 0);
    set_bit_field_v(&desc->LayerCfgScaleEn, f_layer1order, 1);
    set_bit_field_v(&desc->LayerCfgScaleEn, f_layer2order, 2);

    set_bit_field_v(&desc->LayerCfgScaleEn, f_layer3order, 3);

    desc->InterruptControl = 0;
    set_bit_field_v(&desc->InterruptControl, f_EOW_MSK, 1);
}

static void config_writeback_desc(struct fb_layer_mixer_dev *dev)
{
    struct lcdc_frame *frame = &dev->frame;

    struct framedesc *desc = frame->framedesc;

    set_bit_field_v(&desc->FrameSize, f_Width, dev->mixer_cfg.xres);
    set_bit_field_v(&desc->FrameSize, f_Height, dev->mixer_cfg.yres);

    set_bit_field_v(&desc->FrameCtrl, f_WB_Format, dev->mixer_cfg.use_rot ? 7 : wb_format(dev->mixer_cfg.format));

    desc->WritebackBufferAddr = (unsigned long)(dev->mixer_cfg.dst_mem);
    desc->WritebackStride = dev->mixer_cfg.xres;
}

static void fb_layer_mixer_enable(void)
{
    if (layer_mixer.is_enable++ != 0)
        return;

    clk_gate_enable(CLK_GATE_LCD);

    init_lcdc();
}

static void fb_layer_mixer_disable(void)
{
    if (--layer_mixer.is_enable != 0)
        return;

    layer_mixer.is_enable = 0;

    clk_gate_disable(CLK_GATE_LCD);
}

int fb_layer_mixer_irq_handler(int irq, void *data)
{
    unsigned long flags = dpu_read(INT_FLAG);

    if (get_bit_field(flags, WDMA_END)) {
        layer_mixer.frame_state = State_writeback_end;
        dpu_write(CLR_ST, bit_field_val(CLR_WDMA_END, 1));
        return 0;
    }

    return -EINVAL;
}

void fb_layer_mixer_set_output_frame(struct fb_layer_mixer_dev *mixer, struct fb_layer_mixer_output_cfg *mixer_cfg)
{
    assert(mixer);
    assert(mixer_cfg);

    mixer->mixer_cfg = *mixer_cfg;

    mixer->info.xres = mixer->mixer_cfg.xres;
    mixer->info.yres = mixer->mixer_cfg.yres;
    mixer->info.format = mixer->mixer_cfg.format;

    mixer->info.bytes_per_line = bytes_per_line(mixer->mixer_cfg.xres, mixer->mixer_cfg.format);
    mixer->info.bytes_per_frame = ALIGN(bytes_per_frame(mixer->info.bytes_per_line, mixer->info.yres), PAGE_SIZE);

    config_writeback_desc(mixer);
}

struct fb_layer_mixer_dev *fb_layer_mixer_create(void)
{
    if (!layer_mixer.fb_is_srdma && !layer_mixer.fb_is_rot) {
        printf("fb: composer mode, can not use fb_layer_mixer\n");
        return NULL;
    }

    int i = 0;

    struct fb_layer_mixer_dev *mixer = m_dma_alloc_coherent(sizeof(struct fb_layer_mixer_dev));
    mixer->frame.framedesc = uncache_mem_alloc(sizeof(struct framedesc), 64);
    memset(mixer->frame.framedesc, 0, sizeof(*mixer->frame.framedesc));

    for(i = 0; i < 4; i++) {
        mixer->frame.layers[i] = uncache_mem_alloc(sizeof(struct layerdesc), 64);
        memset(mixer->frame.layers[i], 0, sizeof(struct layerdesc));
    }

    init_writeback_desc(mixer);

    fb_layer_mixer_enable();

    return mixer;
}

void fb_layer_mixer_set_input_layer(struct fb_layer_mixer_dev *mixer, int layer_id, struct lcdc_layer *cfg)
{
    assert(layer_id < 4 && layer_id >= 0);
    assert(mixer);

    mixer->set_input_count++;

    struct lcdc_frame *frame = &mixer->frame;
    struct layerdesc *layer = frame->layers[layer_id];
    struct lcdc_layer *layer_cfg = &mixer->layer_cfg[layer_id];

    *layer_cfg = *cfg;

    dpu_init_layer_desc(layer, layer_cfg);

    dpu_enable_layer(frame->framedesc, layer_id, !!layer_cfg->layer_enable);
    dpu_enable_layer_scaling(frame->framedesc, layer_id, !!layer_cfg->scaling.enable);
    dpu_set_layer_order(frame->framedesc, layer_id, layer_cfg->layer_order);
}

void fb_layer_mixer_enable_layer(struct fb_layer_mixer_dev *mixer, unsigned int layer_id, int enable)
{
    assert(layer_id < 4 && layer_id >= 0);
    assert(mixer);

    struct lcdc_frame *frame = &mixer->frame;

    dpu_enable_layer(frame->framedesc, layer_id, !!enable);
}

void fb_layer_mixer_work_out_one_frame(struct fb_layer_mixer_dev *mixer)
{
    assert(mixer);

    if (!mixer->set_input_count) {
        printf("fb: must set fb layer mixer input layer\n");
        return;
    }

    struct lcdc_frame *frame = &mixer->frame;

    dpu_write(FRM_CFG_ADDR, (unsigned long)(frame->framedesc));

    layer_mixer.frame_state = State_writeback_start;
    dpu_start_composer();

    uint64_t usec = systick_get_time_usec();
    while (layer_mixer.frame_state != State_writeback_end) {
        if (systick_get_time_usec() - usec > 300 * 1000)
            break;
    }

    if (layer_mixer.frame_state != State_writeback_end)
        printf("fb: frame write back timeout %d\n", layer_mixer.frame_state);
}

void fb_layer_mixer_delete(struct fb_layer_mixer_dev *mixer)
{
    assert(mixer);

    fb_layer_mixer_disable();

    m_dma_free_coherent(mixer);
}

void fb_layer_mixer_init(void)
{
#ifdef APP_libmcu_x2600_fb_mixer_enable
    layer_mixer.fb_is_srdma = 1;
#endif
#ifdef APP_libmcu_x2600_fb_rotator_enable
    layer_mixer.fb_is_rot = 1;
#endif
}
