#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/lockdep.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/smpboot.h>
#include <linux/sched/rt.h>

#include <asm/irq_regs.h>
#include <linux/kvm_para.h>
#include <linux/perf_event.h>
#include <soc/cpm.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <bit_field.h>

#define WDT0_IOBASE         0x13630000
#define WDT1_IOBASE         0x13640000

#define TCU_TSSR            0x2C
#define TCU_TSCR            0x3C

#define TCU_TFR             0x20
#define TCU_TFSR            0x24
#define TCU_TFCR            0x28
#define TCU_TMR             0x30
#define TCU_TMSR            0x34
#define TCU_TMCR            0x38

#define WDT_TDR             0x0
#define WDT_TCER            0x4
#define WDT_TCNT            0x8
#define WDT_TCSR            0xC

#define WDT_CLK_DIV_1       0
#define WDT_CLK_DIV_4       1
#define WDT_CLK_DIV_16      2
#define WDT_CLK_DIV_64      3
#define WDT_CLK_DIV_256     4
#define WDT_CLK_DIV_1024    5

#define WDT_MAX_COUNT       (0xFFFF)

#define OPCR_ERCS_BIT       2

#define RTC_EN              2

#define WDT_IDLE            0
#define WDT_BUSY            1
#define WDT_CLRZ            10
#define WDT_CLEAN_TIMER     16
#define WDT_HALF_FLAG       24
#define WDT_FULL_FLAG       25

#define IRQ_WDT            (24)
#define IRQ_TCU1           (25)

#define WATCHDOG_MAGIC_NUMBER   'W'
#define WATCHDOG_START              _IOW(WATCHDOG_MAGIC_NUMBER, 13, unsigned long)
#define WATCHDOG_STOP               _IO(WATCHDOG_MAGIC_NUMBER, 14)
#define WATCHDOG_FEED               _IO(WATCHDOG_MAGIC_NUMBER, 15)
#define WATCHDOG_RESET              _IO(WATCHDOG_MAGIC_NUMBER, 16)

struct jz_wdt_drv {
    int id;
    char name[16];
    int half_irq;
    int full_irq;
    const char *half_irq_name;
    const char *full_irq_name;
    struct miscdevice mdev;
    int is_enable;
    int is_finish;
    struct clk *clk;
    char *clk_name;
};

static struct jz_wdt_drv jz_wdt_dev[2] = {
    {
        .id         = 0,
        .name       = "jz_watchdog",
        .clk_name   = "gate_tcu0",
    },
    {
        .id         = 1,
        .name       = "jz_watchdog1",
        .clk_name   = "gate_tcu1",
        .half_irq   = (IRQ_INTC_BASE + IRQ_TCU1),
        .full_irq   = (IRQ_INTC_BASE + IRQ_WDT),
        .half_irq_name   = "jz_half_wtd1",
        .full_irq_name   = "jz_full_wtd1",
    },
};

module_param_named(wdt0_is_enable,  jz_wdt_dev[0].is_enable, int, 0644);
module_param_named(wdt1_is_enable,  jz_wdt_dev[1].is_enable, int, 0644);

static const unsigned long iobase[] = {
    KSEG1ADDR(WDT0_IOBASE),
    KSEG1ADDR(WDT1_IOBASE),
};

#define WDT_ADDR(id, reg) ((volatile unsigned long *)((iobase[id]) + (reg)))

static inline void wdt_write_reg(int id, unsigned int reg, unsigned int value)
{
    *WDT_ADDR(id, reg) = value;
}

static inline unsigned int wdt_read_reg(int id, unsigned int reg)
{
    return *WDT_ADDR(id, reg);
}

static DEFINE_SPINLOCK(lock);

static unsigned int status;

static inline void wdt_set_bit(int id, unsigned long reg, int bit, unsigned int val)
{
    set_bit_field(WDT_ADDR(id, reg), bit, bit, val);
}

static inline unsigned int wdt_get_bit(int id, unsigned long reg, int bit)
{
    return get_bit_field(WDT_ADDR(id, reg), bit, bit);
}

inline void wdt_clean_half_interrupt_flag(int id)
{
    wdt_set_bit(id, TCU_TFCR, WDT_HALF_FLAG, 1);
}

inline void wdt_clean_full_interrupt_flag(int id)
{
    wdt_set_bit(id, TCU_TFCR, WDT_FULL_FLAG, 1);
}

inline void wdt_half_interrupt_enable(int id)
{
    wdt_set_bit(id, TCU_TMSR, WDT_HALF_FLAG, 1);
}

inline void wdt_full_interrupt_enable(int id)
{
    wdt_set_bit(id, TCU_TMSR, WDT_FULL_FLAG, 1);
}

inline void wdt_half_interrupt_disable(int id)
{
    wdt_set_bit(id, TCU_TMCR, WDT_HALF_FLAG, 1);
}

inline void wdt_full_interrupt_disable(int id)
{
    wdt_set_bit(id, TCU_TMCR, WDT_FULL_FLAG, 1);
}

