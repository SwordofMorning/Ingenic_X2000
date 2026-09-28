/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI controller
 *
 */

#include <linux/sched.h>
#include <bit_field.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <common.h>
#include "csi.h"


struct jz_mipi_csi_drv {
    int id;
    int is_enable;
    const char *name;
    const char *csi_clk_name;
    struct clk *csi_gate_clk;
};

/*
 * MIPI CSI information
 */
static struct jz_mipi_csi_drv mipi_csi_dev = {
    .id                     = 0,
    .is_enable              = 0,
    .name                   = "csi",
    .csi_clk_name           = "gate_csi",
};

DEFINE_SPINLOCK(csi_reset_lock);


/*
 * MIPI CSI operation
 */
#define MIPI_CSI_ADDR(reg)          ((volatile unsigned long *)(KSEG1ADDR(MIPI_CSI_IOBASE) + (reg)))

static inline unsigned int mipi_csi_read_reg(unsigned int reg)
{
    return *MIPI_CSI_ADDR(reg);
}

static inline void mipi_csi_write_reg(unsigned int reg, unsigned int val)
{
    *MIPI_CSI_ADDR(reg) = val;
}

static inline unsigned int mipi_csi_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(MIPI_CSI_ADDR(reg), start, end);
}

static inline void mipi_csi_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_CSI_ADDR(reg), start, end, val);
}

static inline void mipi_csi_dphy_test_clear(int value)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclr, value);
}

static inline void mipi_csi_dphy_test_clock(int value)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclk, value);
}

static inline void mipi_csi_dphy_test_en(unsigned char on_falling_edge)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL1, MIPI_PHY_test_ctrl1_testen, on_falling_edge);
}

static inline void mipi_csi_dphy_test_data_in(unsigned char data)
{
    mipi_csi_write_reg(MIPI_PHY_TST_CTRL1, data);
}

static inline void mipi_csi_dphy_set_lanes(int lanes)
{
    mipi_csi_set_bit(MIPI_N_LANES, MIPI_PHY_n_lanes, lanes - 1);
}

static inline void mipi_csi_dphy_phy_shutdown(int enable)
{
    mipi_csi_set_bit(MIPI_PHY_SHUTDOWNZ, MIPI_PHY_phy_shutdown, enable);
}

static inline void mipi_csi_dphy_dphy_reset(int state)
{
    mipi_csi_set_bit(MIPI_DPHY_RSTZ, MIPI_PHY_dphy_reset, state);
}

static inline void mipi_csi_dphy_csi2_reset(int state)
{
    mipi_csi_set_bit(MIPI_CSI2_RESETN, MIPI_PHY_csi2_reset, state);
}

static inline int mipi_csi_dphy_get_state(void)
{
    return mipi_csi_read_reg(MIPI_PHY_STATE);
}

/*
 * MIPI PHY (Only One PHY)
 */
#define MIPI_PHY_ADDR(reg)              ((volatile unsigned long *)CKSEG1ADDR(MIPI_PHY_IOBASE + (reg)))

static inline unsigned int mipi_phy_read_reg(unsigned int reg)
{
    return *MIPI_PHY_ADDR(reg);
}

static inline void mipi_phy_write_reg(unsigned int reg, unsigned int val)
{
    *MIPI_PHY_ADDR(reg) = val;
}

static inline void mipi_phy_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_PHY_ADDR(reg), start, end, val);
}

static inline void mipi_phy_set_clk_settle(int value)
{
    mipi_phy_set_bit(PHY_CK_SETTLE, MIPI_PHY_clk_settle, value);
}

static inline void mipi_phy_set_data0_settle(int value)
{
    mipi_phy_set_bit(PHY_DATA0_SETTLE, MIPI_PHY_data0_settle, value);
}

static inline void mipi_phy_set_data1_settle(int value)
{
    mipi_phy_set_bit(PHY_DATA1_SETTLE, MIPI_PHY_data1_settle, value);
}

