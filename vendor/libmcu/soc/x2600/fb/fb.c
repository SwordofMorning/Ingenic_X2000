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

#include "mipi_dsi.c"
#include "lcdc.c"
#include "fb_layer_mixer.h"
#include "rot.h"
#include "driver/fb.h"

enum display_mode {
    COMPOSER_DISPLAY,
    SRDMA_DISPLAY,
    MIXER_DISPLAY,
    ROT_DISPLAY,
};

struct lcdc_frame {
    struct framedesc *framedesc;
    struct layerdesc *layers[4];
    struct srdmadesc *srdmadesc;
};

#define State_clear 0
#define State_display_start   1
#define State_display_end     2

static struct {
    int underrun_count;

    struct lcdc_frame frames[3];
    struct lcdc_data *pdata;
    enum display_mode display_mode;
    int is_enabled;
    int display_state;
    int pan_display_sync;
    int frame_index;
    int wb_index;
    int wb_frame_count;

    struct fbdev_data fbdev[5];

    int layer_xres;
    int layer_yres;
    struct rot_cfg rot;

    int mixer_enable;
    struct fb_layer_mixer_dev *mixer;
    struct fb_layer_mixer_output_cfg mixer_cfg;

    int use_default_order;
    int enabled_num;
} fb;

static int lcd_is_inited = 0;
static int rot_angle = 0;

static void fb_get_config(void)
{
#ifdef APP_libmcu_x2600_fb_boot_logo_for_kernel
    lcd_is_inited = 1;
#endif

#ifdef APP_libmcu_x2600_fb_pan_display_sync
    fb.pan_display_sync = 1;
#endif

#ifdef APP_libmcu_x2600_fb_cfg_rotator
    fb.rot.is_enable = 1;
    rot_angle = APP_libmcu_x2600_fb_rotator_angle;
    fb.rot.frame_count = APP_libmcu_x2600_fb_rotator_frames;
#endif

#ifdef APP_libmcu_x2600_fb_use_default_order
    fb.use_default_order = 1;
#endif

#ifdef APP_libmcu_x2600_fb_layer0_enable
    fb.fbdev[0].is_enable = 1;
    fb.fbdev[0].frame_count = APP_libmcu_x2600_fb_layer0_frames;
    fb.fbdev[0].alpha = APP_libmcu_x2600_fb_layer0_alpha;
#endif

#ifdef APP_libmcu_x2600_fb_layer1_enable
    fb.fbdev[1].is_enable = 1;
    fb.fbdev[1].frame_count = APP_libmcu_x2600_fb_layer1_frames;
    fb.fbdev[1].alpha = APP_libmcu_x2600_fb_layer1_alpha;
#endif

#ifdef APP_libmcu_x2600_fb_layer2_enable
    fb.fbdev[2].is_enable = 1;
    fb.fbdev[2].frame_count = APP_libmcu_x2600_fb_layer2_frames;
    fb.fbdev[2].alpha = APP_libmcu_x2600_fb_layer2_alpha;
#endif

#ifdef APP_libmcu_x2600_fb_layer3_enable
    fb.fbdev[3].is_enable = 1;
    fb.fbdev[3].frame_count = APP_libmcu_x2600_fb_layer3_frames;
    fb.fbdev[3].alpha = APP_libmcu_x2600_fb_layer3_alpha;
#endif

#ifdef APP_libmcu_x2600_fb_srdma_config
    fb.fbdev[4].is_enable = 1;
    fb.fbdev[4].frame_count = APP_libmcu_x2600_fb_srdma_frames;
#endif

#ifdef APP_libmcu_x2600_fb_mixer_enable
    fb.mixer_enable = 1;
#endif

    fb.underrun_count = 0;

#ifdef APP_libmcu_x2600_fb_layer0_user_enable
    fb.fbdev[0].enable_fbdev_user_cfg = 1;
    fb.fbdev[0].fbdev_user_cfg.width = APP_libmcu_x2600_fb_layer0_user_width;
    fb.fbdev[0].fbdev_user_cfg.height = APP_libmcu_x2600_fb_layer0_user_height;
    fb.fbdev[0].fbdev_user_cfg.xpos = APP_libmcu_x2600_fb_layer0_user_xpos;
    fb.fbdev[0].fbdev_user_cfg.ypos = APP_libmcu_x2600_fb_layer0_user_ypos;
#ifdef APP_libmcu_x2600_fb_layer0_user_scaling_enable
    fb.fbdev[0].fbdev_user_cfg.scaling_enable = 1;
    fb.fbdev[0].fbdev_user_cfg.scaling_width = APP_libmcu_x2600_fb_layer0_user_scaling_width;
    fb.fbdev[0].fbdev_user_cfg.scaling_height = APP_libmcu_x2600_fb_layer0_user_scaling_height;
#endif
#endif

#ifdef APP_libmcu_x2600_fb_layer1_user_enable
    fb.fbdev[1].enable_fbdev_user_cfg = 1;
    fb.fbdev[1].fbdev_user_cfg.width = APP_libmcu_x2600_fb_layer1_user_width;
    fb.fbdev[1].fbdev_user_cfg.height = APP_libmcu_x2600_fb_layer1_user_height;
    fb.fbdev[1].fbdev_user_cfg.xpos = APP_libmcu_x2600_fb_layer1_user_xpos;
    fb.fbdev[1].fbdev_user_cfg.ypos = APP_libmcu_x2600_fb_layer1_user_ypos;
#ifdef APP_libmcu_x2600_fb_layer1_user_scaling_enable
    fb.fbdev[1].fbdev_user_cfg.scaling_enable = 1;
    fb.fbdev[1].fbdev_user_cfg.scaling_width = APP_libmcu_x2600_fb_layer1_user_scaling_width;
    fb.fbdev[1].fbdev_user_cfg.scaling_height = APP_libmcu_x2600_fb_layer1_user_scaling_height;
#endif
#endif

#ifdef APP_libmcu_x2600_fb_layer2_user_enable
    fb.fbdev[2].enable_fbdev_user_cfg = 1;
    fb.fbdev[2].fbdev_user_cfg.width = APP_libmcu_x2600_fb_layer2_user_width;
    fb.fbdev[2].fbdev_user_cfg.height = APP_libmcu_x2600_fb_layer2_user_height;
    fb.fbdev[2].fbdev_user_cfg.xpos = APP_libmcu_x2600_fb_layer2_user_xpos;
    fb.fbdev[2].fbdev_user_cfg.ypos = APP_libmcu_x2600_fb_layer2_user_ypos;
#ifdef APP_libmcu_x2600_fb_layer2_user_scaling_enable
    fb.fbdev[2].fbdev_user_cfg.scaling_enable = 1;
    fb.fbdev[2].fbdev_user_cfg.scaling_width = APP_libmcu_x2600_fb_layer2_user_scaling_width;
    fb.fbdev[2].fbdev_user_cfg.scaling_height = APP_libmcu_x2600_fb_layer2_user_scaling_height;
#endif
#endif

#ifdef APP_libmcu_x2600_fb_layer3_user_enable
    fb.fbdev[3].enable_fbdev_user_cfg = 1;
    fb.fbdev[3].fbdev_user_cfg.width = APP_libmcu_x2600_fb_layer3_user_width;
    fb.fbdev[3].fbdev_user_cfg.height = APP_libmcu_x2600_fb_layer3_user_height;
    fb.fbdev[3].fbdev_user_cfg.xpos = APP_libmcu_x2600_fb_layer3_user_xpos;
    fb.fbdev[3].fbdev_user_cfg.ypos = APP_libmcu_x2600_fb_layer3_user_ypos;
#ifdef APP_libmcu_x2600_fb_layer3_user_scaling_enable
    fb.fbdev[3].fbdev_user_cfg.scaling_enable = 1;
    fb.fbdev[3].fbdev_user_cfg.scaling_width = APP_libmcu_x2600_fb_layer3_user_scaling_width;
    fb.fbdev[3].fbdev_user_cfg.scaling_height = APP_libmcu_x2600_fb_layer3_user_scaling_height;
#endif
#endif
}