inline void wdt_clean_timer(int id)
{
    wdt_set_bit(id, TCU_TSCR, WDT_CLEAN_TIMER, 1);
}

inline unsigned int wdt_match_half_interrupt(int id)
{
    return wdt_get_bit(id, TCU_TFR, WDT_HALF_FLAG);
}

inline unsigned int wdt_match_full_interrupt(int id)
{
    return wdt_get_bit(id, TCU_TFR, WDT_FULL_FLAG);
}

inline unsigned int wdt_half_interrupt_is_enabled(int id)
{
    return !wdt_get_bit(id, TCU_TMR, WDT_HALF_FLAG);
}

inline unsigned int wdt_full_interrupt_is_enabled(int id)
{
    return !wdt_get_bit(id, TCU_TMR, WDT_FULL_FLAG);
}

unsigned long get_rtc_internal_clk_rate(void)
{
    unsigned int rtc_32k_is_on = cpm_test_bit(OPCR_ERCS_BIT, CPM_OPCR);

    if (!rtc_32k_is_on)
        return 24000000 / 512;

    return 32768;
}

static  void wdt_set_half_and_full(struct jz_wdt_drv *drv, unsigned int value, char ch)
{
    int tmp ;
    if (value > WDT_MAX_COUNT)
        value = WDT_MAX_COUNT;
    tmp = wdt_read_reg(drv->id, WDT_TDR);
    if ('H' == ch) {
        tmp &= ~(WDT_MAX_COUNT << 16);
        tmp |= value << 16;
        wdt_write_reg(drv->id, WDT_TDR, tmp);
    }
    if ('F' == ch) {
        tmp &= ~(WDT_MAX_COUNT);
        tmp |= value;
        wdt_write_reg(drv->id, WDT_TDR, tmp);
    }
}

static int jz_wdt_set_timeout(struct jz_wdt_drv *drv, unsigned long ms)
{
    unsigned int val, us;
    unsigned long count = ms;
    int id = drv->id;
    unsigned int clock_div = 0;

    unsigned long rate = get_rtc_internal_clk_rate();
    /* ms < 1400000 */
    us = 1000000 / rate;
    count = ms * 1000 / us;

    while (count > WDT_MAX_COUNT) {
        if (clock_div == WDT_CLK_DIV_1024)
            return -1;

        count /= 4;
        clock_div += 1;
    }

    wdt_write_reg(id, WDT_TCER, 0);

    wdt_clean_half_interrupt_flag(id);
    wdt_clean_full_interrupt_flag(id);

    val = (clock_div << 3) | RTC_EN;

    wdt_write_reg(id, WDT_TCSR, val);

    if (id == 0) {
        wdt_write_reg(id, WDT_TDR, count);
    } else if (id == 1) {
        wdt_set_half_and_full(drv, count, 'F');

        wdt_set_half_and_full(drv, count / 2, 'H');
    }

    wdt_write_reg(id, WDT_TCNT, 0);

    wdt_write_reg(id, WDT_TCER, 1);

    return 0;
}

static int wdt_start(struct jz_wdt_drv *drv, unsigned long ms)
{
    int id = drv->id;

    if (status != WDT_IDLE) {
        printk(KERN_ERR "WDT: watchdog is running ! \n");
        return -1;
    }

    status = WDT_BUSY;

    wdt_clean_timer(id);

    if (jz_wdt_set_timeout(drv, ms)) {
        printk(KERN_ERR "error: count more than the WATCHDOG_MAX_COUNT!\n");
        return -1;
    }

    wdt_write_reg(id, WDT_TCER, 1);

    return 0;
}

int soc_wdt_start(struct jz_wdt_drv *drv, unsigned long ms)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    ret = wdt_start(drv, ms);

    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

static int wdt_stop(struct jz_wdt_drv *drv)
{
    int id = drv->id;

    if (status == WDT_IDLE) {
        printk(KERN_ERR "WDT: watchdog already stopped ! \n");
        return -1;
    }

    wdt_write_reg(id, WDT_TCER, 0);
    wdt_write_reg(id, TCU_TSSR, 1 << 16);// 失能看门狗计数器

    status = WDT_IDLE;

    return 0;
}

int soc_wdt_stop(struct jz_wdt_drv *drv)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    ret = wdt_stop(drv);

    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

int soc_wdt_feed(struct jz_wdt_drv *drv)
{
    int id = drv->id;
    unsigned long flags;
    unsigned int val;
    spin_lock_irqsave(&lock, flags);

    val = wdt_read_reg(id, WDT_TCSR);
    val |= 1<<WDT_CLRZ;
    wdt_write_reg(id, WDT_TCSR, val);

    spin_unlock_irqrestore(&lock, flags);

    return 0;
}

void soc_reset(struct jz_wdt_drv *drv)
{
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    if (status != WDT_IDLE)
        wdt_stop(drv);

    wdt_start(drv, 0);

    while (1) {
        mdelay(10);
        printk(KERN_ERR "WDT: wait for reset\n");
    }

    spin_unlock_irqrestore(&lock, flags);
}

