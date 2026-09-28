/*
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Input file for Ingenic DBOX driver
 *
 * This  program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <soc/base.h>
#include "ingenic_drawbox.h"

// #define DEBUG
#ifdef  DEBUG
#define DBOX_DEBUG(format, ...) { printk(KERN_ERR format, ## __VA_ARGS__);}
#else
#define DBOX_DEBUG(format, ...) do{ } while(0)
#endif

#define DBOX_BUF_SIZE (1024 * 1024 * 2)

#define ALIGN_DOWN(x, a)    ( ( (x) / (a)) * (a) )

static void dbox_dump_regs(void)
{
    printk(KERN_ERR "----- dbox start dump regs -----\n");
    printk(KERN_ERR "DBOX_CTRL:     \t0x%08x\r\n", dbox_reg_read(DBOX_CTRL));
    printk(KERN_ERR "DBOX_YB:       \t0x%08x\r\n", dbox_reg_read(DBOX_YB));
    printk(KERN_ERR "DBOX_CB:       \t0x%08x\r\n", dbox_reg_read(DBOX_CB));
    printk(KERN_ERR "DBOX_STRIDE:   \t0x%08x\r\n", dbox_reg_read(DBOX_STRIDE));
    printk(KERN_ERR "DBOX_IMG_WH:   \t0x%08x\r\n", dbox_reg_read(DBOX_IMG_WH));
    printk(KERN_ERR "DBOX_COLOR_Y:  \t0x%08x\r\n", dbox_reg_read(DBOX_COLOR_Y));
    printk(KERN_ERR "DBOX_COLOR_U:  \t0x%08x\r\n", dbox_reg_read(DBOX_COLOR_U));
    printk(KERN_ERR "DBOX_RAM:      \t0x%08x\r\n", dbox_reg_read(DBOX_RAM));
    printk(KERN_ERR "DBOX_TIMEOUT:  \t0x%08x\r\n", dbox_reg_read(DBOX_TIMEOUT));
    printk(KERN_ERR "------ dbox stop dump regs ------\n");
}

static void dbox_dump_info(struct jz_dbox *dbox)
{
    if (dbox == NULL) {
        dev_err(dbox->dev, "dbox is NULL\n");
        return ;
    }
    printk(KERN_ERR "dbox: dbox->base: %p\n", dbox->iomem);
    dbox_dump_regs();
}

static int dbox_set_drawing_info(struct jz_dbox *dbox, struct dbox_param *dbox_param)
{
    unsigned int imgw = 0;
    unsigned int imgh = 0;
    unsigned int rectw;
    unsigned int recth;
    unsigned int rectx;
    unsigned int recty;
    unsigned int linew;
    unsigned int linel;
    unsigned int img_pbuf_y = 0;
    unsigned int img_pbuf_uv = 0;
    unsigned int boxs_num = 0;
    unsigned int is_rgba = 0;
    unsigned int i = 0;

    unsigned int dbox_colormode;
    unsigned int dbox_mode;


    struct dbox_param *ip = dbox_param;
    if (dbox == NULL) {
        dev_err(dbox->dev, "dbox: dbox is NULL or dbox_param is NULL\n");
        return -1;
    }

    img_pbuf_y = ip->box_pbuf;

    imgw = ip->img_w;
    imgh = ip->img_h;
    boxs_num = ip->boxs_num;
    is_rgba = ip->is_rgba;

    if (1 == is_rgba) {
        dbox_set_bit(DBOX_CTRL, DBOX_IS_BGRA, 1);
    } else {
        dbox_set_bit(DBOX_CTRL, DBOX_IS_BGRA, 0);
    }

    img_pbuf_y = ip->box_pbuf;
    img_pbuf_uv = img_pbuf_y + imgw*imgh;

    dbox_set_bit(DBOX_IMG_WH, DBOX_WIDTH, imgw);
    dbox_set_bit(DBOX_IMG_WH, DBOX_HEIGHT, imgh);

    dbox_set_bit(DBOX_STRIDE, DBOX_Y_STRIDE, imgw);
    dbox_set_bit(DBOX_STRIDE, DBOX_C_STRIDE, imgw);

    /* YUV[0] is red
     * YUV[1] is black
     * YUV[2] is green
     * YUV[3] is yellow
     * rgb2yuv ==> https://www.mikekohn.net/file_formats/yuv_rgb_converter.php */
    dbox_reg_write(DBOX_COLOR_Y, 0xe195004c);
    dbox_reg_write(DBOX_COLOR_U, 0x002b8054);
    dbox_reg_write(DBOX_COLOR_V, 0x941580ff);

    dbox_reg_write(DBOX_YB, img_pbuf_y);
    dbox_reg_write(DBOX_CB, img_pbuf_uv);

    for(i=0; i<boxs_num; i++) {
        rectx = ALIGN_DOWN(ip->ram_para[i].box_x, 4);
        recty = ALIGN_DOWN(ip->ram_para[i].box_y, 4);
        rectw = ALIGN_DOWN(ip->ram_para[i].box_w, 4);
        recth = ALIGN_DOWN(ip->ram_para[i].box_h, 4);
        linew = ip->ram_para[i].line_w;
        linel = ALIGN_DOWN(ip->ram_para[i].line_l, 4);
        dbox_mode = ip->ram_para[i].box_mode;
        dbox_colormode = ip->ram_para[i].color_mode;
        dbox_reg_write(DBOX_RAM, (rectx << DBOX_BOX_X) | (recty << DBOX_BOX_Y) | (rectw << DBOX_BOX_WIDTH));
        dbox_reg_write(DBOX_RAM, (((rectw >> 8) << 0) | (recth << 4) | (dbox_colormode << 16) | (dbox_mode << 18) | (linew << 20) | (linel << 23)));
    }

    return 0;
}

