#ifndef __VIC_REG_H__
#define __VIC_REG_H__

#define VIC_IOBASE                     0x13380000

#define IRQ_VIC                        (30)

#define VIC_GLB_RST                     2, 2
#define VIC_REG_ENABLE                  1, 1
#define VIC_START                       0, 0

#define SCALER_CH2_DONE                 17,17
#define SCALER_CH1_DONE                 16,16
#define SCALER_CH0_DONE                 15,15
// #define VIC_DONE                        14, 14
// #define DVP_HCOMP_ERR                   13, 13
// #define MIPI_VCOMP_ERR_CH0              12, 12
// #define MIPI_HCOMP_ERR_CH0              11, 11
// #define IMAGE_FIFO_OVF                  10, 10
// #define OUTPUT_LIMIT_ERR                9, 9
// #define DMA_FIFO_OVF                    8, 8
// #define DMA_FRD                         7, 7
// #define VIC_HVF_ERR                     6, 6
// #define VIC_VRES_ERR                    5, 5
// #define VIC_HRES_ERR                    4, 4
// #define VIC_HVRES_ERR                   4, 5

#define VIC_MIPI_VCOMP_ERR              28, 31  //STA1
#define VIC_MIPI_HCOMP_ERR              24, 27
#define VIC_MIPI_FID_OVF                23, 23
#define VIC_SYFIFO_OVF                  22, 22
#define VIC_CONTROL_LIMIT               21, 21
#define VIC_DMA_OVF                     20, 20
#define VIC_DVP_HCOMP_ERR               19, 19
#define VIC_HVF_ERR                     18, 18
#define VIC_VER_ERR                     14, 17
#define VIC_HOR_ERR                     10, 13
#define VIC_HV_ERR                      10, 17
#define VIC_FIFO_OVF                    9, 9
#define VIC_FRM_RST                     8, 8
#define VIC_FRM_START                   4, 7    // frame start
#define VIC_FRM_DONE                    0, 3    // END　frame done

#define DMA_FRD                         0, 2    // STA2
#define DMA_ARB_TRANS_DONE              3, 3
#define DMA_CHID_OVF                    4, 4

#define HORIZONTAL_RESOLUTION           16, 31
#define VERTICAL_RESOLUTION             0, 15

#define frame_ecc_mode                  1, 1
#define frame_ecc_en                    0, 0

#define dvp_hcomp                       31, 31
#define dvp_img_chk                     28, 28
#define DVP_BUS_SELECT                  24, 27
#define DVP_RGB_ORDER                   21, 23
#define DVP_RAW_ALIGN                   20, 20
#define DVP_DATA_FORMAT                 17, 19
#define DVP_TIMING_MODE                 15, 16
#define BT_INTF_WIDE                    11, 11
#define BT_SAV_EAV                      10, 10
#define BT601_MODE                      9, 9
#define YUV_DATA_ORDER                  4, 5
#define START_FIELD                     3, 3
#define INTERLACE_EN                    2, 2
#define HSYNC_POLAR                     1, 1
#define VSYNC_POLAR                     0, 0

#define HFB_NUM                         16, 31
#define HACT_NUM                        0, 15

#define hbb_num                         0, 15

#define ODD_VFB                         16, 31
#define ODD_VACT                        0, 15

#define ODD_VBB                         16, 31
#define EVEN_VFB                        0, 15

#define EVEN_VACT                       16, 31
#define EVEN_VBB                        0, 15

#define CB_Y_VALUE                      16, 23
#define CB_CB_VALUE                     8, 15
#define CB_CR_VALUE                     0, 7

#define RAW21_VALUE                     12, 23
#define RAW22_VALUE                     0, 11

#define Dma_en                          31, 31
#define Get_num                         16, 19
#define Yuv422_order                    8, 9
#define Buffer_number                   3, 6
#define Base_mode                       0, 2

#define VC_TIZIANO_ROUTE_isp_out        0, 0
#define VC_DMA_ROUTE_dma_out            0, 0

#define DMA_HORIZONTAL_RESOLUTION       16, 31
#define DMA_VERTICAL_RESOLUTION         0, 15

#define MIPI_HCROP_CH0_all_image_width  16, 31
#define MIPI_HCROP_CH0_start_pixel      0,  15

#define MIPI_VCROP_CH0_all_image_width  16, 31
#define MIPI_VCROP_CH0_start_pixel      0,  15

#define VC_CONTROL_DELEY_hdeley         16, 31  //VC_CONTROL_DELEY
#define VC_CONTROL_DELEY_vdeley         0,  15

