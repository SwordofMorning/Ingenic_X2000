#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/compiler.h>
#include <soc/base.h>

#include "common.h"

#include "drivers/rmem_manager/rmem_manager.h"

#include "../mipi_dsi/jz_mipi_dsi.c"
#include "lcdc.c"
#include "../fb_layer_mixer/fb_layer_mixer.h"
#include "rot.h"

#define CMD_set_cfg       _IOWR('i', 80, struct lcdc_layer)
#define CMD_enable_cfg    _IO('i', 81)
#define CMD_disable_cfg   _IO('i', 82)
#define CMD_read_reg      _IOWR('i', 83, unsigned long)

#define AD100_DDR_CCHC0 0xb3012024
#define AD100_DDR_CCHC1 0xb3012028

enum display_mode {
    COMPOSER_DISPLAY,
    SRDMA_DISPLAY,
    MIXER_DISPLAY,
    ROT_DISPLAY,
};

struct fb_mem_info {
    void *fb_mem;
    enum fb_fmt fb_fmt;
    unsigned int xres;
    unsigned int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    unsigned int frame_count;
};

struct fbdev_user_config {
    int width;
    int height;
    int xpos;
    int ypos;
    int scaling_enable;
    int scaling_width;
    int scaling_height;
};

struct fbdev_data {
    struct mutex dev_lock;
    struct fb_info *fb;
    int is_enable;
    int is_power_on;

    int frame_count;
    unsigned int alpha;
    void *fb_mem;
    unsigned int fb_mem_size;
    struct fb_mem_info user;
    struct fb_videomode videomode;

    int is_user_setting;
    struct lcdc_layer user_cfg;
    struct lcdc_layer layer_cfg;
    struct srdma_cfg srdma_cfg;

    int enable_fbdev_user_cfg;
    struct fbdev_user_config fbdev_user_cfg;

    int is_display;
};

struct lcdc_frame {
    struct framedesc *framedesc;
    struct layerdesc *layers[4];
    struct srdmadesc *srdmadesc;
};

#define State_clear 0
#define State_display_start   1
#define State_display_end     2

#define ROT_BPP_Custome_format 4
#define ROT_Block_Size 512

static struct {
    int underrun_count;
    int display_count;

    struct clk *clk;
    struct clk *pclk;
    struct clk *rot_clk;
    int irq;
    struct mutex lock;
    struct mutex frames_lock;
    spinlock_t spinlock;

    struct lcdc_frame frames[3];
    struct lcdc_data *pdata;
    enum display_mode display_mode;
    int is_enabled;
    int is_open;
    int display_state;
    int pan_display_sync;
    int frame_index;
    int wb_index;
    int wb_frame_count;

    wait_queue_head_t wait_queue;

    struct fbdev_data fbdev[5];

    int layer_xres;
    int layer_yres;
    struct rot_cfg rot;

    int mixer_enable;
    struct fb_layer_mixer_dev *mixer;
    struct fb_layer_mixer_output_cfg mixer_cfg;

    int use_default_order;

    struct device *dev;
} jzfb;

int lcd_is_inited = 0;
module_param(lcd_is_inited, int, 0644);

module_param_named(pan_display_sync, jzfb.pan_display_sync, int, 0644);

int rot_angle = 0;
module_param_named(rot_enable, jzfb.rot.is_enable, int, 0644);
module_param(rot_angle, int, 0444);
module_param_named(rot_frames, jzfb.rot.frame_count, int, 0644);

module_param_named(use_default_order, jzfb.use_default_order, int, 0644);
module_param_named(layer0_enable, jzfb.fbdev[0].is_enable, int, 0644);
module_param_named(layer0_frames, jzfb.fbdev[0].frame_count, int, 0644);
module_param_named(layer0_alpha, jzfb.fbdev[0].alpha, int, 0644);

module_param_named(layer1_enable, jzfb.fbdev[1].is_enable, int, 0644);
module_param_named(layer1_frames, jzfb.fbdev[1].frame_count, int, 0644);
module_param_named(layer1_alpha, jzfb.fbdev[1].alpha, int, 0644);

module_param_named(layer2_enable, jzfb.fbdev[2].is_enable, int, 0644);
module_param_named(layer2_frames, jzfb.fbdev[2].frame_count, int, 0644);
module_param_named(layer2_alpha, jzfb.fbdev[2].alpha, int, 0644);

module_param_named(layer3_enable, jzfb.fbdev[3].is_enable, int, 0644);
module_param_named(layer3_frames, jzfb.fbdev[3].frame_count, int, 0644);
module_param_named(layer3_alpha, jzfb.fbdev[3].alpha, int, 0644);

module_param_named(srdma_enable, jzfb.fbdev[4].is_enable, int, 0644);
module_param_named(srdma_frames, jzfb.fbdev[4].frame_count, int, 0644);

module_param_named(mixer_enable, jzfb.mixer_enable, int, 0644);

module_param_named(tft_underrun_count, jzfb.underrun_count, int, 0644);

module_param_named(user_fb0_enable, jzfb.fbdev[0].enable_fbdev_user_cfg, int, 0644);
module_param_named(user_fb0_width, jzfb.fbdev[0].fbdev_user_cfg.width, int, 0644);
module_param_named(user_fb0_height, jzfb.fbdev[0].fbdev_user_cfg.height, int, 0644);
module_param_named(user_fb0_xpos, jzfb.fbdev[0].fbdev_user_cfg.xpos, int, 0644);
module_param_named(user_fb0_ypos, jzfb.fbdev[0].fbdev_user_cfg.ypos, int, 0644);
module_param_named(user_fb0_scaling_enable, jzfb.fbdev[0].fbdev_user_cfg.scaling_enable, int, 0644);
module_param_named(user_fb0_scaling_width, jzfb.fbdev[0].fbdev_user_cfg.scaling_width, int, 0644);
module_param_named(user_fb0_scaling_height, jzfb.fbdev[0].fbdev_user_cfg.scaling_height, int, 0644);

module_param_named(user_fb1_enable, jzfb.fbdev[1].enable_fbdev_user_cfg, int, 0644);
module_param_named(user_fb1_width, jzfb.fbdev[1].fbdev_user_cfg.width, int, 0644);
module_param_named(user_fb1_height, jzfb.fbdev[1].fbdev_user_cfg.height, int, 0644);
module_param_named(user_fb1_xpos, jzfb.fbdev[1].fbdev_user_cfg.xpos, int, 0644);
module_param_named(user_fb1_ypos, jzfb.fbdev[1].fbdev_user_cfg.ypos, int, 0644);
module_param_named(user_fb1_scaling_enable, jzfb.fbdev[1].fbdev_user_cfg.scaling_enable, int, 0644);
module_param_named(user_fb1_scaling_width, jzfb.fbdev[1].fbdev_user_cfg.scaling_width, int, 0644);
module_param_named(user_fb1_scaling_height, jzfb.fbdev[1].fbdev_user_cfg.scaling_height, int, 0644);

module_param_named(user_fb2_enable, jzfb.fbdev[2].enable_fbdev_user_cfg, int, 0644);
module_param_named(user_fb2_width, jzfb.fbdev[2].fbdev_user_cfg.width, int, 0644);
module_param_named(user_fb2_height, jzfb.fbdev[2].fbdev_user_cfg.height, int, 0644);
module_param_named(user_fb2_xpos, jzfb.fbdev[2].fbdev_user_cfg.xpos, int, 0644);
module_param_named(user_fb2_ypos, jzfb.fbdev[2].fbdev_user_cfg.ypos, int, 0644);
module_param_named(user_fb2_scaling_enable, jzfb.fbdev[2].fbdev_user_cfg.scaling_enable, int, 0644);
module_param_named(user_fb2_scaling_width, jzfb.fbdev[2].fbdev_user_cfg.scaling_width, int, 0644);
module_param_named(user_fb2_scaling_height, jzfb.fbdev[2].fbdev_user_cfg.scaling_height, int, 0644);

