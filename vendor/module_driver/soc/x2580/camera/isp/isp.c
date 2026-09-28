/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP Driver
 */

#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>
#include <linux/vmalloc.h>

#include <vic.h>
#include "tiziano-isp.h"
#include "camera_cpm.h"
#include "tiziano-map.h"
#include "isp.h"
#include "mscaler.h"
#include "isp_tuning.h"
#include "dsys.h"
#include "vic_channel_tiziano.h"
#include "tiziano-core-ctrl.h"
#include "drivers/rmem_manager/rmem_manager.h"
#include "tiziano_core_tuning.h"
#include "system_sensor_drv.h"
#include <x2580_isp_common.h>
#include "version_log.h"

#define IRQ_ISP                        (31)

static struct jz_isp_data jz_isp_dev[1] = {
    {
        .index                  =  0,
        .irq                    = IRQ_INTC_BASE + IRQ_ISP, /* BASE + 31 */
        .irq_name               = "ISP",
        .state[0]               = ISP_MODULE_DEINIT,
    },
};

/*
 * ISP Operation
 */
static const unsigned long isp_iobase[] = {
        KSEG1ADDR(ISP_IOBASE),
};

#define ISP_ADDR(index, reg)            ((volatile unsigned long *)((isp_iobase[index]) + (reg)))

static inline void isp_write_reg(int index, unsigned int reg, unsigned int val)
{
    *ISP_ADDR(index, reg) = val;
}

static inline unsigned int isp_read_reg(int index, unsigned int reg)
{
    return *ISP_ADDR(index, reg);
}

static inline void isp_set_bit(int index, unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(ISP_ADDR(index, reg), start, end, val);
}

static inline unsigned int isp_get_bit(int index, unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(ISP_ADDR(index, reg), start, end);
}

/*
 * interface used by isp-core
 */
int system_reg_write(unsigned int reg, unsigned int value)
{
    isp_write_reg(0, reg, value);

    return 0;
}
EXPORT_SYMBOL(system_reg_write);


unsigned int system_reg_read( unsigned int reg)
{
    return isp_read_reg(0, reg);
}
EXPORT_SYMBOL(system_reg_read);

int (*irq_func_cb[96])(int vinum) = {0};

int system_irq_func_set(int vinum, int irq, void *func)
{
        irq_func_cb[irq + vinum * 42] = func;

        return 0;
}
EXPORT_SYMBOL(system_irq_func_set);

/*
 * ISP Base Interface
 */
