#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>

#include <bit_field.h>

#include "jz_mipi_dsi_reg.h"
#include "jz_mipi_dsi.h"

#define MIPI_DSI_IOBASE     0x10023000
#define MIPI_DSI_PHY_IOBASE 0x10024000
#define MIPI_DSI_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(MIPI_DSI_IOBASE + reg))
#define MIPI_DSI_PHY_ADDR(reg) ((volatile unsigned long *)CKSEG1ADDR(MIPI_DSI_PHY_IOBASE + reg))

static void dsi_phy_write(unsigned int reg, unsigned int val)
{
    *MIPI_DSI_PHY_ADDR(reg) = val;
}

static inline unsigned int dsi_phy_read(unsigned int reg)
{
    return *MIPI_DSI_PHY_ADDR(reg);
}

static void dsi_write(unsigned int reg, unsigned int val)
{
    *MIPI_DSI_ADDR(reg) = val;
}

static inline unsigned int dsi_read(unsigned int reg)
{
    return *MIPI_DSI_ADDR(reg);
}

static inline void dsi_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_DSI_ADDR(reg), start, end, val);
}

static inline unsigned int dsi_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(MIPI_DSI_ADDR(reg), start, end);
}

static void dsi_set_power(unsigned int power)
{
    dsi_set_bit(R_DSI_HOST_PWR_UP, SHUTDOWNZ, power);
}

static int dsi_set_dphy_hs2lp_time(unsigned char time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, PHY_HS2LP_TIME, time);

    return 0;
}

static int dsi_set_dphy_lp2hs_time(unsigned char time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, PHY_LP2HS_TIME, time);

    return 0;
}

void dump_dsi_reg(void)
{
    printk("===========>dump dsi reg\n");
    printk("VERSION--------------:%08x\n", dsi_read(R_DSI_HOST_VERSION));
    printk("PWR_UP:--------------:%08x\n", dsi_read(R_DSI_HOST_PWR_UP));
    printk("CLKMGR_CFG-----------:%08x\n", dsi_read(R_DSI_HOST_CLKMGR_CFG));
    printk("DPI_VCID-------------:%08x\n", dsi_read(R_DSI_HOST_DPI_VCID));
    printk("DPI_COLOR_CODING-----:%08x\n", dsi_read(R_DSI_HOST_DPI_COLOR_CODING));
    printk("DPI_CFG_POL----------:%08x\n", dsi_read(R_DSI_HOST_DPI_CFG_POL));
    printk("DPI_LP_CMD_TIM-------:%08x\n", dsi_read(R_DSI_HOST_DPI_LP_CMD_TIM));
    printk("DBI_VCID-------------:%08x\n", dsi_read(R_DSI_HOST_DBI_VCID));
    printk("DBI_CFG--------------:%08x\n", dsi_read(R_DSI_HOST_DBI_CFG));
    printk("DBI_PARTITIONING_EN--:%08x\n", dsi_read(R_DSI_HOST_DBI_PARTITIONING_EN));
    printk("DBI_CMDSIZE----------:%08x\n", dsi_read(R_DSI_HOST_DBI_CMDSIZE));
    printk("PCKHDL_CFG-----------:%08x\n", dsi_read(R_DSI_HOST_PCKHDL_CFG));
    printk("GEN_VCID-------------:%08x\n", dsi_read(R_DSI_HOST_GEN_VCID));
    printk("MODE_CFG-------------:%08x\n", dsi_read(R_DSI_HOST_MODE_CFG));
    printk("VID_MODE_CFG---------:%08x\n", dsi_read(R_DSI_HOST_VID_MODE_CFG));
    printk("VID_PKT_SIZE---------:%08x\n", dsi_read(R_DSI_HOST_VID_PKT_SIZE));
    printk("VID_NUM_CHUNKS-------:%08x\n", dsi_read(R_DSI_HOST_VID_NUM_CHUNKS));
    printk("VID_NULL_SIZE--------:%08x\n", dsi_read(R_DSI_HOST_VID_NULL_SIZE));
    printk("VID_HSA_TIME---------:%08x\n", dsi_read(R_DSI_HOST_VID_HSA_TIME));
    printk("VID_HBP_TIME---------:%08x\n", dsi_read(R_DSI_HOST_VID_HBP_TIME));
    printk("VID_HLINE_TIME-------:%08x\n", dsi_read(R_DSI_HOST_VID_HLINE_TIME));
    printk("VID_VSA_LINES--------:%08x\n", dsi_read(R_DSI_HOST_VID_VSA_LINES));
    printk("VID_VBP_LINES--------:%08x\n", dsi_read(R_DSI_HOST_VID_VBP_LINES));
    printk("VID_VFP_LINES--------:%08x\n", dsi_read(R_DSI_HOST_VID_VFP_LINES));
    printk("VID_VACTIVE_LINES----:%08x\n", dsi_read(R_DSI_HOST_VID_VACTIVE_LINES));
    printk("EDPI_CMD_SIZE--------:%08x\n", dsi_read(R_DSI_HOST_EDPI_CMD_SIZE));
    printk("CMD_MODE_CFG---------:%08x\n", dsi_read(R_DSI_HOST_CMD_MODE_CFG));
    printk("GEN_HDR--------------:%08x\n", dsi_read(R_DSI_HOST_GEN_HDR));
    printk("GEN_PLD_DATA---------:%08x\n", dsi_read(R_DSI_HOST_GEN_PLD_DATA));
    printk("CMD_PKT_STATUS-------:%08x\n", dsi_read(R_DSI_HOST_CMD_PKT_STATUS));
    printk("TO_CNT_CFG-----------:%08x\n", dsi_read(R_DSI_HOST_TO_CNT_CFG));
    printk("HS_RD_TO_CNT---------:%08x\n", dsi_read(R_DSI_HOST_HS_RD_TO_CNT));
    printk("LP_RD_TO_CNT---------:%08x\n", dsi_read(R_DSI_HOST_LP_RD_TO_CNT));
    printk("HS_WR_TO_CNT---------:%08x\n", dsi_read(R_DSI_HOST_HS_WR_TO_CNT));
    printk("LP_WR_TO_CNT_CFG-----:%08x\n", dsi_read(R_DSI_HOST_LP_WR_TO_CNT));
    printk("BTA_TO_CNT-----------:%08x\n", dsi_read(R_DSI_HOST_BTA_TO_CNT));
    printk("SDF_3D---------------:%08x\n", dsi_read(R_DSI_HOST_SDF_3D));
    printk("LPCLK_CTRL-----------:%08x\n", dsi_read(R_DSI_HOST_LPCLK_CTRL));
    printk("PHY_TMR_LPCLK_CFG----:%08x\n", dsi_read(R_DSI_HOST_PHY_TMR_LPCLK_CFG));
    printk("PHY_TMR_CFG----------:%08x\n", dsi_read(R_DSI_HOST_PHY_TMR_CFG));
    printk("PHY_RSTZ-------------:%08x\n", dsi_read(R_DSI_HOST_PHY_RSTZ));
    printk("PHY_IF_CFG-----------:%08x\n", dsi_read(R_DSI_HOST_PHY_IF_CFG));
    printk("PHY_ULPS_CTRL--------:%08x\n", dsi_read(R_DSI_HOST_PHY_ULPS_CTRL));
    printk("PHY_TX_TRIGGERS------:%08x\n", dsi_read(R_DSI_HOST_PHY_TX_TRIGGERS));
    printk("PHY_STATUS-----------:%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));
    printk("PHY_TST_CTRL0--------:%08x\n", dsi_read(R_DSI_HOST_PHY_TST_CTRL0));
    printk("PHY_TST_CTRL1--------:%08x\n", dsi_read(R_DSI_HOST_PHY_TST_CTRL1));
    printk("INT_ST0--------------:%08x\n", dsi_read(R_DSI_HOST_INT_ST0));
    printk("INT_ST1--------------:%08x\n", dsi_read(R_DSI_HOST_INT_ST1));
    printk("INT_MSK0-------------:%08x\n", dsi_read(R_DSI_HOST_INT_MSK0));
    printk("INT_MSK1-------------:%08x\n", dsi_read(R_DSI_HOST_INT_MSK1));
    printk("INT_FORCE0-----------:%08x\n", dsi_read(R_DSI_HOST_INT_FORCE0));
    printk("INT_FORCE1-----------:%08x\n", dsi_read(R_DSI_HOST_INT_FORCE1));
    printk("VID_SHADOW_CTRL------:%08x\n", dsi_read(R_DSI_HOST_VID_SHADOW_CTRL));
    printk("DPI_VCID_ACT---------:%08x\n", dsi_read(R_DSI_HOST_DPI_VCID_ACT));
    printk("DPI_COLOR_CODING_AC--:%08x\n", dsi_read(R_DSI_HOST_DPI_COLOR_CODING_ACT));
    printk("DPI_LP_CMD_TIM_ACT---:%08x\n", dsi_read(R_DSI_HOST_DPI_LP_CMD_TIM_ACT));
    printk("VID_MODE_CFG_ACT-----:%08x\n", dsi_read(R_DSI_HOST_VID_MODE_CFG_ACT));
    printk("VID_PKT_SIZE_ACT-----:%08x\n", dsi_read(R_DSI_HOST_VID_PKT_SIZE_ACT));
    printk("VID_NUM_CHUNKS_ACT---:%08x\n", dsi_read(R_DSI_HOST_VID_NUM_CHUNKS_ACT));
    printk("VID_HSA_TIME_ACT-----:%08x\n", dsi_read(R_DSI_HOST_VID_HSA_TIME_ACT));
    printk("VID_HBP_TIME_ACT-----:%08x\n", dsi_read(R_DSI_HOST_VID_HBP_TIME_ACT));
    printk("VID_HLINE_TIME_ACT---:%08x\n", dsi_read(R_DSI_HOST_VID_HLINE_TIME_ACT));
    printk("VID_VSA_LINES_ACT----:%08x\n", dsi_read(R_DSI_HOST_VID_VSA_LINES_ACT));
    printk("VID_VBP_LINES_ACT----:%08x\n", dsi_read(R_DSI_HOST_VID_VBP_LINES_ACT));
    printk("VID_VFP_LINES_ACT----:%08x\n", dsi_read(R_DSI_HOST_VID_VFP_LINES_ACT));
    printk("VID_VACTIVE_LINES_ACT:%08x\n", dsi_read(R_DSI_HOST_VID_VACTIVE_LINES_ACT));
    printk("SDF_3D_ACT-----------:%08x\n", dsi_read(R_DSI_HOST_SDF_3D_ACT));
}

static void dsi_set_dphy_bta_time(unsigned short time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, MAX_RD_TIME, time);
}