#define    HCROP_DIFF_EN                25, 25
#define    MIPI_VCOMP_EN                24, 24
#define    MIPI_HCOMP_EN                23, 23
#define    LINE_SYNC_MODE               22, 22
#define    WORK_START_FLAG              20, 21
#define    DATA_TYPE_EN                 18, 18
#define    DATA_TYPE_VALUE              12, 17
#define    DEL_START                    8,  11
#define    SENSOR_FRAME_NUM             4,  5
#define    SENSOR_FID_MODE              2,  2
#define    SENSOR_MODE                  0,  1

#define VIC_BK_CB_CTRL                  0x28
#define VIC_BK_CB_BLK                   0x2C
#define VIC_INPUT_VPARA0                0x30
#define VIC_INPUT_VPARA1                0x34
#define VIC_INPUT_VPARA2                0x38
#define VIC_INPUT_VPARA3                0x3C
#define VIC_VLD_LINE_SAV                0x60
#define VIC_VLD_LINE_EAV                0x64
#define VIC_VLD_FRM_SAV                 0x70
#define VIC_VLD_FRM_EAV                 0x74
#define VIC_VC_CONTROL_FSM              0x8C
#define VIC_VC_CONTROL_CH0_PIX          0x90
#define VIC_VC_CONTROL_CH1_PIX          0x94
#define VIC_VC_CONTROL_CH2_PIX          0x98
#define VIC_VC_CONTROL_CH3_PIX          0x9C
#define VIC_VC_CONTROL_CH0_LINE         0xA0
#define VIC_VC_CONTROL_CH1_LINE         0xA4
#define VIC_VC_CONTROL_CH2_LINE         0xA8
#define VIC_VC_CONTROL_CH3_LINE         0xAC
#define VIC_VC_CONTROL_FIFO_USE         0xB0
#define VIC_CB_1ST                      0xC0
#define VIC_CB_2ND                      0xC4
#define VIC_CB_3RD                      0xC8
#define VIC_CB_4TH                      0xCC
#define VIC_CB_5TH                      0xD0
#define VIC_CB_6TH                      0xD4
#define VIC_CB_7TH                      0xD8
#define VIC_CB_8TH                      0xDC
#define VIC_CB2_1ST                     0xE0
#define VIC_CB2_2ND                     0xE4
#define VIC_CB2_3RD                     0xE8
#define VIC_CB2_4TH                     0xEC
#define VIC_CB2_5TH                     0xF0
#define VIC_CB2_6TH                     0xF4
#define VIC_CB2_7TH                     0xF8
#define VIC_CB2_8TH                     0xFC

#define VIC_DMA_OUTPUT_MAX_WIDTH                    3840
#define VIC_ADDR_VIC_CTRL                           0x0000
#define VIC_ADDR_VIC_RES                            0x0004
#define VIC_ADDR_VIC_FRM_ECC                        0x0008
#define VIC_ADDR_VIC_IN_INTF                        0x000c
#define VIC_ADDR_VIC_IN_DVP                         0x0010
#define VIC_ADDR_VIC_IN_CSI_FMT                     0x0014
#define VIC_ADDR_VIC_IN_HOR_PARA0                   0x0018
#define VIC_ADDR_VIC_IN_HOR_PARA1                   0x001c
#define VIC_ADDR_VIC_IN_VER_PARA0                   0x0030
#define VIC_ADDR_VIC_IN_VER_PARA1                   0x0034
#define VIC_ADDR_VIC_IN_VER_PARA2                   0x0038
#define VIC_ADDR_VIC_IN_VER_PARA3                   0x003c
#define VIC_ADDR_VIC_VLD_LINE_SAV                   0x0060
#define VIC_ADDR_VIC_VLD_LINE_EAV                   0x0064
#define VIC_ADDR_VIC_VLD_FRM_SAV                    0x0070
#define VIC_ADDR_VIC_VLD_FRM_EAV                    0x0074
#define VIC_ADDR_VIC_VC_CONTROL                     0x008c
#define VIC_ADDR_VIC_VC_CONTROL_CH0_PIX             0x0090
#define VIC_ADDR_VIC_VC_CONTROL_CH1_PIX             0x0094
#define VIC_ADDR_VIC_VC_CONTROL_CH2_PIX             0x0098
#define VIC_ADDR_VIC_VC_CONTROL_CH3_PIX             0x009c
#define VIC_ADDR_VIC_VC_CONTROL_CH0_LINE            0x00a0
#define VIC_ADDR_VIC_VC_CONTROL_CH1_LINE            0x00a4
#define VIC_ADDR_VIC_VC_CONTROL_CH2_LINE            0x00a8
#define VIC_ADDR_VIC_VC_CONTROL_CH3_LINE            0x00ac
#define VIC_ADDR_VIC_VC_CONTROL_FIFO_USE            0x00b0
#define VIC_ADDR_MIPI_ALL_WIDTH_4BYTE               0x0100
#define VIC_ADDR_MIPI_VCROP_DEL01                   0x0104
#define VIC_ADDR_MIPI_VCROP_DEL23                   0x0108
#define VIC_ADDR_MIPI_SENSOR_CONTROL                0x010c
#define VIC_ADDR_MIPI_HCROP_CH0                     0x0110
#define VIC_ADDR_MIPI_HCROP_CH1                     0x0114
#define VIC_ADDR_MIPI_HCROP_CH2                     0x0118
#define VIC_ADDR_MIPI_HCROP_CH3                     0x011c
#define MIPI_VCROP_SHADOW_CFG                       0x0120
#define VIC_ADDR_VIC_SAFE_END                       0x0128
#define VIC_ADDR_VC_CONTROL_CONTROL                 0x01a0
#define VIC_ADDR_VC_CONTROL_DELEY                   0x01a4
#define VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE           0x01a8
#define VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE            0x01ac
#define VIC_ADDR_VC_CONTROL_DELEY_BLK               0x01b0
#define VIC_ADDR_VC_CONTROL_LIMIT                   0x01b4
#define VIC_ADDR_VIC_INT_STATU                      0x01e0  // VIC_ADDR_VIC_INT_STATU
#define VIC_ADDR_VIC_INT_STATU2                     0x01e4
#define VIC_ADDR_VIC_INT_MASK                       0x01e8
#define VIC_ADDR_VIC_INT_MASK2                      0x01ec
#define VIC_ADDR_VIC_INT_CLR                        0x01f0
#define VIC_ADDR_VIC_INT_CLR2                       0x01f4