static inline void dump_frame(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        struct lcdc_frame *frame = &fb.frames[i];
        printf("frame%d\n", i);
        printf("FrameCfgAddr %08lx\n", frame->framedesc->FrameCfgAddr);
        printf("FrameSize %08lx\n", frame->framedesc->FrameSize);
        printf("FrameCtrl %08lx\n", frame->framedesc->FrameCtrl);
        printf("WritebackBufferAddr %08lx\n", frame->framedesc->WritebackBufferAddr);
        printf("WritebackStride %08lx\n", frame->framedesc->WritebackStride);
        printf("Layer0CfgAddr %08lx\n", frame->framedesc->Layer0CfgAddr);
        printf("Layer1CfgAddr %08lx\n", frame->framedesc->Layer1CfgAddr);
        printf("Layer2CfgAddr %08lx\n", frame->framedesc->Layer2CfgAddr);
        printf("Layer3CfgAddr %08lx\n", frame->framedesc->Layer3CfgAddr);
        printf("LayerCfgScaleEn %08lx\n", frame->framedesc->LayerCfgScaleEn);
        printf("InterruptControl %08lx\n", frame->framedesc->InterruptControl);

        int j;
        for (j = 0; j < 4; j++) {
            printf("LayerSize %08lx\n", frame->layers[j]->LayerSize);
            printf("LayerCfg %08lx\n", frame->layers[j]->LayerCfg);
            printf("LayerBufferAddr %08lx\n", frame->layers[j]->LayerBufferAddr);
            printf("LayerTargetSize %08lx\n", frame->layers[j]->LayerTargetSize);
            printf("Reserved1 %08lx\n", frame->layers[j]->Reserved1);
            printf("Reserved2 %08lx\n", frame->layers[j]->Reserved2);
            printf("LayerPos %08lx\n", frame->layers[j]->LayerPos);
            printf("Layer_Resize_Coef_X %08lx\n", frame->layers[j]->Layer_Resize_Coef_X);
            printf("Layer_Resize_Coef_Y %08lx\n", frame->layers[j]->Layer_Resize_Coef_Y);
            printf("LayerStride %08lx\n", frame->layers[j]->LayerStride);
            printf("BufferAddr_UV %08lx\n", frame->layers[j]->BufferAddr_UV);
            printf("stride_UV %08lx\n", frame->layers[j]->stride_UV);
            printf("\n");
        }
    }
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

static void init_fbdev_frame_layer(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        init_frame_desc(fb.pdata, fb.frames[i].framedesc, fb.frames[i].layers[0],\
                                                 fb.frames[i].layers[1],\
                                                 fb.frames[i].layers[2],\
                                                 fb.frames[i].layers[3]);
    }
}

