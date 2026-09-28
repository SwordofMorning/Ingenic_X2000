/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera Driver for the Ingenic VIC controller
 *
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>

#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>

#include "camera_gpio.h"
#include "vic.h"
#include "csi.h"
#include "dsys.h"
#include "../isp/vic_channel_tiziano.h"
#include "../vic/vic_channel_mem.h"

struct jz_camera_data {
    int index;
    int is_enable;
    int is_finish;
    int mclk_io;                    /* MCLK输出管脚选择: PC15 / PA15 */

    int is_isp_enable;              /* ISP Enable功能
                                     * =1, VIC ---> ISP
                                     * =0, VIC ---> DDR
                                     */

    const char *isp_ahb0_clk_name;
    const char *isp_gate_clk_name;
    const char *isp_ispm_clk_name;
    const char *isp_ispa_clk_name;
    const char *isp_isps_clk_name;

    struct clk *mclk_div;
    struct clk *cim_gate_clk;

    struct clk *isp_ahb0_clk;
    struct clk *isp_gate_clk;
    struct clk *isp_ispm_clk;       /* isp work clk */
    struct clk *isp_ispa_clk;       /* axi bus clk */
    struct clk *isp_isps_clk;       /* isp scaler clk */
    unsigned long isp_clk_rate;

    /* Camera Device */
    char *device_name;              /* 设备节点名字 */
    unsigned int cam_mem_cnt;       /* 循环buff个数(针对VIC MEM有效, 经过ISP该参数无效) */
    struct camera_device camera;

    struct mutex lock;
    struct spinlock spinlock;
};

static struct jz_camera_data jz_camera_dev[1] = {
    /* VIC */
    {
        .index                  = 0,
        .is_enable              = 1,
        .mclk_io                = -1,       //PA15

        .isp_ahb0_clk_name      = "gate_ahb0",
        .isp_gate_clk_name      = "gate_isp",

        .isp_ispm_clk_name      = "div_ispm",
        .isp_isps_clk_name      = "div_isps",
        .isp_ispa_clk_name      = "div_ispa",

        .is_isp_enable          = 0,
        .cam_mem_cnt            = 1,
    },
};

/* VIC Controller */
module_param_named(vic_is_enable,      jz_camera_dev[0].is_enable,    int, 0644);
module_param_named(vic_is_isp_enable,  jz_camera_dev[0].is_isp_enable,int, 0444);
module_param_named(vic_frame_nums,     jz_camera_dev[0].cam_mem_cnt,  int, 0644);
module_param_gpio_named(vic_mclk_io,   jz_camera_dev[0].mclk_io, 0644);

/*
 * x2580输入为raw8时控制器可以直接输出raw8
*/
unsigned int is_output_y8(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        return ( !jz_camera_dev[index].is_isp_enable && \
                attr->dvp.data_fmt == DVP_RAW8 &&       \
                (sensor_fmt_is_8BIT(attr->sensor_info.fmt)) );

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        return ( !jz_camera_dev[index].is_isp_enable && \
                attr->mipi.data_fmt == MIPI_RAW8 &&     \
                (sensor_fmt_is_8BIT(attr->sensor_info.fmt)) );

    return 0;
}

/*
 * 输入为yuv422时 DMA控制器可以重新排列输出的顺序,
 * 所以YUV422输入可以选择输出NV12/NV21/Grey格式
*/
unsigned int is_output_yuv422(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        return ( !jz_camera_dev[index].is_isp_enable &&             \
                (sensor_fmt_is_YUV422(attr->sensor_info.fmt)) &&    \
                (attr->dvp.data_fmt == DVP_YUV422 || attr->dvp.data_fmt == DVP_YUV422_8BIT) );

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        return ( !jz_camera_dev[index].is_isp_enable &&     \
                attr->mipi.data_fmt == MIPI_YUV422 &&       \
                (sensor_fmt_is_YUV422(attr->sensor_info.fmt)) );

    return 0;
}

static void vic_start(int index)
{
    /* start vic 控制器 */
    vic_set_bit(index, VIC_ADDR_VIC_CTRL, VIC_START, 1);
}

void vic_reset(unsigned int index)
{
    /* reset vic 控制器 */
    vic_set_bit(index, VIC_ADDR_VIC_CTRL, VIC_GLB_RST, 1);
}

static void vic_dma_reset(int index)
{
    /* reset dma */
    vic_write_reg(index, VIC_ADDR_DMA_RESET, 1);
}

static void vic_register_enable(int index)
{
    /* VIC 开始初始化 */
    vic_set_bit(index ,VIC_ADDR_VIC_CTRL, VIC_REG_ENABLE, 1);
#if 0
    int timeout = 3000;
    while (vic_get_bit(index ,VIC_ADDR_VIC_CTRL, VIC_REG_ENABLE)) {
        if (--timeout == 0) {
            printk(KERN_ERR "timeout while wait vic_reg_enable: %x\n", vic_read_reg(index, VIC_ADDR_VIC_CTRL));
            break;
        }
    }
#endif
}

static void vic_data_path_select_route(int index, int route)
{
    if (route) {
        /* ISP Route */
        // vic_write_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4440);
        // vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4404);
    } else {
        /* DMA Route */
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4440);
        // vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4404);

    }
}


static void vic_init_dvp_timing(int index, struct sensor_attr *attr)
{
    unsigned long vic_input_dvp = vic_read_reg(index, VIC_ADDR_VIC_IN_DVP);
    unsigned long yuv_data_order = attr->dvp.yuv_data_order;

    if (attr->dvp.data_fmt <= DVP_RAW10)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, attr->dvp.data_fmt);
    else if (attr->dvp.data_fmt == DVP_YUV422)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, 6); // YUV422(8bit IO)

    set_bit_field(&vic_input_dvp, YUV_DATA_ORDER, yuv_data_order);
    set_bit_field(&vic_input_dvp, DVP_TIMING_MODE, attr->dvp.timing_mode);
    set_bit_field(&vic_input_dvp, HSYNC_POLAR, attr->dvp.hsync_polarity);
    set_bit_field(&vic_input_dvp, VSYNC_POLAR, attr->dvp.vsync_polarity);
    set_bit_field(&vic_input_dvp, INTERLACE_EN, attr->dvp.img_scan_mode);

    set_bit_field(&vic_input_dvp, DVP_RAW_ALIGN, 0);

    vic_write_reg(index, VIC_ADDR_VIC_IN_DVP, vic_input_dvp);

    unsigned long vic_ctrl_delay;
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_DELEY_hdeley, 1);
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_DELEY_vdeley, 1);
    vic_write_reg(index, VIC_ADDR_VC_CONTROL_DELEY, vic_ctrl_delay);
}