static int dbox_start(struct jz_dbox *dbox, struct dbox_param *dbox_param)
{
    int ret = 0;
    struct dbox_param *ip = dbox_param;

    if ((dbox == NULL) || (dbox_param == NULL)) {
        dev_err(dbox->dev, "dbox: dbox is NULL or dbox_param is NULL\n");
        return -1;
    }
    DBOX_DEBUG("dbox: enter dbox_start %d\n", current->pid);

    clk_prepare_enable(dbox->clk);
    clk_prepare_enable(dbox->ahb0_gate);

    dbox_set_bit(DBOX_CTRL, DBOX_RESET, 1);
    /* wait reset complete */
    while(dbox_get_bit(DBOX_CTRL, DBOX_RESET)) {
        udelay(10);
    }

    /* config draw rect info */
    ret = dbox_set_drawing_info(dbox, ip);

    /* uv mask */
    dbox_set_bit(DBOX_CTRL, DBOX_IRQ_MASK, 0);

    DBOX_DEBUG("dbox_start\n");
    /* start dbox */
    dbox_set_bit(DBOX_CTRL, DBOX_START, 1);

#ifdef DEBUG
    dbox_dump_info(dbox);
#endif
    DBOX_DEBUG("dbox_start\n");

    ret = wait_for_completion_interruptible_timeout(&dbox->done_dbox, msecs_to_jiffies(5000));
    if (ret < 0) {
        printk(KERN_ERR "dbox: done_dbox wait_for_completion_interruptible_timeout err %d\n", ret);
        goto err_dbox_wait_for_done;
    } else if (ret == 0 ) {
        ret = -1;
        printk(KERN_ERR "dbox: done_dbox wait_for_completion_interruptible_timeout timeout %d\n", ret);
        dbox_dump_info(dbox);
        goto err_dbox_wait_for_done;
    } else {
        ;
    }

    DBOX_DEBUG("dbox: exit dbox_start %d\n", current->pid);

    clk_disable_unprepare(dbox->clk);
    clk_disable_unprepare(dbox->ahb0_gate);

    return 0;

err_dbox_wait_for_done:
    clk_disable_unprepare(dbox->clk);
    clk_disable_unprepare(dbox->ahb0_gate);

    return ret;

}