static int watchdog_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int watchdog_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long watchdog_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned long ms;

    struct jz_wdt_drv *drv = container_of(filp->private_data,
            struct jz_wdt_drv, mdev);

    switch (cmd) {
        case WATCHDOG_START: {
            if (copy_from_user(&ms, (unsigned long *)arg, sizeof(ms))) {
                printk(KERN_ERR "WATCHDOG: copy_from_user err!\n");
                return -1;
            }

            ret = soc_wdt_start(drv, ms);
            break;
        }
        case WATCHDOG_STOP: {
            ret = soc_wdt_stop(drv);
            break;
        }
        case WATCHDOG_FEED: {
            ret = soc_wdt_feed(drv);
            break;
        }
        case WATCHDOG_RESET: {
            soc_reset(drv);
            break;
        }
        default:
            return -1;
    }

    return ret;
}

static struct file_operations watchdog_fops = {
    .owner= THIS_MODULE,
    .open= watchdog_open,
    .release = watchdog_close,
    .unlocked_ioctl = watchdog_ioctl,
};

static irqreturn_t ingenic_wdt_half_interrupt(int irq, void *dev)
{
    struct jz_wdt_drv *drv = (struct jz_wdt_drv *)(dev);

    if(wdt_match_half_interrupt(drv->id) &&
        wdt_half_interrupt_is_enabled(drv->id))
    {
        printk("watchdog%d HALF IRQ %d.\n", drv->id, drv->half_irq);
        wdt_clean_half_interrupt_flag(drv->id);
    }

    return IRQ_HANDLED;
}

static irqreturn_t ingenic_wdt_full_interrupt(int irq, void *dev)
{
    struct jz_wdt_drv *drv = (struct jz_wdt_drv *)(dev);

    if(wdt_match_full_interrupt(drv->id) &&
        wdt_full_interrupt_is_enabled(drv->id))
    {
        printk("watchdog%d FULL IRQ %d.\n", drv->id, drv->full_irq);
        wdt_clean_full_interrupt_flag(drv->id);
    }

    return IRQ_HANDLED;
}

static void jz_wdt_deinit(int id)
{
    struct jz_wdt_drv *drv = &jz_wdt_dev[id];

    clk_disable_unprepare(drv->clk);
    clk_put(drv->clk);

    if (id == 1) {
        free_irq(drv->half_irq, drv);
        free_irq(drv->full_irq, drv);
    }

    misc_deregister(&drv->mdev);
}

static int jz_wdt_init(int id)
{
    int ret;
    struct jz_wdt_drv *drv = &jz_wdt_dev[id];

    drv->mdev.minor  = MISC_DYNAMIC_MINOR;
    drv->mdev.fops   = &watchdog_fops;
    drv->mdev.name   = drv->name;

    ret = misc_register(&drv->mdev);
    if (ret < 0)
        panic("watchdog: %s, watchdog register misc dev error !\n", __func__);

    drv->clk = clk_get(NULL, drv->clk_name);
    if (IS_ERR(drv->clk)) {
        printk("wdt%d get clock(%s) failed\n", id, drv->clk_name);
        goto err_get_clk;
    }

    clk_prepare_enable(drv->clk);

    /* Only WDT1 can generate WDT full interrupt */
    if (id == 1) {
        wdt_half_interrupt_enable(id);
        wdt_full_interrupt_enable(id);

        ret = request_irq(drv->half_irq, ingenic_wdt_half_interrupt,
            IRQF_SHARED | IRQF_TRIGGER_LOW, drv->half_irq_name, drv);
        if (ret < 0)
            printk("wtd%d failed to request irq\n", id);

        ret = request_irq(drv->full_irq, ingenic_wdt_full_interrupt,
            IRQF_SHARED | IRQF_TRIGGER_LOW, drv->full_irq_name, drv);
        if (ret < 0)
            printk("wtd%d failed to request irq\n", id);

        enable_irq_wake(drv->full_irq);
        wdt_half_interrupt_disable(id);
        wdt_full_interrupt_disable(id);
    }

    drv->is_finish = 1;

    return 0;

err_get_clk:
    misc_deregister(&drv->mdev);
    return -1;
}


static int __init jz_watchdog_init(void)
{
    if (jz_wdt_dev[0].is_enable)
        jz_wdt_init(0);

    if (jz_wdt_dev[1].is_enable)
        jz_wdt_init(1);

    return 0;
}

static void jz_watchdog_exit(void)
{
    if (jz_wdt_dev[0].is_finish)
        jz_wdt_deinit(0);

    if (jz_wdt_dev[1].is_finish)
        jz_wdt_deinit(1);
}

module_init(jz_watchdog_init);
module_exit(jz_watchdog_exit);

MODULE_DESCRIPTION("Ingenic SoC WATCHDOG driver");
MODULE_LICENSE("GPL");