static void init_fbdev_data(struct fbdev_data *fbdev, int frame_count)
{
    int fb_mem_size = 0;
    struct fb_mem_info *info = &fbdev->user;
    struct lcdc_data *pdata = fb.pdata;
    int xpos = 0;
    int ypos = 0;
    int scaling_enable = 0;
    int scaling_width = 0;
    int scaling_height = 0;

    info->fb_fmt = pdata->fb_fmt;
    if (fbdev->enable_fbdev_user_cfg) {
        info->xres = fbdev->fbdev_user_cfg.width;
        info->yres = fbdev->fbdev_user_cfg.height;
        xpos = fbdev->fbdev_user_cfg.xpos;
        ypos = fbdev->fbdev_user_cfg.ypos;
        scaling_enable = fbdev->fbdev_user_cfg.scaling_enable;
        scaling_width = fbdev->fbdev_user_cfg.scaling_width;
        scaling_height = fbdev->fbdev_user_cfg.scaling_height;
    } else {
        info->xres = fb.layer_xres;
        info->yres = fb.layer_yres;
    }

    info->bytes_per_line = bytes_per_line(info->xres, info->fb_fmt);

    info->bytes_per_frame = bytes_per_frame(info->bytes_per_line, info->yres);
    fb_mem_size = info->bytes_per_frame * frame_count;
    fb_mem_size = ALIGN(fb_mem_size, PAGE_SIZE);
    info->frame_count = frame_count;

    if (frame_count) {
        fbdev->fb_mem = m_dma_alloc_coherent(fb_mem_size);
        memset(fbdev->fb_mem, 0, fb_mem_size);
    } else {
        fbdev->fb_mem = NULL;
    }

    fbdev->fb_mem_size = fb_mem_size;
    info->fb_mem = fbdev->fb_mem;

    if (fbdev - fb.fbdev < 4) {
        struct lcdc_layer *cfg = &fbdev->layer_cfg;
        cfg->xpos = xpos;
        cfg->ypos = ypos;
        cfg->scaling.enable = scaling_enable;
        cfg->scaling.xres = scaling_width;
        cfg->scaling.yres = scaling_height;
        cfg->xres = info->xres;
        cfg->yres = info->yres;
        cfg->fb_fmt = info->fb_fmt;
        cfg->rgb.mem = info->fb_mem;
        cfg->rgb.stride = info->bytes_per_line;
        cfg->alpha.enable = 1;
        cfg->alpha.value = fbdev->alpha;
    } else {
        struct srdma_cfg *srdma_cfg = &fbdev->srdma_cfg;
        srdma_cfg->is_video = is_video_mode(fb.pdata);
        srdma_cfg->fb_fmt = info->fb_fmt;
        srdma_cfg->fb_mem = info->fb_mem;
        srdma_cfg->stride = bytes_per_line(pdata->xres, info->fb_fmt) / bytes_per_pixel(info->fb_fmt);
    }
    fb.enabled_num++;
}

static void lcdc_init_fbdev(void)
{
    int i;
    for(i = 0; i < 5; i++) {
        if(fb.fbdev[i].is_enable)
            init_fbdev_data(&fb.fbdev[i], fb.fbdev[i].frame_count);
    }

    fb.fbdev[0].layer_cfg.layer_order = lcdc_layer_0;
    fb.fbdev[1].layer_cfg.layer_order = lcdc_layer_1;
    fb.fbdev[2].layer_cfg.layer_order = lcdc_layer_2;
    fb.fbdev[3].layer_cfg.layer_order = lcdc_layer_3;
}

static void lcdc_alloc_desc(void)
{
    int i;
    for(i = 0; i < 3; i++) {
        fb.frames[i].framedesc = uncache_mem_alloc(sizeof(struct framedesc), 64);
        fb.frames[i].layers[0] = uncache_mem_alloc(sizeof(struct layerdesc), 64);
        fb.frames[i].layers[1] = uncache_mem_alloc(sizeof(struct layerdesc), 64);
        fb.frames[i].layers[2] = uncache_mem_alloc(sizeof(struct layerdesc), 64);
        fb.frames[i].layers[3] = uncache_mem_alloc(sizeof(struct layerdesc), 64);
        fb.frames[i].srdmadesc = uncache_mem_alloc(sizeof(struct srdmadesc), 64);
    }

    fb.frame_index = 0;
}

static int free_mem(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        struct fbdev_data *fbdev = &fb.fbdev[i];
        if (fbdev->frame_count)
            m_dma_free_coherent(fbdev->fb_mem);
    }

    return 0;
}

static void rot_irq_handler(int irq, void *data)
{
    unsigned long status = rot_read_reg(STATUS);

    if (get_bit_field(status, ST_FA))
        rot_set_bits(CLR_STATUS, CLR_FA, 1);

    if (get_bit_field(status, ST_SOF))
        rot_set_bits(CLR_STATUS, CLR_SOF, 1);

    if (get_bit_field(status, ST_EOF)) {
        rot_set_bits(CLR_STATUS, CLR_EOF, 1);
        fb.display_state = State_display_end;
    }

    if (get_bit_field(status, ST_GEN_STOP_ACK))
        rot_set_bits(CLR_STATUS, CLR_GEN_STP_ACK, 1);

    return;
}

static void fb_quick_stop_rot(void)
{
    rot_set_bits(ROT_CTRL, QUICK_STOP, 1);
    rot_set_bits(GLB_CFG, FIFO_GATE, 1);
    rot_set_bits(ROT_CTRL, FLUSH_FIFO, 1);
}

static int rot_check_params(void)
{
    if (!fb.rot.is_enable)
        return 0;

    if (fb.pdata->fb_fmt != fb_fmt_RGB888 && fb.pdata->fb_fmt != fb_fmt_ARGB8888) {
        printf("fb: rot dst_fmt is not fb_fmt_RGB888 or fb_fmt_ARGB8888\n");
        return -EINVAL;
    }

    if (rot_angle == 90)
        fb.rot.angle = 1;
    else if (rot_angle == 180)
        fb.rot.angle = 2;
    else if (rot_angle == 270)
        fb.rot.angle = 3;
    else {
        printf("fb: rot angle can't support this angle: %d°\n", rot_angle);
        return -EINVAL;
    }

    return 0;
}