static long dbox_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct dbox_param iparam;
    struct miscdevice *dev = filp->private_data;
    struct jz_dbox *dbox = container_of(dev, struct jz_dbox, misc_dev);

    DBOX_DEBUG("dbox: %s pid: %d, tgid: %d file: %p, cmd: 0x%08x\n",
            __func__, current->pid, current->tgid, filp, cmd);

    if (_IOC_TYPE(cmd) != JZDBOX_IOC_MAGIC) {
        dev_err(dbox->dev, "invalid cmd!\n");
        return -EFAULT;
    }

    mutex_lock(&dbox->mutex);

    switch (cmd) {
        case IOCTL_DBOX_START: {
            if (copy_from_user(&iparam, (void *)arg, sizeof(struct dbox_param))) {
                dev_err(dbox->dev, "copy_from_user error!!!\n");
                ret = -EFAULT;
                break;
            }
            ret = dbox_start(dbox, &iparam);
            if (ret) {
                dev_err(dbox->dev, "dbox: error dbox start ret = %d\n", ret);
            }
            break;
        }
        case IOCTL_DBOX_GET_PBUFF: {
            if (dbox->pbuf.vaddr_alloc == NULL) {
                unsigned int size = DBOX_BUF_SIZE;
                dbox->pbuf.vaddr_alloc = (void *)kmalloc(size, GFP_KERNEL);
                if (!dbox->pbuf.vaddr_alloc) {
                    dev_err(dbox->dev, "dbox kmalloc is error\n");
                    ret = -ENOMEM;
                }
                memset((dbox->pbuf.vaddr_alloc), 0, size);
                dbox->pbuf.size = size;
                dbox->pbuf.paddr = (void *)virt_to_phys((dbox->pbuf.vaddr_alloc));
                dbox->pbuf.paddr_align = dbox->pbuf.paddr;
            }
            DBOX_DEBUG("dbox: %s dbox->pbuf.vaddr_alloc = %p\ndbox->pbuf.vaddr_align = %p\ndbox->pbuf.size = 0x%x\ndbox->pbuf.paddr = %p\n"
                    , __func__, dbox->pbuf.vaddr_alloc, dbox->pbuf.paddr_align, dbox->pbuf.size, dbox->pbuf.paddr);
            if (copy_to_user((void *)arg, &dbox->pbuf, sizeof(struct dbox_buf_info))) {
                dev_err(dbox->dev, "copy_to_user error!!!\n");
                ret = -EFAULT;
            }
            break;
        }
        case IOCTL_DBOX_RES_PBUFF: {
            if (dbox->pbuf.vaddr_alloc != NULL) {
                kfree(dbox->pbuf.vaddr_alloc);
                dbox->pbuf.vaddr_alloc = NULL;
                dbox->pbuf.size = 0;
                dbox->pbuf.paddr = NULL;
                dbox->pbuf.paddr_align = NULL;
            } else {
                dev_warn(dbox->dev, "buffer wanted to free is null\n");
            }
            break;
        }
        case IOCTL_DBOX_BUF_LOCK: {
            ret = wait_for_completion_interruptible_timeout(&dbox->done_buf, msecs_to_jiffies(2000));
            if (ret < 0) {
                dev_err(dbox->dev, "dbox: done_buf wait_for_completion_interruptible_timeout err %d\n", ret);
            } else if (ret == 0 ) {
                dev_err(dbox->dev, "dbox: done_buf wait_for_completion_interruptible_timeout timeout %d\n", ret);
                ret = -1;
                dbox_dump_info(dbox);
            } else {
                ret = 0;
            }
            break;
        }
        case IOCTL_DBOX_BUF_UNLOCK: {
            complete(&dbox->done_buf);
            break;
        }
        case IOCTL_DBOX_BUF_FLUSH_CACHE: {
            struct dbox_flush_cache_para fc;
            if (copy_from_user(&fc, (void *)arg, sizeof(fc))) {
                dev_err(dbox->dev, "copy_from_user error!!!\n");
                ret = -EFAULT;
                break;
            }
            dma_cache_sync(NULL, fc.addr, fc.size, DMA_BIDIRECTIONAL);
            break;
        }
        default:
            dev_err(dbox->dev, "invalid command: 0x%08x\n", cmd);
            ret = -EINVAL;
    }

    mutex_unlock(&dbox->mutex);
    return ret;
}