module_param_named(user_fb3_enable, jzfb.fbdev[3].enable_fbdev_user_cfg, int, 0644);
module_param_named(user_fb3_width, jzfb.fbdev[3].fbdev_user_cfg.width, int, 0644);
module_param_named(user_fb3_height, jzfb.fbdev[3].fbdev_user_cfg.height, int, 0644);
module_param_named(user_fb3_xpos, jzfb.fbdev[3].fbdev_user_cfg.xpos, int, 0644);
module_param_named(user_fb3_ypos, jzfb.fbdev[3].fbdev_user_cfg.ypos, int, 0644);
module_param_named(user_fb3_scaling_enable, jzfb.fbdev[3].fbdev_user_cfg.scaling_enable, int, 0644);
module_param_named(user_fb3_scaling_width, jzfb.fbdev[3].fbdev_user_cfg.scaling_width, int, 0644);
module_param_named(user_fb3_scaling_height, jzfb.fbdev[3].fbdev_user_cfg.scaling_height, int, 0644);

static inline void dump_frame(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        struct lcdc_frame *frame = &jzfb.frames[i];
        printk("frame%d\n", i);
        printk("FrameCfgAddr %08lx\n", frame->framedesc->FrameCfgAddr);
        printk("FrameSize %08lx\n", frame->framedesc->FrameSize);
        printk("FrameCtrl %08lx\n", frame->framedesc->FrameCtrl);
        printk("WritebackBufferAddr %08lx\n", frame->framedesc->WritebackBufferAddr);
        printk("WritebackStride %08lx\n", frame->framedesc->WritebackStride);
        printk("Layer0CfgAddr %08lx\n", frame->framedesc->Layer0CfgAddr);
        printk("Layer1CfgAddr %08lx\n", frame->framedesc->Layer1CfgAddr);
        printk("Layer2CfgAddr %08lx\n", frame->framedesc->Layer2CfgAddr);
        printk("Layer3CfgAddr %08lx\n", frame->framedesc->Layer3CfgAddr);
        printk("LayerCfgScaleEn %08lx\n", frame->framedesc->LayerCfgScaleEn);
        printk("InterruptControl %08lx\n", frame->framedesc->InterruptControl);

        int j;
        for (j = 0; j < 4; j++) {
            printk("LayerSize %08lx\n", frame->layers[j]->LayerSize);
            printk("LayerCfg %08lx\n", frame->layers[j]->LayerCfg);
            printk("LayerBufferAddr %08lx\n", frame->layers[j]->LayerBufferAddr);
            printk("LayerTargetSize %08lx\n", frame->layers[j]->LayerTargetSize);
            printk("Reserved1 %08lx\n", frame->layers[j]->Reserved1);
            printk("Reserved2 %08lx\n", frame->layers[j]->Reserved2);
            printk("LayerPos %08lx\n", frame->layers[j]->LayerPos);
            printk("Layer_Resize_Coef_X %08lx\n", frame->layers[j]->Layer_Resize_Coef_X);
            printk("Layer_Resize_Coef_Y %08lx\n", frame->layers[j]->Layer_Resize_Coef_Y);
            printk("LayerStride %08lx\n", frame->layers[j]->LayerStride);
            printk("BufferAddr_UV %08lx\n", frame->layers[j]->BufferAddr_UV);
            printk("stride_UV %08lx\n", frame->layers[j]->stride_UV);
            printk("\n");
        }
    }
}

static void init_video_mode(struct fb_mem_info *info, struct fb_videomode *mode)
{
    struct lcdc_data *pdata = jzfb.pdata;

    mode->name = pdata->name;
    mode->refresh = pdata->refresh;
    mode->xres = info->xres;
    mode->yres = info->yres;
    mode->pixclock = pdata->pixclock;
    mode->left_margin = pdata->left_margin;
    mode->right_margin = pdata->right_margin;
    mode->upper_margin = pdata->upper_margin;
    mode->lower_margin = pdata->lower_margin;
    mode->hsync_len = pdata->hsync_len;
    mode->vsync_len = pdata->vsync_len;
    mode->sync = 0;
    mode->vmode = 0;
    mode->flag = 0;
}

static void init_fix_info(struct fb_mem_info *info, struct fb_fix_screeninfo *fix)
{
    strcpy(fix->id, "jzfb");
    fix->type = FB_TYPE_PACKED_PIXELS;
    fix->visual = FB_VISUAL_TRUECOLOR;
    fix->xpanstep = 0;
    fix->ypanstep = 1;
    fix->ywrapstep = 0;
    fix->accel = FB_ACCEL_NONE;
    fix->line_length = info->bytes_per_line;
    fix->smem_start = virt_to_phys(info->fb_mem);
    fix->smem_len = info->frame_count * info->bytes_per_frame;
    fix->mmio_start = 0;
    fix->mmio_len = 0;
}

static inline void set_fb_bitfield(struct fb_bitfield *field, int length, int offset, int msb_right)
{
    field->length = length;
    field->offset = offset;
    field->msb_right = msb_right;
}

static void init_var_info(struct fb_mem_info *info, struct fb_var_screeninfo *var)
{
    struct lcdc_data *pdata = jzfb.pdata;

    var->xres = info->xres;
    var->yres = info->yres;
    var->xres_virtual = info->xres;
    var->yres_virtual = info->yres * info->frame_count;
    var->xoffset = 0;
    var->yoffset = 0;
    var->height = pdata->height;
    var->width = pdata->width;
    var->pixclock = pdata->pixclock;
    var->left_margin = pdata->left_margin;
    var->right_margin = pdata->right_margin;
    var->upper_margin = pdata->upper_margin;
    var->lower_margin = pdata->lower_margin;
    var->hsync_len = pdata->hsync_len;
    var->vsync_len = pdata->vsync_len;
    var->sync = 0;
    var->vmode = 0;

    var->bits_per_pixel = bits_per_pixel(pdata->fb_fmt);
    switch (pdata->fb_fmt) {
    case fb_fmt_RGB555:
        set_fb_bitfield(&var->red, 5, 10, 0);
        set_fb_bitfield(&var->green, 5, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_RGB565:
        set_fb_bitfield(&var->red, 5, 11, 0);
        set_fb_bitfield(&var->green, 6, 5, 0);
        set_fb_bitfield(&var->blue, 5, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_RGB888:
        set_fb_bitfield(&var->red, 8, 16, 0);
        set_fb_bitfield(&var->green, 8, 8, 0);
        set_fb_bitfield(&var->blue, 8, 0, 0);
        set_fb_bitfield(&var->transp, 0, 0, 0);
        break;
    case fb_fmt_ARGB8888:
        set_fb_bitfield(&var->red, 8, 16, 0);
        set_fb_bitfield(&var->green, 8, 8, 0);
        set_fb_bitfield(&var->blue, 8, 0, 0);
        set_fb_bitfield(&var->transp, 8, 24, 0);
        break;
    default:
        assert(0);
    }
}

static inline void *m_dma_alloc_coherent(int size, int align)
{
    void *mem = rmem_alloc_aligned(ALIGN(size, 64), align);
    assert(mem);

    return mem;
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    rmem_free(mem, ALIGN(size, 64));
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_wback((unsigned long)mem, size);
}

static void init_fbdev_frame_layer(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        init_frame_desc(jzfb.pdata, jzfb.frames[i].framedesc, jzfb.frames[i].layers[0],\
                                                 jzfb.frames[i].layers[1],\
                                                 jzfb.frames[i].layers[2],\
                                                 jzfb.frames[i].layers[3]);
        m_cache_sync(jzfb.frames[i].framedesc, sizeof(*jzfb.frames[i].framedesc));
    }
}

static void init_fbdev_data(struct fbdev_data *fbdev, int frame_count)
{
    int fb_mem_size = 0;
    struct fb_mem_info *info = &fbdev->user;
    struct lcdc_data *pdata = jzfb.pdata;
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
        info->xres = jzfb.layer_xres;
        info->yres = jzfb.layer_yres;
    }

    info->bytes_per_line = bytes_per_line(info->xres, info->fb_fmt);

    info->bytes_per_frame = bytes_per_frame(info->bytes_per_line, info->yres);
    fb_mem_size = info->bytes_per_frame * frame_count;
    fb_mem_size = ALIGN(fb_mem_size, PAGE_SIZE);
    info->frame_count = frame_count;

    if (frame_count) {
        if (!fbdev->fb_mem)
            fbdev->fb_mem = m_dma_alloc_coherent(fb_mem_size, PAGE_SIZE);
        if (jzfb.display_state == State_clear)
            memset(fbdev->fb_mem, 0, fb_mem_size);
    } else {
        fbdev->fb_mem = NULL;
    }

    fbdev->fb_mem_size = fb_mem_size;
    info->fb_mem = fbdev->fb_mem;

    if (fbdev - jzfb.fbdev < 4) {
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
        srdma_cfg->is_video = is_video_mode(jzfb.pdata);
        srdma_cfg->fb_fmt = info->fb_fmt;
        srdma_cfg->fb_mem = info->fb_mem;
        srdma_cfg->stride = bytes_per_line(pdata->xres, info->fb_fmt) / bytes_per_pixel(info->fb_fmt);
    }

    mutex_init(&fbdev->dev_lock);
}

