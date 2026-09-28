#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <soc/cpm.h>

#include "adc.h"
#include "adc_reg.h"

#define ADC_MAGIC_NUMBER                'A'
#define ADC_ENABLE                      _IO(ADC_MAGIC_NUMBER, 11)
#define ADC_DISABLE                     _IO(ADC_MAGIC_NUMBER, 22)
#define ADC_SET_VREF                    _IOW(ADC_MAGIC_NUMBER, 33, unsigned int)
#define ADC_GET_VREF                    _IOWR(ADC_MAGIC_NUMBER, 44, unsigned int)
#define ADC_GET_VALUE                   _IOWR(ADC_MAGIC_NUMBER, 60, unsigned int)
#define ADC_SET_CHANNEL                 _IOW(ADC_MAGIC_NUMBER, 61, unsigned int)

#define ADC_SEQ0_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 70, struct adc_seq0_config)
#define ADC_SEQ0_DISABLE                _IOW(ADC_MAGIC_NUMBER, 71, struct adc_seq0_config)
#define ADC_SEQ0_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 72, void *)
#define ADC_SEQ1_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 73, struct adc_seq1_config)
#define ADC_SEQ1_DISABLE                _IOW(ADC_MAGIC_NUMBER, 74, struct adc_seq1_config)
#define ADC_SEQ1_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 75, void *)
#define ADC_SEQ2_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 76, struct adc_seq2_config)
#define ADC_SEQ2_DISABLE                _IOW(ADC_MAGIC_NUMBER, 77, struct adc_seq2_config)
#define ADC_SEQ2_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 78, void *)
#define ADC_AWD_ENABLE                  _IOW(ADC_MAGIC_NUMBER, 79, void *)
#define ADC_AWD_DISABLE                 _IO(ADC_MAGIC_NUMBER, 80)
#define ADC_AWD_READ_TIMEOUT            _IOWR(ADC_MAGIC_NUMBER, 81, void *)

#define ADC_MAX_CHANNELS                16

#define IRQ_SADC                        (IRQ_INTC_BASE + 11)

#define CPM_EXCLK_EN                    20

#define SRBC_ADC_SR                     18

struct adc_dev {
    struct clk *mux_clk;
    struct clk *div_clk;
    struct clk *gate_clk;

    struct mutex mutex;

    wait_queue_head_t seq0_wq;
    volatile unsigned char seq0_is_data_ready;
    unsigned int seq0_channel_cnt;
    unsigned short seq0_buf[4];
    unsigned char seq0_is_enable;

    wait_queue_head_t seq1_wq;
    volatile unsigned char seq1_is_data_ready;
    unsigned int seq1_channel_cnt;
    unsigned short seq1_buf[16];
    unsigned char seq1_is_enable;

    wait_queue_head_t seq2_wq;
    volatile unsigned char seq2_is_data_ready;
    unsigned int seq2_channel_cnt;
    unsigned short seq2_buf[8];
    unsigned char seq2_is_enable;

    wait_queue_head_t awd_wq;
    volatile unsigned char awd_is_trigger;
    unsigned short low_flags;
    unsigned short high_flags;
    unsigned int adc_awd_enable_channels;
};

static unsigned int VREF_ADC = 1800;
module_param_named(adc_vref, VREF_ADC, int, 0644);

static struct adc_dev adc_device;

static volatile int adc_busy = 0;
static DEFINE_SPINLOCK(spinlock);


static int adc_power_ref(void)
{
    int ret;
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);
    ret = adc_busy++;
    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

static int adc_power_unref(void)
{
    int ret;
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);
    ret = --adc_busy;
    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

int adc_enable(void)
{
    mutex_lock(&adc_device.mutex);

    if (!adc_power_ref()) {
        adc_power_on();

        adc_clean_all_interrupt_flag();
    }

    mutex_unlock(&adc_device.mutex);

    return 0;
}
EXPORT_SYMBOL(adc_enable);

int adc_disable(void)
{
    mutex_lock(&adc_device.mutex);

    if (!adc_power_unref()) {
        adc_power_off();

        adc_clean_all_interrupt_flag();
    }

    mutex_unlock(&adc_device.mutex);

    return 0;
}
EXPORT_SYMBOL(adc_disable);

static void seq0_irq_cb(void)
{
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    adc_read_seq0_data(adc_device.seq0_buf, adc_device.seq0_channel_cnt);
    adc_device.seq0_is_data_ready = 1;

    spin_unlock_irqrestore(&spinlock, flags);
    wake_up_interruptible(&adc_device.seq0_wq);
}