static int dbox_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    struct miscdevice *dev = filp->private_data;
    struct jz_dbox *dbox = container_of(dev, struct jz_dbox, misc_dev);

    DBOX_DEBUG("dbox: %s pid: %d, tgid: %d filp: %p\n",
            __func__, current->pid, current->tgid, filp);
    mutex_lock(&dbox->mutex);

    mutex_unlock(&dbox->mutex);
    return ret;
}

static int dbox_release(struct inode *inode, struct file *filp)
{
    int ret = 0;

    struct miscdevice *dev = filp->private_data;
    struct jz_dbox *dbox = container_of(dev, struct jz_dbox, misc_dev);

    DBOX_DEBUG("dbox: %s  pid: %d, tgid: %d filp: %p\n",
            __func__, current->pid, current->tgid, filp);
    mutex_lock(&dbox->mutex);

    mutex_unlock(&dbox->mutex);
    return ret;
}

static struct file_operations dbox_ops = {
    .owner = THIS_MODULE,
    .open = dbox_open,
    .release = dbox_release,
    .unlocked_ioctl = dbox_ioctl,
};

static irqreturn_t dbox_irq_handler(int irq, void *data)
{
    struct jz_dbox *dbox;
    unsigned int status;

    DBOX_DEBUG("dbox: %s\n", __func__);
    dbox = (struct jz_dbox *)data;

    status = dbox_reg_read(DBOX_CTRL);

    DBOX_DEBUG("----- %s, status= 0x%08x\n", __func__, status);
    /* this status doesn't do anything including trigger interrupt,
     * just give a hint */
    if (status & 0x10){
        complete(&dbox->done_dbox);
    }

    dbox_set_bit(DBOX_CTRL, DBOX_IRQ_OPEN, 0);

    return IRQ_HANDLED;
}

static int dbox_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct jz_dbox *dbox;

    DBOX_DEBUG("%s\n", __func__);
    dbox = (struct jz_dbox *)kzalloc(sizeof(struct jz_dbox), GFP_KERNEL);
    if (!dbox) {
        dev_err(&pdev->dev, "alloc jz_dbox failed!\n");
        return -ENOMEM;
    }

    sprintf(dbox->name, "dbox");

    dbox->misc_dev.minor = MISC_DYNAMIC_MINOR;
    dbox->misc_dev.name = dbox->name;
    dbox->misc_dev.fops = &dbox_ops;
    dbox->dev = &pdev->dev;

    mutex_init(&dbox->mutex);
    init_completion(&dbox->done_dbox);
    init_completion(&dbox->done_buf);
    complete(&dbox->done_buf);

    dbox->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!dbox->res) {
        dev_err(&pdev->dev, "failed to get dev resources: %d\n", ret);
        ret = -EINVAL;
        goto err_get_platform_res;
    }

    dbox->res = request_mem_region(dbox->res->start,
            dbox->res->end - dbox->res->start + 1,
            pdev->name);
    if (!dbox->res) {
        dev_err(&pdev->dev, "failed to request regs memory region");
        ret = -EINVAL;
        goto err_get_mem_region;
    }
    dbox->iomem = ioremap(dbox->res->start, resource_size(dbox->res));
    if (!dbox->iomem) {
        dev_err(&pdev->dev, "failed to remap regs memory region: %d\n",ret);
        ret = -EINVAL;
        goto err_ioremap;
    }

    dbox->irq = platform_get_irq(pdev, 0);
    if (request_irq(dbox->irq, dbox_irq_handler, IRQF_SHARED, dbox->name, dbox)) {
        dev_err(&pdev->dev, "request irq failed\n");
        ret = -EINVAL;
        goto err_req_irq;
    }

    dbox->clk = devm_clk_get(dbox->dev, "gate_drawbox");
    if (IS_ERR(dbox->clk)) {
        dev_err(&pdev->dev, "dbox clk get failed!\n");
        ret = -EINVAL;
        goto err_get_dbox_clk;
    }
    dbox->ahb0_gate = devm_clk_get(dbox->dev, "gate_ahb0");
    if (IS_ERR(dbox->clk)) {
        dev_err(&pdev->dev, "dbox clk get failed!\n");
        ret = -EINVAL;
        goto err_get_ahb0_clk;
    }

    dev_set_drvdata(&pdev->dev, dbox);

    ret = misc_register(&dbox->misc_dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "register misc device failed!\n");
        goto err_misc_register;
    }

    DBOX_DEBUG("Ingenic drawbox probe sucess!\n");
    return 0;

