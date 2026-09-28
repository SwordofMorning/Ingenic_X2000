/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI
 *
 */
#ifndef __X2580_MIPI_CSI_H__
#define __X2580_MIPI_CSI_H__

#include "camera_sensor.h"
#include "dsys.h"


#define MIPI_CSI_IOBASE                 0x10023000
#define MIPI_PHY_IOBASE                 0x10022000


/*
 * MIPI CSI controller
 */
#define MIPI_VERSION                    0x000
#define MIPI_N_LANES                    0x004
#define MIPI_PHY_SHUTDOWNZ              0x008
#define MIPI_DPHY_RSTZ                  0x00C
#define MIPI_CSI2_RESETN                0x010
#define MIPI_PHY_STATE                  0x014
#define MIPI_DATA_IDS_1                 0x018
#define MIPI_DATA_IDS_2                 0x01C
#define MIPI_ERR1                       0x020
#define MIPI_ERR2                       0x024
#define MIPI_MASK1                      0x028
#define MIPI_MASK2                      0x02C
#define MIPI_PHY_TST_CTRL0              0x030
#define MIPI_PHY_TST_CTRL1              0x034
#define MIPI_VC0_FRAME_NUM              0x040
#define MIPI_VC1_FRAME_NUM              0x044
#define MIPI_VC2_FRAME_NUM              0x048
#define MIPI_VC3_FRAME_NUM              0x04C

#define CTRL_DUAL_ENABLE                0x080
#define RXVALID_MASK                    0x100
#define RESERVE_REG0                    0x110
#define RESERVE_REG1                    0x114
#define RESERVE_REG2                    0x118
#define RESERVE_REG3                    0x11c

/*
 * MIPI PHY  controller
 */
#define PHY_OFFSET                      0x400
#define PHY_ENB                         (PHY_OFFSET + 0x000)
#define PHY_LVDS_TTL_BANK_ENB           (PHY_OFFSET + 0x080)
#define PHY_CK_CONTI                    (PHY_OFFSET + 0x128)
#define PHY_CK_SETTLE                   (PHY_OFFSET + 0x160)
#define PHY_DATA0_CONTI                 (PHY_OFFSET + 0x1A8)
#define PHY_DATA0_SETTLE                (PHY_OFFSET + 0x1E0)
#define PHY_DATA1_CONTI                 (PHY_OFFSET + 0x228)
#define PHY_DATA1_SETTLE                (PHY_OFFSET + 0x260)
#define PHY_MODEL_SWITCH                (PHY_OFFSET + 0x54C)
#define PHY_LVDS_MODE                   (PHY_OFFSET + 0x580)

#define MIPI_PHY_STATE_data_lane0_stop      4
#define MIPI_PHY_STATE_data_lane1_stop      5
#define MIPI_PHY_STATE_clk_lane_stop        10

#define MIPI_PHY_phy_shutdown           0, 0
#define MIPI_PHY_dphy_reset             0, 0
#define MIPI_PHY_csi2_reset             0, 0
#define MIPI_PHY_n_lanes                0, 0
#define MIPI_PHY_test_ctrl0_testclr     0, 0
#define MIPI_PHY_test_ctrl0_testclk     1, 1
#define MIPI_PHY_test_ctrl1_testen      16, 16

#define MIPI_PHY_clk_settle             0, 7   /* Tclk-settle */
#define MIPI_PHY_data0_settle           0, 7   /* Ths-settle */
#define MIPI_PHY_data1_settle           0, 7   /* Ths-settle */

typedef enum {
    CSI_ERR_MASK_REGISTER1              = 0x1,
    CSI_ERR_MASK_REGISTER2              = 0x2,
} csi_err_mask_reg_t;


int mipi_csi_phy_stop(void);
int mipi_csi_phy_initialization(struct mipi_csi_bus *mipi_info);
#ifdef SOC_CAMERA_DEBUG
int dsysfs_mipi_dump_reg(char *buf);
#endif

#endif /* __X2580_MIPI_CSI_H__ */