static void init_rot_cfg(void)
{
    if (fb.rot.is_enable == 0)
        return;

    const int BPP_Custome_format = 4;
    const int Block_Size = 512;

    int byte_per_line = fb.layer_xres * BPP_Custome_format;
    fb.rot.bytes_per_frame = ALIGN(byte_per_line, Block_Size) * fb.layer_yres;
    fb.rot.mem_size = fb.rot.bytes_per_frame * fb.rot.frame_count;

    fb.rot.frame_mem = m_dma_alloc_coherent(fb.rot.mem_size);
    if (!fb.rot.frame_mem)
        goto err;

    memset(fb.rot.frame_mem, 0, fb.rot.mem_size);

    clk_gate_enable(CLK_GATE_ROTATE);

    rot_set_bits(INT_MASK, EOF_MASK, 1);
    rot_set_bits(INT_MASK, GSA_MASK, 1);
    // 设置固定最高优先级
    rot_write_reg(0x30, 0x7); /* 参考内核驱动, 手册上无相关描述 */

    rot_set_bits(FRM_SIZE, FRAM_HEIGHT, fb.layer_yres);
    rot_set_bits(FRM_SIZE, FRAM_WIDTH, fb.layer_xres);

    unsigned long glb_cfg = rot_read_reg(GLB_CFG);
    set_bit_field_v(&glb_cfg, RDMA_FMT, 3);
    set_bit_field_v(&glb_cfg, FIFO_GATE, 0);
    set_bit_field_v(&glb_cfg, MANUAL_AUTO, 0);//Write BUFF_register by software
    set_bit_field_v(&glb_cfg, ROT_ANGLE, fb.rot.angle);
    set_bit_field_v(&glb_cfg, RDMA_BURST_LEN, 0);
    rot_write_reg(GLB_CFG, glb_cfg);

    return;
err:
    clk_gate_disable(CLK_GATE_ROTATE);
    fb.rot.is_enable = 0;
    return;
}

static void free_rot(void)
{
    if (fb.rot.is_enable == 0)
        return;

    m_dma_free_coherent(fb.rot.frame_mem);
    clk_gate_disable(CLK_GATE_ROTATE);
}

static void process_slcd_data_table(struct smart_lcd_data_table *table, unsigned int length)
{
    int i = 0;

    for (; i < length; i++) {
        switch (table[i].type) {
        case SMART_CONFIG_CMD:
            slcd_send_cmd(table[i].value);
            break;
        case SMART_CONFIG_DATA:
            slcd_send_data(table[i].value);
            break;
        case SMART_CONFIG_UDELAY:
            udelay(table[i].value);
            break;
        default:
            panic("why this type: %d\n", table[i].type);
            break;
        }
    }

    if (slcd_wait_busy(10 * 1000))
        panic("lcdc busy\n");
}

static void calculate_spi_mode_delay_time(void)
{
    if (fb.pdata->lcd_mode < SLCD_SPI_3LINE) {
        spi_send_delay = 0;
        return;
    }

    int cycle = 0;
    int width = fb.pdata->slcd.mcu_data_width;
    int pixclock = fb.pdata->pixclock;

    if (width == MCU_WIDTH_8BITS)
        cycle = 8;
    if (width == MCU_WIDTH_9BITS)
        cycle = 9;
    if (width == MCU_WIDTH_16BITS)
        cycle = 16;

    assert(cycle);

    spi_send_delay = cycle * 1000000 / pixclock + ((cycle * 1000000 % pixclock) != 0);
}

static void enable_fb(void)
{
    int i;
    if (fb.is_enabled++ != 0)
        return;

    if (!lcd_is_inited) {
        fb_quick_stop_rot();
        udelay(200);
        dpu_write(CTRL, bit_field_val(QCK_STP_SRD, 1));
        udelay(200);
        dpu_write(CTRL, bit_field_val(QCK_STP_CMP, 1));
    }

    init_fbdev_frame_layer();

    unsigned int rate = fb.pdata->pixclock;
    if (is_slcd(fb.pdata) && !lcd_is_inited) {
        if (fb.pdata->slcd.pixclock_when_init)
            rate = fb.pdata->slcd.pixclock_when_init;
    }

    if (is_lvds(fb.pdata))
        jz_dsi_lvds_set_clk();
    else
        clk_div_set_rate(CLK_DIV_LPC, rate);

    clk_gate_enable(CLK_GATE_LCD);
    clk_div_enable(CLK_DIV_LPC);

    calculate_spi_mode_delay_time();

    init_lcdc(fb.pdata);

    if (is_lvds(fb.pdata))
        jz_dsi_lvds_cfg();

    fb.display_state = State_clear;
    fb.display_mode = SRDMA_DISPLAY;

    if (fb.mixer_enable || fb.rot.is_enable) {
        fb.mixer_cfg.xres = fb.layer_xres;
        fb.mixer_cfg.yres = fb.layer_yres;
        fb.mixer_cfg.format = fb.pdata->fb_fmt;
        fb.mixer_cfg.use_rot = fb.rot.is_enable;

        if (fb.mixer_enable)
            fb.mixer_cfg.dst_mem = fb.fbdev[4].fb_mem;
        else if (fb.rot.is_enable)
            fb.mixer_cfg.dst_mem = fb.rot.frame_mem;

        fb.mixer = fb_layer_mixer_create();
        fb_layer_mixer_set_output_frame(fb.mixer, &fb.mixer_cfg);

        for(i = 0; i < 4; i++) {
            if (fb.fbdev[i].is_enable)
                fb_layer_mixer_set_input_layer(fb.mixer, i, &fb.fbdev[i].layer_cfg);
        }

        if (fb.mixer_enable)
            fb.display_mode = MIXER_DISPLAY;
        else if (fb.rot.is_enable)
            fb.display_mode = ROT_DISPLAY;
    }

    if (fb.display_mode == ROT_DISPLAY) {
        dpu_config_rot_ch();
    } else if (fb.fbdev[4].is_enable) {
        dpu_config_srdma_ch();
    } else {
        dpu_config_composer_ch();
        fb.display_mode = COMPOSER_DISPLAY;
    }

    if (lcd_is_inited) {
        lcd_is_inited = 0;
        return;
    }

    fb.pdata->power_on(NULL);

    if (is_tft_mipi(fb.pdata) || is_slcd_mipi(fb.pdata))
        jz_enable_mipi_dsi();

    if (fb.pdata->lcd_init)
        fb.pdata->lcd_init();

    process_slcd_data_table(
        fb.pdata->slcd_data_table, fb.pdata->slcd_data_table_length);

    if (rate != fb.pdata->pixclock)
        clk_div_set_rate(CLK_DIV_LPC, fb.pdata->pixclock);

    if (is_slcd(fb.pdata)) {
        slcd_send_cmd(fb.pdata->slcd.cmd_of_start_frame);
        dpu_set_bit(SLCD_CFG, FMT_EN, 1);
    }

    if (is_tft_mipi(fb.pdata))
        jz_dsi_video_cfg();

    if (is_slcd_mipi(fb.pdata))
        jz_dsi_command_cfg();

    if (is_lvds(fb.pdata))
        lvds_rx_enable();
}