static int adc_seq0_read_channel_value(unsigned int channel)
{
    int ret = 0;
    struct adc_seq0_config seq0_adc;

    mutex_lock(&adc_device.mutex);

    seq0_adc.channel_cnt = 1;
    seq0_adc.irq_cb = seq0_irq_cb;
    seq0_adc.channels[0] = channel;

    adc_device.seq0_is_data_ready = 0;
    adc_device.seq0_channel_cnt = 1;

    adc_enable_seq0(&seq0_adc);

    adc_start_seq0();

    ret = wait_event_interruptible_timeout(
        adc_device.seq0_wq, adc_device.seq0_is_data_ready, msecs_to_jiffies(20));
    if (ret == 0) {
        printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
        ret = -EBUSY;
    } else if (ret < 0) {
        ret = -EAGAIN;
    } else {
        ret = adc_device.seq0_buf[0];
    }

    adc_disable_seq0(&seq0_adc);

    mutex_unlock(&adc_device.mutex);

    return ret;
}

int adc_read_channel_voltage(unsigned int channel)
{
    unsigned int sadc_val = 0;

    BUG_ON (channel >= ADC_MAX_CHANNELS);

    sadc_val = adc_seq0_read_channel_value(channel);
    if (sadc_val < 0)
        return sadc_val;

    sadc_val = sadc_val * VREF_ADC / 4096;

    return sadc_val;
}
EXPORT_SYMBOL(adc_read_channel_voltage);


static void adc_seq0_enable(struct adc_seq0_config seq0_adc)
{
    if (!adc_device.seq0_is_enable) {
        adc_device.seq0_is_enable = 1;
        adc_power_ref();
    }

    adc_device.seq0_channel_cnt = seq0_adc.channel_cnt;
    adc_device.seq0_is_data_ready = 0;

    if (seq0_adc.irq_cb)
        seq0_adc.irq_cb = seq0_irq_cb;
    else
        seq0_adc.irq_cb = NULL;

    adc_enable_seq0(&seq0_adc);

    adc_start_seq0();
}

static void adc_seq0_disable(struct adc_seq0_config seq0_adc)
{
    if (adc_device.seq0_is_enable) {
        adc_device.seq0_is_enable = 0;
        adc_power_unref();
    }

    adc_disable_seq0(&seq0_adc);
}

