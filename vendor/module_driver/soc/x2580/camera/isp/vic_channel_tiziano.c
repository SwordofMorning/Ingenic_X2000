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
#include "drivers/rmem_manager/rmem_manager.h"

#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>
#include "camera_gpio.h"
#include "csi.h"
#include "vic.h"
#include "isp.h"
#include "dsys.h"
#include <x2580_isp_common.h>

struct jz_vic_tiziano_data {
    int index;
    int is_enable;
    int is_finish;

    int irq;
    const char *irq_name;

    struct camera_device camera;
    struct mutex lock;
    spinlock_t slock;

    unsigned int vic_frd_c;     /* frame done cnt */
    unsigned int vic_fre_c;     /* frame err cnt */
    unsigned int vic_frov_c;    /* frame overflow cnt */

#ifdef SOC_CAMERA_DEBUG
    struct kobject dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
    struct completion snap_raw_comp;
    unsigned int dma_complete;
#endif
};


static struct jz_vic_tiziano_data jz_vic_tiziano_dev[1] = {
    {
        .index                  = 0,
        .is_enable              = 0,
        .irq                    = IRQ_INTC_BASE + IRQ_VIC, /* BASE + 30 */
        .irq_name               = "VIC",
    },
};


static irqreturn_t vic_irq_isp_handler(int irq, void *data)
{
    struct jz_vic_tiziano_data *drv = (struct jz_vic_tiziano_data *)data;
    int index = drv->index;

    volatile unsigned long state, pending, mask;
    volatile unsigned long state2, pending2, mask2;
    unsigned int complete_num = 0;

    mask = vic_read_reg(index, VIC_ADDR_VIC_INT_MASK);
    mask2 = vic_read_reg(index, VIC_ADDR_VIC_INT_MASK2);
    state = vic_read_reg(index, VIC_ADDR_VIC_INT_STATU);
    state2 = vic_read_reg(index, VIC_ADDR_VIC_INT_STATU2);
    pending = state & (~mask);
    pending2 = state2 & (~mask2);

    vic_write_reg(index, VIC_ADDR_VIC_INT_CLR, pending);
    vic_write_reg(index, VIC_ADDR_VIC_INT_CLR2, pending2);

    // printk(KERN_ERR "## pending status = 0x%08lx pending2 status = 0x%08lx\n", pending, pending2);

    if (get_bit_field(&pending, VIC_HV_ERR)) {
        vic_reset(index); /* 复位vic后，下帧重头开始取（防裂屏） */

        drv->vic_fre_c++;
        printk(KERN_ERR "## pending status = 0x%08lx pending2 status = 0x%08lx\n", pending, pending2);
    }
    if (get_bit_field(&pending, VIC_FIFO_OVF)) {
        drv->vic_frov_c++;
    }
    if (get_bit_field(&pending2, DMA_FRD)) {
        // printk(KERN_INFO "## DMA_FRD  pending status = 0x%08lx pending2 status = 0x%08lx\n", pending, pending2);
#ifdef SOC_CAMERA_DEBUG
        drv->dma_complete++;
        // printk(KERN_ERR "## pending status = 0x%08lx pending2 status = 0x%08lx dma_complete %d\n", pending, pending2, drv->dma_complete);
        if(drv->camera.sensor->sensor_info.wdr_en)
            complete_num = 2;
        else
            complete_num = 1;

        if(drv->dma_complete == complete_num){
            complete(&drv->snap_raw_comp);
        }
#endif
    }
    if (get_bit_field(&pending, VIC_FRM_DONE)) {
        // printk(KERN_INFO "##VIC_FRM_DONE  pending status = 0x%08lx pending2 status = 0x%08lx\n", pending, pending2);
        drv->vic_frd_c++;
    }

    return IRQ_HANDLED;
}