static void lcdc_init_fbdev(void)
{
    int i;
    for(i = 0; i < 5; i++) {
        if(jzfb.fbdev[i].is_enable)
            init_fbdev_data(&jzfb.fbdev[i], jzfb.fbdev[i].frame_count);
    }

    jzfb.fbdev[0].layer_cfg.layer_order = lcdc_layer_0;
    jzfb.fbdev[1].layer_cfg.layer_order = lcdc_layer_1;
    jzfb.fbdev[2].layer_cfg.layer_order = lcdc_layer_2;
    jzfb.fbdev[3].layer_cfg.layer_order = lcdc_layer_3;
}

static void lcdc_alloc_desc(void)
{
    int i;
    for (i = 0; i < 3;i++) {
        jzfb.frames[i].framedesc = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].framedesc), 64);
        jzfb.frames[i].layers[0] = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layers[0]), 64);
        jzfb.frames[i].layers[1] = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layers[1]), 64);
        jzfb.frames[i].layers[2] = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layers[2]), 64);
        jzfb.frames[i].layers[3] = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].layers[3]), 64);

        jzfb.frames[i].srdmadesc = m_dma_alloc_coherent(sizeof(*jzfb.frames[i].srdmadesc), 64);
    }

    mutex_init(&jzfb.frames_lock);
    jzfb.frame_index = 0;
}

static int free_mem(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        struct fbdev_data *fbdev = &jzfb.fbdev[i];
        if (fbdev->frame_count)
            m_dma_free_coherent(fbdev->fb_mem, fbdev->fb_mem_size);
    }

    for (i = 0; i < 3; i++) {
        m_dma_free_coherent(jzfb.frames[i].framedesc, sizeof(*jzfb.frames[i].framedesc));
        m_dma_free_coherent(jzfb.frames[i].layers[0], sizeof(*jzfb.frames[i].layers[0]));
        m_dma_free_coherent(jzfb.frames[i].layers[1], sizeof(*jzfb.frames[i].layers[1]));
        m_dma_free_coherent(jzfb.frames[i].layers[2], sizeof(*jzfb.frames[i].layers[2]));
        m_dma_free_coherent(jzfb.frames[i].layers[3], sizeof(*jzfb.frames[i].layers[3]));
        m_dma_free_coherent(jzfb.frames[i].srdmadesc, sizeof(*jzfb.frames[i].srdmadesc));
    }

    return 0;
}

static irqreturn_t rot_irq_handler(int irq, void *data)
{
    unsigned long status = rot_read_reg(STATUS);
    if (get_bit_field(&status, FA))
        rot_set_bits(CLR_STATUS, CLR_FA, 1);

    if (get_bit_field(&status, SOF))
        rot_set_bits(CLR_STATUS, CLR_SOF, 1);

    if (get_bit_field(&status, EOF)) {
        rot_set_bits(CLR_STATUS, CLR_EOF, 1);
        jzfb.display_state = State_display_end;
        wake_up_all(&jzfb.wait_queue);
    }

    if (get_bit_field(&status, GEN_STOP_ACK))
        rot_set_bits(CLR_STATUS, CLR_GEN_STP_ACK, 1);

    return IRQ_HANDLED;
}

static void jzfb_quick_stop_rot(void)
{
    rot_set_bits(ROT_CTRL, QUICK_STOP, 1);
    rot_set_bits(GLB_CFG, FIFO_GATE, 1);
    rot_set_bits(ROT_CTRL, FLUSH_FIFO, 1);
}

static int rot_check_params(void)
{
    if (rot_angle == 90)
        jzfb.rot.angle = 1;
    else if (rot_angle == 180)
        jzfb.rot.angle = 2;
    else if (rot_angle == 270)
        jzfb.rot.angle = 3;
    else if (rot_angle == 0)
        jzfb.rot.angle = 0;
    else {
        printk(KERN_EMERG "jzfb: rot angle can't support this angle: %d°\n", rot_angle);
        return -EINVAL;
    }

    return 0;
}

static void init_rotator(void)
{
    /* Custom format: pixel are ARGB8888, and each line have to align with block_size(512) */
    int byte_per_line = jzfb.layer_xres * ROT_BPP_Custome_format;
    jzfb.rot.bytes_per_frame = ALIGN(byte_per_line, ROT_Block_Size) * jzfb.layer_yres;

    byte_per_line = jzfb.layer_yres * ROT_BPP_Custome_format;
    int max_bytes_per_frame = ALIGN(byte_per_line, ROT_Block_Size) * jzfb.layer_xres;
    if (max_bytes_per_frame < jzfb.rot.bytes_per_frame)
        max_bytes_per_frame = jzfb.rot.bytes_per_frame;

    jzfb.rot.mem_size = max_bytes_per_frame * jzfb.rot.frame_count;

    jzfb.rot.frame_mem = m_dma_alloc_coherent(jzfb.rot.mem_size, PAGE_SIZE);
    memset(jzfb.rot.frame_mem, 0, jzfb.rot.mem_size);

    return;
}

static void enable_rotator(void)
{
    clk_prepare_enable(jzfb.rot_clk);

    rot_set_bits(INT_MASK, EOF_MASK, 1);
    rot_set_bits(INT_MASK, GSA_MASK, 1);

    /*调整rotater ddr 优先级。rotater 慢可能这里有点儿用*/
    // int ddr_cchc0 = 0x88404010;
    // int ddr_cchc1 = 0x88404030;

    // *(volatile unsigned long *)(AD100_DDR_CCHC0) = ddr_cchc0;
    // *(volatile unsigned long *)(AD100_DDR_CCHC1) = ddr_cchc1;

    // 设置固定最高优先级
    rot_write_reg(0x30, 0x7); /* 参考内核驱动, 手册上无相关描述 */

    rot_set_bits(FRM_SIZE, FRAM_HEIGHT, jzfb.layer_yres);
    rot_set_bits(FRM_SIZE, FRAM_WIDTH, jzfb.layer_xres);

    unsigned long glb_cfg = rot_read_reg(GLB_CFG);
    set_bit_field(&glb_cfg, RDMA_FMT, 3);
    set_bit_field(&glb_cfg, FIFO_GATE, 0);
    set_bit_field(&glb_cfg, MANUAL_AUTO, 0);//Write BUFF_register by software
    set_bit_field(&glb_cfg, ROT_ANGLE, jzfb.rot.angle);
    set_bit_field(&glb_cfg, RDMA_BURST_LEN, 0);
    rot_write_reg(GLB_CFG, glb_cfg);
}