static int mipi_hvcrop_settings(int index, struct sensor_attr *attr)
{
    unsigned long hcrop_ch0 =  0x0;
    unsigned long vcrop_ch0 =  0x0;

    set_bit_field(&hcrop_ch0, MIPI_HCROP_CH0_all_image_width, attr->sensor_info.width);
    set_bit_field(&hcrop_ch0, MIPI_HCROP_CH0_start_pixel, attr->mipi.mipi_crop.hcrop_start);
    vic_write_reg(index, VIC_ADDR_MIPI_HCROP_CH0, hcrop_ch0);

    set_bit_field(&vcrop_ch0, MIPI_VCROP_CH0_start_pixel, attr->mipi.mipi_crop.vcrop_start);
    set_bit_field(&vcrop_ch0, MIPI_VCROP_CH0_all_image_width, 0);
    vic_write_reg(index, VIC_ADDR_MIPI_VCROP_DEL01, vcrop_ch0);
    set_bit_field(&vcrop_ch0, MIPI_VCROP_CH0_start_pixel, attr->mipi.mipi_crop.vcrop_start);
    set_bit_field(&vcrop_ch0, MIPI_VCROP_CH0_all_image_width, 0);
    vic_write_reg(index, VIC_ADDR_MIPI_VCROP_DEL23, vcrop_ch0);

    return 0;
}

static void vic_init_mipi_timing(int index, struct sensor_attr *attr)
{
    int width_4byte;
    int pixel_wdith;
    unsigned long horizontal_resolution;
    unsigned long sensor_control = 0x0;

    if (attr->mipi.mipi_crop.enable) {
        horizontal_resolution = attr->mipi.mipi_crop.output_width;
    } else {
        horizontal_resolution = attr->sensor_info.width;
    }

    vic_write_reg(index, VIC_ADDR_VIC_IN_CSI_FMT, attr->mipi.data_fmt);

    switch (attr->mipi.data_fmt) {
    case MIPI_RAW8:
        pixel_wdith = 8;
        break;
    case MIPI_RAW10:
        pixel_wdith = 10;
        break;
    case MIPI_RAW12:
        pixel_wdith = 12;
        break;
    default:
        pixel_wdith = 8;
        break;
    }

    /* 每行前有0个无效像素, 每行之后有0个无效像素 */
    width_4byte = ((horizontal_resolution + 0 + 0) * pixel_wdith + 31) / 32;
    vic_write_reg(index, VIC_ADDR_MIPI_ALL_WIDTH_4BYTE, width_4byte);

    vic_write_reg(index, VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE, 1);    //MIPI

    set_bit_field(&sensor_control, HCROP_DIFF_EN, attr->mipi.mipi_crop.sensor_ctrl.hcrop_diff_en);
    set_bit_field(&sensor_control, MIPI_VCOMP_EN, attr->mipi.mipi_crop.sensor_ctrl.mipi_vcomp_en);
    set_bit_field(&sensor_control, MIPI_HCOMP_EN, attr->mipi.mipi_crop.sensor_ctrl.mipi_hcomp_en);
    set_bit_field(&sensor_control, LINE_SYNC_MODE, attr->mipi.mipi_crop.sensor_ctrl.line_sync_mode);
    set_bit_field(&sensor_control, WORK_START_FLAG, attr->mipi.mipi_crop.sensor_ctrl.work_start_flag);
    set_bit_field(&sensor_control, DATA_TYPE_EN, attr->mipi.mipi_crop.sensor_ctrl.data_type_en);
    set_bit_field(&sensor_control, DATA_TYPE_VALUE, attr->mipi.mipi_crop.sensor_ctrl.data_type_value);
    set_bit_field(&sensor_control, DEL_START, attr->mipi.mipi_crop.sensor_ctrl.del_start);
    set_bit_field(&sensor_control, SENSOR_FRAME_NUM, attr->sensor_info.frame_mode);
    set_bit_field(&sensor_control, SENSOR_FID_MODE, attr->mipi.mipi_crop.sensor_ctrl.sensor_fid_mode);
    set_bit_field(&sensor_control, SENSOR_MODE, attr->vc_mode);

    vic_write_reg(index, VIC_ADDR_MIPI_SENSOR_CONTROL, sensor_control);

    mipi_hvcrop_settings(index, attr);

    if(attr->sensor_info.frame_mode == SENSOR_DEFAULT_FRAME_MODE){
        vic_write_reg(index,VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4440);
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4404);
    } else if (attr->sensor_info.frame_mode == SENSOR_WDR_2_FRAME_MODE){
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4410);
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4410);
    } else if (attr->sensor_info.frame_mode == SENSOR_WDR_3_FRAME_MODE){
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4420);
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4420);
    } else {
        printk("[ %s:%d ] Can not support this frame mode!!!\n", __func__, __LINE__);
    }

    vic_write_reg(index, VIC_ADDR_VC_CONTROL_DELEY, (10 << 16) | (10));
    vic_write_reg(index, VIC_ADDR_VC_CONTROL_DELEY_BLK, 0x10);

    vic_write_reg(index, VIC_ADDR_VIC_CTRL, (0x1 << 1));    //vic enabel
    vic_write_reg(index, VIC_ADDR_VIC_CTRL, (0x1 << 2));    //vic glb rst

    if(attr->sensor_info.wdr_en) {
        unsigned long vc_control;
        set_bit_field(&vc_control, SENSOR_FRAME_NUM, attr->sensor_info.data_type);
        set_bit_field(&vc_control, SENSOR_MODE, attr->vc_mode);
        vic_write_reg(index, VIC_ADDR_VC_CONTROL_CONTROL, vc_control);
    }

    vic_write_reg(index, VIC_ADDR_VIC_CTRL, 0x1);    //vic start
}