static void disable_fb(void)
{
    if (--fb.is_enabled != 0)
        return;

    if (fb.display_state == State_display_start)
        while (fb.display_state != State_display_end);

    if (fb.display_mode == ROT_DISPLAY)
        fb_quick_stop_rot();
    else if (fb.display_mode == COMPOSER_DISPLAY)
        dpu_quick_stop_display();
    else
        dpu_quick_stop_srdma();

    udelay(1000);

    if (is_tft_mipi(fb.pdata) || is_slcd_mipi(fb.pdata))
        jz_disable_mipi_dsi();

    fb.pdata->power_off(NULL);

    clk_gate_disable(CLK_GATE_LCD);
    clk_div_disable(CLK_DIV_LPC);

    if (fb.display_mode == MIXER_DISPLAY)
        fb_layer_mixer_delete(fb.mixer);
}

static void lcdc_config_layer(struct lcdc_frame *frame,
     unsigned int layer_id, struct lcdc_layer *cfg)
{
    assert(layer_id < 4);
    struct layerdesc *layer = frame->layers[layer_id];

    dpu_init_layer_desc(layer, cfg);
    dpu_enable_layer(frame->framedesc, layer_id, !!cfg->layer_enable);
    dpu_enable_layer_scaling(frame->framedesc, layer_id, !!cfg->scaling.enable);
    dpu_set_layer_order(frame->framedesc, layer_id, cfg->layer_order);
}

static int lcdc_tft_pan_display(struct lcdc_frame *frame, struct fbdev_data *fbdev)
{
    fb.display_state = State_display_start;

    if (fb.display_mode == ROT_DISPLAY) {
        rot_write_reg(BUFF_CFG_ADDR, (unsigned long)(fb.mixer_cfg.dst_mem) | 1);
    } else if (fb.display_mode != COMPOSER_DISPLAY) {
        dpu_write(SRD_CHAIN_ADDR, (unsigned long)(frame->srdmadesc));
        dpu_start_simple_dma();
    } else {
        dpu_write(FRM_CFG_ADDR, (unsigned long)(frame->framedesc));
        dpu_start_composer();
    }

    uint64_t usec = systick_get_time_usec();
    while (fb.display_state != State_display_end) {
        if (systick_get_time_usec() - usec > 300 * 1000)
            break;
    }
    if (fb.display_state != State_display_end) {
        printf("fb: tft pan display wait timeout %d\n", fb.display_state);
        return -1;
    }

    return 0;
}

static int lcdc_slcd_pan_display(struct lcdc_frame *frame, struct fbdev_data *fbdev)
{
    /*等待旧的一帧刷新完成*/
    if (fb.display_state != State_clear) {
        uint64_t usec = systick_get_time_usec();
        while (fb.display_state != State_display_end) {
            if (systick_get_time_usec() - usec > 300 * 1000)
                break;
        }
        if (fb.display_state != State_display_end) {
            printf("fb: slcd pan display wait timeout\n");
            return -1;
        }
    }

    slcd_wait_busy_us(10*1000);
    fb.display_state = State_display_start;

    if (fb.display_mode == ROT_DISPLAY) {
        rot_write_reg(BUFF_CFG_ADDR, (unsigned long)(fb.mixer_cfg.dst_mem) | 1);
    } else if (fb.display_mode != COMPOSER_DISPLAY) {
        dpu_write(SRD_CHAIN_ADDR, (unsigned long)(frame->srdmadesc));
        dpu_start_simple_dma();
    } else {
        dpu_write(FRM_CFG_ADDR, (unsigned long)(frame->framedesc));
        dpu_start_composer();
    }

    return 0;
}

static void lcdc_config_composer_layer(struct fbdev_data *fbdev,  struct lcdc_frame *frame, unsigned int fb_index)
{
    int layer_id = fbdev - fb.fbdev;
    fbdev->layer_cfg.rgb.mem = fbdev->user.fb_mem + fb_index * fbdev->user.bytes_per_frame;

    if (!fbdev->is_user_setting)
        lcdc_config_layer(frame, layer_id, &fbdev->layer_cfg);
    else
        lcdc_config_layer(frame, layer_id, &fbdev->user_cfg);
}