int vic_tiziano_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    unsigned long flags = 0;

    int ret = vic_stream_on(index, attr);
    if (ret) {
        printk(KERN_ERR "vic(tiziano) : vic stream on failed\n");
        return ret;
    }

    private_spin_lock_irqsave(&drv->slock, flags);
    enable_irq(drv->irq);
    private_spin_unlock_irqrestore(&drv->slock, flags);

    return 0;
}


void vic_tiziano_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    unsigned long flags = 0;

    private_spin_lock_irqsave(&drv->slock, flags);
    disable_irq(drv->irq);
    private_spin_unlock_irqrestore(&drv->slock, flags);

    vic_stream_off(index, attr);
}

int vic_tiziano_power_on(int index)
{
    return vic_power_on(index);
}

void vic_tiziano_power_off(int index)
{
    vic_power_off(index);
}

#ifdef SOC_CAMERA_DEBUG
static ssize_t dsysfs_vic_tiziano_show_frame_cnt(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    char *p = buf;

    p += sprintf(p, "vic frame done: %u\n", drv->vic_frd_c);
    p += sprintf(p, "vic frame error: %u\n", drv->vic_fre_c);
    p += sprintf(p, "vic frame overflow: %u\n", drv->vic_frov_c);
    return p - buf;
}

static ssize_t dsysfs_vic_tiziano_dump_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_dump_reg(index, p);

    return p - buf;
}

static ssize_t dsysfs_vic_tiziano_show_sensor_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_show_sensor_info(index, p);

    return p - buf;
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_FROM_DEVICE);
}