static void vic_init_common_setting(int index, struct sensor_attr *attr)
{
    unsigned long resolution = 0;
    unsigned long horizontal_resolution;
    unsigned long vertical_resolution;

    if ( (attr->dbus_type == SENSOR_DATA_BUS_MIPI) && (attr->mipi.mipi_crop.enable) ) {
        horizontal_resolution = attr->mipi.mipi_crop.output_width;
        vertical_resolution =  attr->mipi.mipi_crop.output_height;
    } else {
        horizontal_resolution = attr->sensor_info.width;
        vertical_resolution =  attr->sensor_info.height;
    }

    set_bit_field(&resolution, HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&resolution, VERTICAL_RESOLUTION, vertical_resolution);
    vic_write_reg(index ,VIC_ADDR_VIC_RES, resolution);

    int vic_interface = 0;
    switch (attr->dbus_type) {
    case SENSOR_DATA_BUS_BT656:
        vic_interface = 0;
        break;
    case SENSOR_DATA_BUS_BT601:
        vic_interface = 1;
        break;
    case SENSOR_DATA_BUS_MIPI:
        vic_interface = 2;
        break;
    case SENSOR_DATA_BUS_DVP:
        vic_interface = 3;
        break;
    case SENSOR_DATA_BUS_BT1120:
        vic_interface = 4;
        break;
    default:
        printk(KERN_ERR "vic unknown dbus_type: %d\n", attr->dbus_type);
    }
    vic_write_reg(index ,VIC_ADDR_VIC_IN_INTF, vic_interface);
}