static void free_rot(void)
{
    if (jzfb.rot.is_enable == 0)
        return;

    m_dma_free_coherent(jzfb.rot.frame_mem, jzfb.rot.mem_size);
    clk_disable_unprepare(jzfb.rot_clk);
}

static void rot_reconfig(void)
{
    if ((rot_angle / 90) == jzfb.rot.angle)
        return;

    if (jzfb.display_state == State_display_start)
        wait_event(jzfb.wait_queue, jzfb.display_state == State_display_end);

    if (jzfb.display_mode == ROT_DISPLAY)
        jzfb_quick_stop_rot();
    else
        dpu_quick_stop_srdma();

    unsigned long wait_count = 0;
    while (dpu_get_bit(ST, WORKING)) {
        usleep_range(100, 100);
        wait_count++;
        if (wait_count >= 300) {
            printk(KERN_ERR "wait dpu work stop timeout!\n");
            break;
        }
    }

    jzfb.rot.angle = rot_angle / 90;
    jzfb.mixer_cfg.xres = jzfb.layer_xres;
    jzfb.mixer_cfg.yres = jzfb.layer_yres;

    if (jzfb.rot.is_enable) {
        jzfb.mixer_cfg.format = fb_fmt_ROTATE;
        jzfb.display_mode = ROT_DISPLAY;
        dpu_config_rot_ch();
        enable_rotator();
    } else {
        jzfb.mixer_cfg.format = jzfb.pdata->fb_fmt;
        jzfb.display_mode = MIXER_DISPLAY;
        dpu_config_srdma_ch();
    }
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
            if (table[i].value >= 500)
                usleep_range(table[i].value, table[i].value);
            else
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

int slcd_read_data(int reg, int count, char *buffer)
{
    int i, ret;
    ret = slcd_wait_busy_us(100*1000);
    if (ret) {
        printk(KERN_ERR "slcd is busy\n");
        return -1;
    }

    ret = slcd_init_gpio_status(1, jzfb.pdata);
    if (ret < 0) {
        printk(KERN_ERR "not set rd gpio\n");
        return -1;
    }

    slcd_gpio_send(reg, jzfb.pdata);

    slcd_init_gpio_status(0, jzfb.pdata);
    for (i = 0; i < count; i++)
        buffer[i] = slcd_gpio_receive(jzfb.pdata);

    slcd_reset_gpio_status();

    slcd_send_cmd(jzfb.pdata->slcd.cmd_of_start_frame);

    return 0;
}

int slcd_read_data_only(int reg, int count, char *buffer)
{
    int i, ret;

    if (jzfb.is_enabled) {
        ret = slcd_wait_busy_us(100*1000);
        if (ret) {
            printk(KERN_ERR "slcd is busy\n");
            return -1;
        }
    }

    ret = slcd_init_gpio_status(1, jzfb.pdata);
    if (ret < 0) {
        printk(KERN_ERR "not set rd gpio\n");
        return -1;
    }

    slcd_gpio_send(reg, jzfb.pdata);

    slcd_init_gpio_status(0, jzfb.pdata);
    for (i = 0; i < count; i++)
        buffer[i] = slcd_gpio_receive(jzfb.pdata);

    slcd_reset_gpio_status();

    return 0;
}

static void calculate_spi_mode_delay_time(void)
{
    if (jzfb.pdata->lcd_mode < SLCD_SPI_3LINE) {
        spi_send_delay = 0;
        return;
    }

    int cycle = 0;
    int width = jzfb.pdata->slcd.mcu_data_width;
    int pixclock = jzfb.pdata->pixclock;

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
    if (jzfb.is_enabled++ != 0)
        return;

    init_fbdev_frame_layer();

    unsigned int rate = jzfb.pdata->pixclock;
    if (is_slcd(jzfb.pdata) && !lcd_is_inited) {
        if (jzfb.pdata->slcd.pixclock_when_init)
            rate = jzfb.pdata->slcd.pixclock_when_init;
    }

    if (is_lvds(jzfb.pdata))
        jz_lvds_set_clk(jzfb.pclk);
    else
        clk_set_rate(jzfb.pclk, rate);

    clk_prepare_enable(jzfb.clk);
    clk_prepare_enable(jzfb.pclk);

    calculate_spi_mode_delay_time();

    init_lcdc(jzfb.pdata);

    jzfb.display_state = State_clear;
    jzfb.display_mode = SRDMA_DISPLAY;

    if (jzfb.mixer_enable || jzfb.rot.is_enable) {
        jzfb.mixer_cfg.xres = jzfb.layer_xres;
        jzfb.mixer_cfg.yres = jzfb.layer_yres;
        jzfb.mixer_cfg.format = jzfb.pdata->fb_fmt;
        if (jzfb.rot.is_enable)
            jzfb.mixer_cfg.format = fb_fmt_ROTATE;

        if (jzfb.mixer_enable)
            jzfb.mixer_cfg.dst_mem = jzfb.fbdev[4].fb_mem;
        else if (jzfb.rot.is_enable)
            jzfb.mixer_cfg.dst_mem = jzfb.rot.frame_mem;

        jzfb.mixer = fb_layer_mixer_create(jzfb.dev);
        fb_layer_mixer_set_output_frame(jzfb.mixer, &jzfb.mixer_cfg);

        for(i = 0; i < 4; i++) {
            if (jzfb.fbdev[i].is_enable)
                fb_layer_mixer_set_input_layer(jzfb.mixer, i, &jzfb.fbdev[i].layer_cfg);
        }

        if (jzfb.mixer_enable)
            jzfb.display_mode = MIXER_DISPLAY;
        else if (jzfb.rot.is_enable)
            jzfb.display_mode = ROT_DISPLAY;
    }

    if (jzfb.display_mode == ROT_DISPLAY) {
        dpu_config_rot_ch();
    } else if (jzfb.fbdev[4].is_enable) {
        dpu_config_srdma_ch();
    } else {
        dpu_config_composer_ch();
        jzfb.display_mode = COMPOSER_DISPLAY;
    }

    if (lcd_is_inited)
        return;

    if (jzfb.rot.is_enable)
        enable_rotator();

    jzfb.pdata->power_on(NULL);

    if (is_tft_mipi(jzfb.pdata) || is_slcd_mipi(jzfb.pdata))
        jz_enable_mipi_dsi();
    if (is_lvds(jzfb.pdata))
        jz_enable_lvds();

    if (jzfb.pdata->lcd_init)
        jzfb.pdata->lcd_init();

    process_slcd_data_table(
        jzfb.pdata->slcd_data_table, jzfb.pdata->slcd_data_table_length);

    if (rate != jzfb.pdata->pixclock)
        clk_set_rate(jzfb.pclk, jzfb.pdata->pixclock);

    if (is_slcd(jzfb.pdata)) {
        slcd_send_cmd(jzfb.pdata->slcd.cmd_of_start_frame);
        dpu_set_bit(SLCD_CFG, FMT_EN, 1);
    }

    if (is_tft_mipi(jzfb.pdata))
        jz_dsi_video_cfg();

    if (is_slcd_mipi(jzfb.pdata))
        jz_dsi_command_cfg();

    if (is_lvds(jzfb.pdata))
        lvds_rx_enable();
}

static void disable_fb(void)
{
    unsigned long wait_count = 0;

    if (--jzfb.is_enabled != 0)
        return;

    if (jzfb.display_state == State_display_start)
        wait_event(jzfb.wait_queue, jzfb.display_state == State_display_end);

    if (jzfb.display_mode == ROT_DISPLAY)
        jzfb_quick_stop_rot();
    else if (jzfb.display_mode == COMPOSER_DISPLAY)
        dpu_quick_stop_display();
    else
        dpu_quick_stop_srdma();

    while(dpu_get_bit(ST, WORKING)) {
        usleep_range(100, 100);
        wait_count++;
        if (wait_count >= 300) {
            printk(KERN_ERR "wait dpu work stop timeout!\n");
            break;
        }
    }

    if (is_tft_mipi(jzfb.pdata) || is_slcd_mipi(jzfb.pdata))
        jz_disable_mipi_dsi();

    jzfb.pdata->power_off(NULL);

    clk_disable_unprepare(jzfb.clk);
    clk_disable_unprepare(jzfb.pclk);

    if (jzfb.display_mode == MIXER_DISPLAY || jzfb.display_mode == ROT_DISPLAY)
        fb_layer_mixer_delete(jzfb.mixer);

    if (lcd_is_inited)
        lcd_is_inited = 0;
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

    m_cache_sync(layer, sizeof(*layer));
    m_cache_sync(frame->framedesc, sizeof(*frame->framedesc));
}

static int lcdc_tft_pan_display(struct lcdc_frame *frame, struct fbdev_data *fbdev)
{
    unsigned long flags;

    spin_lock_irqsave(&jzfb.spinlock, flags);
    jzfb.display_state = State_display_start;

    if (jzfb.display_mode == ROT_DISPLAY) {
        rot_write_reg(BUFF_CFG_ADDR, (unsigned long)virt_to_phys(jzfb.mixer_cfg.dst_mem) | 1);
    } else if (jzfb.display_mode != COMPOSER_DISPLAY) {
        dpu_write(SRD_CHAIN_ADDR, virt_to_phys(frame->srdmadesc));
        dpu_start_simple_dma();
    } else {
        dpu_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));
        dpu_start_composer();
    }

    spin_unlock_irqrestore(&jzfb.spinlock, flags);

    wait_event_timeout(jzfb.wait_queue,
    jzfb.display_state == State_display_end, msecs_to_jiffies(300));
    if (jzfb.display_state != State_display_end) {
        printk(KERN_EMERG "jzfb: tft pan display wait timeout %d\n", jzfb.display_state);
        return -1;
    }

    return 0;
}