err_misc_register:
    devm_clk_put(dbox->dev, dbox->ahb0_gate);
err_get_ahb0_clk:
    devm_clk_put(dbox->dev, dbox->clk);
err_get_dbox_clk:
    free_irq(dbox->irq, dbox);
err_req_irq:
    iounmap(dbox->iomem);
err_ioremap:
    release_mem_region(dbox->res->start, dbox->res->end - dbox->res->start + 1);
err_get_platform_res:
err_get_mem_region:
    kfree(dbox);

    return ret;
}

static int dbox_remove(struct platform_device *pdev)
{
    struct jz_dbox *dbox;
    struct resource *res;
    DBOX_DEBUG("%s\n", __func__);

    dbox = dev_get_drvdata(&pdev->dev);
    misc_deregister(&dbox->misc_dev);
    devm_clk_put(dbox->dev, dbox->ahb0_gate);
    devm_clk_put(dbox->dev, dbox->clk);
    res = dbox->res;
    free_irq(dbox->irq, dbox);
    iounmap(dbox->iomem);
    release_mem_region(res->start, res->end - res->start + 1);

    if (dbox->pbuf.vaddr_alloc) {
        kfree(dbox->pbuf.vaddr_alloc);
        dbox->pbuf.vaddr_alloc = NULL;
    }
    if (dbox) {
        kfree(dbox);
    }

    return 0;
}


#ifdef CONFIG_PM_SLEEP
static int ingenic_drawbox_suspend(struct device *dev)
{
    struct jz_dbox *dbox = dev_get_drvdata(dev);
    if(__clk_is_enabled(dbox->clk)){
        clk_disable_unprepare(dbox->clk);
    }
    return 0;
}
static int ingenic_drawbox_resume(struct device *dev)
{
    struct jz_dbox *dbox = dev_get_drvdata(dev);
    clk_prepare_enable(dbox->clk);
    return 0;
}
static SIMPLE_DEV_PM_OPS(ingenic_drawbox_pm_ops, ingenic_drawbox_suspend, ingenic_drawbox_resume);
#endif



static const struct of_device_id ingenic_dbox_dt_match[] = {
    { .compatible = "ingenic,x2580-drawbox", .data = NULL },
    {},
};

static struct platform_driver jz_dbox_driver = {
    .probe    = dbox_probe,
    .remove = dbox_remove,
    .driver = {
        .name = "jz-drawbox",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(ingenic_dbox_dt_match),
#ifdef CONFIG_PM_SLEEP
        .pm = &ingenic_drawbox_pm_ops,
#endif

    },
};

int __init dboxdev_init(void)
{
    DBOX_DEBUG("%s\n", __func__);
    platform_driver_register(&jz_dbox_driver);
    return 0;
}

void __exit dboxdev_exit(void)
{
    DBOX_DEBUG("%s\n", __func__);
    platform_driver_unregister(&jz_dbox_driver);
}
module_init(dboxdev_init);
module_exit(dboxdev_exit);

MODULE_DESCRIPTION("Used to draw rectangular wire frame and other");
MODULE_LICENSE("GPL");