static void init_dvp_dma(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution = attr->sensor_info.width;

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, attr->sensor_info.height);
    vic_write_reg(index ,VIC_ADDR_DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;
    unsigned int horizon_time = 0;

    switch (attr->dvp.data_fmt) {
    case DVP_RAW8:
        base_mode = 1;
        y_stride = attr->sensor_info.width;
        horizon_time = attr->sensor_info.width;
        break;
    case DVP_RAW10:
        base_mode = 0;
        y_stride = attr->sensor_info.width * 2;
        horizon_time = attr->sensor_info.width;
        break;

    case DVP_YUV422:
        if (attr->info.data_fmt == CAMERA_PIX_FMT_GREY) {
            base_mode = 6;
            y_stride = attr->sensor_info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 6;
            uv_stride = attr->sensor_info.width;
            y_stride = attr->sensor_info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 7;
            uv_stride = attr->sensor_info.width;
            y_stride = attr->sensor_info.width;
        } else {
            base_mode = 3;
            y_stride = attr->sensor_info.width * 2;
        }

        horizon_time = attr->sensor_info.width * 2;
        break;

    default:
        break;
    }

    vic_set_bit(index, VIC_ADDR_VIC_IN_HOR_PARA0, HACT_NUM, horizon_time);
    vic_write_reg(index, VIC_ADDR_DMA_Y_CH_STRIDE, y_stride);
    vic_write_reg(index, VIC_ADDR_DMA_UV_CH_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(index ,VIC_ADDR_DMA_CONFIGURE);
    // set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);
    set_bit_field(&dma_configure, Yuv422_order, 2);
    vic_write_reg(index ,VIC_ADDR_DMA_CONFIGURE, dma_configure);

    /* default DMA Route */
    vic_data_path_select_route(index, drv->is_isp_enable);
}

static void init_mipi_dma(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct camera_device *camera = &drv->camera;

    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution;
    unsigned long vertical_resolution;

    if (!camera->sensor->mipi.mipi_crop.enable) {
        horizontal_resolution = camera->sensor->sensor_info.width;
        vertical_resolution = camera->sensor->sensor_info.height;
    } else {
        horizontal_resolution = camera->sensor->mipi.mipi_crop.output_width;
        vertical_resolution = camera->sensor->mipi.mipi_crop.output_height;
    }

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, vertical_resolution);
    vic_write_reg(index ,VIC_ADDR_DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;

    switch (attr->mipi.data_fmt) {
    case MIPI_RAW8:
        base_mode = 1;  /* raw8 */
        y_stride = horizontal_resolution;
        break;
    case MIPI_RAW10:
    case MIPI_RAW12:
        base_mode = 0;
        y_stride = horizontal_resolution * 2;
        break;
    case MIPI_YUV422:
        if (attr->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 7; //maybe spec is err
            uv_stride = attr->info.width;
            y_stride = attr->info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 6;
            uv_stride = attr->info.width;
            y_stride = attr->info.width;
        } else {
            base_mode = 3;
            y_stride = attr->info.width * 2;
        }

    default:
        break;
    }

    vic_write_reg(index ,VIC_ADDR_DMA_Y_CH_STRIDE, y_stride);
    vic_write_reg(index ,VIC_ADDR_DMA_UV_CH_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(index, VIC_ADDR_DMA_CONFIGURE);
    //set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);

    vic_write_reg(index ,VIC_ADDR_DMA_CONFIGURE, dma_configure);

    /* default DMA Route */
    vic_data_path_select_route(index, drv->is_isp_enable);
}

static void init_dvp_irq(int index)
{
    unsigned long vic_int_mask = 0;

    set_bit_field(&vic_int_mask, VIC_FRM_START, 1);
    set_bit_field(&vic_int_mask, VIC_FRM_RST, 1);
    // set_bit_field(&vic_int_mask, VIC_HVF_ERR, 0);
    // set_bit_field(&vic_int_mask, VIC_DVP_HCOMP_ERR, 0);

    vic_write_reg(index, VIC_ADDR_VIC_INT_CLR, vic_int_mask);
    vic_write_reg(index, VIC_ADDR_VIC_INT_MASK, vic_int_mask);
}

static void init_mipi_irq(int index)
{
    unsigned long vic_int_mask = 0xFFFFF;
    unsigned long vic_int_mask2 = 0xFFFFF;

    // set_bit_field(&vic_int_mask, VIC_DONE, 0);
    set_bit_field(&vic_int_mask, VIC_MIPI_VCOMP_ERR, 0);
    set_bit_field(&vic_int_mask, VIC_MIPI_HCOMP_ERR, 0);
    set_bit_field(&vic_int_mask, VIC_HV_ERR, 0);
    set_bit_field(&vic_int_mask, VIC_FRM_START, 0);
    set_bit_field(&vic_int_mask, VIC_FRM_DONE, 0);
    set_bit_field(&vic_int_mask2, DMA_FRD, 0);

    vic_write_reg(index, VIC_ADDR_VIC_INT_CLR, vic_int_mask);
    vic_write_reg(index, VIC_ADDR_VIC_INT_MASK, vic_int_mask);

    vic_write_reg(index, VIC_ADDR_VIC_INT_CLR2, vic_int_mask2);
    vic_write_reg(index, VIC_ADDR_VIC_INT_MASK2, vic_int_mask2);
}

static int vic_isp_div_clock_enable(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    if ( !__clk_is_enabled(drv->isp_ispm_clk) ) {
        clk_set_rate(drv->isp_ispm_clk, drv->isp_clk_rate);

    } else  if (drv->isp_clk_rate != clk_get_rate(drv->isp_ispm_clk)) {
        printk(KERN_ERR "vic already enable isp clock(%ld) not change to %ld\n",  \
                clk_get_rate(drv->isp_ispm_clk), drv->isp_clk_rate);
    }
    clk_enable(drv->isp_ispm_clk);

    return 0;
}

static int vic_isp_div_clock_disable(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    clk_disable(drv->isp_ispm_clk);

    return 0;
}

void vic_dump_reg(int index)
{
    printk("==========dump vic register============\n");

    printk("VIC_ADDR_VIC_CTRL           : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_CTRL));
    printk("VIC_ADDR_VIC_RES            : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_RES));
    printk("VIC_ADDR_VIC_FRM_ECC        : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_FRM_ECC));
    printk("VIC_ADDR_VIC_IN_INTF        : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_INTF));
    printk("VIC_ADDR_VIC_IN_DVP         : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_DVP));
    printk("VIC_ADDR_VIC_IN_CSI_FMT     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_CSI_FMT));
    printk("VIC_ADDR_VIC_IN_HOR_PARA0   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_HOR_PARA0));
    printk("VIC_ADDR_VIC_IN_HOR_PARA1   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_HOR_PARA1));
    printk("VIC_BK_CB_CTRL              : 0x%08x\n", vic_read_reg(index, VIC_BK_CB_CTRL));
    printk("VIC_BK_CB_BLK               : 0x%08x\n", vic_read_reg(index, VIC_BK_CB_BLK));
    printk("VIC_IN_VER_PARA0            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA0));
    printk("VIC_IN_VER_PARA1            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA1));
    printk("VIC_IN_VER_PARA2            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA2));
    printk("VIC_IN_VER_PARA3            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA3));
    printk("VIC_VLD_LINE_SAV            : 0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_SAV));
    printk("VIC_VLD_LINE_EAV            : 0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_EAV));
    printk("VIC_VLD_FRM_SAV             : 0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_SAV));
    printk("VIC_VLD_FRM_EAV             : 0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_EAV));
    printk("VIC_VC_CONTROL_FSM          : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FSM));
    printk("VIC_VC_CONTROL_CH0_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_PIX));
    printk("VIC_VC_CONTROL_CH1_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_PIX));
    printk("VIC_VC_CONTROL_CH2_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_PIX));
    printk("VIC_VC_CONTROL_CH3_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_PIX));
    printk("VIC_VC_CONTROL_CH0_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_LINE));
    printk("VIC_VC_CONTROL_CH1_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_LINE));
    printk("VIC_VC_CONTROL_CH2_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_LINE));
    printk("VIC_VC_CONTROL_CH3_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_LINE));
    printk("VIC_VC_CONTROL_FIFO_USE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FIFO_USE));
    printk("VIC_CB_1ST                  : 0x%08x\n", vic_read_reg(index, VIC_CB_1ST));
    printk("VIC_CB_2ND                  : 0x%08x\n", vic_read_reg(index, VIC_CB_2ND));
    printk("VIC_CB_3RD                  : 0x%08x\n", vic_read_reg(index, VIC_CB_3RD));
    printk("VIC_CB_4TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_4TH));
    printk("VIC_CB_5TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_5TH));
    printk("VIC_CB_6TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_6TH));
    printk("VIC_CB_7TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_7TH));
    printk("VIC_CB_8TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_8TH));
    printk("VIC_CB2_1ST                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_1ST));
    printk("VIC_CB2_2ND                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_2ND));
    printk("VIC_CB2_3RD                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_3RD));
    printk("VIC_CB2_4TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_4TH));
    printk("VIC_CB2_5TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_5TH));
    printk("VIC_CB2_6TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_6TH));
    printk("VIC_CB2_7TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_7TH));
    printk("VIC_CB2_8TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_8TH));
    printk("VIC_ADDR_MIPI_ALL_WIDTH_4BYTE        : 0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_ALL_WIDTH_4BYTE));
    printk("VIC_ADDR_MIPI_VCROP_DEL01            : 0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_VCROP_DEL01));
    printk("VIC_ADDR_MIPI_SENSOR_CONTROL         : 0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_SENSOR_CONTROL));
    printk("VIC_ADDR_MIPI_HCROP_CH0              : 0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_HCROP_CH0));
    printk("MIPI_VCROP_SHADOW_CFG       : 0x%08x\n", vic_read_reg(index, MIPI_VCROP_SHADOW_CFG));
    printk("VIC_ADDR_VC_CONTROL_LIMIT           : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_LIMIT));
    printk("VIC_ADDR_VC_CONTROL_DELEY           : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_DELEY));
    printk("VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE));
    printk("VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE       : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE));
    printk("VIC_ADDR_VIC_INT_STATU                 : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_STATU));
    printk("VIC_ADDR_VIC_INT_MASK                : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_MASK));
    printk("VIC_ADDR_VIC_INT_CLR                 : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_CLR));

    printk("DMA reg:\n");
    printk("VIC_ADDR_DMA_CONFIGURE      : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_CONFIGURE));
    printk("VIC_ADDR_DMA_RESOLUTION     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_RESOLUTION));
    printk("VIC_ADDR_DMA_RESET          : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_RESET));
    printk("VIC_ADDR_DMA_Y_CH_STRIDE    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH_STRIDE));
    printk("VIC_ADDR_DMA_UV_CH_STRIDE   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH_STRIDE));
    printk("VIC_ADDR_DMA_GET_ADD_ADDR   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_GET_ADD_ADDR));
    printk("VIC_ADDR_DMA_Y_CH0_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH0_ADDR));
    printk("VIC_ADDR_DMA_UV_CH0_ADDR    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH0_ADDR));
    printk("VIC_ADDR_DMA_Y_CH1_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH1_ADDR));
    printk("VIC_ADDR_DMA_UV_CH1_ADDR    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH1_ADDR));
    printk("VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE));
    printk("=========================================\n");
}

static void vic_dvp_init(int index, struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 2048);
    assert_range(attr->sensor_info.height, 1, 2048);
    assert_range(attr->dvp.data_fmt, DVP_RAW8, DVP_YUV422);
    assert(attr->dbus_type == SENSOR_DATA_BUS_DVP);
    assert(attr->dvp.timing_mode == DVP_HREF_MODE);

    vic_reset(index);

    vic_init_common_setting(index, attr);

    vic_init_dvp_timing(index, attr);

    vic_dma_reset(index);

    init_dvp_dma(index, attr);

    init_dvp_irq(index);

    vic_register_enable(index);

    vic_start(index);

    //vic_dump_reg(index);
}

static void vic_mipi_init(int index, struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 3840);
    assert_range(attr->sensor_info.height, 1, 4096);
    assert_range(attr->mipi.data_fmt, MIPI_RAW8, MIPI_YUV422);
    assert_range(attr->mipi.lanes, 1, 2);
    assert(attr->dbus_type == SENSOR_DATA_BUS_MIPI);

    vic_dma_reset(index);

    init_mipi_dma(index, attr);

    init_mipi_irq(index);

    int csi_ret = mipi_csi_phy_initialization(&attr->mipi);  // 1,init phy and stream on
    assert(csi_ret >= 0);

    vic_register_enable(index); // 2,vic enable; 3, wait vic enable

    vic_reset(index);

    vic_init_common_setting(index, attr);

    vic_init_mipi_timing(index, attr);  // 4, config vic register

    vic_start(index);   // 5, start vic

    // vic_dump_reg(index);
}

static void vic_hal_stream_on(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        vic_dvp_init(index, attr);
    else if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        vic_mipi_init(index, attr);
}

static void vic_hal_stream_off(int index, struct sensor_attr *attr)
{
    vic_reset(index);
    usleep_range(1000, 1000);

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        mipi_csi_phy_stop();
}

int vic_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    int ret = 0;

    /* 当前仅支持MIPI DVP */
    assert_range(attr->dbus_type, SENSOR_DATA_BUS_MIPI, SENSOR_DATA_BUS_DVP);

    mutex_lock(&drv->lock);

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "vic can't stream on when not power on\n");
        ret = -EINVAL;
        goto out;
    }

    /* vic & data bus init, put it back of sensor power_on to ensure mipi phy ready */
    vic_hal_stream_on(index, attr);

    /* sensor stream on */
    ret = attr->ops.stream_on();
    if (ret) {
        vic_hal_stream_off(index, attr);
        goto out;
    }

    drv->camera.is_stream_on = 1;

out:
    mutex_unlock(&drv->lock);
    return ret;
}

void vic_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    mutex_lock(&drv->lock);

    if (!drv->camera.is_stream_on) {
        printk(KERN_ERR "vic is already steam off\n");
        goto unlock;
    }

    /* vic & data bus deinit check */
    vic_hal_stream_off(index, attr);

    /* sensor stream off */
    attr->ops.stream_off();
    /* vic stream off */
    vic_write_reg(index, VIC_ADDR_VIC_SAFE_END, 0X4444);

    drv->camera.is_stream_on = 0;

unlock:
    mutex_unlock(&drv->lock);
}

int vic_power_on(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on) {
        printk(KERN_ERR "vic is already power on, no need power on again\n");
        goto unlock;
    }

    /* enable clock */
    if (attr->isp_clk_rate)
        drv->isp_clk_rate = attr->isp_clk_rate;

    vic_isp_div_clock_enable(index);
    usleep_range(1500, 1500);

    /* sensor power on */
    ret = attr->ops.power_on();
    if (ret != 0)
        goto unlock;

    drv->camera.is_power_on = 1;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void vic_power_off(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "vic is already power off\n");
        return ;
    }

    if (drv->camera.is_stream_on)
        vic_stream_off(index, drv->camera.sensor);

    mutex_lock(&drv->lock);

    /* sensor power off */
    attr->ops.power_off();

    /* disable clock */
    vic_isp_div_clock_disable(index);

    drv->camera.is_power_on = 0;

    mutex_unlock(&drv->lock);
}

/*
 * VIC && device power state
 * return =1: is power on
 *        =0: is power off
 */
int vic_power_state(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    return drv->camera.is_power_on;
}

/*
 * VIC && device stream state
 * return =1: is stream on
 *        =0: is stream off
 */
int vic_stream_state(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    return drv->camera.is_stream_on;
}


#ifdef SOC_CAMERA_DEBUG

ssize_t dsysfs_vic_dump_reg(int index, char *buf)
{
    char *p = buf;

    p += sprintf(p, "VIC REG:\n");
    p += sprintf(p, "\t VIC_ADDR_VIC_CTRL               :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_CTRL));
    p += sprintf(p, "\t VIC_ADDR_VIC_RES                :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_RES));
    p += sprintf(p, "\t VIC_ADDR_VIC_FRM_ECC            :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_FRM_ECC));
    p += sprintf(p, "\t VIC_ADDR_VIC_IN_INTF            :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_INTF));
    p += sprintf(p, "\t VIC_ADDR_VIC_IN_CSI_FMT         :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_CSI_FMT));
    p += sprintf(p, "\t VIC_ADDR_VIC_IN_HOR_PARA0       :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_HOR_PARA0));
    p += sprintf(p, "\t VIC_ADDR_VIC_IN_HOR_PARA1       :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_IN_HOR_PARA1));
    p += sprintf(p, "\t VIC_BK_CB_CTRL                  :0x%08x\n", vic_read_reg(index, VIC_BK_CB_CTRL));
    p += sprintf(p, "\t VIC_BK_CB_BLK                   :0x%08x\n", vic_read_reg(index, VIC_BK_CB_BLK));
    p += sprintf(p, "\t VIC_IN_VER_PARA0                :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA0));
    p += sprintf(p, "\t VIC_IN_VER_PARA1                :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA1));
    p += sprintf(p, "\t VIC_IN_VER_PARA2                :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA2));
    p += sprintf(p, "\t VIC_IN_VER_PARA3                :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA3));
    p += sprintf(p, "\t VIC_VLD_LINE_SAV                :0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_SAV));
    p += sprintf(p, "\t VIC_VLD_LINE_EAV                :0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_EAV));
    p += sprintf(p, "\t VIC_VLD_FRM_SAV                 :0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_SAV));
    p += sprintf(p, "\t VIC_VLD_FRM_EAV                 :0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_EAV));
    p += sprintf(p, "\t VIC_VC_CONTROL_FSM              :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FSM));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH0_PIX          :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH1_PIX          :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH2_PIX          :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH3_PIX          :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH0_LINE         :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH1_LINE         :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH2_LINE         :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH3_LINE         :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_FIFO_USE         :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FIFO_USE));
    p += sprintf(p, "\t VIC_CB_1ST                      :0x%08x\n", vic_read_reg(index, VIC_CB_1ST));
    p += sprintf(p, "\t VIC_CB_2ND                      :0x%08x\n", vic_read_reg(index, VIC_CB_2ND));
    p += sprintf(p, "\t VIC_CB_3RD                      :0x%08x\n", vic_read_reg(index, VIC_CB_3RD));
    p += sprintf(p, "\t VIC_CB_4TH                      :0x%08x\n", vic_read_reg(index, VIC_CB_4TH));
    p += sprintf(p, "\t VIC_CB_5TH                      :0x%08x\n", vic_read_reg(index, VIC_CB_5TH));
    p += sprintf(p, "\t VIC_CB_6TH                      :0x%08x\n", vic_read_reg(index, VIC_CB_6TH));
    p += sprintf(p, "\t VIC_CB_7TH                      :0x%08x\n", vic_read_reg(index, VIC_CB_7TH));
    p += sprintf(p, "\t VIC_CB_8TH                      :0x%08x\n", vic_read_reg(index, VIC_CB_8TH));
    p += sprintf(p, "\t VIC_CB2_1ST                     :0x%08x\n", vic_read_reg(index, VIC_CB2_1ST));
    p += sprintf(p, "\t VIC_CB2_2ND                     :0x%08x\n", vic_read_reg(index, VIC_CB2_2ND));
    p += sprintf(p, "\t VIC_CB2_3RD                     :0x%08x\n", vic_read_reg(index, VIC_CB2_3RD));
    p += sprintf(p, "\t VIC_CB2_4TH                     :0x%08x\n", vic_read_reg(index, VIC_CB2_4TH));
    p += sprintf(p, "\t VIC_CB2_5TH                     :0x%08x\n", vic_read_reg(index, VIC_CB2_5TH));
    p += sprintf(p, "\t VIC_CB2_6TH                     :0x%08x\n", vic_read_reg(index, VIC_CB2_6TH));
    p += sprintf(p, "\t VIC_CB2_7TH                     :0x%08x\n", vic_read_reg(index, VIC_CB2_7TH));
    p += sprintf(p, "\t VIC_CB2_8TH                     :0x%08x\n", vic_read_reg(index, VIC_CB2_8TH));
    p += sprintf(p, "\t VIC_ADDR_MIPI_ALL_WIDTH_4BYTE   :0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_ALL_WIDTH_4BYTE));
    p += sprintf(p, "\t VIC_ADDR_MIPI_VCROP_DEL01       :0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_VCROP_DEL01));
    p += sprintf(p, "\t VIC_ADDR_MIPI_SENSOR_CONTROL    :0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_SENSOR_CONTROL));
    p += sprintf(p, "\t VIC_ADDR_MIPI_HCROP_CH0         :0x%08x\n", vic_read_reg(index, VIC_ADDR_MIPI_HCROP_CH0));
    p += sprintf(p, "\t MIPI_VCROP_SHADOW_CFG           :0x%08x\n", vic_read_reg(index, MIPI_VCROP_SHADOW_CFG));
    p += sprintf(p, "\t VIC_ADDR_VC_CONTROL_LIMIT         :0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_LIMIT));
    p += sprintf(p, "\t VIC_ADDR_VC_CONTROL_DELEY         :0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_DELEY));
    p += sprintf(p, "\t VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE :0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE));
    p += sprintf(p, "\t VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE  :0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE));
    p += sprintf(p, "\t VIC_ADDR_VIC_INT_STATU            :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_STATU));
    p += sprintf(p, "\t VIC_ADDR_VIC_INT_MASK             :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_MASK));
    p += sprintf(p, "\t VIC_ADDR_VIC_INT_CLR              :0x%08x\n", vic_read_reg(index, VIC_ADDR_VIC_INT_CLR));
    p += sprintf(p, "DMA REG:\n");
    p += sprintf(p, "\t VIC_ADDR_DMA_CONFIGURE      : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_CONFIGURE));
    p += sprintf(p, "\t VIC_ADDR_DMA_RESOLUTION     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_RESOLUTION));
    p += sprintf(p, "\t VIC_ADDR_DMA_RESET          : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_RESET));
    p += sprintf(p, "\t VIC_ADDR_DMA_Y_CH_STRIDE    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH_STRIDE));
    p += sprintf(p, "\t VIC_ADDR_DMA_UV_CH_STRIDE   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH_STRIDE));
    p += sprintf(p, "\t VIC_ADDR_DMA_GET_ADD_ADDR   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_GET_ADD_ADDR));
    p += sprintf(p, "\t VIC_ADDR_DMA_Y_CH0_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH0_ADDR));
    p += sprintf(p, "\t VIC_ADDR_DMA_UV_CH0_ADDR    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH0_ADDR));
    p += sprintf(p, "\t VIC_ADDR_DMA_Y_CH1_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_Y_CH1_ADDR));
    p += sprintf(p, "\t VIC_ADDR_DMA_UV_CH1_ADDR    : 0x%08x\n", vic_read_reg(index, VIC_ADDR_DMA_UV_CH1_ADDR));
    p += sprintf(p, "\t VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE   : 0x%08x\n", vic_read_reg(index, VIC_ADDR_VC_CONTROL_INPUT_MUX_OUTROUTE));

    p += sprintf(p, "MIPI REG:\n");
    p += dsysfs_mipi_dump_reg(p);

    return p - buf;
}

int dsysfs_vic_show_sensor_info(int index, char *buf)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    char *p = buf;

    char *sensor_data_bus_type_to_str(sensor_data_bus_type type)
    {
        static char *type_str[SENSOR_DATA_BUS_BUTT] = {
            [SENSOR_DATA_BUS_MIPI] = "SENSOR_DBUS_MIPI",
            [SENSOR_DATA_BUS_DVP] = "SENSOR_DBUS_DVP",
            [SENSOR_DATA_BUS_BT601] = "SENSOR_DBUS_601",
            [SENSOR_DATA_BUS_BT656] = "SENSOR_DBUS_BT656",
            [SENSOR_DATA_BUS_BT1120] = "SENSOR_DBUS_BT1120",
        };

        if (type < SENSOR_DATA_BUS_BUTT)
            return type_str[type];
        return NULL;
    }

    p += sprintf(p, "name: %s\n", sensor->device_name);
    p += sprintf(p, "cbus addr: 0x%x\n", sensor->cbus_addr);
    p += sprintf(p, "dbus type: %s\n", sensor_data_bus_type_to_str(sensor->dbus_type));
    if (SENSOR_DATA_BUS_MIPI == sensor->dbus_type) {
        p += sprintf(p, "\t lanes: %d\n", sensor->mipi.lanes);
        p += sprintf(p, "\t clk: %d\n", sensor->mipi.clk);
    } else if (SENSOR_DATA_BUS_DVP == sensor->dbus_type) {
        p += sprintf(p, "\t gpio_mode: %d\n", sensor->dvp.gpio_mode);
        p += sprintf(p, "\t timing_mode: %d\n", sensor->dvp.timing_mode);
        p += sprintf(p, "\t hsync_polarity: %d\n", sensor->dvp.hsync_polarity);
        p += sprintf(p, "\t vsync_polarity: %d\n", sensor->dvp.vsync_polarity);
        p += sprintf(p, "\t img_scan_mode: %d\n", sensor->dvp.img_scan_mode);
    } else {
        //TODO
    }

    p += sprintf(p, "mclk_rate: %ld\n", clk_get_rate(drv->mclk_div));
    p += sprintf(p, "isp_clk_rate: %ld\n", clk_get_rate(drv->isp_ispm_clk));

    p += sprintf(p, "sensor info:\n");
    p += sprintf(p, "\t width: %d\n", sensor->sensor_info.width);
    p += sprintf(p, "\t height: %d\n", sensor->sensor_info.height);
    p += sprintf(p, "\t fmt: %d\n", sensor->sensor_info.fmt);
    p += sprintf(p, "\t fps: %d / %d\n", sensor->sensor_info.fps >> 16, sensor->sensor_info.fps & 0xffff);
    p += sprintf(p, "\t max fps: %d / %d\n", sensor->sensor_info.max_fps >> 16, sensor->sensor_info.max_fps & 0xffff);
    p += sprintf(p, "\t min fps: %d / %d\n", sensor->sensor_info.min_fps >> 16, sensor->sensor_info.min_fps & 0xffff);
    p += sprintf(p, "\t total_width: %d\n", sensor->sensor_info.total_width);
    p += sprintf(p, "\t total_height: %d\n", sensor->sensor_info.total_height);
    p += sprintf(p, "\t integration_time: %d\n", sensor->sensor_info.integration_time);
    p += sprintf(p, "\t min_integration_time: %d\n", sensor->sensor_info.min_integration_time);
    p += sprintf(p, "\t max_integration_time: %d\n", sensor->sensor_info.max_integration_time);
    p += sprintf(p, "\t one_line_expr_in_us: %d\n", sensor->sensor_info.one_line_expr_in_us);
    // p += sprintf(p, "\t again: %d\n", sensor->sensor_info.again);
    p += sprintf(p, "\t max_again: %d\n", sensor->sensor_info.max_again);
    // p += sprintf(p, "\t dgain: %d\n", sensor->sensor_info.dgain);
    p += sprintf(p, "\t max_dgain: %d\n", sensor->sensor_info.max_dgain);


    return p - buf;
}

int dsysfs_vic_ctrl(int index, char *buf)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (!strncmp(buf, "r_sen_reg", sizeof("r_sen_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        struct sensor_dbg_register reg;
        reg.reg = simple_strtoull(buf+sizeof("r_sen_reg"), NULL, 0);
        if (sensor->ops.get_register) {
            ret = sensor->ops.get_register(&reg);
            if (ret < 0) {
                printk(KERN_ERR "%s get_register fail\n", __func__);
                goto exit;
            }
            printk(KERN_ERR "r_sen_reg: 0x%llx 0x%llx \n", reg.reg, reg.val);
        } else {
            printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
            ret = -EINVAL;
            goto exit;
        }
    } else if (!strncmp(buf, "w_sen_reg", sizeof("w_sen_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        struct sensor_dbg_register reg;
        char *p = 0;
        reg.reg = simple_strtoull(buf+sizeof("w_sen_reg"), &p, 0);
        reg.val = simple_strtoull(p+1, NULL, 0);
        if (sensor->ops.set_register) {
            ret = sensor->ops.set_register(&reg);
            if (ret < 0) {
                printk(KERN_ERR "%s set_register fail\n", __func__);
                return ret;
            }
            printk(KERN_ERR "w_sen_reg: 0x%llx 0x%llx \n", reg.reg, reg.val);
        } else {
            printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
            ret = -EINVAL;
            goto exit;
        }
    } else if (!strncmp(buf, "r_vic_reg", sizeof("r_vic_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        unsigned int reg = simple_strtoul(buf+sizeof("r_vic_reg"), NULL, 0);
        printk(KERN_ERR "r_vic_reg: 0x%x 0x%x \n", reg, vic_read_reg(index, reg));
    } else if (!strncmp(buf, "w_vic_reg", sizeof("w_vic_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        char *p = 0;
        unsigned int reg = simple_strtoul(buf+sizeof("w_vic_reg"), &p, 0);
        unsigned int val = simple_strtoul(p+1, NULL, 0);
        vic_write_reg(index, reg, val);
        printk(KERN_ERR "w_vic_reg: 0x%x 0x%x \n", reg, val);
    } else {
        printk(KERN_ERR "%s, unknow cmd: %s\n", __func__, buf);
        printk(KERN_ERR "usage:\n");
        printk(KERN_ERR "1. snap raw\n");
        printk(KERN_ERR "\t\t echo \"snapraw\" > /sys/ispX/vic/ctrl\n");
        printk(KERN_ERR "2. rw sensor reg(rw reg val)\n");
        printk(KERN_ERR "\t\t echo \"r_sen_reg 0x55\" > /sys/ispX/vic/ctrl(/sys/vicX/ctrl)\n");
        printk(KERN_ERR "\t\t echo \"w_sen_reg 0x55 0xaa\" > /sys/ispX/vic/ctrl(/sys/vicX/ctrl)\n");
    }

exit:
    return ret;
}

#endif


static int jz_vic_resources_init(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    int ret;

    ret = camera_mclk_gpio_init(drv->mclk_io);
    if(ret < 0) {
        printk(KERN_ERR "vic driver mclk gpio init failed\n");
        return ret;
    }

    drv->mclk_div = clk_get(NULL, "div_cim");   /* VIC 可共用 */
    assert(!IS_ERR(drv->mclk_div));
    assert(!clk_prepare(drv->mclk_div));

    drv->isp_ahb0_clk = clk_get(NULL, drv->isp_ahb0_clk_name);
    assert(!IS_ERR(drv->isp_ahb0_clk));
    assert(!clk_prepare(drv->isp_ahb0_clk));
    clk_enable(drv->isp_ahb0_clk);

    drv->isp_gate_clk = clk_get(NULL, drv->isp_gate_clk_name);
    assert(!IS_ERR(drv->isp_gate_clk));
    assert(!clk_prepare(drv->isp_gate_clk));
    clk_enable(drv->isp_gate_clk);

    drv->isp_ispm_clk = clk_get(NULL, drv->isp_ispm_clk_name);
    assert(!IS_ERR(drv->isp_ispm_clk));
    assert(!clk_prepare(drv->isp_ispm_clk));
    clk_set_rate(drv->isp_ispm_clk, 300 * 1000 * 1000);
    clk_enable(drv->isp_ispm_clk);

    drv->isp_isps_clk = clk_get(NULL, drv->isp_isps_clk_name);
    assert(!IS_ERR(drv->isp_isps_clk));
    assert(!clk_prepare(drv->isp_isps_clk));
    clk_set_rate(drv->isp_isps_clk, 500 * 1000 * 1000);
    clk_enable(drv->isp_isps_clk);

    drv->isp_ispa_clk = clk_get(NULL, drv->isp_ispa_clk_name);
    assert(!IS_ERR(drv->isp_ispa_clk));
    assert(!clk_prepare(drv->isp_ispa_clk));
    clk_set_rate(drv->isp_ispa_clk, 500 * 1000 * 1000);
    clk_enable(drv->isp_ispa_clk);

    mutex_init(&drv->lock);

    if (drv->is_isp_enable) {
        ret = jz_vic_tiziano_drv_init(index);
    } else {
        ret = jz_vic_mem_drv_init(index);
    }

    if (ret) {
        printk(KERN_ERR "camera: failed to init vic resources\n");
        goto error_vic_resources_init;
    }

    drv->is_finish = 1;

    return 0;

error_vic_resources_init:
    clk_put(drv->mclk_div);
    clk_put(drv->isp_ahb0_clk);
    clk_put(drv->isp_gate_clk);
    clk_put(drv->isp_ispm_clk);
    clk_put(drv->isp_isps_clk);
    clk_put(drv->isp_ispa_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);

    return ret;
}

static void jz_vic_resources_deinit(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    drv->is_finish = 0;

    if (drv->is_isp_enable)
        jz_vic_tiziano_drv_deinit(index);
    else
        jz_vic_mem_drv_deinit(index);

    clk_put(drv->mclk_div);
    clk_put(drv->isp_ahb0_clk);
    clk_put(drv->isp_gate_clk);
    clk_put(drv->isp_ispm_clk);
    clk_put(drv->isp_isps_clk);
    clk_put(drv->isp_ispa_clk);

    camera_mclk_gpio_deinit(drv->mclk_io);
}

/*
 * VIC mclk, 使用index参数方便获取mclk的变量
 */
void camera_enable_sensor_mclk(int index, unsigned long clk_rate)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned long rate = 0;

    if ( !__clk_is_enabled(drv->mclk_div) ) {
        clk_set_rate(drv->mclk_div, clk_rate);
        clk_enable(drv->mclk_div);
        return ;
    }

    rate = clk_get_rate(drv->mclk_div);
    if (rate != clk_rate) {
        printk(KERN_ERR "mclk already enabled rate=%ld, not change to %ld\n", rate, clk_rate);
    }

    clk_enable(drv->mclk_div);
}

void camera_disable_sensor_mclk(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    clk_disable(drv->mclk_div);
}

int camera_register_sensor(int index, struct sensor_attr *sensor)
{
    assert(index < 1);

    struct jz_camera_data *drv = &jz_camera_dev[index];
    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    int ret = -EINVAL;

    switch (index) {
    case 0:
        /* VIC */
        if (drv->is_isp_enable)
            ret = vic_register_sensor_route_tiziano(index, sensor);
        else
            ret = vic_register_sensor_route_mem(index, drv->cam_mem_cnt, sensor);

        break;

    default:
        printk(KERN_ERR "camera register is invalid\n");
        break;
    }

    if (!ret) {
        drv->camera.sensor = sensor;
        drv->camera.is_power_on = 0;
        drv->camera.is_stream_on = 0;
    }

    return ret;
}

void camera_unregister_sensor(int index, struct sensor_attr *sensor)
{
    assert(index < 1);

    struct jz_camera_data *drv = &jz_camera_dev[index];
    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    switch (index) {
    case 0:
        /* VIC */
        if (drv->is_isp_enable)
            vic_unregister_sensor_route_tiziano(index, sensor);
        else
            vic_unregister_sensor_route_mem(index, sensor);

        break;

    default:
        printk(KERN_ERR "camera unregister is invalid\n");
        break;
    }

    printk(KERN_DEBUG "vic unregister successfully\n");

    drv->camera.sensor = NULL;
}

int jz_arch_vic_init(void)
{
    /* VIC */
    if (jz_camera_dev[0].is_enable)
        jz_vic_resources_init(jz_camera_dev[0].index);

    return 0;
}

void jz_arch_vic_exit(void)
{
    /* VIC */
    if (jz_camera_dev[0].is_finish)
        jz_vic_resources_deinit(jz_camera_dev[0].index);

}

EXPORT_SYMBOL(camera_enable_sensor_mclk);
EXPORT_SYMBOL(camera_disable_sensor_mclk);

EXPORT_SYMBOL(camera_register_sensor);
EXPORT_SYMBOL(camera_unregister_sensor);