static int dsysfs_vic_tiziano_snap_raw(struct jz_vic_tiziano_data *drv)
{
    struct sensor_attr *sensor = drv->camera.sensor;
    int index = drv->index;
    unsigned int image_width;
    unsigned int image_height;
    int loop = 600;

    if ( (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (sensor->mipi.mipi_crop.enable) ) {
        image_width = sensor->mipi.mipi_crop.output_width;
        image_height = sensor->mipi.mipi_crop.output_height;
    } else {
        image_width = sensor->sensor_info.width;
        image_height = sensor->sensor_info.height;
    }

    unsigned int lineoffset = image_width * 2;
    unsigned int imagesize = lineoffset * image_height;
    void *snap_vaddr;
    unsigned int snap_paddr;

    struct file *fd = NULL;
    mm_segment_t old_fs;
    loff_t *pos;
    int ret = 0;
    unsigned int num = 0;

    if(drv->camera.sensor->sensor_info.wdr_en)
        num = 2;
    else
        num = 1;

    snap_vaddr = rmem_alloc_aligned(imagesize * num, PAGE_SIZE);
    if (NULL == snap_vaddr){
        printk(KERN_ERR "not enough mem(%u) space for vic dma ~~~\n", imagesize*num);
        return -1;
    }

    memset(snap_vaddr, 0xaa, imagesize * num);
    dma_cache_sync(NULL, snap_vaddr, imagesize * num, DMA_TO_DEVICE);

    snap_paddr = virt_to_phys((void *)snap_vaddr);

    if (snap_paddr){
        vic_set_bit(index, VIC_ADDR_DMA_CONFIGURE, Dma_en, 0);
        // vic_write_reg(index, VIC_ADDR_VC_CONTROL_TIZIANO_ROUTE, 0x4444);
        // vic_write_reg(index, VIC_ADDR_VC_CONTROL_DMAOUT_ROUTE, 0x4410);
        vic_write_reg(index, VIC_ADDR_DMA_RESET, 0x01);
        vic_write_reg(index, VIC_ADDR_DMA_RESOLUTION, image_width << 16 | image_height);
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH_STRIDE, lineoffset);
        vic_write_reg(index, VIC_ADDR_DMA_UV_CH_STRIDE, lineoffset);

        if(drv->camera.sensor->sensor_info.wdr_en) {
            printk(KERN_ERR "imagesize %u wdren %d\n", imagesize*num, drv->camera.sensor->sensor_info.wdr_en);

            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK0_ADDR, snap_paddr + imagesize * 0);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK1_ADDR, snap_paddr + imagesize * 2);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK2_ADDR, snap_paddr + imagesize * 4);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK3_ADDR, snap_paddr + imagesize * 6);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK4_ADDR, snap_paddr + imagesize * 8);
        } else {
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK0_ADDR, snap_paddr + imagesize * 0);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK1_ADDR, snap_paddr + imagesize * 1);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK2_ADDR, snap_paddr + imagesize * 2);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK3_ADDR, snap_paddr + imagesize * 3);
            vic_write_reg(index, VIC_ADDR_DMA_Y_CH0_BANK4_ADDR, snap_paddr + imagesize * 4);
        }
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH1_BANK0_ADDR, snap_paddr + imagesize * 1);
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH1_BANK1_ADDR, snap_paddr + imagesize * 3);
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH1_BANK2_ADDR, snap_paddr + imagesize * 5);
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH1_BANK3_ADDR, snap_paddr + imagesize * 7);
        vic_write_reg(index, VIC_ADDR_DMA_Y_CH1_BANK4_ADDR, snap_paddr + imagesize * 9);

        vic_write_reg(index, VIC_ADDR_DMA_CONFIGURE, (0x1<<31) | (num << 16) | (4 << 3) | (0 << 0));

        while(loop){
            ret = private_wait_for_completion_interruptible(&drv->snap_raw_comp);
            if (ret >= 0)
                break;
            loop--;
        }
        if(!loop){
            printk(KERN_ERR "snapraw timeout!\n");
            goto exit;
        }
        drv->dma_complete = 0;
        vic_set_bit(index, VIC_ADDR_DMA_CONFIGURE, Dma_en, 0);

        m_cache_sync(snap_vaddr, imagesize * num);

        /* save raw */
        char dmaout_file_name[64];
        int i = 0;
        for(i = 0; i < num; i++) {
            snprintf(dmaout_file_name, sizeof(dmaout_file_name), "/tmp/snap%d.%s", i, "raw");
            fd = private_filp_open(dmaout_file_name, O_CREAT | O_WRONLY | O_TRUNC, 00766);
            if (fd < 0) {
                printk(KERN_ERR "Failed to open %s\n", dmaout_file_name);
                goto exit;
            }
            old_fs = private_get_fs();
            private_set_fs(KERNEL_DS);
            pos = &(fd->f_pos);
            private_vfs_write(fd, snap_vaddr + imagesize * i, imagesize, pos);
            private_filp_close(fd, NULL);
            private_set_fs(old_fs);
            printk(KERN_ERR "snapraw %s sucess addr %p\n", dmaout_file_name, snap_vaddr + imagesize * i);
        }
    }

exit:
    if (snap_paddr){
        rmem_free(snap_vaddr, imagesize *num);
        // private_kfree(snap_vaddr);
        snap_paddr = 0;
    }

    return ret;
}