static int adc_seq0_read_data(unsigned short *buf, int timeout)
{
    int ret = 0;

    if (timeout < 0) {
        ret = wait_event_interruptible(adc_device.seq0_wq, adc_device.seq0_is_data_ready);
        if (ret < 0) {
            printk(KERN_ERR "%s:wait get value failed!\n", __func__);
            return ret;
        }
    } else {
        ret = wait_event_interruptible_timeout(
            adc_device.seq0_wq, adc_device.seq0_is_data_ready, msecs_to_jiffies(timeout));
        if (ret == 0) {
            printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
            return -EBUSY;
        } else if (ret < 0)
            return -EAGAIN;
    }

    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    memcpy(buf, adc_device.seq0_buf, sizeof(adc_device.seq0_buf));

    ret = adc_device.seq0_channel_cnt;

    adc_device.seq0_is_data_ready = 0;

    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

static void seq1_irq_cb(void)
{
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    adc_read_seq1_data(adc_device.seq1_buf, adc_device.seq1_channel_cnt);
    adc_device.seq1_is_data_ready = 1;

    spin_unlock_irqrestore(&spinlock, flags);
    wake_up_interruptible(&adc_device.seq1_wq);
}

static void adc_seq1_enable(struct adc_seq1_config seq1_adc)
{
    if (!adc_device.seq1_is_enable) {
        adc_device.seq1_is_enable = 1;
        adc_power_ref();
    }

    adc_device.seq1_channel_cnt = seq1_adc.channel_cnt;
    adc_device.seq1_is_data_ready = 0;

    if (seq1_adc.irq_cb)
        seq1_adc.irq_cb = seq1_irq_cb;

    adc_enable_seq1(&seq1_adc);

    adc_start_seq1();
}

static void adc_seq1_disable(struct adc_seq1_config seq1_adc)
{
    if (adc_device.seq1_is_enable) {
        adc_device.seq1_is_enable = 0;
        adc_power_unref();
    }

    adc_disable_seq1(&seq1_adc);
}

static int adc_seq1_read_data(unsigned short *buf, int timeout)
{
    int ret = 0;

    if (timeout < 0) {
        ret = wait_event_interruptible(adc_device.seq1_wq, adc_device.seq1_is_data_ready);
        if (ret < 0) {
            printk(KERN_ERR "%s:wait get value failed!\n", __func__);
            return ret;
        }
    } else {
        ret = wait_event_interruptible_timeout(
            adc_device.seq1_wq, adc_device.seq1_is_data_ready, msecs_to_jiffies(timeout));
        if (ret == 0) {
            printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
            return -EBUSY;
        } else if (ret < 0)
            return -EAGAIN;
    }

    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    memcpy(buf, adc_device.seq1_buf, sizeof(adc_device.seq1_buf));

    ret = adc_device.seq1_channel_cnt;

    adc_device.seq1_is_data_ready = 0;

    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

static void seq2_irq_cb(void)
{
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    adc_read_seq2_data(adc_device.seq2_buf, adc_device.seq2_channel_cnt);
    adc_device.seq2_is_data_ready = 1;

    spin_unlock_irqrestore(&spinlock, flags);
    wake_up_interruptible(&adc_device.seq2_wq);
}

static void adc_seq2_enable(struct adc_seq2_config seq2_adc)
{
    if (!adc_device.seq2_is_enable) {
        adc_device.seq2_is_enable = 1;
        adc_power_ref();
    }

    adc_device.seq2_channel_cnt = seq2_adc.channel_cnt;
    adc_device.seq2_is_data_ready = 0;

    if (seq2_adc.irq_cb)
        seq2_adc.irq_cb = seq2_irq_cb;
    else
        seq2_adc.irq_cb = NULL;

    adc_enable_seq2(&seq2_adc);

    adc_start_seq2();
}

static void adc_seq2_disable(struct adc_seq2_config seq2_adc)
{
    if (adc_device.seq2_is_enable) {
        adc_device.seq2_is_enable = 0;
        adc_power_unref();
    }

    adc_disable_seq2(&seq2_adc);
}

static int adc_seq2_read_data(unsigned short *buf, int timeout)
{
    int ret = 0;

    if (timeout < 0) {
        ret = wait_event_interruptible(adc_device.seq2_wq, adc_device.seq2_is_data_ready);
        if (ret < 0) {
            printk(KERN_ERR "%s:wait get value failed!\n", __func__);
            return ret;
        }
    } else {
        ret = wait_event_interruptible_timeout(
            adc_device.seq2_wq, adc_device.seq2_is_data_ready, msecs_to_jiffies(timeout));
        if (ret == 0) {
            printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
            return -EBUSY;
        } else if (ret < 0)
            return -EAGAIN;
    }

    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    memcpy(buf, adc_device.seq2_buf, sizeof(adc_device.seq2_buf));

    ret = adc_device.seq2_channel_cnt;

    adc_device.seq2_is_data_ready = 0;

    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

static void awd_irq_cb(unsigned short low_flags, unsigned short high_flags)
{
    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    adc_device.low_flags |= low_flags;
    adc_device.high_flags |= high_flags;
    adc_device.awd_is_trigger = 1;

    spin_unlock_irqrestore(&spinlock, flags);
    wake_up_interruptible(&adc_device.awd_wq);
}

static void adc_enable_channel_awd(unsigned int channel, int low_threshold, int high_threshold)
{
    if (!(adc_device.adc_awd_enable_channels & BIT(channel))) {
        adc_device.adc_awd_enable_channels |= BIT(channel);
        adc_power_ref();
    }

    adc_set_awd_cb(awd_irq_cb);
    adc_enable_awd(channel, low_threshold, high_threshold);
}

static void adc_disable_channel_awd(unsigned int channel)
{
    if (adc_device.adc_awd_enable_channels & BIT(channel)) {
        adc_device.adc_awd_enable_channels &= ~BIT(channel);
        adc_power_unref();
    }

    adc_disable_awd(channel);
}

static int adc_awd_read_channels(unsigned short *awd_flags, int timeout)
{
    int ret = 0;

    if (timeout < 0) {
        ret = wait_event_interruptible(adc_device.awd_wq, adc_device.awd_is_trigger);
        if (ret < 0) {
            printk(KERN_ERR "%s:wait get value failed!\n", __func__);
            return ret;
        }
    }
    else {
        ret = wait_event_interruptible_timeout(
            adc_device.awd_wq, adc_device.awd_is_trigger, msecs_to_jiffies(timeout));
        if (ret == 0) {
            printk(KERN_ERR "%s:adc get value timeout!\n", __func__);
            return -EBUSY;
        } else if (ret < 0)
            return -EAGAIN;
    }

    unsigned long flags;
    spin_lock_irqsave(&spinlock, flags);

    awd_flags[0] = adc_device.low_flags;
    awd_flags[1] = adc_device.high_flags;

    adc_device.low_flags = 0;
    adc_device.high_flags = 0;
    adc_device.awd_is_trigger = 0;

    spin_unlock_irqrestore(&spinlock, flags);

    return ret;
}

/* ---------------------------------------------------------------------- */

static int adc_open(struct inode *inode, struct file *filp)
{
    filp->private_data = (void *)0;

    adc_enable();

    return 0;
}

static int adc_release(struct inode *inode, struct file *filp)
{
    adc_disable();

    return 0;
}

ssize_t adc_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    unsigned int sadc_val = 0;
    unsigned int channel = (unsigned int)filp->private_data;

    BUG_ON (channel >= ADC_MAX_CHANNELS);

    sadc_val = adc_seq0_read_channel_value(channel);
    if (sadc_val < 0)
        return sadc_val;

    sadc_val = sadc_val * VREF_ADC / 4096;

    if (copy_to_user(buf, &sadc_val, sizeof(int)))
        return -EFAULT;

    return sizeof(int);
}

static long adc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int channel = (int)filp->private_data;

    /* handle ioctls */
    switch (cmd) {
        case ADC_GET_VALUE: {
            /*
            * Cannot modify the JZ_REG_ADC_ENABLE if the controller is working,
            * so need time division multiplexing.
            */

            ret = adc_seq0_read_channel_value(channel);

            break;
        }
        case ADC_SET_CHANNEL: {
            filp->private_data = (void *)*(unsigned int *)arg;

            break;
        }
        case ADC_ENABLE: {
            break;
        }
        case ADC_DISABLE: {
            break;
        }
        case ADC_SET_VREF: {
            VREF_ADC = *(unsigned int *)arg;
            break;
        }
        case ADC_GET_VREF: {
            ret = VREF_ADC;
            break;
        }
        case ADC_SEQ0_ENABLE: {
            struct adc_seq0_config adc_seq0;
            if (copy_from_user(&adc_seq0, (void *)arg, sizeof(adc_seq0))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq0_enable(adc_seq0);

            break;
        }
        case ADC_SEQ0_DISABLE: {
            struct adc_seq0_config adc_seq0;
            if (copy_from_user(&adc_seq0, (void *)arg, sizeof(adc_seq0))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq0_disable(adc_seq0);

            break;
        }
        case ADC_SEQ0_READ_TIMEOUT: {
            unsigned int *argv = (void *)arg;
            int timeout = (int)(argv[0]);
            unsigned short *buf = (unsigned short *)argv[1];

            unsigned short tmp[4];
            ret = adc_seq0_read_data(tmp, timeout);

            if (copy_to_user(buf, tmp, sizeof(tmp))) {
                printk(KERN_ERR "ADC: copy_to_user err!\n");
                return -1;
            }

            break;
        }
        case ADC_SEQ1_ENABLE: {
            struct adc_seq1_config adc_seq1;
            if (copy_from_user(&adc_seq1, (void *)arg, sizeof(adc_seq1))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq1_enable(adc_seq1);

            break;
        }
        case ADC_SEQ1_DISABLE: {
            struct adc_seq1_config adc_seq1;
            if (copy_from_user(&adc_seq1, (void *)arg, sizeof(adc_seq1))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq1_disable(adc_seq1);

            break;
        }
        case ADC_SEQ1_READ_TIMEOUT: {
            unsigned int *argv = (void *)arg;
            int timeout = (int)(argv[0]);
            unsigned short *buf = (unsigned short *)argv[1];

            unsigned short tmp[16];
            ret = adc_seq1_read_data(tmp, timeout);

            if (copy_to_user(buf, tmp, sizeof(tmp))) {
                printk(KERN_ERR "ADC: copy_to_user err!\n");
                return -1;
            }

            break;
        }
        case ADC_SEQ2_ENABLE: {
            struct adc_seq2_config adc_seq2;
            if (copy_from_user(&adc_seq2, (void *)arg, sizeof(adc_seq2))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq2_enable(adc_seq2);

            break;
        }
        case ADC_SEQ2_DISABLE: {
            struct adc_seq2_config adc_seq2;
            if (copy_from_user(&adc_seq2, (void *)arg, sizeof(adc_seq2))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_seq2_disable(adc_seq2);

            break;
        }
        case ADC_SEQ2_READ_TIMEOUT: {
            unsigned int *argv = (void *)arg;
            int timeout = (int)(argv[0]);
            unsigned short *buf = (unsigned short *)argv[1];

            unsigned short tmp[8];
            ret = adc_seq2_read_data(tmp, timeout);

            if (copy_to_user(buf, tmp, sizeof(tmp))) {
                printk(KERN_ERR "ADC: copy_to_user err!\n");
                return -1;
            }

            break;
        }
        case ADC_AWD_ENABLE: {
            int awd_threshold[2];
            if (copy_from_user(awd_threshold, (void *)arg, sizeof(awd_threshold))) {
                printk(KERN_ERR "ADC: copy_from_user err!\n");
                return -1;
            }

            adc_enable_channel_awd(channel, awd_threshold[0], awd_threshold[1]);

            break;
        }
        case ADC_AWD_DISABLE: {
            adc_disable_channel_awd(channel);

            break;
        }
        case ADC_AWD_READ_TIMEOUT: {
            unsigned int *argv = (void *)arg;
            int timeout = (int)(argv[0]);
            unsigned short *awd_flags = (unsigned short *)argv[1];

            unsigned short tmp[2];
            ret = adc_awd_read_channels(tmp, timeout);

            if (copy_to_user(awd_flags, tmp, sizeof(tmp))) {
                printk(KERN_ERR "ADC: copy_to_user err!\n");
                return -1;
            }

            break;
        }
        default: {
            printk(KERN_ERR "%s:unsupported ioctl cmd %x\n",__func__, cmd);
            ret = -EINVAL;
            break;
        }
    }

    return ret;
}


static struct file_operations adc_fops = {
    .owner = THIS_MODULE,
    .open = adc_open,
    .read = adc_read,
    .release = adc_release,
    .unlocked_ioctl = adc_ioctl,
};

struct miscdevice adc_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "jz_adc_seq",
    .fops = &adc_fops,
};

static void adc_set_cdr_exclk_en(int enable)
{
    if (!!enable)
        cpm_set_bit(CPM_EXCLK_EN, CPM_SADCCDR);
    else
        cpm_clear_bit(CPM_EXCLK_EN, CPM_SADCCDR);
}

static void adc_controller_reset(void)
{
    // adc soft reset
    cpm_set_bit(SRBC_ADC_SR, CPM_SRBC);
    usleep_range(10, 10);
    cpm_clear_bit(SRBC_ADC_SR, CPM_SRBC);
}

static int __init jz_adc_init(void)
{
    int ret;

    struct clk *parent;

    adc_device.mux_clk = clk_get(NULL, "mux_sadc");
    BUG_ON(IS_ERR(adc_device.mux_clk));
    parent = clk_get(NULL, "ext");
    BUG_ON(IS_ERR(parent));

    clk_set_parent(adc_device.mux_clk, parent);
    clk_prepare_enable(adc_device.mux_clk);
    clk_put(parent);

    adc_device.div_clk = clk_get(NULL, "div_sadc");
    clk_prepare_enable(adc_device.div_clk);

    adc_device.gate_clk = clk_get(NULL, "gate_sadc");
    clk_prepare_enable(adc_device.gate_clk);

    clk_set_rate(adc_device.div_clk, 24 * 1000 * 1000);

    adc_set_cdr_exclk_en(1);

    adc_controller_reset();

    mutex_init(&adc_device.mutex);
    init_waitqueue_head(&adc_device.seq0_wq);
    init_waitqueue_head(&adc_device.seq1_wq);
    init_waitqueue_head(&adc_device.seq2_wq);
    init_waitqueue_head(&adc_device.awd_wq);
    adc_device.seq0_is_enable = 0;
    adc_device.seq1_is_enable = 0;
    adc_device.seq2_is_enable = 0;
    adc_device.adc_awd_enable_channels = 0;

    ret = request_irq(IRQ_SADC, adc_irq_handler, 0, "adc", NULL);
    BUG_ON(ret);
    enable_irq_wake(IRQ_SADC);

    ret = misc_register(&adc_mdev);
    BUG_ON(ret < 0);

    return 0;
}
module_init(jz_adc_init);

static void __exit jz_adc_exit(void)
{
    misc_deregister(&adc_mdev);

    free_irq(IRQ_SADC, NULL);

    clk_disable_unprepare(adc_device.gate_clk);
    clk_put(adc_device.gate_clk);
    clk_disable_unprepare(adc_device.div_clk);
    clk_put(adc_device.div_clk);
    clk_disable_unprepare(adc_device.mux_clk);
    clk_put(adc_device.mux_clk);
}
module_exit(jz_adc_exit);

MODULE_DESCRIPTION("JZ x2600 ADC driver");
MODULE_LICENSE("GPL");