static void lcdc_config_mixer_layer(struct fbdev_data *fbdev, struct fb_layer_mixer_dev *mixer, unsigned int fb_index)
{
    int layer_id = fbdev - fb.fbdev;
    fbdev->layer_cfg.rgb.mem = fbdev->user.fb_mem + fb_index * fbdev->user.bytes_per_frame;

    if (!fbdev->is_user_setting)
        fb_layer_mixer_set_input_layer(mixer, layer_id, &fbdev->layer_cfg);
    else
        fb_layer_mixer_set_input_layer(mixer, layer_id, &fbdev->user_cfg);
}

static void lcdc_config_srdma(struct fbdev_data *fbdev, struct lcdc_frame *frame, unsigned int wb_index)
{
    fbdev->srdma_cfg.fb_mem = fbdev->user.fb_mem + wb_index * fbdev->user.bytes_per_frame;

    lcdc_config_srdma_desc(frame->srdmadesc, &fbdev->srdma_cfg);
}

static void run_fb_mixer(int index)
{
    struct fbdev_data *fb_srdma = &fb.fbdev[4];

    fb.mixer_cfg.dst_mem = fb_srdma->user.fb_mem + index * fb_srdma->user.bytes_per_frame;
    fb_layer_mixer_set_output_frame(fb.mixer, &fb.mixer_cfg);

    fb_layer_mixer_work_out_one_frame(fb.mixer);
}

static void run_fb_rot(int index)
{
    fb.mixer_cfg.dst_mem = fb.rot.frame_mem + index * fb.rot.bytes_per_frame;

    fb_layer_mixer_set_output_frame(fb.mixer, &fb.mixer_cfg);

    fb_layer_mixer_work_out_one_frame(fb.mixer);
}

static struct lcdc_frame *get_display_frame(void)
{
    int frame_index;
    struct lcdc_frame *display_frame;

    fb.frame_index++;
    frame_index = fb.frame_index % 2;
    display_frame = &fb.frames[frame_index];

    return display_frame;
}

static int get_wb_index(void)
{
    int index;

    index = fb.wb_index % fb.wb_frame_count;
    fb.wb_index++;

    return index;
}

static void lcdc_pan_display(struct fbdev_data *fbdev, unsigned int fb_index)
{
    assert(fbdev);
    assert(fbdev->is_enable);

    if (!fbdev->is_user_setting)
        assert(fb_index < fbdev->frame_count);
    int ret;

    unsigned int wb_index;

    struct lcdc_frame *ready_frame = &fb.frames[2];

    if (fb.display_mode == COMPOSER_DISPLAY)
        lcdc_config_composer_layer(fbdev, ready_frame, fb_index);
    else if (fb.display_mode != SRDMA_DISPLAY)
        lcdc_config_mixer_layer(fbdev, fb.mixer, fb_index);

    struct lcdc_frame *display_frame = get_display_frame();
    *display_frame = *ready_frame;

    if (fb.display_mode == ROT_DISPLAY) {
        wb_index = get_wb_index();
        run_fb_rot(wb_index);
    } else if (fb.display_mode == MIXER_DISPLAY) {
        wb_index = get_wb_index();
        run_fb_mixer(wb_index);
        lcdc_config_srdma(&fb.fbdev[4], display_frame, wb_index);
    } else if (fb.display_mode == SRDMA_DISPLAY) {
        wb_index = fb_index;
        lcdc_config_srdma(&fb.fbdev[4], display_frame, wb_index);
    }

    if (!is_video_mode(fb.pdata))
        ret = lcdc_slcd_pan_display(display_frame, fbdev);
    else
        ret = lcdc_tft_pan_display(display_frame, fbdev);

    if (ret < 0)
        return;

    if (fb.pdata->pan_display_cb)
        fb.pdata->pan_display_cb();

    if (fb.pan_display_sync) {
        uint64_t usec = systick_get_time_usec();
        while (fb.display_state != State_display_end) {
            if (systick_get_time_usec() - usec > 300 * 1000)
                break;
        }
        if (fb.display_state != State_display_end)
            printf("fb: sync lcd pan display wait timeout\n");
    }
}

int fb_irq_handler(int irq, void *data)
{
    unsigned long flags = dpu_read(INT_FLAG);

    if (get_bit_field(flags, DISP_END)) {
        dpu_write(CLR_ST, bit_field_val(CLR_DISP_END, 1));
        fb.display_state = State_display_end;
        return 0;
    }

    if (get_bit_field(flags, SRD_END)) {
        dpu_write(CLR_ST, bit_field_val(CLR_SRD_END, 1));
        fb.display_state = State_display_end;
        return 0;
    }

    if (get_bit_field(flags, TFT_UNDR)) {
        fb.underrun_count++;
        if (!(fb.underrun_count % 1000))
            printf("err: lcd underrun, tft_underrun_count = %d\n", fb.underrun_count);

        dpu_write(CLR_ST, bit_field_val(CLR_TFT_UNDR, 1));
        return 0;
    }

    return -EINVAL;
}