static int lcdc_slcd_pan_display(struct lcdc_frame *frame, struct fbdev_data *fbdev)
{
    /*等待旧的一帧刷新完成*/
    if (jzfb.display_state != State_clear) {
        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == State_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != State_display_end) {
            printk(KERN_EMERG "jzfb: slcd pan display wait timeout\n");
            return -1;
        }
    }

    slcd_wait_busy_us(10*1000);
    jzfb.display_state = State_display_start;

    if (jzfb.display_mode == ROT_DISPLAY) {
        rot_write_reg(BUFF_CFG_ADDR, (unsigned long)virt_to_phys(jzfb.mixer_cfg.dst_mem) | 1);
    } else if (jzfb.display_mode != COMPOSER_DISPLAY) {
        dpu_write(SRD_CHAIN_ADDR, virt_to_phys(frame->srdmadesc));
        dpu_start_simple_dma();
    } else {
        dpu_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));
        dpu_start_composer();
    }

    return 0;
}

static void lcdc_config_composer_layer(struct fbdev_data *fbdev,  struct lcdc_frame *frame, unsigned int fb_index)
{
    int layer_id = fbdev - jzfb.fbdev;
    fbdev->layer_cfg.rgb.mem = fbdev->user.fb_mem + fb_index * fbdev->user.bytes_per_frame;

    if (!fbdev->is_user_setting)
        lcdc_config_layer(frame, layer_id, &fbdev->layer_cfg);
    else
        lcdc_config_layer(frame, layer_id, &fbdev->user_cfg);
}

static void lcdc_config_mixer_layer(struct fbdev_data *fbdev, struct fb_layer_mixer_dev *mixer, unsigned int fb_index)
{
    int layer_id = fbdev - jzfb.fbdev;
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
    m_cache_sync(frame->srdmadesc, sizeof(struct srdmadesc));
}

static void run_fb_mixer(int index)
{
    struct fbdev_data *fb_srdma = &jzfb.fbdev[4];

    jzfb.mixer_cfg.dst_mem = fb_srdma->user.fb_mem + index * fb_srdma->user.bytes_per_frame;
    fb_layer_mixer_set_output_frame(jzfb.mixer, &jzfb.mixer_cfg);

    fb_layer_mixer_work_out_one_frame(jzfb.mixer);
}

static void run_fb_rot(int index)
{
    jzfb.mixer_cfg.dst_mem = jzfb.rot.frame_mem + index * jzfb.rot.bytes_per_frame;

    fb_layer_mixer_set_output_frame(jzfb.mixer, &jzfb.mixer_cfg);

    fb_layer_mixer_work_out_one_frame(jzfb.mixer);
}

static struct lcdc_frame *get_display_frame(void)
{
    int frame_index;
    struct lcdc_frame *display_frame;

    jzfb.frame_index++;
    frame_index = jzfb.frame_index % 2;
    display_frame = &jzfb.frames[frame_index];

    return display_frame;
}

static int get_wb_index(void)
{
    int index;

    index = jzfb.wb_index % jzfb.wb_frame_count;
    jzfb.wb_index++;

    return index;
}

void lcdc_pan_display(struct fbdev_data *fbdev, unsigned int fb_index)
{
    assert(fbdev);
    assert(fbdev->is_enable);

    if (!fbdev->is_user_setting)
        assert(fb_index < fbdev->frame_count);
    int ret;

    unsigned int wb_index;

    /* 刷新非cacheline对齐的写穿透cache/内存 */
    fast_iob();

    /* 2600 貌似需要刷新cache 才行 */
    void *fb_mem = fbdev->user.fb_mem + fb_index * fbdev->user.bytes_per_frame;
    dma_cache_wback((unsigned long)fb_mem, fbdev->user.bytes_per_frame);

    struct lcdc_frame *ready_frame = &jzfb.frames[2];

    mutex_lock(&jzfb.frames_lock);

    jzfb.display_count++;

    if (jzfb.display_mode == COMPOSER_DISPLAY)
        lcdc_config_composer_layer(fbdev, ready_frame, fb_index);
    else if (jzfb.display_mode != SRDMA_DISPLAY)
        lcdc_config_mixer_layer(fbdev, jzfb.mixer, fb_index);

    mutex_unlock(&jzfb.frames_lock);

    mutex_lock(&jzfb.lock);

    mutex_lock(&jzfb.frames_lock);

    if (!jzfb.display_count) {
        mutex_unlock(&jzfb.lock);
        mutex_unlock(&jzfb.frames_lock);
        return;
    }
    jzfb.display_count = 0;

    struct lcdc_frame *display_frame = get_display_frame();
    *display_frame = *ready_frame;

    rot_reconfig();

    if (jzfb.display_mode == ROT_DISPLAY) {
        wb_index = get_wb_index();
        run_fb_rot(wb_index);
    } else if (jzfb.display_mode == MIXER_DISPLAY) {
        wb_index = get_wb_index();
        run_fb_mixer(wb_index);
        lcdc_config_srdma(&jzfb.fbdev[4], display_frame, wb_index);
    } else if (jzfb.display_mode == SRDMA_DISPLAY) {
        wb_index = fb_index;
        lcdc_config_srdma(&jzfb.fbdev[4], display_frame, wb_index);
    }

    mutex_unlock(&jzfb.frames_lock);

    if (!is_video_mode(jzfb.pdata))
        ret = lcdc_slcd_pan_display(display_frame, fbdev);
    else
        ret = lcdc_tft_pan_display(display_frame, fbdev);

    mutex_unlock(&jzfb.lock);

    if (ret < 0)
        return;

    if (jzfb.pdata->pan_display_cb)
        jzfb.pdata->pan_display_cb();

    if (jzfb.pan_display_sync) {
        wait_event_timeout(jzfb.wait_queue,
            jzfb.display_state == State_display_end, msecs_to_jiffies(300));
        if (jzfb.display_state != State_display_end)
            printk(KERN_EMERG "jzfb: sync lcd pan display wait timeout\n");
    }
}