#define VIC_ADDR_DMA_CONFIGURE                      0x0300
#define VIC_ADDR_DMA_RESOLUTION                     0x0304
#define VIC_ADDR_DMA_RESET                          0x0308
#define VIC_ADDR_DMA_Y_CH_STRIDE                    0x0310
#define VIC_ADDR_DMA_UV_CH_STRIDE                   0x0314
#define VIC_ADDR_DMA_Y_CH0_BANK0_ADDR               0x0318
#define VIC_ADDR_DMA_Y_CH0_BANK1_ADDR               0x031c
#define VIC_ADDR_DMA_Y_CH0_BANK2_ADDR               0x0320
#define VIC_ADDR_DMA_Y_CH0_BANK3_ADDR               0x0324
#define VIC_ADDR_DMA_Y_CH0_BANK4_ADDR               0x0328
#define VIC_ADDR_DMA_UV_CH0_BANK0_ADDR              0x032c
#define VIC_ADDR_DMA_UV_CH0_BANK1_ADDR              0x0330
#define VIC_ADDR_DMA_UV_CH0_BANK2_ADDR              0x0334
#define VIC_ADDR_DMA_UV_CH0_BANK3_ADDR              0x0338
#define VIC_ADDR_DMA_UV_CH0_BANK4_ADDR              0x033c
#define VIC_ADDR_DMA_Y_CH1_BANK0_ADDR               0x0340
#define VIC_ADDR_DMA_Y_CH1_BANK1_ADDR               0x0344
#define VIC_ADDR_DMA_Y_CH1_BANK2_ADDR               0x0348
#define VIC_ADDR_DMA_Y_CH1_BANK3_ADDR               0x034c
#define VIC_ADDR_DMA_Y_CH1_BANK4_ADDR               0x0350
#define VIC_ADDR_DMA_UV_CH1_BANK0_ADDR              0x0354
#define VIC_ADDR_DMA_UV_CH1_BANK1_ADDR              0x0358
#define VIC_ADDR_DMA_UV_CH1_BANK2_ADDR              0x035c
#define VIC_ADDR_DMA_UV_CH1_BANK3_ADDR              0x0360
#define VIC_ADDR_DMA_UV_CH1_BANK4_ADDR              0x0364
#define VIC_ADDR_DMA_GET_ADD_ADDR                   0x0370
#define VIC_ADDR_DMA_Y_CH0_ADDR                     0x0380
#define VIC_ADDR_DMA_UV_CH0_ADDR                    0x0384
#define VIC_ADDR_DMA_Y_CH1_ADDR                     0x0388
#define VIC_ADDR_DMA_UV_CH1_ADDR                    0x038c
#define VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE      0x01c0

#define INPUT_MXU_MIPI_EN                           (1 << 0)
#define INPUT_MXU_OUT                               (1 << 1)
#define INPUT_MXU_DVP_EN                            (1 << 4)


#endif