int fb_set_cfg(struct fbdev_data *fbdev, struct lcdc_layer *cfg)
{
    if (!cfg || fbdev - fb.fbdev > 4)
        return -EINVAL;

    if (cfg->layer_order > lcdc_layer_3) {
        printf("fb: invalid order: %d\n", cfg->layer_order);
        return -EINVAL;
    }

    if (cfg->alpha.value >= 256) {
        printf("fb: invalid alpha: %x\n", cfg->alpha.value);
        return -EINVAL;
    }

    if (cfg->fb_fmt > fb_fmt_yuv422) {
        printf("fb: invalid fmt: %d\n", cfg->fb_fmt);
        return -EINVAL;
    }

    if (cfg->fb_fmt == fb_fmt_NV12 || cfg->fb_fmt == fb_fmt_NV21) {
        if (cfg->y.mem >= (void *)(512*1024*1024)) {
            printf("fb: must be phys address\n");
            return -EINVAL;
        }
        if (cfg->uv.mem >= (void *)(512*1024*1024)) {
            printf("fb: must be phys address\n");
            return -EINVAL;
        }
    } else {
        if (cfg->rgb.mem >= (void *)(512*1024*1024)) {
            printf("fb: must be phys address\n");
            return -EINVAL;
        }
    }

    if (!cfg->scaling.enable) {
        if (cfg->xres + cfg->xpos > fb.layer_xres) {
            printf("fb: invalid xres: %d %d\n", cfg->xres, cfg->xpos);
            return -EINVAL;
        }
        if (cfg->yres + cfg->ypos > fb.layer_yres) {
            printf("fb: invalid yres: %d %d\n", cfg->yres, cfg->ypos);
            return -EINVAL;
        }
    } else {
        if (cfg->scaling.xres + cfg->xpos > fb.layer_xres) {
            printf("fb: invalid scaling xres: %d %d\n", cfg->scaling.xres, cfg->xpos);
            return -EINVAL;
        }
        if (cfg->scaling.yres + cfg->ypos > fb.layer_yres) {
            printf("fb: invalid scaling yres: %d %d\n", cfg->scaling.yres, cfg->ypos);
            return -EINVAL;
        }
    }

    fbdev->user_cfg = *cfg;
    if (fb.use_default_order)
        fbdev->user_cfg.layer_order = (fbdev - fb.fbdev) + 2;

    if (cfg->fb_fmt == fb_fmt_NV12 || cfg->fb_fmt == fb_fmt_NV21) {
        fbdev->user_cfg.y.mem = (void *)(cfg->y.mem);
        fbdev->user_cfg.uv.mem = (void *)(cfg->uv.mem);
    } else {
        fbdev->user_cfg.rgb.mem = (void *)(cfg->rgb.mem);
    }

    return 0;
}

void fb_enable_cfg(struct fbdev_data *fbdev)
{
    fbdev->is_user_setting = 1;
}

void fb_disable_cfg(struct fbdev_data *fbdev)
{
    fbdev->is_user_setting = 0;
}

struct fbdev_data *fb_open(int fbnum)
{
    if (fbnum < 0 || fbnum > (fb.enabled_num - 1)) {
        printf("fb: fb%d not be enabled\n", fbnum);
        return NULL;
    }

    int i, j = 0, k = fb.enabled_num;
    struct fbdev_data *data = NULL;
    for (i = 0; i < 5 && k != 0; i++) {
        if (fb.fbdev[i].is_enable) {
            if (j == fbnum) {
                data = &fb.fbdev[i];
                break;
            }
            j++;k--;
        }
    }

    if (data)
        printf("fb: use fb%d, actually layer%d\n", fbnum, data - fb.fbdev);
    else
        printf("fb: failed to open fb%d\n", fbnum);

    return data;
}

int fb_enable(struct fbdev_data *fbdev)
{
    if (!fbdev->is_power_on) {
        enable_fb();
        fbdev->is_power_on = 1;
        fbdev->layer_cfg.layer_enable = 1;
        fbdev->user_cfg.layer_enable = 1;
    }

    return 0;
}

int fb_disable(struct fbdev_data *fbdev)
{
    if (fbdev->is_power_on) {
        disable_fb();
        fbdev->is_power_on = 0;
        fbdev->layer_cfg.layer_enable = 0;
        fbdev->user_cfg.layer_enable = 0;
    }

    if (fb.is_enabled != 0)
        lcdc_pan_display(fbdev, 0);

    return 0;
}

int fb_pan_display(struct fbdev_data *fbdev, int fb_index)
{
    struct fb_mem_info *user = &fbdev->user;

    if (!fbdev->user.frame_count && !fbdev->is_user_setting)
        return -EINVAL;

    if (fb_index >= user->frame_count && !fbdev->is_user_setting) {
        printf("fb: can't support frame_index: %d\n", fb_index);
        return -EINVAL;
    }

    if (!fbdev->is_power_on) {
        printf("fb: layer%d is not enabled\n", fbdev - fb.fbdev);
        return -EBUSY;
    }

    if (fbdev->is_user_setting && fb_index) {
        printf("fb: can't use yoffset, if use cfg\n");
        return -EINVAL;
    }

    lcdc_pan_display(fbdev, fb_index);

    return 0;
}

int fb_is_enable(struct fbdev_data *fbdev)
{
    assert(fbdev);

    return fbdev->is_enable;
}

void fb_get_info(struct fbdev_data *fbdev, struct fb_mem_info *info)
{
    assert(fbdev && info);

    *info = fbdev->user;
}

static int check_scld_fmt(struct lcdc_data *pdata)
{
    int pix_fmt = pdata->out_format;

    int width = pdata->slcd.mcu_data_width;
    if (width == MCU_WIDTH_8BITS) {
        if (pix_fmt != OUT_FORMAT_RGB565 && pix_fmt != OUT_FORMAT_RGB888)
            return -1;
    }
    if (width == MCU_WIDTH_9BITS) {
        if (pix_fmt != OUT_FORMAT_RGB666)
            return -1;
    }
    if (width == MCU_WIDTH_16BITS) {
        if (pix_fmt != OUT_FORMAT_RGB565)
            return -1;
    }

    return 0;
}