static irqreturn_t jzfb_irq_handler(int irq, void *data)
{
    unsigned long flags = dpu_read(INT_FLAG);

    if (get_bit_field(&flags, DISP_END)) {
        dpu_write(CLR_ST, bit_field_val(CLR_DISP_END, 1));
        jzfb.display_state = State_display_end;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (get_bit_field(&flags, SRD_END)) {
        dpu_write(CLR_ST, bit_field_val(CLR_SRD_END, 1));
        jzfb.display_state = State_display_end;
        wake_up_all(&jzfb.wait_queue);
        return IRQ_HANDLED;
    }

    if (get_bit_field(&flags, TFT_UNDR)) {
        jzfb.underrun_count++;
        if (!(jzfb.underrun_count % 1000))
            printk(KERN_ERR "err: lcd underrun, tft_underrun_count = %d\n", jzfb.underrun_count);

        dpu_write(CLR_ST, bit_field_val(CLR_TFT_UNDR, 1));
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    struct fbdev_data *fbdev = info->par;
    unsigned long start;
    unsigned long off;
    u32 len;

    if (fbdev->fb->fix.smem_len == 0)
        return -ENOMEM;

    off = vma->vm_pgoff << PAGE_SHIFT;

    len = fbdev->fb->fix.smem_len;
    len = ALIGN(len, PAGE_SIZE);
    if ((vma->vm_end - vma->vm_start + off) > len)
        return -EINVAL;

    start = fbdev->fb->fix.smem_start;
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     * pan_display 模式需要调用pan_display 接口,在那里去fast_iob()
     * 把非cacheline 对齐的内存同步一下
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;

    if (var->xres != fbdev->user.xres ||
        var->yres != fbdev->user.yres)
        return -EINVAL;

    init_var_info(&fbdev->user, var);

    return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
    return jzfb_check_var(&info->var, info);
}

static void lock_frame_and_dev(struct fbdev_data *fbdev)
{
    mutex_lock(&jzfb.frames_lock);
    mutex_lock(&fbdev->dev_lock);
}

static void unlock_frame_and_dev(struct fbdev_data *fbdev)
{
    mutex_unlock(&jzfb.frames_lock);
    mutex_unlock(&fbdev->dev_lock);
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    struct fbdev_data *fbdev = info->par;
    int ret;

    if (fbdev - jzfb.fbdev > 4) {
        printk(KERN_ERR "jzfb: srdma fb not support ioctl\n");
        return -1;
    }

    switch (cmd) {
    case CMD_set_cfg: {
        struct lcdc_layer *cfg = (void *)arg;
        if (!cfg)
            return -EINVAL;
        if (cfg->layer_order > lcdc_layer_3) {
            printk(KERN_ERR "jzfb: invalid order: %d\n", cfg->layer_order);
            return -EINVAL;
        }

        if (cfg->alpha.value >= 256) {
            printk(KERN_ERR "jzfb: invalid alpha: %x\n", cfg->alpha.value);
            return -EINVAL;
        }

        if (cfg->fb_fmt > fb_fmt_yuv422) {
            printk(KERN_ERR "jzfb: invalid fmt: %d\n", cfg->fb_fmt);
            return -EINVAL;
        }

        if (cfg->fb_fmt == fb_fmt_NV12 || cfg->fb_fmt == fb_fmt_NV21) {
            if (cfg->y.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
            if (cfg->uv.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
        } else {
            if (cfg->rgb.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "jzfb: must be phys address\n");
                return -EINVAL;
            }
        }

        if (!cfg->scaling.enable) {
            if (cfg->xres + cfg->xpos > jzfb.layer_xres) {
                printk(KERN_ERR "jzfb: invalid xres: %d %d\n", cfg->xres, cfg->xpos);
                return -EINVAL;
            }
            if (cfg->yres + cfg->ypos > jzfb.layer_yres) {
                printk(KERN_ERR "jzfb: invalid yres: %d %d\n", cfg->yres, cfg->ypos);
                return -EINVAL;
            }
        } else {
            if (cfg->scaling.xres + cfg->xpos > jzfb.layer_xres) {
                printk(KERN_ERR "jzfb: invalid scaling xres: %d %d\n", cfg->scaling.xres, cfg->xpos);
                return -EINVAL;
            }
            if (cfg->scaling.yres + cfg->ypos > jzfb.layer_yres) {
                printk(KERN_ERR "jzfb: invalid scaling yres: %d %d\n", cfg->scaling.yres, cfg->ypos);
                return -EINVAL;
            }
        }

        lock_frame_and_dev(fbdev);
        fbdev->user_cfg = *cfg;
        if (jzfb.use_default_order)
            fbdev->user_cfg.layer_order = (fbdev - jzfb.fbdev) + 2;

        if (cfg->fb_fmt == fb_fmt_NV12 || cfg->fb_fmt == fb_fmt_NV21) {
            fbdev->user_cfg.y.mem = (void *)CKSEG0ADDR(cfg->y.mem);
            fbdev->user_cfg.uv.mem = (void *)CKSEG0ADDR(cfg->uv.mem);
        } else {
            fbdev->user_cfg.rgb.mem = (void *)CKSEG0ADDR(cfg->rgb.mem);
        }
        unlock_frame_and_dev(fbdev);

        return 0;
    }

    case CMD_enable_cfg:
        lock_frame_and_dev(fbdev);
        fbdev->is_user_setting = 1;
        unlock_frame_and_dev(fbdev);
        return 0;

    case CMD_disable_cfg:
        lock_frame_and_dev(fbdev);
        fbdev->is_user_setting = 0;
        unlock_frame_and_dev(fbdev);
        return 0;

    case CMD_read_reg:
        mutex_lock(&jzfb.lock);
        unsigned long *data = (unsigned long *)arg;
        int reg = data[0];
        int count = data[1];
        void *buffer = (void *)data[2];
        ret = slcd_read_data(reg, count, buffer);
        mutex_unlock(&jzfb.lock);
        if (ret < 0)
            return -1;
        return 0;

    default:
        printk("jzfb: not support this cmd: %x\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int jzfb_open(struct fb_info *info, int user)
{
    mutex_lock(&jzfb.lock);
    jzfb.is_open++;
    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_release(struct fb_info *info, int user)
{
    struct fbdev_data *fbdev = info->par;

    mutex_lock(&jzfb.lock);
    jzfb.is_open--;
    fbdev->is_user_setting = 0;
    mutex_unlock(&jzfb.lock);
    return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;

    mutex_lock(&fbdev->dev_lock);
    mutex_lock(&jzfb.lock);

    if (blank_mode == FB_BLANK_UNBLANK) {
        if (!fbdev->is_power_on) {
            enable_fb();
            fbdev->is_power_on = 1;
            fbdev->layer_cfg.layer_enable = 1;
            fbdev->user_cfg.layer_enable = 1;
        }
    } else {
        if (fbdev->is_power_on) {
            disable_fb();
            fbdev->is_power_on = 0;
            fbdev->layer_cfg.layer_enable = 0;
            fbdev->user_cfg.layer_enable = 0;
        }
    }

    mutex_unlock(&jzfb.lock);
    mutex_unlock(&fbdev->dev_lock);
    return 0;
}

static inline int is_bottom(int layer_order)
{
    return layer_order == lcdc_layer_bottom || layer_order == lcdc_layer_3;
}

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct fbdev_data *fbdev = info->par;
    struct fb_mem_info *user = &fbdev->user;
    int next_frm;
    int ret = -EINVAL;

    mutex_lock(&fbdev->dev_lock);

    if (fbdev - jzfb.fbdev > 4) {
        printk(KERN_ERR "jzfb: srdma fb not support pan_display\n");
        return -1;
    }

    if (!fbdev->user.frame_count && !fbdev->is_user_setting)
        goto unlock;

    if (var->xoffset - info->var.xoffset) {
        printk(KERN_ERR "jzfb: No support for X panning for now\n");
        goto unlock;
    }

    next_frm = var->yoffset / var->yres;
    if (next_frm >= user->frame_count && !fbdev->is_user_setting) {
        printk(KERN_ERR "jzfb: yoffset is out of framebuffer: %d\n", var->yoffset);
        goto unlock;
    }

    if (!fbdev->is_power_on) {
        printk(KERN_ERR "jzfb: layer%d is not enabled\n", fbdev - jzfb.fbdev);
        ret = -EBUSY;
        goto unlock;
    }

    if (fbdev->is_user_setting && next_frm) {
        printk(KERN_ERR "jzfb: can't use yoffset, if use cfg\n");
        goto unlock;
    }

    lcdc_pan_display(fbdev, next_frm);
    ret = 0;

unlock:
    mutex_unlock(&fbdev->dev_lock);
    return ret;
}

static struct fb_ops jzfb_ops = {
    .owner = THIS_MODULE,
    .fb_open = jzfb_open,
    .fb_release = jzfb_release,
    .fb_check_var = jzfb_check_var,
    .fb_set_par = jzfb_set_par,
    .fb_blank = jzfb_blank,
    .fb_pan_display = jzfb_pan_display,
    // .fb_fillrect = cfb_fillrect,
    // .fb_copyarea = cfb_copyarea,
    // .fb_imageblit = cfb_imageblit,
    .fb_ioctl = jzfb_ioctl,
    .fb_mmap = jzfb_mmap,
};


static void init_one_fb(struct fbdev_data *fbdev)
{
    struct fb_info *fb;
    struct fb_mem_info *info;
    int ret;

    if (!fbdev->is_enable)
        return;

    fb = framebuffer_alloc(0, jzfb.dev);
    assert(fb);

    fb->par = fbdev;
    fbdev->fb = fb;
    info = &fbdev->user;
    init_video_mode(info, &fbdev->videomode);
    fb_videomode_to_modelist(&fbdev->videomode, 1, &fb->modelist);
    init_fix_info(info, &fb->fix);
    init_var_info(info, &fb->var);
    fb->fbops = &jzfb_ops;
    fb->flags = FBINFO_DEFAULT;
    fb->screen_base = info->fb_mem;
    fb->screen_size = info->bytes_per_frame;
    ret = register_framebuffer(fb);
    assert(!ret);
}

static void release_one_fb(struct fbdev_data *fbdev)
{
    if (!fbdev->is_enable)
        return;

    unregister_framebuffer(fbdev->fb);
    framebuffer_release(fbdev->fb);
}

static void init_fb(void)
{
    init_one_fb(&jzfb.fbdev[0]);
    init_one_fb(&jzfb.fbdev[1]);
    init_one_fb(&jzfb.fbdev[2]);
    init_one_fb(&jzfb.fbdev[3]);
    init_one_fb(&jzfb.fbdev[4]);
}

static void release_fb(void)
{
    release_one_fb(&jzfb.fbdev[0]);
    release_one_fb(&jzfb.fbdev[1]);
    release_one_fb(&jzfb.fbdev[2]);
    release_one_fb(&jzfb.fbdev[3]);
    release_one_fb(&jzfb.fbdev[4]);
}

static void rot_mode_set(void)
{
    if (!rot_angle) {
        jzfb.mixer_enable = 1;
        jzfb.fbdev[4].is_enable = 1;
        jzfb.fbdev[4].frame_count = jzfb.rot.frame_count;
        jzfb.fbdev[4].fb_mem = jzfb.rot.frame_mem;
    } else {
        jzfb.mixer_enable = 0;
        jzfb.fbdev[4].is_enable = 0;
        jzfb.fbdev[4].frame_count = 0;
    }

    jzfb.rot.is_enable = !!rot_angle;
    jzfb.wb_frame_count = jzfb.rot.is_enable ? jzfb.rot.frame_count : jzfb.fbdev[4].frame_count;
}

static void rot_angle_update(void)
{
    if ((rot_angle / 90) == jzfb.rot.angle)
        return;

    jzfb.layer_xres = rot_angle % 180 ? jzfb.pdata->yres : jzfb.pdata->xres;
    jzfb.layer_yres = rot_angle % 180 ? jzfb.pdata->xres : jzfb.pdata->yres;
    jzfb.rot.bytes_per_frame = ALIGN(jzfb.layer_xres * ROT_BPP_Custome_format, ROT_Block_Size) * jzfb.layer_yres;

    if (!rot_angle && !jzfb.fbdev[4].fb) {
        init_fbdev_data(&jzfb.fbdev[4], jzfb.fbdev[4].frame_count);
        init_one_fb(&jzfb.fbdev[4]);
    }

    int i;
    for (i = 0; i < 4; i++) {
        if (!jzfb.fbdev[i].is_enable)
            continue;
        init_fbdev_data(&jzfb.fbdev[i], jzfb.fbdev[i].frame_count);
        init_video_mode(&jzfb.fbdev[i].user, &jzfb.fbdev[i].videomode);
        init_fix_info(&jzfb.fbdev[i].user, &jzfb.fbdev[i].fb->fix);
        init_var_info(&jzfb.fbdev[i].user, &jzfb.fbdev[i].fb->var);
    }
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
            printk(KERN_ERR "jzfb: failed to check: %s\n", #_cond); \
            ret = -1; \
            goto unlock; \
        } \
    } while (0)

static int check_fbdev_user_cfg(struct fbdev_data *fbdev)
{
    if (!fbdev->enable_fbdev_user_cfg)
        return 0;

    if (fbdev->fbdev_user_cfg.scaling_enable) {
        if (fbdev->fbdev_user_cfg.xpos + fbdev->fbdev_user_cfg.scaling_width > jzfb.layer_xres) {
            printk("jzfb: invalid fbdev user scaling_width and xpos: %d, %d\n", fbdev->fbdev_user_cfg.scaling_width, fbdev->fbdev_user_cfg.xpos);
            return -1;
        }

        if (fbdev->fbdev_user_cfg.ypos + fbdev->fbdev_user_cfg.scaling_height > jzfb.layer_yres) {
            printk("jzfb: invalid fbdev user scaling_height and xpos: %d, %d\n", fbdev->fbdev_user_cfg.scaling_height, fbdev->fbdev_user_cfg.ypos);
            return -1;
        }
    } else {
        if (fbdev->fbdev_user_cfg.xpos + fbdev->fbdev_user_cfg.width > jzfb.layer_xres) {
            printk("jzfb: invalid fbdev user width and xpos: %d, %d\n", fbdev->fbdev_user_cfg.width, fbdev->fbdev_user_cfg.xpos);
            return -1;
        }

        if (fbdev->fbdev_user_cfg.ypos + fbdev->fbdev_user_cfg.height > jzfb.layer_yres) {
            printk("jzfb: invalid fbdev user height and xpos: %d, %d\n", fbdev->fbdev_user_cfg.height, fbdev->fbdev_user_cfg.ypos);
            return -1;
        }
    }

    return 0;
}

int jzfb_register_lcd(struct lcdc_data *pdata)
{
    int ret;
    int i;

    mutex_lock(&jzfb.lock);

    error_if(jzfb.pdata != NULL);
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

    if (is_slcd_mipi(pdata) || is_tft_mipi(pdata)) {
        ret = jz_mipi_dsi_data_init(pdata);
        if (ret < 0)
            goto unlock;
    }

    if (is_lvds(pdata))
        jz_lvds_data_init(pdata);

    ret = init_gpio(pdata);
    if (ret)
        goto unlock;

    jzfb.pdata = pdata;

    ret = rot_check_params();
    if (ret)
        goto unlock;

    jzfb.layer_xres = jzfb.rot.angle % 2 ? jzfb.pdata->yres : jzfb.pdata->xres;
    jzfb.layer_yres = jzfb.rot.angle % 2 ? jzfb.pdata->xres : jzfb.pdata->yres;
    jzfb.wb_frame_count = jzfb.rot.is_enable ? jzfb.rot.frame_count : jzfb.fbdev[4].frame_count;

    for (i = 0; i < 4; i++) {
        ret = check_fbdev_user_cfg(&jzfb.fbdev[i]);
        if (ret < 0)
            goto unlock;
    }

    lcdc_alloc_desc();

    if (jzfb.rot.is_enable) {
        init_rotator();
        rot_mode_set();
    }

    lcdc_init_fbdev();

    init_fb();

unlock:
    mutex_unlock(&jzfb.lock);

    return ret;
}

void jzfb_unregister_lcd(struct lcdc_data *pdata)
{
    mutex_lock(&jzfb.lock);

    assert(pdata == jzfb.pdata);

    assert(!jzfb.is_open);

    if (jzfb.is_enabled) {
        jzfb.is_enabled = 1;
        disable_fb();
    }

    free_mem();

    free_rot();

    release_fb();

    jzfb_release_pins();

    jzfb.pdata = NULL;

    mutex_unlock(&jzfb.lock);
}

static ssize_t show_angle(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", rot_angle);
}

static ssize_t set_angle(struct device *dev, struct device_attribute *attr,
              const char *buf, size_t count)
{
    int angle = 0;
    assert(count <= sizeof(int));
    sscanf(buf, "%d", &angle);
    if (angle != 0 && angle != 90 && angle != 180 && angle != 270) {
        printk(KERN_ERR "no support this angle: %d°\n", angle);
        return -EFAULT;
    }

    if (lcd_is_inited && !jzfb.is_enabled) {
        printk(KERN_ERR "please enable first when take over rtos resources\n");
        return -EFAULT;
    }

    mutex_lock(&jzfb.frames_lock);

    rot_angle = angle;
    rot_mode_set();
    rot_angle_update();

    mutex_unlock(&jzfb.frames_lock);

    return count;
}

static DEVICE_ATTR(rotator_angle, S_IRUGO|S_IWUSR, show_angle, set_angle);

static int ingenic_fb_probe(struct platform_device *pdev)
{
    int ret;

    jzfb.dev = &pdev->dev;

    /* srdma 不使能时 layer0 也就是fb0 一定会有
     */
    if(jzfb.fbdev[4].is_enable) {
        if (jzfb.fbdev[4].frame_count < 0) {
            printk(KERN_ERR "jzfb: frame_count invalid: srdma\n");
            return -EINVAL;
        }
    } else {
        jzfb.fbdev[4].frame_count = 0;
        jzfb.fbdev[0].is_enable = 1;
    }

    int i, save_i = 4;
    for (i = 0; i < 4; i++) {
        struct fbdev_data *fbdev = &jzfb.fbdev[i];
        if (!fbdev->is_enable) {
            save_i = i;
            break;
        }
    }

    for (i = save_i + 1; i < 4; i++) {
        struct fbdev_data *fbdev = &jzfb.fbdev[i];
        if (fbdev->is_enable) {
            printk(KERN_ERR "jzfb: can't enable layer%d, because layer%d is disabled\n", i, save_i);
            return -EINVAL;
        }
    }

    for (i = 0; i < 4; i++) {
        struct fbdev_data *fbdev = &jzfb.fbdev[i];
        if (!fbdev->is_enable)
            fbdev->frame_count = 0;

        if (fbdev->frame_count < 0) {
            printk(KERN_ERR "jzfb: frame_count invalid: layer%d\n", i);
            return -EINVAL;
        }

        if (fbdev->alpha >= 256)
            fbdev->alpha = 255;
    }

    mutex_init(&jzfb.lock);
    spin_lock_init(&jzfb.spinlock);
    init_waitqueue_head(&jzfb.wait_queue);
    jzfb.irq = IRQ_INTC_BASE + 31;

    jzfb.clk = clk_get(NULL, "gate_lcd");
    assert(!IS_ERR(jzfb.clk));

    jzfb.pclk = clk_get(NULL, "div_lcd");
    assert(!IS_ERR(jzfb.pclk));

    ret = request_irq(jzfb.irq, jzfb_irq_handler, IRQF_SHARED, "jzfb", &jzfb);
    assert(!ret);

    jz_mipi_dsi_init();

    jzfb.rot_clk = clk_get(NULL, "gate_rot");
    assert(!IS_ERR(jzfb.rot_clk));

    ret = request_irq(IRQ_ROT, rot_irq_handler, IRQF_SHARED, "jzfb_rot", &jzfb);
    assert(!ret);

    if (!jzfb.rot.is_enable)
        return 0;

    // path: /sys/bus/platform/drivers/ingenic-fb/ingenic-fb.0/rotator_angle
    ret = device_create_file(&pdev->dev, &dev_attr_rotator_angle);
    assert(!ret);

    if (lcd_is_inited)
        rot_angle = rot_get_bits(GLB_CFG, ROT_ANGLE) * 90;

    return 0;
}

static int ingenic_fb_remove(struct platform_device *pdev)
{
    assert(!jzfb.pdata);

    jz_mipi_dsi_exit();

    clk_put(jzfb.clk);
    clk_put(jzfb.pclk);
    clk_put(jzfb.rot_clk);

    free_irq(jzfb.irq, &jzfb);

    free_irq(IRQ_ROT, &jzfb);

    if (!jzfb.rot.is_enable)
        return 0;

    device_remove_file(&pdev->dev, &dev_attr_rotator_angle);

    return 0;
}

static struct platform_driver ingenic_fb_driver = {
    .probe = ingenic_fb_probe,
    .remove = ingenic_fb_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-fb",
    },
};

/* stop no dev release warning */
static void jz_fb_dev_release(struct device *dev){}

struct platform_device ingenic_fb_device = {
    .name = "ingenic-fb",
    .dev  = {
        .release = jz_fb_dev_release,
    },
};

static int __init jzfb_module_init(void)
{
    int ret = platform_device_register(&ingenic_fb_device);
    if (ret)
        return ret;

    return platform_driver_register(&ingenic_fb_driver);
}

static void __exit jzfb_module_exit(void)
{
    platform_device_unregister(&ingenic_fb_device);

    platform_driver_unregister(&ingenic_fb_driver);
}

module_init(jzfb_module_init);

module_exit(jzfb_module_exit);

EXPORT_SYMBOL(jzfb_register_lcd);
EXPORT_SYMBOL(jzfb_unregister_lcd);
EXPORT_SYMBOL(slcd_read_data_only);

MODULE_DESCRIPTION("Ingenic Soc FB driver");
MODULE_LICENSE("GPL");