static int dsi_get_cmd_full(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_CMD_FULL);
}

static int dsi_get_pld_w_full(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_PLD_W_FUL);
}

static int dsi_get_rd_cmd_busy(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_RD_CMD_BUSY);
}

static int dsi_get_rd_empty(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_PLD_R_EMPTY);
}

static int dsi_wait_pld_w_not_full(int count)
{
    int status;
    status = dsi_get_pld_w_full();

    while (count-- && status) {
        status = dsi_get_pld_w_full();
    }

    return status;
}

static int dsi_wait_cmd_not_full(int count)
{
    int status;
    status = dsi_get_cmd_full();

    while (count-- && status) {
        status = dsi_get_cmd_full();
    }

    return status;
}

static int dsi_wait_rd_cmd_busy(int count)
{
    int status;
    status = dsi_get_rd_cmd_busy();

    while (count-- && status) {
        status = dsi_get_rd_cmd_busy();
    }

    return status;
}

static int dsi_wait_rd_fifo_empty(int count)
{
    int status;
    status = dsi_get_rd_empty();

    while (count-- && status) {
        status = dsi_get_rd_empty();
    }

    return status;
}

static void dsi_set_transfer_mode(int mode)
{
    unsigned long cmd_mode_cfg = dsi_read(R_DSI_HOST_CMD_MODE_CFG);
    set_bit_field(&cmd_mode_cfg, MAX_RD_PKT_SIZE, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SW_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SW_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SR_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_LW_TX, mode);

    set_bit_field(&cmd_mode_cfg, GEN_SW_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SW_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SW_2P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_LW_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_2P_TX, mode);

    dsi_write(R_DSI_HOST_CMD_MODE_CFG, cmd_mode_cfg);
}

static void dsi_set_cmd_mode(void)
{
    dsi_set_bit(R_DSI_HOST_MODE_CFG, CMD_VIDEO_MODE, 1);
}

static void dsi_set_video_mode(void)
{
    dsi_set_bit(R_DSI_HOST_MODE_CFG, CMD_VIDEO_MODE, 0);
}

static void dsi_set_edpi_cmd_size(unsigned short size)
{
    dsi_set_bit(R_DSI_HOST_EDPI_CMD_SIZE, EDPI_ALLOWED_CMD_SIZE, size);
}

