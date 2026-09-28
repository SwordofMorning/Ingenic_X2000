#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <common.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <soc/base.h>
#include <bit_field.h>

#include <linux/gpio.h>
#include <utils/gpio.h>

#include <utils/clock.h>
#include <utils/ring_mem.h>

#include "core/x2580_riscv.h"

static DEFINE_MUTEX(m_lock);

int mcu_write_data(void *buf, unsigned int size, unsigned long timeout)
{
    int w_size;
    int ret = size;
    unsigned long min_timeout = 100;
    unsigned long long now;

    struct chip_header *header = riscv_get_header();
    struct ring_mem *ring = (struct ring_mem *)CKSEG1ADDR(header->ring_mem_for_host_write);

    mutex_lock(&m_lock);

    if (!riscv_resume()) {
        printk(KERN_ERR "mcu: mcu is not ready\n");
        ret = -ENODEV;
        goto unlock;
    }

    if (!ring->mem_size) {
        printk(KERN_ERR "mcu: ring mem is not inited\n");
        ret = -EBUSY;
        goto unlock;
    }

    if (!size)
        goto unlock;

    ring_mem_set_virt_addr_for_write(ring, (void *)CKSEG1ADDR(ring->mem_addr));

    if (timeout && timeout < min_timeout)
        timeout = min_timeout;

    now = local_clock_us();
    while (size) {
        w_size = ring_mem_write(ring, buf, size);
        if (!w_size) {
            if (!timeout)
                break;

            if (local_clock_us() - now > timeout) {
                printk(KERN_ERR "mcu: mcu write mem timeout\n");
                break;
            }
            usleep_range(min_timeout, min_timeout);
        }
        size -= w_size;
        buf += w_size;
    }

    ret = ret - size;
    if (ret)
        host_notify_riscv();

unlock:
    mutex_unlock(&m_lock);

    return ret;
}
EXPORT_SYMBOL(mcu_write_data);

int mcu_read_data(void *buf, unsigned int size, unsigned long timeout)
{
    int r_size;
    int ret = size;
    unsigned long min_timeout = 100;

    struct chip_header *header = riscv_get_header();
    struct ring_mem *ring = (struct ring_mem *)CKSEG1ADDR(header->ring_mem_for_host_read);

    mutex_lock(&m_lock);

    if (!riscv_resume()) {
        printk(KERN_ERR "mcu: mcu is not ready\n");
        ret = -ENODEV;
        goto unlock;
    }

    if (!ring->mem_size) {
        printk(KERN_ERR "mcu: ring mem is not inited\n");
        ret = -EBUSY;
        goto unlock;
    }

    if (!size)
        goto unlock;

    ring_mem_set_virt_addr_for_read(ring, (void *)CKSEG1ADDR(ring->mem_addr));

    if (ring_mem_readable_size(ring))
        riscv_set_status(1);

    if (timeout && timeout < min_timeout)
        timeout = min_timeout;

    if (wait_riscv_wakeup(&timeout)) {
        ret = -ETIMEDOUT;
        goto unlock;
    }

    while (size) {
        r_size = ring_mem_read(ring, buf, size);
        if (!r_size) {
            if (!timeout)
                break;

            if (wait_riscv_wakeup(&timeout)) {
                printk(KERN_ERR "mcu: mcu read mem timeout\n");
                break;
            }
        }
        size -= r_size;
        buf += r_size;
    }

    ret = ret - size;

unlock:
    mutex_unlock(&m_lock);

    return ret;
}
EXPORT_SYMBOL(mcu_read_data);

int mcu_read_str(char *buf, int buf_size, unsigned long timeout)
{
    int r_size;
    int ret = buf_size;
    unsigned long min_timeout = 100;

    struct chip_header *header = riscv_get_header();
    struct ring_mem *ring = (struct ring_mem *)CKSEG1ADDR(header->ring_mem_for_host_read);

    mutex_lock(&m_lock);

    if (!riscv_resume()) {
        printk(KERN_ERR "mcu: mcu is not ready\n");
        ret = -ENODEV;
        goto unlock;
    }

    if (!ring->mem_size) {
        printk(KERN_ERR "mcu: ring mem is not inited\n");
        ret = -EBUSY;
        goto unlock;
    }

    if (!buf_size)
        goto unlock;

    ring_mem_set_virt_addr_for_read(ring, (void *)CKSEG1ADDR(ring->mem_addr));

    if (ring_mem_readable_size(ring))
        riscv_set_status(1);

    if (timeout && timeout < min_timeout)
        timeout = min_timeout;

    if (wait_riscv_wakeup(&timeout)) {
        ret = -ETIMEDOUT;
        goto unlock;
    }

    while (buf_size) {
        r_size = ring_mem_read(ring, buf, 1);
        if (!r_size) {
            if (!timeout)
                break;

            if (wait_riscv_wakeup(&timeout)) {
                printk(KERN_ERR "mcu: mcu read mem timeout\n");
                break;
            }
        }

        buf_size -= r_size;

        if (buf[0] == '\0')
            break;

        buf += r_size;
    }

    ret = ret - buf_size;

unlock:
    mutex_unlock(&m_lock);

    return ret;
}

#define MCU_MAGIC_NUMBER    'M'

#define MCU_WRITE_DATA_TIMEOUT    _IOW(MCU_MAGIC_NUMBER, 118, void *)
#define MCU_READ_DATA_TIMEOUT     _IOW(MCU_MAGIC_NUMBER, 119, void *)
#define MCU_READ_STR_TIMEOUT      _IOW(MCU_MAGIC_NUMBER, 120, void *)

static long mcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case MCU_WRITE_DATA_TIMEOUT: {
        unsigned long *array = (void *) arg;
        void *src = (void *)array[0];
        int len = array[1];
        unsigned long timeout_us = array[2];

        return mcu_write_data(src, len, timeout_us);
    }

    case MCU_READ_DATA_TIMEOUT: {
        unsigned long *array = (void *) arg;
        void *dst = (void *)array[0];
        int len = array[1];
        unsigned long timeout_us = array[2];

        return mcu_read_data(dst, len, timeout_us);
    }

    case MCU_READ_STR_TIMEOUT: {
        unsigned long *array = (void *) arg;
        void *dst = (void *)array[0];
        int len = array[1];
        unsigned long timeout_us = array[2];

        return mcu_read_str(dst, len, timeout_us);
    }

    default:
        return riscv_core_ioctl(cmd, arg);
    }

    return -ENODEV;
}

static int mcu_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int mcu_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations mcu_misc_fops = {
    .open = mcu_open,
    .release = mcu_release,
    .unlocked_ioctl = mcu_ioctl,
};

static struct miscdevice mcu_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mcu",
    .fops = &mcu_misc_fops,
};

static int __init mcu_init(void)
{
    int ret = misc_register(&mcu_mdev);
    assert(!ret);

    ret = riscv_core_init(&m_lock);
    assert(!ret);

    return 0;
}

static void mcu_exit(void)
{
    misc_deregister(&mcu_mdev);
    riscv_core_deinit();
}

module_init(mcu_init);
module_exit(mcu_exit);
MODULE_LICENSE("GPL");