static ssize_t dsysfs_vic_tiziano_ctrl(struct file *file, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t pos, size_t count)
{
    struct jz_vic_tiziano_data *drv = container_of(kobj, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;

    if (!strncmp(buf, "snapraw", sizeof("snapraw")-1)) {
        if(!vic_stream_state(index)) {
            printk(KERN_ERR "%s sensor doesn't work, please stream on\n", __func__);
            goto exit;
        }
        dsysfs_vic_tiziano_snap_raw(drv);

    } else {
        dsysfs_vic_ctrl(index, buf);
    }

exit:
    return count;
}


static DSYSFS_DEV_ATTR(show_frm_cnt, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_show_frame_cnt, NULL);
static DSYSFS_DEV_ATTR(dump_vic_reg, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_dump_reg, NULL);
static DSYSFS_DEV_ATTR(show_sensor_info, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_show_sensor_info, NULL);
static struct attribute *dsysfs_vic_dev_attrs[] = {
    &dsysfs_dev_attr_show_frm_cnt.attr,
    &dsysfs_dev_attr_dump_vic_reg.attr,
    &dsysfs_dev_attr_show_sensor_info.attr,
    NULL,
};

static DSYSFS_BIN_ATTR(ctrl, S_IRUGO|S_IWUSR, NULL, dsysfs_vic_tiziano_ctrl, 0);
static struct bin_attribute *dsysfs_vic_bin_attrs[] = {
    &dsysfs_bin_attr_ctrl,
    NULL,
};

static const struct attribute_group dsysfs_vic_attr_group = {
    .attrs  = dsysfs_vic_dev_attrs,
    .bin_attrs = dsysfs_vic_bin_attrs,
};

struct kobject *dsysfs_get_root_dir(int index)
{
    return &(jz_vic_tiziano_dev[index].dsysfs_parent_kobj);
}
#endif


#define error_if(_cond)                                                 \
    do {                                                                \
        if (_cond) {                                                    \
            printk(KERN_ERR "vic(tiziano): failed to check: %s\n", #_cond);\
            ret = -1;                                                   \
            goto unlock;                                                \
        }                                                               \
    } while (0)


/*
 * 格式信息转换 转换后的格式提供给ISP
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                    Camera 格式
 * [成员0 sensor format]  <--->  [成员1 camera format]
 *
 */
struct fmt_pair {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};

static struct fmt_pair fmts[] = {
    {SENSOR_PIXEL_FMT_SBGGR8_1X8,       CAMERA_PIX_FMT_SBGGR8},
    {SENSOR_PIXEL_FMT_SGBRG8_1X8,       CAMERA_PIX_FMT_SGBRG8},
    {SENSOR_PIXEL_FMT_SGRBG8_1X8,       CAMERA_PIX_FMT_SGRBG8},
    {SENSOR_PIXEL_FMT_SRGGB8_1X8,       CAMERA_PIX_FMT_SRGGB8},
    {SENSOR_PIXEL_FMT_SBGGR10_1X10,     CAMERA_PIX_FMT_SBGGR10},
    {SENSOR_PIXEL_FMT_SGBRG10_1X10,     CAMERA_PIX_FMT_SGBRG10},
    {SENSOR_PIXEL_FMT_SGRBG10_1X10,     CAMERA_PIX_FMT_SGRBG10},
    {SENSOR_PIXEL_FMT_SRGGB10_1X10,     CAMERA_PIX_FMT_SRGGB10},
    {SENSOR_PIXEL_FMT_SBGGR12_1X12,     CAMERA_PIX_FMT_SBGGR12},
    {SENSOR_PIXEL_FMT_SGBRG12_1X12,     CAMERA_PIX_FMT_SGBRG12},
    {SENSOR_PIXEL_FMT_SGRBG12_1X12,     CAMERA_PIX_FMT_SGRBG12},
    {SENSOR_PIXEL_FMT_SRGGB12_1X12,     CAMERA_PIX_FMT_SRGGB12},
};

static int sensor_attribute_check_init(int index, struct sensor_attr *sensor)
{
    int ret = -EINVAL;

    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 128 || sensor->sensor_info.width > 4096);
    error_if(sensor->sensor_info.height < 128);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    if ( (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (sensor->mipi.mipi_crop.enable) ) {
        sensor->info.width =  sensor->mipi.mipi_crop.output_width;
        sensor->info.height =  sensor->mipi.mipi_crop.output_height;
    } else {
        sensor->info.width =  sensor->sensor_info.width;
        sensor->info.height =  sensor->sensor_info.height;
    }

    int i = 0;
    int size = ARRAY_SIZE(fmts);
    for (i = 0; i < size; i++) {
        if (sensor->sensor_info.fmt == fmts[i].sensor_fmt)
            break;
    }

    if (i >= size) {
        printk(KERN_ERR "attribute check: sensor data_fmt(0x%x) is NOT support.\n", sensor->sensor_info.fmt);
        goto unlock;
    }

    sensor->info.data_fmt = fmts[i].camera_fmt;

    if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
        switch (sensor->dvp.gpio_mode) {
        case DVP_PA_10BIT:
            if (sensor->dvp.data_fmt > DVP_RAW10) {
                printk(KERN_ERR "attribute check: data_fmt set error,should be less than DVP_RAW10.\n");
                goto unlock;
            }
            break;

        case DVP_PA_8BIT:
            if (sensor->dvp.data_fmt < DVP_YUV422){
                if (sensor->dvp.data_fmt > DVP_RAW8) {
                    printk(KERN_ERR "attribute check: data_fmt set error,should be DVP_RAW8.\n");
                    goto unlock;
                }
            }
            break;

        default:
            printk(KERN_ERR "attribute check: Unsupported this format.\n");
            goto unlock;
        }
    }
    return 0;

unlock:
    return ret;
}

int vic_register_sensor_route_tiziano(int index, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    ret = sensor_attribute_check_init(index, sensor);
    assert(ret == 0);

    mutex_lock(&drv->lock);

    ret = isp_component_bind_sensor(index, sensor);
    if (!ret)
        drv->camera.sensor = sensor;

    mutex_unlock(&drv->lock);

    return ret;
}

void vic_unregister_sensor_route_tiziano(int index, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    isp_component_unbind_sensor(index, sensor);

    drv->camera.sensor = NULL;

    mutex_unlock(&drv->lock);
}


int jz_vic_tiziano_drv_init(int index)
{
    assert(index < 2);
    unsigned long flags = 0;

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    int ret;

    mutex_init(&drv->lock);

    ret = request_irq(drv->irq, vic_irq_isp_handler, 0, drv->irq_name, (void *)drv);
    if (ret) {
        printk(KERN_ERR "camera: vic(tiziano) failed to request irq\n");
        goto error_request_irq;
    }

    private_spin_lock_irqsave(&drv->slock, flags);
    disable_irq(drv->irq);
    private_spin_unlock_irqrestore(&drv->slock, flags);

#ifdef SOC_CAMERA_DEBUG
    char dsysfs_root_dir_name[16];

    sprintf(dsysfs_root_dir_name ,"isp%d", drv->index);
    ret = dsysfs_create_dir(&drv->dsysfs_parent_kobj, NULL, dsysfs_root_dir_name);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create root dir fail\n", index);
        goto error_dsys_create_root;
    }

    ret = dsysfs_create_group(&drv->dsysfs_kobj, &drv->dsysfs_parent_kobj, "vic", &dsysfs_vic_attr_group);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create sub dir vic fail\n", index);
        goto error_dsys_create_vic;
    }

    init_completion(&drv->snap_raw_comp);
#endif

    ret = jz_isp_drv_init(index);
    if (ret) {
        printk(KERN_ERR "camera: failed to init isp%d resources\n", index);
        goto error_isp_drv_init;
    }

    drv->is_finish = 1;

    printk(KERN_DEBUG "vic(tiziano) register successfully\n");

    return 0;

error_isp_drv_init:
#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_vic_attr_group);
error_dsys_create_vic:
    dsysfs_remove_dir(&drv->dsysfs_parent_kobj);
error_dsys_create_root:
#endif
    free_irq(drv->irq, drv);

error_request_irq:
    return ret;
}

void jz_vic_tiziano_drv_deinit(int index)
{
    assert(index < 2);
    unsigned long flags = 0;

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;
    private_spin_lock_irqsave(&drv->slock, flags);
    free_irq(drv->irq, drv);
    private_spin_unlock_irqrestore(&drv->slock, flags);

    jz_isp_drv_deinit(index);

#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_vic_attr_group);
    dsysfs_remove_dir(&drv->dsysfs_parent_kobj);
#endif
}