static inline void mipi_csi_dphy_dump_reg(void)
{
    printk("================ dump mipi csi reg ================\n");
    printk("VERSION             : 0x%08x\n", mipi_csi_read_reg(MIPI_VERSION));
    printk("N_LANES             : 0x%08x\n", mipi_csi_read_reg(MIPI_N_LANES));
    printk("PHY_SHUTDOWNZ       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_SHUTDOWNZ));
    printk("DPHY_RSTZ           : 0x%08x\n", mipi_csi_read_reg(MIPI_DPHY_RSTZ));
    printk("CSI2_RESETN         : 0x%08x\n", mipi_csi_read_reg(MIPI_CSI2_RESETN));
    printk("PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_STATE));
    printk("DATA_IDS_1          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_1));
    printk("DATA_IDS_2          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_2));
    printk("ERR1                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR1));
    printk("ERR2                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR2));
    printk("MASK1               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK1));
    printk("MASK2               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK2));
    printk("PHY_TST_CTRL0       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL0));
    printk("PHY_TST_CTRL1       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL1));
    printk("VC0_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC0_FRAME_NUM));
    printk("VC1_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC1_FRAME_NUM));
    printk("VC2_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC2_FRAME_NUM));
    printk("VC3_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC3_FRAME_NUM));
}

#ifdef SOC_CAMERA_DEBUG
int dsysfs_mipi_dump_reg(char *buf)
{
    char *p = buf;

    p += sprintf(p, "\t VERSION             : 0x%08x\n", mipi_csi_read_reg(MIPI_VERSION));
    p += sprintf(p, "\t N_LANES             : 0x%08x\n", mipi_csi_read_reg(MIPI_N_LANES));
    p += sprintf(p, "\t PHY_SHUTDOWNZ       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_SHUTDOWNZ));
    p += sprintf(p, "\t DPHY_RSTZ           : 0x%08x\n", mipi_csi_read_reg(MIPI_DPHY_RSTZ));
    p += sprintf(p, "\t CSI2_RESETN         : 0x%08x\n", mipi_csi_read_reg(MIPI_CSI2_RESETN));
    p += sprintf(p, "\t PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_STATE));
    p += sprintf(p, "\t DATA_IDS_1          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_1));
    p += sprintf(p, "\t DATA_IDS_2          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_2));
    p += sprintf(p, "\t ERR1                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR1));
    p += sprintf(p, "\t ERR2                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR2));
    p += sprintf(p, "\t MASK1               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK1));
    p += sprintf(p, "\t MASK2               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK2));
    p += sprintf(p, "\t PHY_TST_CTRL0       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL0));
    p += sprintf(p, "\t PHY_TST_CTRL1       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL1));

    p += sprintf(p, "\t VC0_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC0_FRAME_NUM));
    p += sprintf(p, "\t VC1_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC1_FRAME_NUM));
    p += sprintf(p, "\t VC2_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC2_FRAME_NUM));
    p += sprintf(p, "\t VC3_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(MIPI_VC3_FRAME_NUM));

    return p - buf;
}
#endif

static unsigned char mipi_csi_event_disable(unsigned int  mask, unsigned char err_reg_no)
{
    switch (err_reg_no) {
    case CSI_ERR_MASK_REGISTER1:
        mipi_csi_write_reg(MIPI_MASK1, mask | mipi_csi_read_reg(MIPI_MASK1));
        break;
    case CSI_ERR_MASK_REGISTER2:
        mipi_csi_write_reg(MIPI_MASK2, mask | mipi_csi_read_reg(MIPI_MASK2));
        break;
    default:
        return -EINVAL;
    }

    return 0;
}


static void mipi_dphy_phy_settle_time(int lanes, int clk)
{
    int settle_value = clk * 115 / 2000 + 4; /* spec maybe is err */
    settle_value = settle_value < 0 ? 0 : settle_value;
    settle_value = settle_value > 255 ? 255 : settle_value;

    mipi_phy_set_clk_settle(settle_value);
    mipi_phy_set_data0_settle(settle_value);
    mipi_phy_set_data1_settle(settle_value);
}

static int mipi_csi_gate_clock_enable(void)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev;

    if (!drv->csi_gate_clk) {
        drv->csi_gate_clk = clk_get(NULL, drv->csi_clk_name);
        assert(!IS_ERR(drv->csi_gate_clk));
        assert(!clk_prepare(drv->csi_gate_clk));
    }

    clk_enable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_gate_clock_disable(void)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev;

    assert(!IS_ERR(drv->csi_gate_clk));

    clk_disable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_phy_ready(int lanes)
{
    int ready;
    int ret = 0;

    ready = mipi_csi_dphy_get_state();
    ret = ready & (1 << MIPI_PHY_STATE_clk_lane_stop);

    switch (lanes) {
    case 2:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_data_lane1_stop));
    case 1:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_data_lane0_stop));
        break;
    default:
        printk(KERN_ERR "Do not support lane num %d!\n", lanes);
        ret = -EINVAL;
        break;
    }

    return !!ret;
}

static int mipi_csi_phy_configure(struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    unsigned long flags;
    spin_lock_irqsave(&csi_reset_lock, flags);

    int lanes = mipi_info->lanes;

    /* 参数检查 */
    if (mipi_csi_dev.is_enable) {
        printk(KERN_ERR "csi is already used by other controller, please check config paramer!\n");
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    if (lanes > 2) {
        printk(KERN_ERR "csi lane num %d must less the 2!\n", lanes);
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    mipi_csi_gate_clock_enable();

    /*
     * Reset PHY CSI
     */
    mipi_csi_dphy_phy_shutdown(0);
    mipi_csi_dphy_dphy_reset(0);
    mipi_csi_dphy_csi2_reset(0);

    udelay(1000);
    mipi_csi_set_bit(RXVALID_MASK, 0, 1, 0x3);
    mipi_csi_dphy_phy_shutdown(1);
    mipi_csi_dphy_dphy_reset(1);
    mipi_csi_dphy_csi2_reset(1);

    udelay(1000);

    mipi_phy_write_reg(PHY_ENB, 0x7d);
    mipi_phy_write_reg(PHY_CK_CONTI, 0x3f);
    mipi_phy_write_reg(PHY_DATA0_CONTI, 0x3f);
    mipi_phy_write_reg(PHY_DATA1_CONTI, 0x3f);

    mipi_dphy_phy_settle_time(lanes, mipi_info->clk);

    mipi_csi_dphy_test_clear(1);
    udelay(100);

    mipi_csi_dphy_set_lanes(lanes);

    /* MASK all interrupts */
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER1);
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER2);

    mipi_csi_dev.is_enable = 1;

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);
    return ret;
}

int mipi_csi_phy_stop(void)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&csi_reset_lock, flags);

    if (!mipi_csi_dev.is_enable) {
        printk(KERN_ERR "csi is not enabled, please enable first!\n");
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    mipi_csi_dev.is_enable = 0;

    mipi_csi_dphy_csi2_reset(0);
    mipi_csi_dphy_dphy_reset(0);
    mipi_csi_dphy_phy_shutdown(0);

    mipi_csi_gate_clock_disable();

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);

    return ret;
}

int mipi_csi_phy_initialization(struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    int csi_lanes = mipi_info->lanes;

    ret = mipi_csi_phy_configure(mipi_info);
    if  (ret < 0) {
        printk(KERN_ERR "mipi csi configure failed\n");
        return -EINVAL;
    }

    /*
     * 检查MIPI CLK DATA的状态是否为停止状态
     */
    int i;
    int retries = 30;
    for (i = 0; i < retries; i++) {
        if (mipi_csi_phy_ready(csi_lanes))
            break;

        udelay(2);
    }

    if (i >= retries) {
        /* 非错误,待确定是否一定需进入stop状态(LP11), 现以警告的形式给予提示 */
        printk("warning: mipi csi clk/data lanes NOT in stop state\n");
        printk("VERSION             : 0x%08x\n", mipi_csi_read_reg(MIPI_VERSION));
        printk("N_LANES             : 0x%08x\n", mipi_csi_read_reg(MIPI_N_LANES));
        printk("PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_STATE));
        ret = EIO;
    }

    return ret;
}