static inline void isp_dump_reg(int index)
{
    printk("==========dump isp%d register============\n", index);

    printk("TOP_ADDR_VERSION                    :0x%08x\n", isp_read_reg(index, TOP_ADDR_VERSION));
    printk("TOP_ADDR_TOP_CON                    :0x%08x\n", isp_read_reg(index, TOP_ADDR_TOP_CON            ));
    printk("TOP_ADDR_BYPASS                     :0x%08x\n", isp_read_reg(index, TOP_ADDR_BYPASS(index)         ));
    printk("TOP_ADDR_STATIC_EN                  :0x%08x\n", isp_read_reg(index, TOP_ADDR_STATIC_EN(index)         ));
    printk("TOP_ADDR_S0_FRM_SIZE                :0x%08x\n", isp_read_reg(index, TOP_ADDR_S0_FRM_SIZE            ));
    printk("TOP_ADDR_S1_FRM_SIZE                :0x%08x\n", isp_read_reg(index, TOP_ADDR_S1_FRM_SIZE          ));
    printk("TOP_ADDR_S0_BAYER_TYPE              :0x%08x\n", isp_read_reg(index, TOP_ADDR_S0_BAYER_TYPE         ));
    printk("TOP_ADDR_S1_BAYER_TYPE              :0x%08x\n", isp_read_reg(index, TOP_ADDR_S1_BAYER_TYPE            ));
    printk("TOP_ADDR_STOP_CON                   :0x%08x\n", isp_read_reg(index, TOP_ADDR_STOP_CON        ));
    printk("TOP_ADDR_STOP_STATE                 :0x%08x\n", isp_read_reg(index, TOP_ADDR_STOP_STATE         ));
    printk("TOP_ADDR_DMA_RD_CON                 :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_RD_CON        ));
    printk("TOP_ADDR_DMA_RD_INFO                :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_RD_INFO       ));
    printk("TOP_ADDR_DMA_WR_CON_0               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_0 ));
    printk("TOP_ADDR_DMA_WR_CON_1               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_1 ));
    printk("TOP_ADDR_DMA_WR_CON_2               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_2         ));
    printk("TOP_ADDR_DMA_WR_CON_3               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_3         ));
    printk("TOP_ADDR_DMA_WR_CON_4               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_4      ));
    printk("TOP_ADDR_DMA_WR_CON_5               :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_CON_5     ));
    printk("TOP_ADDR_DMA_WR_INFO_0              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_0       ));
    printk("TOP_ADDR_DMA_WR_INFO_1              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_1       ));
    printk("TOP_ADDR_DMA_WR_INFO_2              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_2    ));
    printk("TOP_ADDR_DMA_WR_INFO_3              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_3   ));
    printk("TOP_ADDR_DMA_WR_INFO_4              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_4             ));
    printk("TOP_ADDR_DMA_WR_INFO_5              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_5            ));
    printk("TOP_ADDR_DMA_WR_INFO_6              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_6            ));
    printk("TOP_ADDR_DMA_WR_INFO_7              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_7         ));
    printk("TOP_ADDR_DMA_WR_INFO_8              :0x%08x\n", isp_read_reg(index, TOP_ADDR_DMA_WR_INFO_8             ));
    printk("TOP_ADDR_SUSPEND_START              :0x%08x\n", isp_read_reg(index, TOP_ADDR_SUSPEND_START            ));
    printk("TOP_ADDR_SUSPEND_END                :0x%08x\n", isp_read_reg(index, TOP_ADDR_SUSPEND_END            ));
}

/*
 * ISP总线复位后mscaler,isp相关模块均被复位
 */
inline static int private_cpm_reset(unsigned int addr, unsigned int bit)
{
    int timeout = 500;
    unsigned int value = 0;

    value = *(volatile unsigned int*)(addr);
    value |= 1 << (bit - 1);
    *(volatile unsigned int*)(addr) = value;
    while(timeout && ((*(volatile unsigned int*)(addr)) & (1 << (bit - 2))) == 0){
        private_msleep(2);
        timeout--;
    }

    if(timeout == 0) {
        printk("cpm_reset timeout %d ... \n", timeout*2);
        return -1;
    }

    value = *(volatile unsigned int*)(addr);
    value &= ~(1 << (bit - 1));
    value |= 1 << bit;
    *(volatile unsigned int*)(addr) = value;

    value = *(volatile unsigned int*)(addr);
    value &= ~(1 << bit);
    *(volatile unsigned int*)(addr) = value;

    return 0;
}

inline static int tiziano_cpm_reset(int index)
{
    int timeout = 0xffffff;
    struct jz_isp_data *drv = &jz_isp_dev[index];

    if (drv->camera.is_power_on != 0) /* 同一ISP的不同通道只需reset一次 */
        return 0;

    switch (index) {
    case 0: {   /* ISP */
        /* stop request */
        cpm_set_bit(CPM_SRBC, CPM_SRBC_ISP_STOP_TRANSFER, 1);
        while ( !cpm_get_bit(CPM_SRBC, CPM_SRBC_ISP_STOP_ACK) && --timeout );
        if (timeout == 0) {
            printk(KERN_ERR "isp%d wait stop timeout\n", index);
            return  -ETIMEDOUT;
        }

        /* reset */
        unsigned long isp_reset = cpm_read_reg(CPM_SRBC);

        set_bit_field(&isp_reset, CPM_SRBC_ISP_STOP_TRANSFER, 0);
        set_bit_field(&isp_reset, CPM_SRBC_ISP_SOFT_RESET, 1);
        cpm_write_reg(CPM_SRBC, isp_reset);

        udelay(10);

        isp_reset = cpm_read_reg(CPM_SRBC);
        set_bit_field(&isp_reset, CPM_SRBC_ISP_SOFT_RESET, 0);
        cpm_write_reg(CPM_SRBC, isp_reset);

        break;
    }

    default:
        printk(KERN_ERR "isp%d out of range\n", index);
        break;
    }

    return 0;
}

static int isp_firmware_process(void *data)
{
    while(!kthread_should_stop()){
        tisp_fw_process(0);
    }

    return 0;
}

#define WDR_CHAN 2
static int isp_wdr_get_buf(int index, int width, int height)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    uint32_t bitdepth = 10;

    struct isp_buf_info bufinfo;
    bufinfo.paddr = 0;
    bufinfo.size = 0;
    if(drv->camera.sensor->sensor_info.fmt <= SENSOR_PIXEL_FMT_SRGGB12_1X12 && drv->camera.sensor->sensor_info.fmt <= SENSOR_PIXEL_FMT_SBGGR12_1X12)
        bitdepth = 12;
    else
        bitdepth =10;
    if(drv->camera.sensor->sensor_info.data_type == SENSOR_DATA_TYPE_WDR_FS){
            bufinfo.size = height * width * 2 * WDR_CHAN;
    }else if (drv->camera.sensor->sensor_info.data_type == SENSOR_DATA_TYPE_WDR_DOL){
            bufinfo.size = drv->camera.sensor->sensor_info.wdr_cache / 2;
            bufinfo.size = bufinfo.size * bitdepth / 8 + bufinfo.size / 32;

    }else
        printk("[ %s:%d ] Not the wdr mode, do not need to alloc buf!!!\n", __func__, __LINE__);

    return bufinfo.size;
}

static int isp_wdr_set_buf(int width, int height, uint32_t size, void *vaddr)
{
    struct isp_buf_info bufinfo;
    uint32_t vinum = 0;
    uint32_t tsize = 0;
    uint32_t end_point = 0;
    uint32_t bitdepth = 10;
    struct jz_isp_data *drv = &jz_isp_dev[vinum];

    bufinfo.paddr = virt_to_phys(vaddr);
    bufinfo.size = size;

    if(drv->camera.sensor->sensor_info.data_type == SENSOR_DATA_TYPE_WDR_FS){
            tsize = height * width * 2 * WDR_CHAN;
            end_point = height;
    } else if (drv->camera.sensor->sensor_info.data_type == SENSOR_DATA_TYPE_WDR_DOL){

            tsize = drv->camera.sensor->sensor_info.wdr_cache / 2;
            tsize = tsize * bitdepth / 8 + tsize / 32;
            end_point = tsize / (width * 2);
    } else {
            printk("[ %s:%d ] Not the wdr mode, do not need to alloc buf!!!\n", __func__, __LINE__);
            return -1;
    }

    if (tsize > bufinfo.size) {
            printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
            drv->wdr_en[vinum] = 0;
            tisp_s_wdr_en(vinum, 0);
            return -EFAULT;
    }

    system_reg_write(IPC_ADDR_DF_CH0_ADDR, bufinfo.paddr);
    system_reg_write(IPC_ADDR_DF_CH0_SIZE , tsize);
    system_reg_write(IPC_ADDR_DF_CH1_ADDR, bufinfo.paddr);
    system_reg_write(IPC_ADDR_DF_CH1_SIZE , tsize);
    system_reg_write(IPC_ADDR_DF_CHN_COMP_EN, 0x0);

    drv->wdr_en[vinum] = 1;

    return 0;
}

int isp_set_format(int sensor_width, int sensor_height){

    int ret = 0;
    tisp_channel_attr_t attr;
    memset(&attr, 0, sizeof(tisp_channel_attr_t));

    attr.fcrop_width = sensor_width;
    attr.fcrop_height = sensor_height;
    attr.scaler_width = attr.fcrop_width;
    attr.scaler_height = attr.fcrop_height;
    attr.crop_width = attr.scaler_width;
    attr.crop_height = attr.scaler_height;

    return ret;
}

static uint32_t isp_get_mdns_size(int width, int height) {

    uint32_t tsize = 0;
    uint32_t tmpsize = 0;
    uint32_t stride = 0;
    uint32_t mdns_res_end_point_y;
    uint32_t mdns_res_end_point_c;
    int isp_memopt = 0;
    int vinum = 0;

    //Ref Y
    stride = ((width + 15) / 16) * 16;
    tmpsize = stride * height + ((stride * height) >> 5) ;
    tmpsize = ((tmpsize + 1023) / 1024 * 1024);
    mdns_res_end_point_y = tmpsize / 256;
    tsize += tmpsize;

    //Ref UV
    stride = ((width + 31) / 32) * 32;
    tmpsize = stride*height / 2 + ((stride*height / 2)>>6);
    tmpsize = ((tmpsize + 1023) / 1024 * 1024);
    mdns_res_end_point_c = tmpsize / 256;
    tsize += tmpsize;

    //Bsn
    stride = (width  + 7) / 8;
    stride = (((stride + 1) / 2 + 7) / 8) * 8;
    tmpsize = (stride * height) / 8;
    tmpsize = ((tmpsize + 1023) / 1024 * 1024);
    tsize += tmpsize;

    //Ass
    isp_memopt = (isp_memopt >> (4*vinum)) & 0xf;
    if (isp_memopt == 1) {
            stride = ((width / 2 + 31) / 32) * 32;
            tmpsize = stride * height / 2;
            tmpsize = ((tmpsize + 1023) / 1024 * 1024);
    } else if(isp_memopt == 2) {
            tmpsize = 0;    /**< close Ass **/
    } else {
            stride = ((width + 31) / 32) * 32;
            tmpsize = stride * height / 2;
            tmpsize = ((tmpsize + 1023) / 1024 * 1024);
    }
    tsize += tmpsize;

    //Lynne
    if (isp_memopt < 1 || isp_memopt > 2) {
            stride = ((width + 31) / 32) * 16;
            tmpsize = stride * height / 4;
            tmpsize = ((tmpsize + 1023) / 1024 * 1024);
            tsize += tmpsize;
    }

    return tsize;
}

static int isp_set_mdns_buf(int width, int height, uint32_t size, void *vaddr)
{
        struct isp_buf_info bufinfo;
        uint32_t tsize = 0;
        uint32_t tmpsize = 0;
        uint32_t stride = 0;
        uint32_t mdns_res_end_point_y;
        uint32_t mdns_res_end_point_c;
        int isp_memopt = 0;

        int vinum = 0;
        bufinfo.paddr = virt_to_phys(vaddr);
        bufinfo.size = size;

        if(vinum < 2) {
            //Ref Y
            stride = ((width + 15) / 16) * 16;
            tmpsize = stride * height + ((stride * height) >> 5) ;
            tmpsize = ((tmpsize + 1023) / 1024 * 1024);
            mdns_res_end_point_y = tmpsize / 256;
            system_reg_write(MDNS_ADDR_YRESENDPOINT(vinum), mdns_res_end_point_y);
            system_reg_write(MDNS_ADDR_YREF_ADDR(vinum), bufinfo.paddr + tsize);
            system_reg_write(MDNS_ADDR_YREF_STRIDE(vinum), stride);
            tsize += tmpsize;
            if (tsize > bufinfo.size) {
                    printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
                    return -EFAULT;
            }

            //REF UV
            stride = ((width + 31) / 32) * 32;
            tmpsize = stride*height / 2 + ((stride*height / 2)>>6);
            tmpsize = ((tmpsize + 1023) / 1024 * 1024);
            mdns_res_end_point_c = tmpsize / 256;
            system_reg_write(MDNS_ADDR_CRESENDPOINT(vinum), mdns_res_end_point_c);
            system_reg_write(MDNS_ADDR_CREF_ADDR(vinum), bufinfo.paddr + tsize);
            system_reg_write(MDNS_ADDR_CREF_STRIDE(vinum), stride);
            tsize += tmpsize;
            if (tsize > bufinfo.size) {
                    printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
                    return -EFAULT;
            }

            //Bsn
            stride = (width  + 7) / 8;
            stride = (((stride + 1) / 2 + 7) / 8) * 8;
            tmpsize = (stride * height) / 8;
            tmpsize = ((tmpsize + 1023) /1024 * 1024);
            system_reg_write(MDNS_ADDR_BSN_ADDR(vinum), bufinfo.paddr + tsize);
            system_reg_write(MDNS_ADDR_BSN_STRIDE(vinum), stride);
            tsize += tmpsize;
            if (tsize > bufinfo.size) {
                    printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
                    return -EFAULT;
            }

            //Ass
            isp_memopt = (isp_memopt >> (4*vinum)) & 0xf;
            if (isp_memopt == 1) {
                    stride = ((width / 2 + 31) / 32) * 32;
                    tmpsize = stride * height / 2;
                    tmpsize = ((tmpsize + 1023) /1024 * 1024);
            } else if (isp_memopt == 2) {
                    tmpsize = 0;    /**< close Ass **/
            } else {
                    stride = ((width + 31) / 32) * 32;
                    tmpsize = stride * height / 2;
                    tmpsize = ((tmpsize + 1023) /1024 * 1024);
            }
            system_reg_write(MDNS_ADDR_ASS_ADDR(vinum), bufinfo.paddr + tsize);
            system_reg_write(MDNS_ADDR_ASS_STRIDE(vinum), stride);
            tsize += tmpsize;
            if (tsize > bufinfo.size) {
                    printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
                    return -EFAULT;
            }

            //Lynne
            if (isp_memopt < 1 || isp_memopt > 2) {
                    stride = ((width + 31) / 32) * 16;
                    tmpsize = stride * height / 4;
                    tmpsize = ((tmpsize + 1023) / 1024 * 1024);
                    system_reg_write(MDNS_ADDR_LYN_ADDR(vinum), bufinfo.paddr + tsize);
                    system_reg_write(MDNS_ADDR_LYN_STRIDE(vinum), stride);
                    tsize += tmpsize;
                    if (tsize > bufinfo.size) {
                            printk("[ %s:%d ] buf size too small\n", __func__, __LINE__);
                            return -EFAULT;
                    }
            }
        }

        return 0;
}

void *mdns_vaddr = NULL;
uint32_t mdns_size = 0;
void *wdr_vaddr = NULL;
uint32_t wdr_size = 0;

static int mdns_init(int width, int height)
{
    int ret = 0;

    mdns_size = isp_get_mdns_size(width, height);
    mdns_vaddr = rmem_alloc_aligned(mdns_size, PAGE_SIZE);
    if(!mdns_vaddr) {
        return -1;
    }

    ret = isp_set_mdns_buf(width, height, mdns_size, mdns_vaddr);
    if (ret)
        return -1;

    ret = isp_set_format(width, height);
    if (ret)
        return -1;

    return ret;
}

static int mdns_deinit(int width, int height)
{
    rmem_free(mdns_vaddr, mdns_size);

    return 0;
}

static  void isp_interrupts_enable(int index)
{
    /* isp interrupt */
    isp_write_reg(index, RESP_ADDR_INT_COMMON_0_EN, 0xffffffff); //common intp
    isp_write_reg(index, RESP_ADDR_INT_UNUSUAL_0_EN, 0xffffffff); //unusual intp
    /* ivdc need bit20 */
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_0_EN, 0xffffffff); //back0 intp
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_1_EN, 0x3f); //back1 intp
}

static  void isp_interrupts_disable(int index)
{
    if(index == 0){
        isp_write_reg(index, RESP_ADDR_INT_COMMON_0_EN, 0x0);
        isp_write_reg(index, RESP_ADDR_INT_UNUSUAL_0_EN, 0x0); //unusual intp
        isp_write_reg(index, RESP_ADDR_INT_BACKUP_0_EN, 0x0); //back0 intp
        isp_write_reg(index, RESP_ADDR_INT_BACKUP_1_EN, 0x0); //back1 intp
    } else if (index == 1){
        isp_write_reg(index, RESP_ADDR_INT_COMMON_1_EN, 0x0);
    }

    isp_write_reg(index, RESP_ADDR_INT_COMMON_0_CLR, 0xffffffff);
    isp_write_reg(index, RESP_ADDR_INT_UNUSUAL_0_CLR, 0xffffffff);
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_0_CLR, 0xffffffff);
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_1_CLR, 0xffffffff);
}

extern uint32_t isp_nv12_wbit;
tisp_init_param_t iparam;
static int tiziano_init(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;
    char file_name[64];
    tisp_module_control_t bypass_top;
    int ret;

    /*
     * ISP firmware parameter
     */
    memset(&iparam,0,sizeof(tisp_init_param_t));
    strncpy(iparam.sensor, attr->device_name, sizeof(iparam.sensor)); /* IQ file name */

    if ( (attr->dbus_type == SENSOR_DATA_BUS_MIPI) && (attr->mipi.mipi_crop.enable)) {
        iparam.width  = attr->mipi.mipi_crop.output_width;
        iparam.height = attr->mipi.mipi_crop.output_height;
    } else {
        iparam.width  = attr->sensor_info.width;
        iparam.height = attr->sensor_info.height;
    }

    //Set ISP ddr channel priority
    outl(0x0, 0x1301206c);
    outl(0x8840400f, 0x13012028);

    switch(attr->sensor_info.fmt) {
    case SENSOR_PIXEL_FMT_SBGGR8_1X8:
    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
        iparam.bayer = 2;   /* BGGR */
        break;

    case SENSOR_PIXEL_FMT_SGBRG8_1X8:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
        iparam.bayer = 3;   /* GBRG */
        break;

    case SENSOR_PIXEL_FMT_SGRBG8_1X8:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
        iparam.bayer = 1;   /* GRBG */
        break;

    case SENSOR_PIXEL_FMT_SRGGB8_1X8:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        iparam.bayer = 0;   /* RGGB */
        break;

    default:
        printk(KERN_ERR "%s, the input format(0x%08x) not support!\n", __func__, attr->sensor_info.fmt);
        return -EINVAL;
    }
    /* GRBG */
    uint32_t bayer = -1;
    bayer = 1 << 16;
    bayer += 2;
    iparam.bayer = bayer;

    iparam.sensorId = 0;
    iparam.width = attr->sensor_info.width;
    iparam.height = attr->sensor_info.height;
    strcpy(iparam.sensor, attr->device_name);

    iparam.sensor_info.max_again = attr->sensor_info.max_again;    //the format is .16
    iparam.sensor_info.max_dgain = attr->sensor_info.max_dgain;    //the format is .16
    iparam.sensor_info.again = attr->sensor_info.again;
    iparam.sensor_info.again_short = attr->sensor_info.again_short;
    iparam.sensor_info.dgain = attr->sensor_info.dgain;
    iparam.sensor_info.fps = attr->sensor_info.fps;

    iparam.sensor_info.max_fps = attr->sensor_info.max_fps;
    iparam.sensor_info.min_fps = attr->sensor_info.min_fps;

    iparam.sensor_info.min_integration_time = attr->sensor_info.min_integration_time;
    iparam.sensor_info.min_integration_time_short = attr->sensor_info.min_integration_time_short;
    iparam.sensor_info.max_integration_time_short = attr->sensor_info.max_integration_time_short;
    iparam.sensor_info.min_integration_time_native = attr->sensor_info.min_integration_time_native;
    iparam.sensor_info.max_integration_time_native = attr->sensor_info.max_integration_time_native;
    iparam.sensor_info.integration_time_limit = attr->sensor_info.integration_time_limit;
    iparam.sensor_info.integration_time = attr->sensor_info.integration_time;
    iparam.sensor_info.integration_time_short = attr->sensor_info.integration_time_short;
    iparam.sensor_info.total_width = attr->sensor_info.total_width;
    iparam.sensor_info.total_height = attr->sensor_info.total_height;
    iparam.sensor_info.max_integration_time = attr->sensor_info.max_integration_time;
    iparam.sensor_info.integration_time_apply_delay = attr->sensor_info.integration_time_apply_delay;
    iparam.sensor_info.again_apply_delay = attr->sensor_info.again_apply_delay;
    iparam.sensor_info.dgain_apply_delay = attr->sensor_info.dgain_apply_delay;
    iparam.sensor_info.one_line_expr_in_us = attr->sensor_info.one_line_expr_in_us;

    iparam.sensor_info.min_integration_time_short = attr->sensor_info.min_integration_time_short;
    iparam.sensor_info.max_integration_time_short = attr->sensor_info.max_integration_time_short;
    iparam.sensor_info.integration_time_short = attr->sensor_info.integration_time_short;
    iparam.sensor_info.max_again_short = attr->sensor_info.max_again_short;        //the format is .16
    iparam.sensor_info.again_short = attr->sensor_info.again_short;
    iparam.multi_mode.sensor_num = IMPISP_TOTAL_ONE;
    iparam.WdrEn = attr->sensor_info.wdr_en;
    isp_nv12_wbit = 8;  //width aligned to 8
    snprintf(file_name, sizeof(file_name), "/etc/sensor/%s-x2580.bin", attr->device_name);

    ret = tisp_init(index, &iparam, file_name);
    if (ret) {
        printk(KERN_ERR "%s, tisp_core_init failed!\n", __func__);
        return ret;
    }

    if(drv->state[index] == ISP_MODULE_DEINIT)
        tisp_process_init();

    tisp_g_module_control(index, &bypass_top);
    if( !(bypass_top.key & MDNS_BYPASS)){
        ret = mdns_init( attr->sensor_info.width,  attr->sensor_info.height);
        if (ret)
            goto set_mdns_err;
    }
    if(drv->camera.sensor->sensor_info.wdr_en == 1) {
        wdr_size = isp_wdr_get_buf(index, attr->sensor_info.width,  attr->sensor_info.height);
        wdr_vaddr = rmem_alloc_aligned(wdr_size, PAGE_SIZE);
        if(wdr_vaddr == NULL) {
            goto set_wdr_err;
        }
        ret = isp_wdr_set_buf(attr->sensor_info.width,  attr->sensor_info.height, wdr_size, wdr_vaddr);
        if (ret)
            return -1;
    }

    /*
     * For event engine
     */
    if(IS_ERR_OR_NULL(drv->process_thread[0])){
        drv->process_thread[0] = private_kthread_run(isp_firmware_process, NULL, "isp_firmware_process");
        if(IS_ERR_OR_NULL(drv->process_thread[0])){
            printk(KERN_ERR "%s, kthread_run was failed!\n", __func__);
            return -EINVAL;
        }
    }

   drv->state[index] = ISP_MODULE_INIT;

    return 0;

set_wdr_err:
wdr_vaddr = 0;
set_mdns_err:
mdns_vaddr = 0;

    return ret;
}

static void tiziano_deinit(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    unsigned int core_state = -1;
    int ret;

    if(drv->state[index] == ISP_MODULE_INIT){
        drv->state[index] = ISP_MODULE_DEINIT;
    }
    if(drv->process_thread[0] != NULL) {
        ret = private_kthread_stop(drv->process_thread[0]);
        if(!ret){
            drv->process_thread[0] = NULL;
        }else
            printk("[ %s:%d ] kthread_stop was failed!\n", __func__,__LINE__);
    }

    isp_interrupts_disable(index);
    tisp_process_deinit(index);

    core_state = core_state && drv->state[index] <= ISP_MODULE_DEINIT;
    if(core_state){
        tisp_disable_tuning();
    }

    tisp_slake_all();

    tisp_deinit(index);
}

int isp_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret;
    unsigned long flags = 0;

    drv->tuning->event(drv->tuning, TISP_EVENT_ACTIVATE_MODULE, NULL);
    ret = tiziano_init(index);
    if (ret) {
        printk(KERN_ERR " tiziano_init failed!\n");
        ret = -EINVAL;
        goto tiziano_init_failed;
    }

    tiziano_sync_sensor_attr(&iparam);
    tisp_stream_on(&iparam);

    private_spin_lock_irqsave(&(drv->slock), flags);
    isp_interrupts_enable(index);       // INT_EN
    enable_irq(drv->irq);
    private_spin_unlock_irqrestore(&(drv->slock), flags);

    tisp_ipc_triger();
    tisp_activate_all();

    ret = vic_tiziano_stream_on(index, attr);  /* VIC Interface Stream ON */
    if (ret) {
        printk(KERN_ERR "%s[%d] vic tiziano stream on failed!\n",__func__,__LINE__);
        ret = -EINVAL;
        goto tiziano_stream_on_failed;
    }

    drv->camera.is_stream_on = 1;

    return 0;

tiziano_stream_on_failed:
tiziano_init_failed:
    drv->tuning->event(drv->tuning, TISP_EVENT_SLAVE_MODULE, NULL);

    return ret;
}

int isp_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    uint32_t top_val = 0;
    unsigned long flags = 0;
    int width = drv->camera.sensor->sensor_info.width;
    int height = drv->camera.sensor->sensor_info.height;

    vic_tiziano_stream_off(index, attr);

    /* disable irq */
    private_spin_lock_irqsave(&(drv->slock), flags);
    disable_irq(drv->irq);
    isp_interrupts_disable(index);
    private_spin_unlock_irqrestore(&(drv->slock), flags);

    top_val = isp_read_reg(index, TOP_ADDR_STOP_CON);
    isp_write_reg(index, TOP_ADDR_STOP_CON, top_val | 0x1);
    while(1){
        top_val = isp_read_reg(index, TOP_ADDR_STOP_STATE);
        if(top_val & 0x01)
                break;
    }
    isp_write_reg(index, TOP_ADDR_TOP_RST, 0x1);

    drv->tuning->event(drv->tuning, TISP_EVENT_SLAVE_MODULE, NULL);

    if(drv->camera.sensor->sensor_info.wdr_en && wdr_vaddr) {
        rmem_free(wdr_vaddr, wdr_size);
    }

    if(mdns_vaddr) {
        mdns_deinit(width, height);
    }

    tiziano_deinit(index);
    drv->camera.is_stream_on = 0;

    return 0;
}

int isp_power_on(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on == 0) {
        ret = private_cpm_reset(0xb00000c4, 22);
        if (0 != ret) {
            printk("private_cpm_reset failed..ret %d\n", ret);
            ret = -EINVAL;
            goto unlock;
        }

        ret = vic_tiziano_power_on(index);
        if (ret)
            goto unlock;
    }

    if (!ret)
        drv->camera.is_power_on++;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void isp_power_off(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    mutex_lock(&drv->lock);

    vic_tiziano_power_off(index);

    drv->camera.is_power_on--;

    mutex_unlock(&drv->lock);
}

static int isp_ch0_frm_done[3] = {0};
static int isp_err = 0;

static int csc_switch[2] = {0};
static int staticIntpCont[32] = {0};
static irqreturn_t isp_irq_handler(int irq, void *data)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)data;
    int index = drv->index;
    unsigned int irqstatus,irqstatus1;
    unsigned int irqstatusUnusual;
    unsigned int irqstatusBack0;
    unsigned int irqstatusBack1;
    int ret;
    struct isp_event_initarg init;
    int irqret = IRQ_HANDLED;

    irqstatus = isp_read_reg(index, RESP_ADDR_INT_COMMON_0_INFO);
    isp_write_reg(index, RESP_ADDR_INT_COMMON_0_CLR, irqstatus);
    irqstatus1 = isp_read_reg(index, RESP_ADDR_INT_COMMON_1_INFO);
    isp_write_reg(index, RESP_ADDR_INT_COMMON_1_CLR, irqstatus1);
    irqstatusBack0 = isp_read_reg(index, RESP_ADDR_INT_BACKUP_0_INFO);
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_0_CLR, irqstatusBack0);

    irqstatusBack1 = isp_read_reg(index, RESP_ADDR_INT_BACKUP_1_INFO);
    isp_write_reg(index, RESP_ADDR_INT_BACKUP_1_CLR, irqstatusBack1);

    irqstatusUnusual = isp_read_reg(index, RESP_ADDR_INT_UNUSUAL_0_INFO);
    if (0 != irqstatusUnusual) {
        isp_write_reg(index, RESP_ADDR_INT_UNUSUAL_0_CLR, irqstatusUnusual);
        isp_err++;
        if (irqstatusUnusual & IP_OF_ERR_INT){
            printk(KERN_ERR "isp overflow:irq-status unusual 0x%08x, overflow count %d !!!\n",  irqstatusUnusual, drv->isp_overflow++);
            drv->isp_overflow++;
        }
        if (irqstatusUnusual & BROKEN_FRAME_INT){
            drv->isp_breakfrm++;
        }
    }

    if (irqstatus & CH0_FRM_DONE_INT) {
        init.vinum = 0;
        isp_ch0_frm_done[0]++;

        if(csc_switch[0] == 1){
            if(drv->tuning->ctrls[0].daynight == TISP_RUNING_MODE_DAY_MODE)
                isp_write_reg(index, CSC_ADDR_CLIP, 0xff00ff00);

            isp_write_reg(index, CSC_ADDR_CTRL, 0xffffffff);
            csc_switch[0] = 0;
        }
        if ((1 == drv->isp_daynight_switch[0])) {
            printk(KERN_ERR "isp daynight %d\n", drv->tuning->ctrls[0].daynight);
            if(drv->tuning->ctrls[0].daynight == TISP_RUNING_MODE_NIGHT_MODE)
                isp_write_reg(index, CSC_ADDR_CLIP, 0xff008080);

            isp_write_reg(index, CSC_ADDR_CTRL, 0xffffffff);
            if(drv->tuning)
                drv->tuning->event(drv->tuning, TISP_EVENT_CORE_DAY_NIGHT, &init);

            drv->isp_daynight_switch[0] = 0;
            csc_switch[0] = 1;

        } else if(2 == drv->isp_daynight_switch[0]) {

            isp_write_reg(index, CSC_ADDR_CLIP, 0xff00ff00);
            drv->isp_daynight_switch[0] = 0;

        } else if(3 == drv->isp_daynight_switch[0]) {

            isp_write_reg(index, CSC_ADDR_CLIP, 0xff008080);
            drv->isp_daynight_switch[0] = 0;
        }
    }

    /*
     * 1. mscaler irq callback
     */
    ret = mscaler_interrupt_service_routine(index, irqstatus);
    if(ret < 0)
        printk(KERN_ERR "mscaler interrupt handle error. ret=%d\n", ret);

    /*
     * 2. isp-core irq callbacks
     */
    int i;
    for (i = 0; i < 96; i++) {
        if(i < 32){
            if ((irqstatus & (0x1 << i)) && (NULL != irq_func_cb[i])) {
                    staticIntpCont[i]++;
                    ret = irq_func_cb[i](0);
                    if (ret != IRQ_HANDLED) {
                            irqret = ret;
                    }
            }
        } else if(i < 64){
            if ((irqstatusBack0 & (0x1 << (i - 32))) && (NULL != irq_func_cb[i])) {
                    ret = irq_func_cb[i](0);
                    if (ret != IRQ_HANDLED) {
                            irqret = ret;
                    }
            }
        } else {
            if ((irqstatusBack1 & (0x1 << (i - 64))) && (NULL != irq_func_cb[i])) {
                    ret = irq_func_cb[i](0);
                    if (ret != IRQ_HANDLED) {
                            irqret = ret;
                    }
            }
        }
    }

    return irqret;
}

static irqreturn_t isp_irq_thread_handler(int irq, void *data)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)data;
    struct sensor_ctrl_ops *ops = &isp->camera.sensor->ops;
    int i = 0;

    if(!isp || !ops)
        return 0;

    if(tisp_msca_state() == 0){
        tisp_msca_Shd_ctrl(0);
    }

    for(i = 0; i < TISP_I2C_SET_BUTTON; i++){
        if(isp->i2c_msgs[0][i].flag == 0)
            continue;

        switch(i){
        case TISP_I2C_SET_AGAIN:
            if (ops->set_analog_gain)
                ops->set_analog_gain(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_analog_gain is NULL!\n");
            break;
        case TISP_I2C_SET_AGAIN_SHORT:
            if (ops->set_analog_gain_short)
                ops->set_analog_gain_short(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_analog_gain_short is NULL!\n");
            break;
        case TISP_I2C_SET_DGAIN:
            if (ops->set_digital_gain)
                ops->set_digital_gain(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_digital_gain is NULL!\n");
            break;
        case TISP_I2C_SET_INTEGRATION:
            if (ops->set_integration_time)
                ops->set_integration_time(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_integration_time is NULL!\n");
            break;
        case TISP_I2C_SET_INTEGRATION_SHORT:
            if (ops->set_integration_time_short)
                ops->set_integration_time_short(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_integration_time_short is NULL!\n");
            break;
        case TISP_I2C_SET_EXPO:
            if (ops->set_expo)
                ops->set_expo(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_expo is NULL!\n");
            break;

        case TISP_I2C_SET_EXPO_SHORT:
            if (ops->set_expo_short)
                ops->set_expo_short(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_expo_short is NULL!\n");
            break;
        case TISP_I2C_SET_HVFLIP:
            if (ops->set_hvflip){
                tisp_hv_flip_t hvflip;
                hvflip.sensor_mode = isp->i2c_msgs[0][i].value;
                ops->set_hvflip(&hvflip);
            }
            break;
        case TISP_I2C_SET_FPS:
            if (ops->set_fps)
                ops->set_fps(isp->i2c_msgs[0][i].value);
            else
                printk(KERN_ERR "sensor_attr->ops.set_fps is NULL!\n");
            break;

        default:
            break;
        }
    }

    return 0;
}

#ifdef SOC_CAMERA_DEBUG
static int dsysfs_isp_show_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_isp_data *drv = container_of((struct kobject *)dev, struct jz_isp_data, dsysfs_kobj);
    struct sensor_attr *sensor = drv->camera.sensor;
    int vinum = 0;
    char *p = buf;

    char *colorspace = NULL;
    tisp_ae_exprinfo_t ev_attr;
    tisp_awb_attr_t wb_attr;
    TISP_MODE_DN_E dn;
    char *dump_string = NULL;
    tisp_anfiflicker_attr_t flicker;
    tisp_core_tuning_attr *tisp_tattr = NULL;
    uint16_t *gdeflick_lut = NULL;
    uint16_t gnodes = 0;
    int ret;

    p += sprintf(p ,"************** ISP INFO **************\n");
    if(!drv->camera.is_stream_on){
        p += sprintf(p ,"sensor doesn't work, please stream on\n");
        return p - buf;
    }

    if(drv->tuning->state < ISP_MODULE_INIT){
        p += sprintf(p ,"fristly, please open device : /dev/isp-tuning\n");
        return p - buf;
    }

    tisp_tattr = tisp_get_tuning();
    tisp_g_ae_exprinfo_attr(vinum, &ev_attr);
    tisp_g_awb_attr(vinum, &wb_attr);

    switch(sensor->sensor_info.fmt){
    case SENSOR_PIXEL_FMT_SBGGR8_1X8:
    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
        colorspace = "BGGR";
        break;
    case SENSOR_PIXEL_FMT_SGBRG8_1X8:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
        colorspace = "GBRG";
        break;
    case SENSOR_PIXEL_FMT_SGRBG8_1X8:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
        colorspace = "GRBG";
        break;
    case SENSOR_PIXEL_FMT_SRGGB8_1X8:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        colorspace = "RGGB";
        break;
#if 0
    case SENSOR_PIXEL_FMT_SRGIB_8BIT:
    case SENSOR_PIXEL_FMT_SRGIB_10BIT:
    case SENSOR_PIXEL_FMT_SRGIB_12BIT:
        colorspace = "RGIB";
        break;
    case SENSOR_PIXEL_FMT_SBGIR_8BIT:
    case SENSOR_PIXEL_FMT_SBGIR_10BIT:
    case SENSOR_PIXEL_FMT_SBGIR_12BIT:
        colorspace = "BGIR";
        break;
    case SENSOR_PIXEL_FMT_SRIGB_8BIT:
    case SENSOR_PIXEL_FMT_SRIGB_10BIT:
    case SENSOR_PIXEL_FMT_SRIGB_12BIT:
        colorspace = "RIGB";
        break;
    case SENSOR_PIXEL_FMT_SBIGR_8BIT:
    case SENSOR_PIXEL_FMT_SBIGR_10BIT:
    case SENSOR_PIXEL_FMT_SBIGR_12BIT:
        colorspace = "BIGR";
        break;
    case SENSOR_PIXEL_FMT_SGRBI_8BIT:
    case SENSOR_PIXEL_FMT_SGRBI_10BIT:
    case SENSOR_PIXEL_FMT_SGRBI_12BIT:
        colorspace = "GRBI";
        break;
    case SENSOR_PIXEL_FMT_SGBRI_8BIT:
    case SENSOR_PIXEL_FMT_SGBRI_10BIT:
    case SENSOR_PIXEL_FMT_SGBRI_12BIT:
        colorspace = "GBRI";
        break;
    case SENSOR_PIXEL_FMT_SIRBG_8BIT:
    case SENSOR_PIXEL_FMT_SIRBG_10BIT:
    case SENSOR_PIXEL_FMT_SIRBG_12BIT:
        colorspace = "IRBG";
        break;
    case SENSOR_PIXEL_FMT_SIBRG_8BIT:
    case SENSOR_PIXEL_FMT_SIBRG_10BIT:
    case SENSOR_PIXEL_FMT_SIBRG_12BIT:
        colorspace = "IBRG";
        break;
    case SENSOR_PIXEL_FMT_SRGGI_8BIT:
    case SENSOR_PIXEL_FMT_SRGGI_10BIT:
    case SENSOR_PIXEL_FMT_SRGGI_12BIT:
        colorspace = "RGGI";
        break;
    case SENSOR_PIXEL_FMT_SBGGI_8BIT:
    case SENSOR_PIXEL_FMT_SBGGI_10BIT:
    case SENSOR_PIXEL_FMT_SBGGI_12BIT:
        colorspace = "BGGI";
        break;
    case SENSOR_PIXEL_FMT_SGRIG_8BIT:
    case SENSOR_PIXEL_FMT_SGRIG_10BIT:
    case SENSOR_PIXEL_FMT_SGRIG_12BIT:
        colorspace = "GRIG";
        break;
    case SENSOR_PIXEL_FMT_SGBIG_8BIT:
    case SENSOR_PIXEL_FMT_SGBIG_10BIT:
    case SENSOR_PIXEL_FMT_SGBIG_12BIT:
        colorspace = "GBIG";
        break;
    case SENSOR_PIXEL_FMT_SGIRG_8BIT:
    case SENSOR_PIXEL_FMT_SGIRG_10BIT:
    case SENSOR_PIXEL_FMT_SGIRG_12BIT:
        colorspace = "GIRG";
        break;
    case SENSOR_PIXEL_FMT_SGIBG_8BIT:
    case SENSOR_PIXEL_FMT_SGIBG_10BIT:
    case SENSOR_PIXEL_FMT_SGIBG_12BIT:
        colorspace = "SGIBG";
        break;
    case SENSOR_PIXEL_FMT_SIGGR_8BIT:
    case SENSOR_PIXEL_FMT_SIGGR_10BIT:
    case SENSOR_PIXEL_FMT_SIGGR_12BIT:
        colorspace = "IGGR";
        break;
    case SENSOR_PIXEL_FMT_SIGGB_8BIT:
    case SENSOR_PIXEL_FMT_SIGGB_10BIT:
    case SENSOR_PIXEL_FMT_SIGGB_12BIT:
        colorspace = "IGGB";
        break;
#endif
    default:
        colorspace = "The format of isp input is YUV422 or RGB";
        break;
    }

    p += sprintf(p ,"TISP Core Version : %s\n", TISP_CORE_VERSION);
    p += sprintf(p ,"Driver Version : %s\n", DRIVER_VERSION);
    p += sprintf(p ,"SENSOR NAME : %s\n", sensor->device_name);
    p += sprintf(p ,"SENSOR WIDTH : %d\n", sensor->sensor_info.width);
    p += sprintf(p ,"SENSOR HEIGHT : %d\n", sensor->sensor_info.height);
    p += sprintf(p ,"SENSOR RAW PATTERN : %s\n", colorspace);
    p += sprintf(p ,"SENSOR FPS : %d / %d\n", sensor->sensor_info.fps >> 16, sensor->sensor_info.fps & 0xffff);
    dn = tisp_day_or_night_g_ctrl(vinum);
    if(dn == TISP_RUNING_MODE_DAY_MODE)
        dump_string = "Day";
    else if(dn == TISP_RUNING_MODE_NIGHT_MODE)
        dump_string = "Night";
    else
        dump_string = "Custom";
    p += sprintf(p ,"ISP Runing Mode : %s\n", dump_string);
    p += sprintf(p,"Saturation : %d\n", tisp_tattr->saturation[vinum]);
    p += sprintf(p,"Sharpness : %d\n", tisp_tattr->sharpness[vinum]);
    p += sprintf(p,"Contrast : %d\n", tisp_tattr->contrast[vinum]);
    p += sprintf(p,"Brightness : %d\n", tisp_tattr->brightness[vinum]);
    p += sprintf(p,"Hue : %d\n", tisp_tattr->hue[vinum]);

    p += sprintf(p , "\nAntiflicker:\n");
    tisp_g_antiflick(vinum, &flicker);
    if(flicker.mode == ISP_ANTIFLICKER_DISABLE_MODE)
        dump_string = "disable";
    else if (flicker.mode == ISP_ANTIFLICKER_AUTO_MODE)
        dump_string = "auto";
    else
        dump_string = "normal";
    p += sprintf(p ,"Antiflicker : %s freq: %d\n", dump_string, flicker.mode == ISP_ANTIFLICKER_DISABLE_MODE ? 0 : flicker.freq);

    gdeflick_lut = private_kmalloc(120 * sizeof(uint16_t), GFP_KERNEL);
    memset(gdeflick_lut, 0, sizeof(uint16_t) * 120);
    if(gdeflick_lut == NULL)
        ret = -1;
    else
        ret = tisp_get_antiflicker_step(vinum, gdeflick_lut, &gnodes);
    if(ret == 0){
        int i;
        p += sprintf(p ,"Antiflicker nodes: %d: step : ", gnodes + 1);
        for(i = 0; i <= gnodes; i++)
            p += sprintf(p ,"%d. ", gdeflick_lut[i]);
        p += sprintf(p ,"\n");
    }
    private_kfree(gdeflick_lut);
    p += sprintf(p , "\nAE:\n");
    p += sprintf(p ,"ISP AE: %s\n", ev_attr.AeMode ? "MANUAL" : "AUTO");
    p += sprintf(p ,"AeIntegrationTimeUnit : %d\n", ev_attr.AeIntegrationTimeUnit);
    p += sprintf(p ,"AeIntegrationTimeMode : %d\n", ev_attr.AeIntegrationTimeMode);
    p += sprintf(p ,"AeAGainManualMode : %d\n", ev_attr.AeAGainManualMode);
    p += sprintf(p ,"AeDGainManualMode : %d\n", ev_attr.AeDGainManualMode);
    p += sprintf(p ,"AeIspDGainManualMode : %d\n", ev_attr.AeIspDGainManualMode);
    p += sprintf(p ,"AeIntegrationTime : %d\n", ev_attr.AeIntegrationTime);
    p += sprintf(p ,"AeAGain : %d\n", ev_attr.AeAGain);
    p += sprintf(p ,"AeDGain : %d\n", ev_attr.AeDGain);
    p += sprintf(p ,"AeIspDGain : %d\n", ev_attr.AeIspDGain);
    p += sprintf(p ,"AeMinIntegrationTimeMode : %d\n", ev_attr.AeMinIntegrationTimeMode);
    p += sprintf(p ,"AeMinAGainMode : %d\n", ev_attr.AeMinAGainMode);
    p += sprintf(p ,"AeMinDgainMode : %d\n", ev_attr.AeMinDgainMode);
    p += sprintf(p ,"AeMinIspDGainMode : %d\n", ev_attr.AeMinIspDGainMode);
    p += sprintf(p ,"AeMaxIntegrationTimeMode : %d\n", ev_attr.AeMaxIntegrationTimeMode);
    p += sprintf(p ,"AeMaxAGainMode : %d\n", ev_attr.AeMaxAGainMode);
    p += sprintf(p ,"AeMaxDgainMode : %d\n", ev_attr.AeMaxDgainMode);
    p += sprintf(p ,"AeMaxIspDGainMode : %d\n", ev_attr.AeMaxIspDGainMode);
    p += sprintf(p ,"AeMinIntegrationTime : %d\n", ev_attr.AeMinIntegrationTime);
    p += sprintf(p ,"AeMinAGain : %d\n", ev_attr.AeMinAGain);
    p += sprintf(p ,"AeMinDgain : %d\n", ev_attr.AeMinDgain);
    p += sprintf(p ,"AeMinIspDGain : %d\n", ev_attr.AeMinIspDGain);
    p += sprintf(p ,"AeMaxIntegrationTime : %d\n", ev_attr.AeMaxIntegrationTime);
    p += sprintf(p ,"AeMaxAGain : %d\n", ev_attr.AeMaxAGain);
    p += sprintf(p ,"AeMaxDgain : %d\n", ev_attr.AeMaxDgain);
    p += sprintf(p ,"AeMaxIspDGain : %d\n", ev_attr.AeMaxIspDGain);
    p += sprintf(p ,"AeShortMode : %d\n", ev_attr.AeShortMode);
    p += sprintf(p ,"AeShortIntegrationTimeMode : %d\n", ev_attr.AeShortIntegrationTimeMode);
    p += sprintf(p ,"AeShortAGainManualMode : %d\n", ev_attr.AeShortAGainManualMode);
    p += sprintf(p ,"AeShortDGainManualMode : %d\n", ev_attr.AeShortDGainManualMode);
    p += sprintf(p ,"AeShortIspDGainManualMode : %d\n", ev_attr.AeShortIspDGainManualMode);
    p += sprintf(p ,"AeShortIntegrationTime : %d\n", ev_attr.AeShortIntegrationTime);
    p += sprintf(p ,"AeShortAGain : %d\n", ev_attr.AeShortAGain);
    p += sprintf(p ,"AeShortDGain : %d\n", ev_attr.AeShortDGain);
    p += sprintf(p ,"AeShortIspDGain : %d\n", ev_attr.AeShortIspDGain);
    p += sprintf(p ,"AeShortMinIntegrationTimeMode : %d\n", ev_attr.AeShortMinIntegrationTimeMode);
    p += sprintf(p ,"AeShortMinAGainMode : %d\n", ev_attr.AeShortMinAGainMode);
    p += sprintf(p ,"AeShortMinDgainMode : %d\n", ev_attr.AeShortMinDgainMode);
    p += sprintf(p ,"AeShortMinIspDGainMode : %d\n", ev_attr.AeShortMinIspDGainMode);
    p += sprintf(p ,"AeShortMaxIntegrationTimeMode : %d\n", ev_attr.AeShortMaxIntegrationTimeMode);
    p += sprintf(p ,"AeShortMaxAGainMode : %d\n", ev_attr.AeShortMaxAGainMode);
    p += sprintf(p ,"AeShortMaxDgainMode : %d\n", ev_attr.AeShortMaxDgainMode);
    p += sprintf(p ,"AeShortMaxIspDGainMode : %d\n", ev_attr.AeShortMaxIspDGainMode);
    p += sprintf(p ,"AeShortMinIntegrationTime : %d\n", ev_attr.AeShortMinIntegrationTime);
    p += sprintf(p ,"AeShortMinAGain : %d\n", ev_attr.AeShortMinAGain);
    p += sprintf(p ,"AeShortMinDgain : %d\n", ev_attr.AeShortMinDgain);
    p += sprintf(p ,"AeShortMinIspDGain : %d\n", ev_attr.AeShortMinIspDGain);
    p += sprintf(p ,"AeShortMaxIntegrationTime : %d\n", ev_attr.AeShortMaxIntegrationTime);
    p += sprintf(p ,"AeShortMaxAGain : %d\n", ev_attr.AeShortMaxAGain);
    p += sprintf(p ,"AeShortMaxDgain : %d\n", ev_attr.AeShortMaxDgain);
    p += sprintf(p ,"AeShortMaxIspDGain : %d\n", ev_attr.AeShortMaxIspDGain);
    p += sprintf(p ,"TotalGainDb : %d\n", ev_attr.TotalGainDb);
    p += sprintf(p ,"TotalGainDbShort : %d\n", ev_attr.TotalGainDbShort);
    p += sprintf(p ,"ExposureValue : %lld\n", ev_attr.ExposureValue);
    p += sprintf(p ,"EVLog2 : %d\n", ev_attr.EVLog2);
    p += sprintf(p , "\nAWB:\n");
    p += sprintf(p ,"awb mode : %d\n", wb_attr.mode);
    p += sprintf(p ,"ct : %d\n", wb_attr.ct);
    p += sprintf(p ,"rgain : %d\n", wb_attr.gain_val.rgain);
    p += sprintf(p ,"bgain : %d\n", wb_attr.gain_val.bgain);
    p += sprintf(p ,"awb_start_en : %d\n", wb_attr.awb_start_en);
    p += sprintf(p ,"algo_rgain : %d\n", wb_attr.awb_start.rgain);
    p += sprintf(p ,"algo_bgain : %d\n", wb_attr.awb_start.bgain);
    p += sprintf(p , "\nDEBUG:\n");
    p += sprintf(p ,"debug : %d,%d,%d,%d,%d\n", isp_ch0_frm_done[0], drv->isp_err,drv->isp_err1,drv->isp_overflow,drv->isp_breakfrm);
    p += sprintf(p ,"static debug :AWB: %d, AE_S: %d, AE_H: %d, AF: %d, GSM: %d\n", staticIntpCont[3], staticIntpCont[4], staticIntpCont[5], staticIntpCont[6], staticIntpCont[7]);
    p += sprintf(p ,"static debug :LCE: %d, ADR: %d, DEFOG: %d, WDR: %d, RMO: %d\n", staticIntpCont[8], staticIntpCont[9], staticIntpCont[10], staticIntpCont[11], staticIntpCont[17]);
#endif
    return p - buf;
}

static ssize_t dsysfs_isp_ctrl(struct file *file, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t pos, size_t count)
{
    struct jz_isp_data *drv = container_of(kobj, struct jz_isp_data, dsysfs_kobj);

    if (!strncmp(buf, "r_isp_reg", sizeof("r_isp_reg")-1)) {
        char *s, *tmp;
        int i = 0;
        unsigned int reg, reg_list[2] = {0, 0}; //[0]:start, [1]:stop
        unsigned int val;

        s = buf+sizeof("r_isp_reg");
        tmp = strsep(&s, " ");
        while (tmp != NULL) {
            if (*tmp != '\0') {
                reg_list[i] = simple_strtoul(tmp, NULL, 0);
                i++;
                if (i >= 2)
                    break;
            }
            tmp = strsep(&s, " ");
        }

        if (reg_list[0] % 4 || reg_list[1] % 4) {
            printk(KERN_ERR "err reg list: 0x%08x - 0x%08x", reg_list[0], reg_list[1]);
            return count;
        }

        if (reg_list[1] == 0) {
            //read one reg
            val = isp_read_reg(drv->index, reg_list[0]);
            printk(KERN_ERR "r_isp_reg: 0x%08x 0x%08x \n", reg_list[0], val);
        } else if (reg_list[1] > reg_list[0]) {
            //read reg list
            printk(KERN_ERR "r_isp_reg_list: 0x%08x - 0x%08x \n", reg_list[0], reg_list[1]);
            reg = reg_list[0];
            while (1) {
                val = isp_read_reg(drv->index, reg);
                printk(KERN_ERR "0x%08x 0x%08x \n", reg, val);

                reg += 4;
                if (reg >= reg_list[1])
                    break;
            }
        } else {
            printk(KERN_ERR "err reg list: 0x%08x - 0x%08x", reg_list[0], reg_list[1]);
        }

        return count;
    } else if (!strncmp(buf, "w_isp_reg", sizeof("w_isp_reg")-1)) {
        unsigned int reg;
        unsigned int val;
        char *p = 0;
        reg = simple_strtoul(buf+sizeof("w_isp_reg"), &p, 0);
        val = simple_strtoul(p+1, NULL, 0);
        isp_write_reg(drv->index, reg, val);
        printk(KERN_ERR "w_isp_reg: 0x%08x 0x%08x \n", reg, val);
    }

    return count;
}

static DSYSFS_DEV_ATTR(show_isp_info, S_IRUGO|S_IWUSR, dsysfs_isp_show_info, NULL);
static struct attribute *dsysfs_isp_dev_attrs[] = {
    &dsysfs_dev_attr_show_isp_info.attr,
    NULL,
};

static DSYSFS_BIN_ATTR(ctrl, S_IRUGO|S_IWUSR, NULL, dsysfs_isp_ctrl, 0);
static struct bin_attribute *dsysfs_isp_bin_attrs[] = {
    &dsysfs_bin_attr_ctrl,
    NULL,
};

static const struct attribute_group dsysfs_isp_attr_group = {
    .attrs  = dsysfs_isp_dev_attrs,
    .bin_attrs = dsysfs_isp_bin_attrs,
};

int isp_component_bind_sensor(int index, struct sensor_attr *sensor)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    mutex_lock(&drv->lock);

    ret = mscaler_register_sensor(index, sensor);
    if (ret != 0) {
        printk(KERN_ERR "Failed to register sensor!\n");
        goto failed_to_register_sensor;
    }

    //tuning init
    ret = isp_core_tuning_init(drv);
    if (ret != 0) {
        printk(KERN_ERR "Failed to init tuning module!\n");
        ret = -EINVAL;
        goto failed_to_tuning;
    }

    ret = sensor_early_init(drv);
    if (ret != 0) {
        printk(KERN_ERR "sensor_early_init Failed!\n");
        ret = -EINVAL;
        goto failed_to_tuning;
    }

    drv->camera.sensor = sensor;
    drv->camera.is_power_on = 0;
    drv->camera.is_stream_on = 0;

    mutex_unlock(&drv->lock);

    return 0;

failed_to_tuning:
    mscaler_unregister_sensor(index, sensor);
failed_to_register_sensor:
    mutex_unlock(&drv->lock);
    return ret;
}

void isp_component_unbind_sensor(int index, struct sensor_attr *sensor)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    isp_core_tuning_deinit(drv);

    mscaler_unregister_sensor(index, sensor);

    drv->camera.sensor = NULL;

    mutex_unlock(&drv->lock);
}

int jz_isp_drv_init(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret;

    mutex_init(&drv->lock);
    private_spin_lock_init(&(drv->slock));
    ret = request_threaded_irq(drv->irq, isp_irq_handler, isp_irq_thread_handler, IRQF_ONESHOT, drv->irq_name, (void *)drv);
    if (ret) {
        printk(KERN_ERR "camera: isp%d failed to request irq\n", index);
        goto error_request_irq;
    }

    disable_irq(drv->irq);

    ret = jz_mscaler_drv_init(index);
    if (ret) {
        printk(KERN_ERR "camera: failed to init mscaler\n");
        goto error_mscaler_init;
    }

#ifdef SOC_CAMERA_DEBUG
    ret = dsysfs_create_group(&drv->dsysfs_kobj, dsysfs_get_root_dir(index), "isp", &dsysfs_isp_attr_group);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create sub dir isp fail\n", index);
        goto error_dsys_create_isp;
    }
#endif

    drv->isp_err = 0;
    drv->isp_err1 = 0;
    drv->isp_overflow = 0;
    drv->isp_breakfrm = 0;

    drv->camera.is_stream_on = 0;
    drv->is_finish = 1;

    printk(KERN_DEBUG "isp%d initialization successfully\n", index);

    return 0;

#ifdef SOC_CAMERA_DEBUG
error_dsys_create_isp:
    jz_mscaler_drv_deinit(index);
#endif

error_mscaler_init:
    free_irq(drv->irq, drv);

error_request_irq:
    return ret;
}

void jz_isp_drv_deinit(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;
    free_irq(drv->irq, drv);

    jz_mscaler_drv_deinit(index);

#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_isp_attr_group);
#endif

    return ;
}