#define error_if(_cond) \
    do { \
        if (_cond) { \
            printf("fb: failed to check: %s\n", #_cond); \
            return -1; \
        } \
    } while (0)

static int check_fbdev_user_cfg(struct fbdev_data *fbdev)
{
    if (!fbdev->enable_fbdev_user_cfg)
        return 0;

    if (fbdev->fbdev_user_cfg.scaling_enable) {
        if (fbdev->fbdev_user_cfg.xpos + fbdev->fbdev_user_cfg.scaling_width > fb.layer_xres) {
            printf("fb: invalid fbdev user scaling_width and xpos: %d, %d\n", fbdev->fbdev_user_cfg.scaling_width, fbdev->fbdev_user_cfg.xpos);
            return -1;
        }

        if (fbdev->fbdev_user_cfg.ypos + fbdev->fbdev_user_cfg.scaling_height > fb.layer_yres) {
            printf("fb: invalid fbdev user scaling_height and xpos: %d, %d\n", fbdev->fbdev_user_cfg.scaling_height, fbdev->fbdev_user_cfg.ypos);
            return -1;
        }
    } else {
        if (fbdev->fbdev_user_cfg.xpos + fbdev->fbdev_user_cfg.width > fb.layer_xres) {
            printf("fb: invalid fbdev user width and xpos: %d, %d\n", fbdev->fbdev_user_cfg.width, fbdev->fbdev_user_cfg.xpos);
            return -1;
        }

        if (fbdev->fbdev_user_cfg.ypos + fbdev->fbdev_user_cfg.height > fb.layer_yres) {
            printf("fb: invalid fbdev user height and xpos: %d, %d\n", fbdev->fbdev_user_cfg.height, fbdev->fbdev_user_cfg.ypos);
            return -1;
        }
    }

    return 0;
}

int fb_register_lcd(struct lcdc_data *pdata)
{
    int ret;
    int i;

    error_if(fb.pdata != NULL);
    error_if(pdata == NULL);
    error_if(pdata->name == NULL);
    error_if(pdata->power_on == NULL);
    error_if(pdata->power_off == NULL);
    error_if(pdata->xres < 32 || pdata->xres >= 2048);
    error_if(pdata->yres < 32 || pdata->yres >= 2048);
    error_if(pdata->fb_fmt >= fb_fmt_NV12);

    if (is_slcd(pdata))
        error_if(check_scld_fmt(pdata));

    auto_calculate_pixel_clock(pdata);

    if (is_slcd_mipi(pdata) || is_tft_mipi(pdata) || is_lvds(pdata)) {
        ret = jz_mipi_dsi_data_init(pdata);
        if (ret < 0)
            return ret;
    }

    ret = init_gpio(pdata);
    if (ret)
        return ret;

    fb.pdata = pdata;

    ret = rot_check_params();
    if (ret)
        return ret;

    fb.layer_xres = fb.rot.angle % 2 ? fb.pdata->yres : fb.pdata->xres;
    fb.layer_yres = fb.rot.angle % 2 ? fb.pdata->xres : fb.pdata->yres;
    fb.wb_frame_count = fb.rot.is_enable ? fb.rot.frame_count : fb.fbdev[4].frame_count;

    for (i = 0; i < 4; i++) {
        ret = check_fbdev_user_cfg(&fb.fbdev[i]);
        if (ret < 0)
            return ret;
    }

    lcdc_alloc_desc();

    lcdc_init_fbdev();

    init_rot_cfg();

    return 0;
}

void fb_unregister_lcd(struct lcdc_data *pdata)
{
    assert(pdata == fb.pdata);

    if (fb.is_enabled) {
        fb.is_enabled = 1;
        disable_fb();
    }

    free_mem();

    free_rot();

    fb.pdata = NULL;
}

void fb_init(void)
{
    fb_get_config();
    fb_layer_mixer_init();
    jz_mipi_dsi_init();

    /* srdma 不使能时 layer0 也就是fb0 一定会有
     */
    if(fb.fbdev[4].is_enable) {
        if (fb.fbdev[4].frame_count < 0) {
            printf("fb: frame_count invalid: srdma\n");
            return;
        }
    } else {
        fb.fbdev[4].frame_count = 0;
        fb.fbdev[0].is_enable = 1;
    }

    int i, save_i = 4;
    for (i = 0; i < 4; i++) {
        struct fbdev_data *fbdev = &fb.fbdev[i];
        if (!fbdev->is_enable) {
            save_i = i;
            break;
        }
    }

    for (i = save_i + 1; i < 4; i++) {
        struct fbdev_data *fbdev = &fb.fbdev[i];
        if (fbdev->is_enable) {
            printf("fb: can't enable layer%d, because layer%d is disabled\n", i, save_i);
            return;
        }
    }

    for (i = 0; i < 4; i++) {
        struct fbdev_data *fbdev = &fb.fbdev[i];
        if (!fbdev->is_enable)
            fbdev->frame_count = 0;

        if (fbdev->is_enable && fbdev->frame_count <= 0) {
            printf("fb: frame_count invalid: layer%d\n", i);
            return;
        }

        if (fbdev->alpha >= 256)
            fbdev->alpha = 255;
    }

    dpu_request_irq();

    request_irq(IRQ_ROTATE, 0, rot_irq_handler, "fb_mcu_rot", NULL);
}

void fb_deinit(void)
{
    assert(!fb.pdata);

    disable_irq(IRQ_LCD);
    release_irq(IRQ_LCD);

    disable_irq(IRQ_ROTATE);
    release_irq(IRQ_ROTATE);
}
