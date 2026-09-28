#ifndef _JZ_IPU_H_
#define _JZ_IPU_H_

#include <bit_field.h>

struct ipu_param
{
    unsigned int cmd;                   /* IPU command */

    unsigned int bg_w;                  /* background weight */
    unsigned int bg_h;                  /* background hight */
    unsigned int bg_fmt;                /* background format */
    unsigned int bg_buf_p;              /* background buffer physical addr */

    unsigned int out_fmt;               /* out format */

    unsigned int osd_ch0_fmt;
    unsigned int osd_ch0_para;
    unsigned int osd_ch0_bak_argb;
    unsigned int osd_ch0_pos_x;
    unsigned int osd_ch0_pos_y;
    unsigned int osd_ch0_src_w;
    unsigned int osd_ch0_src_h;
    unsigned int osd_ch0_buf_p;


    unsigned int osd_ch1_fmt;
    unsigned int osd_ch1_para;
    unsigned int osd_ch1_bak_argb;
    unsigned int osd_ch1_pos_x;
    unsigned int osd_ch1_pos_y;
    unsigned int osd_ch1_src_w;
    unsigned int osd_ch1_src_h;
    unsigned int osd_ch1_buf_p;


    unsigned int osd_ch2_fmt;
    unsigned int osd_ch2_para;
    unsigned int osd_ch2_bak_argb;
    unsigned int osd_ch2_pos_x;
    unsigned int osd_ch2_pos_y;
    unsigned int osd_ch2_src_w;
    unsigned int osd_ch2_src_h;
    unsigned int osd_ch2_buf_p;

    unsigned int osd_ch3_fmt;
    unsigned int osd_ch3_para;
    unsigned int osd_ch3_bak_argb;
    unsigned int osd_ch3_pos_x;
    unsigned int osd_ch3_pos_y;
    unsigned int osd_ch3_src_w;
    unsigned int osd_ch3_src_h;
    unsigned int osd_ch3_buf_p;

};

enum IPU_CMD_OSD_CH{
    IPU_CMD_OSD_CH0,
    IPU_CMD_OSD_CH1,
    IPU_CMD_OSD_CH2,
    IPU_CMD_OSD_CH3,
};

#define IPU_OSD_CH_INDEX_MIN            (IPU_CMD_OSD_CH0)
#define IPU_OSD_CH_INDEX_MAX            (IPU_CMD_OSD_CH3)

#define IPU_CMD_CSC                     (1<<4)
#define IPU_CMD_OSD                     (0xF<<0)

/* define for isrgb */
#define IPU_PIXEL_RGB_16                (2)
#define IPU_PIXEL_RGB_32                (1)
#define IPU_PIXEL_NO_RGB                (0)
#define IPU_PIXEL_ERROR                 (-1)


#ifdef __KERNEL__
/*
 * IPU driver's native data
 */

struct ipu_reg_struct {
    char *name;
    unsigned int addr;
};

struct ipu_buf_info {
    void *vaddr_alloc;
    void *paddr;
    void *paddr_align;
    unsigned int size;
};

struct jz_ipu {

    int irq;
    char name[16];

    struct clk *clk;
    struct clk *ahb0_gate;
    void __iomem *iomem;
    struct device *dev;
    struct resource *res;
    struct miscdevice misc_dev;

    struct mutex mutex;
    struct completion done_ipu;
    struct completion done_buf;
    struct ipu_buf_info pbuf;
};

struct ipu_flush_cache_para
{
    void *addr;
    unsigned int size;
};

#define JZIPU_IOC_MAGIC                 'I'
#define IOCTL_IPU_START                 _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_RES_PBUFF             _IO(JZIPU_IOC_MAGIC, 114)
#define IOCTL_IPU_GET_PBUFF             _IO(JZIPU_IOC_MAGIC, 115)
#define IOCTL_IPU_BUF_LOCK              _IO(JZIPU_IOC_MAGIC, 116)
#define IOCTL_IPU_BUF_UNLOCK            _IO(JZIPU_IOC_MAGIC, 117)
#define IOCTL_IPU_BUF_FLUSH_CACHE       _IO(JZIPU_IOC_MAGIC, 118)

#define IPU_IOBASE                      (0x13080000)

static const unsigned long ipu_iobase[] = {
        KSEG1ADDR(IPU_IOBASE),
};

#define IPU_ADDR(reg)                   ((volatile unsigned long *)((ipu_iobase[0]) + (reg)))

static inline unsigned int ipu_reg_read(int offset)
{
    return *IPU_ADDR(offset);
}

static inline void ipu_reg_write(int offset, unsigned int val)
{
    *IPU_ADDR(offset) = val;
}

static inline void ipu_set_bit(unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(IPU_ADDR(reg), start, end, val);
}

static inline unsigned int ipu_get_bit(unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(IPU_ADDR(reg), start, end);
}

#endif    /* #ifdef __KERNEL__ */

#endif /* _JZ_IPU_H_ */