void lvds_rx_enable(void)
{
    unsigned int temp = 0;
    usleep_range(5000, 5100); // wait
    temp = dsi_phy_read(DSI_PHY_LVDS_REG00);
    temp |= (1 << 3); // enable lvds rx
    dsi_phy_write(DSI_PHY_LVDS_REG00, temp);
    pr_debug("DSI_PHY_LVDS_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG00));
}

struct ad100_lvds_pclk_param {
    unsigned int pclk_freq;
    unsigned int pll_freq;
};

struct ad100_lvds_pclk_param pclk_epll_tab[] = {
    {30000000, 420000000},
    {31000000, 434000000},
    {31500000, 441000000},
    {32000000, 448000000},
    {33000000, 462000000},
    {34000000, 476000000},
    {34500000, 483000000},
    {35000000, 490000000},
    {36000000, 504000000},
    {37000000, 518000000},
    {37500000, 525000000},
    {38000000, 532000000},
    {39000000, 546000000},
    {40500000, 567000000},
    {42000000, 588000000},
    {43500000, 609000000},
    {45000000, 630000000},
    {46500000, 651000000},
    {48000000, 672000000},
    {49500000, 693000000},
    {50000000, 700000000},
    {51000000, 714000000},
    {52000000, 728000000},
    {53000000, 742000000},
    {54000000, 756000000},
    {56000000, 784000000},
    {57000000, 798000000},
    {58000000, 812000000},
    {60000000, 840000000},
    {62000000, 868000000},
    {63000000, 882000000},
    {64000000, 896000000},
    {66000000, 924000000},
    {68000000, 952000000},
    {69000000, 966000000},
    {70000000, 980000000},
    {72000000, 1008000000},
    {74000000, 1036000000},
    {75000000, 1050000000},
    {76000000, 1064000000},
    {78000000, 1092000000},
    {80000000, 1120000000},
    {81000000, 1134000000},
    {82000000, 1148000000},
    {84000000, 1176000000},
    {86000000, 1204000000},
    {87000000, 1218000000},
    {88000000, 1232000000},
    {90000000, 1260000000},
};

static void jz_lvds_set_rate(struct clk *pclk, struct video_config *video_config)
{
    unsigned int pclk_rate = video_config->pixel_clock * 1000;
    int min_diff = 10000000;//初始化最小差值为10M
    int index = -1;
    int i = 0;
    unsigned int param_num = sizeof(pclk_epll_tab) / sizeof(struct ad100_lvds_pclk_param);

    for (i = 0; i < param_num; i++) {
        unsigned int diff = abs(pclk_rate - pclk_epll_tab[i].pclk_freq);
        if (diff < min_diff) {
            min_diff = diff;
            index = i;
        }
    }
    if(index == -1)
        printk("ERROR: cannot get pclk epll param \n");

    unsigned long parent_rate = pclk_epll_tab[index].pll_freq;
    struct clk *parent_clk = clk_get(NULL, "epll");
    struct clk *mux_pclk = clk_get(NULL, "mux_lcd");

    clk_set_rate(parent_clk, parent_rate);
    clk_set_parent(mux_pclk, parent_clk);
    clk_set_rate(pclk, pclk_rate);
}

struct lvds_pll_param {
    unsigned int pll_out_freq; //KHz
    unsigned short postdiv;
    unsigned short prediv;
    unsigned int fbdiv;
};

struct lvds_pll_param lvds_pll_param_tab[] = {
    {30000000, 1, 1, 70},
    {31000000, 3, 1, 217},
    {31500000, 2, 1, 147},
    {32000000, 3, 1, 224},
    {33000000, 1, 1, 77},
    {34000000, 3, 1, 238},
    {34500000, 2, 1, 161},
    {35000000, 3, 1, 245},
    {36000000, 1, 1, 84},
    {37000000, 3, 1, 259},
    {37500000, 2, 1, 175},
    {38000000, 3, 1, 266},
    {39000000, 1, 1, 91},
    {40500000, 2, 1, 189},
    {42000000, 1, 1, 98},
    {43500000, 2, 1, 203},
    {45000000, 1, 1, 105},
    {46500000, 2, 1, 217},
    {48000000, 1, 1, 112},
    {49500000, 2, 1, 231},
    {51000000, 1, 1, 119},
    {52000000, 3, 1, 364},
    {53000000, 3, 1, 371},
    {54000000, 1, 1, 126},
    {56000000, 3, 1, 392},
    {57000000, 3, 1, 399},
    {58000000, 3, 1, 406},
    {57000000, 1, 1, 133},
    {60000000, 1, 1, 140},
    {62000000, 3, 1, 434},
    {63000000, 1, 1, 147},
    {64000000, 3, 1, 448},
    {66000000, 1, 1, 154},
    {68000000, 3, 1, 476},
    {69000000, 1, 1, 161},
    {70000000, 3, 1, 490},
    {72000000, 1, 1, 168},
    {74000000, 3, 1, 518},
    {75000000, 1, 1, 175},
    {76000000, 3, 1, 532},
    {78000000, 1, 1, 182},
    {80000000, 3, 1, 560},
    {81000000, 1, 1, 189},
    {82000000, 3, 1, 574},
    {84000000, 1, 1, 196},
    {86000000, 3, 1, 602},
    {87000000, 1, 1, 203},
    {88000000, 3, 1, 616},
    {90000000, 1, 1, 210},
};

static int get_pll_param_index(unsigned int pclk)
{
    unsigned int pclk_rate = pclk * 1000;
    int min_diff = 10000000;//初始化最小差值为10M
    int index = -1;
    int i = 0;
    unsigned int param_num = sizeof(pclk_epll_tab) / sizeof(struct ad100_lvds_pclk_param);

    for(i = 0; i < param_num; i++){
        unsigned int diff = abs(pclk_rate - lvds_pll_param_tab[i].pll_out_freq);
        if (diff < min_diff) {
            min_diff = diff;
            index = i;
        }
    }

    if(index == -1)
        printk("ERROR: cannot find epll param index\n");

    return index;
}

static void jz_dsih_dphy_cfg_lvds_ad100(struct lvds_config *lvds_config, struct video_config *video_config)
{
    unsigned int temp = 0;

    //S1 select lvds mode
    temp = dsi_phy_read(DSI_PHY_LVDS_REG03);
    temp &= ~0xff;
    temp |= 0x2;
    dsi_phy_write(DSI_PHY_LVDS_REG03, temp);
    pr_debug("DSI_PHY_LVDS_REG03 : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG03));

    // S1.1 LVDS color-coding / data_fmt{JEIDA,VESA(SPWG),vsync-pol,hsync-pol,de-pol}
    temp = dsi_phy_read(DSI_PHY_LVDS_REG0C);
    temp &= ~0xff;
    if(lvds_config->data_width == DATA_24_BPP)
        temp |= (1 << 5);
    if(lvds_config->mapping_mode == JEIDA)
        temp |= (1 << 4);
    if(lvds_config->v_polarity == AT_HIGH_LEVEL)
        temp &= ~(1 << 2);
    else
        temp |= (1 << 2);
    if(lvds_config->h_polarity == AT_HIGH_LEVEL)
        temp &= ~(1 << 1);
    else
        temp |= (1 << 1);
    if(lvds_config->data_en_polarity == AT_RISING_EDGE)
        temp &= ~(1 << 0);
    else
        temp |= (1 << 0);
    dsi_phy_write(DSI_PHY_LVDS_REG0C, temp);
    pr_debug("DSI_PHY_LVDS_REG0C : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG0C));

    temp = dsi_phy_read(DSI_PHY_LVDS_REG0D);
    temp &= ~0xff;
    temp |= 0x0;
    dsi_phy_write(DSI_PHY_LVDS_REG0D, temp);
    pr_debug("DSI_PHY_LVDS_REG0D : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG0D));

    int param_idx = get_pll_param_index(video_config->pixel_clock);
    if(param_idx < 0){
        printk("LVDS freq adapte failed !!!!!\n");
        return;
    }
    unsigned char prediv = lvds_pll_param_tab[param_idx].prediv;
    unsigned char postdiv = lvds_pll_param_tab[param_idx].postdiv;
    unsigned short fbdiv = lvds_pll_param_tab[param_idx].fbdiv;

    //S2 set prediv
    temp = dsi_phy_read(DSI_PHY_ANA_REG03);
    temp &= ~0xff;
    temp |= prediv;
    dsi_phy_write(DSI_PHY_ANA_REG03, temp);
    pr_debug("DSI_PHY_ANA_REG03 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG03));

    //S2-1 set postdiv
    temp = dsi_phy_read(DSI_PHY_ANA_REG1E);
    temp |= (postdiv << 0);
    dsi_phy_write(DSI_PHY_ANA_REG1E, temp);
    pr_debug("DSI_PHY_ANA_REG1E : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG1E));

    //S3 set fbdiv
    if(fbdiv > 0x1ff){
        printk("ERROR fbdiv over than 0x1ff %s \n",__func__);
    }
    if(fbdiv & 0x100){
        temp = dsi_phy_read(DSI_PHY_ANA_REG03);
        temp |= (1 << 5); //fbdiv[8]
        dsi_phy_write(DSI_PHY_ANA_REG03, temp);
        pr_debug("DSI_PHY_ANA_REG03 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG03));
    }
    temp = fbdiv & 0xff;   //
    /* printk("%s pixel_clock = %d ======\n",__func__,video_config->pixel_clock); */
    dsi_phy_write(DSI_PHY_ANA_REG04, temp);
    pr_debug("DSI_PHY_ANA_REG04 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG04));

    //S4 enable ldo
    temp = dsi_phy_read(DSI_PHY_ANA_REG08);
    temp &= ~0xff;
    temp |= 0x6e;
    dsi_phy_write(DSI_PHY_ANA_REG08, temp);
    pr_debug("DSI_PHY_ANA_REG08 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG08));

    //S5
    temp = dsi_phy_read(DSI_PHY_ANA_REG01);
    temp &= ~0xff;
    temp |= 0xE4;
    dsi_phy_write(DSI_PHY_ANA_REG01, temp);
    pr_debug("DSI_PHY_ANA_REG01 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG01));

    //S6 enable lane
    temp = dsi_phy_read(DSI_PHY_ANA_REG00);
    temp &= ~0xff;
    temp |= 0x7d;
    dsi_phy_write(DSI_PHY_ANA_REG00, temp);
    pr_debug("DSI_PHY_ANA_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG00));

    //S7
    temp = dsi_phy_read(DSI_PHY_ANA_REG01);
    temp &= ~0xff;
    temp |= 0xE0;
    dsi_phy_write(DSI_PHY_ANA_REG01, temp);
    pr_debug("DSI_PHY_ANA_REG01 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG01));

    //S8 MSB/LSB
    temp = dsi_phy_read(DSI_PHY_LVDS_REG00);
    temp &= ~0xff;
    temp |= 0x05;        // 0x0D-->MSB  0x0C-->LSB
    dsi_phy_write(DSI_PHY_LVDS_REG00, temp);
    pr_debug("DSI_PHY_LVDS_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG00));

    //S9 enable lvds digital logic
    temp = dsi_phy_read(DSI_PHY_LVDS_REG01);
    temp &= ~0xff;
    temp |= 0x92;
    dsi_phy_write(DSI_PHY_LVDS_REG01, temp);
    pr_debug("DSI_PHY_LVDS_REG01 : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG01));

    //S10 enable lvds analog logic
    temp = dsi_phy_read(DSI_PHY_LVDS_REG0B);
    temp &= ~0xff;
    temp |= 0xF8;
    dsi_phy_write(DSI_PHY_LVDS_REG0B, temp);
    pr_debug("DSI_PHY_LVDS_REG0B : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG0B));

    usleep_range(5000, 5000);
    //S11 config PLL to LVDS mode
    temp = dsi_phy_read(DSI_PHY_ANA_REG1E);
    temp &= ~(3 << 5);
    dsi_phy_write(DSI_PHY_ANA_REG1E, temp);
    pr_debug("DSI_PHY_ANA_REG1E : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG1E));
    usleep_range(5000, 5000);
}

static unsigned short calc_inno_dphy_fbdiv(unsigned int output_freq)
{
    unsigned int real_mipi_clk = 0;
    real_mipi_clk = output_freq / 1000000;    //  hz --- Mhz
    unsigned short fbdiv = 0;
    unsigned int prediv = 1;    //Fix value
    fbdiv = (real_mipi_clk * 2) / (12 / prediv);
    if (fbdiv > 0x1ff)
        fbdiv = 0x1ff;

    pr_debug("%s   output_freq = %d >>>>>>>>>>>>>>>>>>>>>>>>>>> \n",__func__,real_mipi_clk);
    return fbdiv;
}

struct dphy_timming_param {
    unsigned int freq_low;
    unsigned int freq_up;
    unsigned int val;
};

enum lane_index {
    CLK_LANE,
    DATA_LANE0,
    DATA_LANE1,
    DATA_LANE2,
    DATA_LANE3,
};

struct dphy_timming_param hs_tlpx_param_tab[] = {
    {80  ,110 ,0x02},
    {110 ,300 ,0x02},
    {300 ,600 ,0x03},
    {600 ,1000,0x05},
    {1000,1200,0x06},
    {1200,1400,0x09},
    {1400,1600,0x0d},
    {1600,1800,0x0e},
    {1800,2000,0x11},
    {2000,2400,0x13},
    {2400,2500,0x15},
};

static int set_mipi_hs_tlpx(unsigned int output_freq, unsigned int lane_idx)
{
    unsigned int clk_freq = output_freq / 1000000;
    unsigned int reg = 0;
    unsigned int val = 0;
    int i = 0;
    unsigned int param_num = sizeof(hs_tlpx_param_tab) / sizeof(struct dphy_timming_param);
    switch(lane_idx){
        case CLK_LANE:
            reg = 0x500 + 0x14;break;
        case DATA_LANE0:
            reg = 0x580 + 0x14;break;
        case DATA_LANE1:
            reg = 0x600 + 0x14;break;
        case DATA_LANE2:
            reg = 0x680 + 0x14;break;
        case DATA_LANE3:
            reg = 0x700 + 0x14;break;
        default:
            printk("%s %d: lane index error \r\n",__func__,__LINE__);
            return -1;
    }

    for(i = 0; i < param_num; i++){
        if(clk_freq >= hs_tlpx_param_tab[i].freq_low && clk_freq < hs_tlpx_param_tab[i].freq_up)
            break;
    }
    if(i >= param_num){
        printk("%s %d : can not find avalid param\r\n",__func__,__LINE__);
        return -1;
    }

    val = hs_tlpx_param_tab[i].val;
    dsi_phy_write(reg, val);

    return 0;
}

struct dphy_timming_param ths_prepare_param_tab[] = {
    {80  ,300 ,0x7f},
    {300 ,400 ,0x7e},
    {400 ,500 ,0x7c},
    {500 ,600 ,0x70},
    {600 ,700 ,0x40},
    {700 ,800 ,0x02},
    {800 ,1000,0x08},
    {1000,1400,0x03},
    {1400,1600,0x42},
    {1600,1800,0x47},
    {1800,2000,0x64},
    {2000,2200,0x64},
    {2200,2400,0x33},
    {2400,2500,0x54},
};

static int set_mipi_hs_ths_prepare(unsigned int output_freq, unsigned int lane_idx)
{
    unsigned int clk_freq = output_freq / 1000000;
    unsigned int reg = 0;
    unsigned int val = 0;
    int i = 0;
    unsigned int param_num = sizeof(ths_prepare_param_tab) / sizeof(struct dphy_timming_param);
    switch(lane_idx){
        case CLK_LANE:
            reg = 0x500 + 0x18;break;
        case DATA_LANE0:
            reg = 0x580 + 0x18;break;
        case DATA_LANE1:
            reg = 0x600 + 0x18;break;
        case DATA_LANE2:
            reg = 0x680 + 0x18;break;
        case DATA_LANE3:
            reg = 0x700 + 0x18;break;
        default:
            printk("%s %d: lane index error \r\n",__func__,__LINE__);
            return -1;
    }

    for(i = 0; i < param_num; i++){
        if(clk_freq >= ths_prepare_param_tab[i].freq_low && clk_freq < ths_prepare_param_tab[i].freq_up)
            break;
    }
    if(i >= param_num){
        printk("%s %d : can not find avalid param\r\n",__func__,__LINE__);
        return -1;
    }

    val = ths_prepare_param_tab[i].val;
    dsi_phy_write(reg, val);

    return 0;
}

struct dphy_timming_param ths_trail_param_tab[] = {
    {80 ,110,0x02},
    {110,150,0x02},
    {150,200,0x02},
    {200,250,0x04},
    {250,300,0x04},
    {300,400,0x04},
    {400,500,0x08},
    {500,600,0x10},
    {600,700,0x30},
    {700 ,800  ,0x30},
    {800 ,1000 ,0x30},
    {1000,1200 ,0x0f},
    {1200,1400 ,0x0f},
    {1400,1600 ,0x0f},
    {1600,1800 ,0x0f},
    {1800,2000 ,0x0b},
    {2000,2200 ,0x0b},
    {2200,2400 ,0x6a},
    {2400,2500 ,0x6a},

};

static int set_mipi_hs_ths_trail(unsigned int output_freq, unsigned int lane_idx)
{
    unsigned int clk_freq = output_freq / 1000000;
    unsigned int reg = 0;
    unsigned int val = 0;
    int i = 0;
    unsigned int param_num = sizeof(ths_trail_param_tab) / sizeof(struct dphy_timming_param);
    switch(lane_idx){
        case CLK_LANE:
            reg = 0x500 + 0x20;break;
        case DATA_LANE0:
            reg = 0x580 + 0x20;break;
        case DATA_LANE1:
            reg = 0x600 + 0x20;break;
        case DATA_LANE2:
            reg = 0x680 + 0x20;break;
        case DATA_LANE3:
            reg = 0x700 + 0x20;break;
        default:
            printk("%s %d: lane index error \r\n",__func__,__LINE__);
            return -1;
    }

    for(i = 0; i < param_num; i++){
        if(clk_freq >= ths_trail_param_tab[i].freq_low && clk_freq < ths_trail_param_tab[i].freq_up)
            break;
    }
    if(i >= param_num){
        printk("%s %d : can not find avalid param\r\n",__func__,__LINE__);
        return -1;
    }

    val = ths_trail_param_tab[i].val;
    dsi_phy_write(reg, val);

    return 0;
}

struct dphy_timming_param clklane_ths_zero_param_tab[] = {
    {80  ,110 ,0x16},
    {110 ,150 ,0x16},
    {150 ,200 ,0x17},
    {200 ,250 ,0x17},
    {250 ,300 ,0x18},
    {300 ,400 ,0x19},
    {400 ,500 ,0x1B},
    {500 ,600 ,0x1D},
    {600 ,700 ,0x1E},
    {700 ,800 ,0x1F},
    {800 ,1000,0x20},
    {1000,1200,0x32},
    {1200,1400,0x32},
    {1400,1600,0x36},
    {1600,1800,0x7a},
    {1800,2000,0x7a},
    {2000,2200,0x7e},
    {2200,2400,0x7f},
    {2400,2500,0x7f},
};

struct dphy_timming_param datalane_ths_zero_param_tab[] = {
    {80 ,110,0x2},
    {110,150,0x3},
    {150,200,0x4},
    {200,250,0x5},
    {250,300,0x6},
    {300,400,0x7},
    {400,500,0x7},
    {500,600,0x8},
    {600,700,0x8},
    {700 ,800  ,0x9},
    {800 ,1000 ,0x9},
    {1000,1200 ,0x14},
    {1200,1400 ,0x14},
    {1400,1600 ,0x0E},
    {1600,1800 ,0x0E},
    {1800,2000 ,0x0E},
    {2000,2200 ,0x15},
    {2200,2400 ,0x15},
    {2400,2500 ,0x15},
};

static int set_mipi_ths_zero(unsigned int output_freq, unsigned int lane_idx)
{
    unsigned int clk_freq = output_freq / 1000000;
    unsigned int reg = 0;
    unsigned int reg_hi = 0;
    unsigned int val = 0;
    unsigned int tmp = 0;
    int i = 0;
    unsigned int param_num = 0;
    struct dphy_timming_param *param = NULL;
    if(lane_idx == CLK_LANE){
        param_num = sizeof(clklane_ths_zero_param_tab) / sizeof(struct dphy_timming_param);
        param = clklane_ths_zero_param_tab;
    } else {
        param_num = sizeof(datalane_ths_zero_param_tab) / sizeof(struct dphy_timming_param);
        param = datalane_ths_zero_param_tab;
    }
    switch(lane_idx){
        case CLK_LANE:
            reg = 0x500 + 0x1C;
            reg_hi = 0x500 + 0x18;
            break;
        case DATA_LANE0:
            reg = 0x580 + 0x1C;
            reg_hi = 0x580 + 0x18;
            break;
        case DATA_LANE1:
            reg = 0x600 + 0x1C;
            reg_hi = 0x600 + 0x18;
            break;
        case DATA_LANE2:
            reg = 0x680 + 0x1C;
            reg_hi = 0x680 + 0x18;
            break;
        case DATA_LANE3:
            reg = 0x700 + 0x1C;
            reg_hi = 0x700 + 0x18;
            break;
        default:
            printk("%s %d: lane index error \r\n",__func__,__LINE__);
            return -1;
    }

    for(i = 0; i < param_num; i++){
        if(clk_freq >= param[i].freq_low && clk_freq < param[i].freq_up)
            break;
    }
    if(i >= param_num){
        printk("%s %d : can not find avalid param\r\n",__func__,__LINE__);
        return -1;
    }

    val = param[i].val;
    tmp = dsi_phy_read(reg_hi);
    tmp &= ~(1 << 7);
    if(val & (1 << 6))
        tmp |= (1 << 7);
    dsi_phy_write(reg_hi, tmp);

    dsi_phy_write(reg, val & 0x3F);

    return 0;
}

static int jz_dsih_dphy_cfg_mipi_ad100(unsigned char lanes, struct video_config *video_config)
{
    unsigned int temp = 0;
    unsigned int freq = video_config->byte_clock * 1000 * 8;
    unsigned short fbdiv = calc_inno_dphy_fbdiv(freq);

    //S1 set prediv
    temp = dsi_phy_read(DSI_PHY_ANA_REG03);
    temp &= ~0xff;
    if (fbdiv > 0xff)
        temp |= (1 << 5); // fbdiv bit8
    temp |= 0x01;
    dsi_phy_write(DSI_PHY_ANA_REG03, temp);
    pr_debug("DSI_PHY_ANA_REG03 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG03));

    //S2 set fbdiv
    temp = dsi_phy_read(DSI_PHY_ANA_REG04);
    temp &= ~0xff;
    temp = fbdiv & 0xff; // fbdiv bit 0-7
    dsi_phy_write(DSI_PHY_ANA_REG04, temp);
    pr_debug("DSI_PHY_ANA_REG04 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG04));

    //S3 PLL LDO
    temp = dsi_phy_read(DSI_PHY_ANA_REG01);
    temp &= ~0xff;
    temp |= 0xE4;
    dsi_phy_write(DSI_PHY_ANA_REG01, temp);
    pr_debug("DSI_PHY_ANA_REG01 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG01));

    //S4 set lane num
    temp = dsi_phy_read(DSI_PHY_ANA_REG00);
    temp &= ~0xff;
    if (lanes == 4) {
        temp |= 0x7d;
        dsi_phy_write(DSI_PHY_ANA_REG00, temp);
    } else {
        temp |= 0x4d;
        dsi_phy_write(DSI_PHY_ANA_REG00, temp);
    }
    pr_debug("DSI_PHY_ANA_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG00));

    //S5 reset analog
    temp = dsi_phy_read(DSI_PHY_ANA_REG01);
    temp &= ~0xff;
    temp |= 0xe0;
    dsi_phy_write(DSI_PHY_ANA_REG01, temp);
    pr_debug("DSI_PHY_ANA_REG01 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG01));

    // S6
    msleep(20);

    //S7  reset digital
    temp = dsi_phy_read(DSI_PHY_DIG_REG00);
    temp &= ~0xff;
    temp |= 0x1e;
    dsi_phy_write(DSI_PHY_DIG_REG00, temp);
    pr_debug("DSI_PHY_DIG_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_DIG_REG00));
    msleep(5);

    // S8 digital normal
    temp = dsi_phy_read(DSI_PHY_DIG_REG00);
    temp &= ~0xff;
    temp |= 0x1f;
    dsi_phy_write(DSI_PHY_DIG_REG00, temp);
    pr_debug("DSI_PHY_DIG_REG00 : 0x%x\n",dsi_phy_read(DSI_PHY_DIG_REG00));

    // S9 Func Mode select
    temp = dsi_phy_read(DSI_PHY_LVDS_REG03);
    temp &= ~0xff;
    temp |= 0x01;
    dsi_phy_write(DSI_PHY_LVDS_REG03, temp);
    pr_debug("DSI_PHY_LVDS_REG03 : 0x%x\n",dsi_phy_read(DSI_PHY_LVDS_REG03));

    // select MIPI DPHY pll mode
    temp = dsi_phy_read(DSI_PHY_ANA_REG1E);
    temp &= ~(3 << 5);
    temp |= (1 << 5);
    dsi_phy_write(DSI_PHY_ANA_REG1E, temp);
    pr_debug("DSI_PHY_ANA_REG1E : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG1E));

    // S9-1  LPDT LANE0 PPI SYNC may not need
    temp = dsi_phy_read(0x580 + 0x30);
    temp |= 0x4;
    dsi_phy_write(0x580 + 0x30, temp);

    int lane = 0;
    for(lane = 0; lane <= DATA_LANE3; lane++){
        set_mipi_hs_tlpx(freq,lane);
        set_mipi_hs_ths_prepare(freq,lane);
        set_mipi_hs_ths_trail(freq,lane);
        set_mipi_ths_zero(freq,lane);
    }

    if (!video_config->en_swap)
        goto swap_done;

    if(video_config->clk_lane_pn_swap){
        // CLK LANE P/N SWAP
        temp = dsi_phy_read(0x500);
        temp |= (1 << 4);
        dsi_phy_write(0x500, temp);
    }
    if(video_config->lane0_pn_swap){
        // LANE0 P/N SWAP
        temp = dsi_phy_read(0x580);
        temp |= (1 << 4);
        dsi_phy_write(0x580, temp);
    }
    if(video_config->lane1_pn_swap){
        // LANE1 P/N SWAP
        temp = dsi_phy_read(0x600);
        temp |= (1 << 4);
        dsi_phy_write(0x600, temp);
    }
    if(video_config->lane2_pn_swap){
        // LANE2 P/N SWAP
        temp = dsi_phy_read(0x680);
        temp |= (1 << 4);
        dsi_phy_write(0x680, temp);
    }
    if(video_config->lane3_pn_swap){
        // LANE3 P/N SWAP
        temp = dsi_phy_read(0x700);
        temp |= (1 << 4);
        dsi_phy_write(0x700, temp);
    }

swap_done:
    temp = dsi_phy_read(DSI_PHY_ANA_REG05);
    temp &= ~0x7;
    temp |= 0x0;
    dsi_phy_write(DSI_PHY_ANA_REG05, temp);
    pr_debug("DSI_PHY_ANA_REG05 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG05));

    temp = dsi_phy_read(DSI_PHY_ANA_REG06);
    temp &= ~0xFF;
    temp |= 0x0;
    dsi_phy_write(DSI_PHY_ANA_REG06, temp);
    pr_debug("DSI_PHY_ANA_REG06 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG06));

    temp = dsi_phy_read(DSI_PHY_ANA_REG07);
    temp &= ~0x77;
    temp |= 0x0;
    dsi_phy_write(DSI_PHY_ANA_REG07, temp);
    pr_debug("DSI_PHY_ANA_REG07 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG07));

    temp = dsi_phy_read(DSI_PHY_ANA_REG08);
    temp &= ~0xf;
    temp |= 0x7;
    dsi_phy_write(DSI_PHY_ANA_REG08, temp);
    pr_debug("DSI_PHY_ANA_REG08 : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG08));

    temp = dsi_phy_read(DSI_PHY_ANA_REG0B);
    temp &= ~0xf;
    temp |= 0xf;
    dsi_phy_write(DSI_PHY_ANA_REG0B, temp);
    pr_debug("DSI_PHY_ANA_REG0B : 0x%x\n",dsi_phy_read(DSI_PHY_ANA_REG0B));

    //S10
    msleep(10);

    return 0;
}

/**现象: LP模式关闭mipi控制器后, mipi数据脚电平为1.2V, 时钟脚为470mV,
 * 可能会漏电到屏幕, 导致mipi控制器再次使能后, 屏幕影响mipi输出时钟波形异常;
 * 目前解决方法: 在关闭mipi控制器之前, 进入ULPM模式(极低功耗), 此时时钟以及数据脚为0V
 * 且先屏上电复位后初始化mipi控制器, 退出ULPM模式, 进入LP模式与屏通讯 */
static void jz_dsi_dphy_enter_ulpm(int num_of_lanes)
{
    int reg_end = 4+num_of_lanes*2;
    /* Verify that all active lanes are in ULPM */
    if (dsi_get_bit(R_DSI_HOST_PHY_STATUS, 1, reg_end) == 0x0) {
        printk(KERN_DEBUG "jz_mipi_dsi_hal: already in ULPM state now\n");
        return;
    }

    /* Step1.1 PHY_RSTZ[3:0] = 4'hF */
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, 0, 3, 0xf);
    /* Step1.2 PHY_ULPS_CTRL[3:0] = 4'h0 */
    dsi_set_bit(R_DSI_HOST_PHY_ULPS_CTRL, 0, 3, 0);
    /* Step1.3 PHY_TX_TRIGGERS[3:0] = 4'h0 */
    dsi_set_bit(R_DSI_HOST_PHY_TX_TRIGGERS, 0, 3, 0);
    /* Step1.4 Verify that all active lanes are in Stop state and the D-PHY PLL is locked */
    int times = 10;
    int reg_val = 0x1FB;
    int data = get_bit_field((volatile long unsigned int *)&reg_val, 0, num_of_lanes*2);
    while ((dsi_get_bit(R_DSI_HOST_PHY_STATUS, 4, reg_end) != data ||
           dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, 1) != 0x1) && times--);
    if (!times)
        printk(KERN_ERR "1R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));

    /* Step2 enter ULPM in the data and the clock lanes */
    dsi_set_bit(R_DSI_HOST_PHY_ULPS_CTRL, 0, 3, 0x5);
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, 0, 1, 0x2);
    times = 10;

    /* Step3 Wait until the D-PHY active lanes enter into ULPM */
    while ((dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, reg_end) != 0x1) && times--);
    if (!times)
        printk(KERN_ERR "2R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));

    /* Step4-5 turn off the D-PHY PLL */
    // dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_FORCEPLL, 0);
    // times = 10;
    // while ((dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, 0) != 0x0) && times--);
    // if (!times)
    //     printk(KERN_ERR "3R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));
    pr_debug("jz_mipi_dsi_hal: enter ULPM state now\n");
}

static void jz_dsi_dphy_exit_ulpm(int num_of_lanes)
{
    int times = 10;
    int reg_end = 4+num_of_lanes*2;
    /* Verify that all active lanes are in ULPM */
    if (dsi_get_bit(R_DSI_HOST_PHY_STATUS, 1, reg_end) != 0x0) {
        printk(KERN_DEBUG "jz_mipi_dsi_hal: not in ULPM state now\n");
        return;
    }

    /* Step2 If the D-PHY PLL is already locked */
    if (dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, 0) == 0x1)
        goto unlock;

    /* Step3 Turn on the D-PHY PLL */
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_FORCEPLL, 1);

    /* Step4 Wait until D-PHY PLL locked */
    while ((dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, 0) != 0x1) && times--);
    if (!times)
        printk(KERN_ERR "4R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));

unlock:
    /* Step5 assert the Exit ULPM bits */
    dsi_set_bit(R_DSI_HOST_PHY_ULPS_CTRL, 0, 3, 0xf);
    times = 10;
    int reg_val = 0x1528;
    int data = get_bit_field((volatile long unsigned int *)&reg_val, 0, num_of_lanes*2);
    while (((dsi_read(R_DSI_HOST_PHY_STATUS) & data) != data) && times--);
    if (!times)
        printk(KERN_ERR "5R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));

    /* Step7 Wait for 1 ms */
    usleep_range(1000, 1000);

    /* Step8 Deassert the ULPM requests and the ULPM exit bits */
    dsi_set_bit(R_DSI_HOST_PHY_ULPS_CTRL, 0, 3, 0);
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, 0, 1, 0x1);

    /* Step9 in Stop state and the D-PHY PLL is locked */
    times = 10;
    reg_val = 0x1FB;
    data = get_bit_field((volatile long unsigned int *)&reg_val, 0, num_of_lanes*2);
    while ((dsi_get_bit(R_DSI_HOST_PHY_STATUS, 4, reg_end) != data ||
           dsi_get_bit(R_DSI_HOST_PHY_STATUS, 0, 1) != 0x1) && times--);
    if (!times)
        printk(KERN_ERR "6R_DSI_HOST_PHY_STATUS: 0x%08x\n", dsi_read(R_DSI_HOST_PHY_STATUS));

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_FORCEPLL, 0);
    pr_debug("jz_mipi_dsi_hal: exit ULPM state now\n");
}

static void jz_lvds_dphy_init(struct lvds_config *lvds_config, struct video_config *video_config)
{
    if (lvds_config->mapping_mode == VESA && lvds_config->data_width == DATA_18_BPP)
        lvds_config->mapping_mode = JEIDA;

    jz_dsih_dphy_cfg_lvds_ad100(lvds_config, video_config);
}

static void jz_dsi_dphy_init(struct dsi_config *dsi_config, struct video_config *video_config)
{
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 0);

    dsi_set_bit(R_DSI_HOST_PHY_IF_CFG, PHY_STOP_WAIT_TIME, 0x1c);

    dsi_set_bit(R_DSI_HOST_PHY_IF_CFG, N_LANES, dsi_config->num_of_lanes - 1);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_ENABLECLK, 1);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_SHUTDOWMZ, 1);
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 1);

    dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TX_ESC_CLK_DIV, 7);

    jz_dsih_dphy_cfg_mipi_ad100(dsi_config->num_of_lanes, video_config);
}

void jz_dsi_dphy_deinit(void)
{
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_ENABLECLK, 0);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_SHUTDOWMZ, 0);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 0);
}

static void jz_dsi_gen_init(struct dsi_config *dsi_config)
{
    dsi_set_bit(R_DSI_HOST_DPI_CFG_POL, COLORM_ACTIVE_LOW, !dsi_config->color_mode_polarity);
    dsi_set_bit(R_DSI_HOST_DPI_CFG_POL, SHUTD_ACTIVE_LOW, !dsi_config->shut_down_polarity);

    dsi_set_dphy_hs2lp_time(dsi_config->max_hs_to_lp_cycles);
    dsi_set_dphy_lp2hs_time(dsi_config->max_lp_to_hs_cycles);
    dsi_set_dphy_bta_time(dsi_config->max_bta_cycles);

    dsi_set_bit(R_DSI_HOST_GEN_VCID, GEN_VICD_RX, 0);

    unsigned long pckhdl_cfg = dsi_read(R_DSI_HOST_PCKHDL_CFG);

    set_bit_field(&pckhdl_cfg, ETOP_RX_EN, 1);
    set_bit_field(&pckhdl_cfg, ETOP_TX_EN, 0);
    set_bit_field(&pckhdl_cfg, BAT_EN, 1);
    set_bit_field(&pckhdl_cfg, ECC_RX_EN, 1);
    set_bit_field(&pckhdl_cfg, CRC_RX_EN, 1);

    dsi_write(R_DSI_HOST_PCKHDL_CFG, pckhdl_cfg);

    dsi_set_bit(R_DSI_HOST_DPI_COLOR_CODING, DIP_COLOR_CODING, dsi_config->color_coding);
    dsi_set_bit(R_DSI_HOST_DPI_COLOR_CODING, LOOSELY18_EN, dsi_config->color_type_18bit);
}

static int dsi_write_gen_data(unsigned int data)
{
    if (dsi_wait_pld_w_not_full(500)) {
        printk("pld_w_fifo full!\n");
        return -1;
    }

    dsi_write(R_DSI_HOST_GEN_PLD_DATA, data);

    return 0;
}

static int dsi_write_short_packet(unsigned char vc, unsigned char packet_type, unsigned short cmd_data)
{
    if (dsi_wait_cmd_not_full(10*1000)) {
        printk("cmd fifo full!\n");
        return -1;
    }

    unsigned char data[2];
    data[0] = cmd_data;
    data[1] = cmd_data >> 8;

    unsigned long gen_hdr = 0;
    set_bit_field(&gen_hdr, GEN_DT, packet_type);
    set_bit_field(&gen_hdr, GEN_VC, vc);
    set_bit_field(&gen_hdr, GEN_WC_LSBYTE, data[0]);
    set_bit_field(&gen_hdr, GEN_WC_MSBYTE, data[1]);
    dsi_write(R_DSI_HOST_GEN_HDR, gen_hdr);

    return 0;
}

static int dsi_write_long_packet(unsigned char vc, unsigned char packet_type, unsigned char *cmd_data, unsigned short word_count)
{
    int ret;
    int i;

    unsigned int gen_pld_data = 0;
    for (i = 0; i < word_count; i++) {
        gen_pld_data |= cmd_data[i] << (i % 4) * 8;
        if ((i + 1) % 4 == 0 && i != 0) {
            ret = dsi_write_gen_data(gen_pld_data);
            if (ret < 0)
                return -1;

            gen_pld_data = 0;
        }
    }

    if (word_count % 4 != 0) {
        ret = dsi_write_gen_data(gen_pld_data);
        if (ret < 0)
            return -1;
    }

    ret = dsi_write_short_packet(vc, packet_type, word_count);
    if (ret < 0)
        return -1;

    return 0;
}

static int dsi_read_packet(int bytes, char *rd_buf)
{
    int i;
    int off = 0;
    if (dsi_wait_rd_cmd_busy(10*1000)) {
        printk("read cmd busy!\n");
        return -1;
    }

    if (dsi_wait_rd_fifo_empty(10*1000)) {
        printk("read fifo is empty!\n");
        return -1;
    }

    for (i = 0; i < bytes; i++) {
        rd_buf[i] = dsi_get_bit(R_DSI_HOST_GEN_PLD_DATA, off, off + 7);
        off += 8;

        if ((i + 1) % 4 == 0 && i != 0) {
            off = 0;
        }
    }

    return 0;
}

static void jz_dsi_dpi_config(struct video_config *video_config)
{
    unsigned int hs_timeout;
    int counter;

    unsigned int temp;

    temp = video_config->byte_clock * 1000 / (video_config->pixel_clock);

    dsi_write(R_DSI_HOST_VID_HBP_TIME, video_config->hbp * temp / 1000);
    dsi_write(R_DSI_HOST_VID_HSA_TIME, video_config->hs * temp / 1000);
    dsi_write(R_DSI_HOST_VID_HLINE_TIME, video_config->h_total_pixels * temp / 1000);

    dsi_write(R_DSI_HOST_VID_VBP_LINES, video_config->vbp);
    dsi_write(R_DSI_HOST_VID_VFP_LINES, video_config->vfp);
    dsi_write(R_DSI_HOST_VID_VSA_LINES, video_config->vs);
    dsi_write(R_DSI_HOST_VID_VACTIVE_LINES, video_config->v_active_lines);

    unsigned long dpi_cfg_pol = dsi_read(R_DSI_HOST_DPI_CFG_POL);
    set_bit_field(&dpi_cfg_pol, DATAEN_ACTIVE_LOW, !video_config->data_en_polarity);
    set_bit_field(&dpi_cfg_pol, VSYNC_ACTIVE_LOW, !video_config->v_polarity);
    set_bit_field(&dpi_cfg_pol, HSYNC_ACTIVE_LOW, !video_config->h_polarity);
    dsi_write(R_DSI_HOST_DPI_CFG_POL, dpi_cfg_pol);

    //timeout ???
    hs_timeout = (video_config->h_total_pixels * video_config->v_active_lines) \
                  + (2 * (video_config->bpp_info * 100 / 8) / 100);

    for (counter = 0x80; (counter < hs_timeout) && (counter > 2); counter--) {
        if ((hs_timeout % counter) == 0) {
            dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TO_CLK_DIV, counter);
            dsi_set_bit(R_DSI_HOST_TO_CNT_CFG, HSTX_TO_CNT, (unsigned short)(hs_timeout / counter));
            dsi_set_bit(R_DSI_HOST_TO_CNT_CFG, LPRX_TO_CNT, (unsigned short)(hs_timeout / counter));
            break;
        }
    }
}

int jz_dsi_video_init(struct video_config *video_config)
{
    //high speed
    dsi_set_transfer_mode(0);

    //set video mode
    dsi_set_video_mode();

    unsigned long vid_mode_cfg = dsi_read(R_DSI_HOST_VID_MODE_CFG);

    set_bit_field(&vid_mode_cfg, VID_MODE_TYPE, video_config->video_mode);
    set_bit_field(&vid_mode_cfg, LP_VSA_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VBP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VFP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VACT_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_HBP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_HFP_EN, 1);

    set_bit_field(&vid_mode_cfg, VPG_EN, 0);
    set_bit_field(&vid_mode_cfg, VPG_MODE, 0);
    set_bit_field(&vid_mode_cfg, VPG_ORIENTATION, 1);

    dsi_write(R_DSI_HOST_VID_MODE_CFG, vid_mode_cfg);

    //dpi config
    jz_dsi_dpi_config(video_config);

    //set R_DSI_HOST_CLKMGR_CFG tx_esc_clk_division????
    dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TX_ESC_CLK_DIV, 7);

    //pack_size chun_no null_size????
    dsi_set_bit(R_DSI_HOST_VID_NUM_CHUNKS, VID_NUM_CHUNKS, video_config->chunk);
    dsi_set_bit(R_DSI_HOST_VID_PKT_SIZE, VID_PKT_SIZE, video_config->video_size);
    dsi_set_bit(R_DSI_HOST_VID_NULL_SIZE, VID_NULL_SIZE, video_config->null_size);

    //set dpi channel
    dsi_set_bit(R_DSI_HOST_DPI_VCID, DPI_VCID, video_config->virtual_channel);

    //enable hs clk
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, PHY_TXREQUESTCLKHS, 1);

    return 0;
}
