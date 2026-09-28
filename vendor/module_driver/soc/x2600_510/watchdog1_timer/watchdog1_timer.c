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

#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <bit_field.h>
#include <soc/cpm.h>
#include <linux/interrupt.h>


#define WDT1_IOBASE                     0x13640000
#define WDT1_ADDR(reg)                  ((volatile unsigned long *)CKSEG1ADDR(WDT1_IOBASE + reg))

#define WDT_FULL                        0x0
#define WDT_ENABLE                      0x4
#define WDT_COUNT                       0x8
#define WDT_CTRL                        0xC

#define TCU_TSR                         0x1C
#define TCU_TSSR                        0x2C
#define TCU_TSCR                        0x3C
#define TCU_TFR                         0x20
#define TCU_TFCR                        0x28
#define TCU_TMR                         0x30
#define TCU_TMSR                        0x34
#define TCU_TMCR                        0x38

/* TCU_TSR & TCU_TFR & TCU_TMR */
#define TSR_WDTS                        16
#define TFR_HFALGW                      24
#define TFR_FFALGW                      25
#define TMR_HMASKW                      24
#define TMR_FMASKW                      25

/* WDT_FULL */
#define FULL_THR                        16, 31
#define FULL_TDR                        0, 15

/* WDT_CTRL */
#define CTRL_CLRZ                       10
#define CTRL_PRESCALE                   3, 5
#define CTRL_RTC_EN                     1

/* CPM */
#define OPCR_ERCS_BIT                   2

#define WDT_IDLE                        0
#define WDT_BUSY                        1
#define WDT_COMPLETE                    2
#define WDT_MAX_COUNT                   (0xFFFF)

#define IRQ_WDT                         (IRQ_INTC_BASE + 24)

#define HW_TIMER_MAGIC_NUMBER           'T'
#define HW_TIMER_START                  _IOW(HW_TIMER_MAGIC_NUMBER, 131, unsigned long)
#define HW_TIMER_STOP                   _IO(HW_TIMER_MAGIC_NUMBER, 132)
#define HW_TIMER_WAIT                   _IOWR(HW_TIMER_MAGIC_NUMBER, 133, unsigned int)

enum WDT_CLK_DIV {
    WDT_CLK_DIV_1       = 0,
    WDT_CLK_DIV_4       = 1,
    WDT_CLK_DIV_16      = 2,
    WDT_CLK_DIV_64      = 3,
    WDT_CLK_DIV_256     = 4,
    WDT_CLK_DIV_1024    = 5,
    WDT_CLK_DIV_ERR     = 6,
};

static DEFINE_SPINLOCK(lock);
struct clk *tcu_clk;

static wait_queue_head_t wdt_irq_wq;

static volatile unsigned char wdt_status = WDT_IDLE;

static inline void wdt_write_reg(unsigned long reg, unsigned int val)
{
    *WDT1_ADDR(reg) = val;
}

static inline int wdt_read_reg(unsigned long reg)
{
    return *WDT1_ADDR(reg);
}

static inline void wdt_set_bit(unsigned long reg, int bit, unsigned int val)
{
    set_bit_field(WDT1_ADDR(reg), bit, bit, val);
}

static inline unsigned int wdt_get_bit(unsigned long reg, int bit)
{
    return get_bit_field(WDT1_ADDR(reg), bit, bit);
}

static inline void wdt_set_bits(unsigned long reg, int start, int end, unsigned int val)
{
    set_bit_field(WDT1_ADDR(reg), start, end, val);
}

static inline unsigned int wdt_get_bits(unsigned long reg, int start, int end)
{
    return get_bit_field(WDT1_ADDR(reg), start, end);
}

/*----------------------------------------------------------------------------*/

static inline int watchdog1_timer_get_full_irq_flags(void)
{
    return wdt_get_bit(TCU_TFR, TFR_FFALGW);
}

static inline void watchdog1_timer_clean_full_irq_flags(void)
{
    wdt_set_bit(TCU_TFCR, TFR_FFALGW, 1);
}

static inline int watchdog1_timer_full_irq_is_enabled(void)
{
    return !wdt_get_bit(TCU_TMR, TMR_FMASKW);
}

static inline void watchdog1_timer_enable_full_irq(void)
{
    wdt_set_bit(TCU_TMCR, TMR_FMASKW, 1);
}

static inline void watchdog1_timer_mask_full_irq(void)
{
    wdt_set_bit(TCU_TMSR, TMR_FMASKW, 1);
}

static unsigned long get_rtc_internal_clk_rate(void)
{
    unsigned int rtc_32k_is_on = cpm_test_bit(OPCR_ERCS_BIT, CPM_OPCR);

    if (!rtc_32k_is_on)
        return 24000000 / 512;

    return 32768;
}

static int watchdog1_timer_set_timeout(unsigned long us)
{
    unsigned long rate = get_rtc_internal_clk_rate();
    unsigned long time = 1000000 / rate;
    unsigned long count = us / time;
    unsigned int clock_div = 0;

    while (count > WDT_MAX_COUNT) {
        if (clock_div >= WDT_CLK_DIV_ERR)
            return -1;

        count /= 4;
        clock_div += 1;
    }

    /* The clock supplies to WDT is supplied */
    wdt_set_bit(TCU_TSCR, TSR_WDTS, 1);

    /* clean counter */
    wdt_set_bit(WDT_CTRL, CTRL_CLRZ, 1);

    /* set clock div */
    wdt_set_bits(WDT_CTRL, CTRL_PRESCALE, clock_div);

    /* enable clock timer input */
    wdt_set_bit(WDT_CTRL, CTRL_RTC_EN, 1);

    if (count > WDT_MAX_COUNT)
        count = WDT_MAX_COUNT;
    wdt_set_bits(WDT_FULL, FULL_TDR, count);

    /* wdt enable */
    wdt_write_reg(WDT_ENABLE, 1);

    return 0;
}

static int watchdog1_timer_start(unsigned long us)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    if (wdt_status == WDT_BUSY) {
        printk(KERN_ERR "watchdog1_timer: watchdog is running!\n");
        ret = -1;
        goto unlock;
    }

    ret = watchdog1_timer_set_timeout(us);
    if (ret < 0) {
        printk(KERN_ERR "watchdog1_timer: set timer failed!\n");
        goto unlock;
    }

    wdt_status = WDT_BUSY;

unlock:
    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

static void watchdog1_timer_stop_reg(void)
{
    /* disable wdt */
    wdt_write_reg(WDT_ENABLE, 0);

    /* clean counter */
    wdt_set_bit(WDT_CTRL, CTRL_CLRZ, 1);

    /* clean wdt full data */
    wdt_write_reg(WDT_FULL, 0);

    wdt_set_bit(WDT_CTRL, CTRL_RTC_EN, 0);

    wdt_set_bit(TCU_TSSR, TSR_WDTS, 1);
}

static int watchdog1_timer_stop(void)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    watchdog1_timer_stop_reg();

    wdt_status = WDT_IDLE;

    spin_unlock_irqrestore(&lock, flags);

    return ret;
}

static int watchdog1_timer_wait(void)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    if (wdt_status == WDT_IDLE) {
        printk(KERN_ERR "watchdog1_timer: timer not enabled!\n");

        ret = -1;
        spin_unlock_irqrestore(&lock, flags);
        return ret;
    }

    spin_unlock_irqrestore(&lock, flags);

    ret = wait_event_interruptible(wdt_irq_wq, (wdt_status == WDT_COMPLETE));
    if (ret < 0) {
        printk(KERN_ERR "watchdog1_timer: wait wdt irq failed!\n");
        return ret;
    }

    wdt_status = WDT_IDLE;

    return 0;
}

static int watchdog1_timer_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int watchdog1_timer_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long watchdog1_timer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned long us;

    switch (cmd) {
        case HW_TIMER_START:
            if (copy_from_user(&us, (unsigned long *)arg, sizeof(us))) {
                printk(KERN_ERR "watchdog1_timer: copy_from_user err!\n");
                return -1;
            }

            ret = watchdog1_timer_start(us);
            break;
        case HW_TIMER_STOP:
            ret = watchdog1_timer_stop();

            break;
        case HW_TIMER_WAIT:
            ret = watchdog1_timer_wait();

            break;
        default:
            return -1;
    }

    return ret;
}

static struct file_operations watchdog1_timer_fops = {
    .owner = THIS_MODULE,
    .open = watchdog1_timer_open,
    .release = watchdog1_timer_close,
    .unlocked_ioctl = watchdog1_timer_ioctl,
};

struct miscdevice watchdog1_timer_mdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .fops   = &watchdog1_timer_fops,
    .name   = "timer_watchdog1",
};

irqreturn_t wdt_irq_handler(int irq, void *data)
{
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);

    if (watchdog1_timer_get_full_irq_flags()
        && watchdog1_timer_full_irq_is_enabled())
    {
        watchdog1_timer_clean_full_irq_flags();
        watchdog1_timer_stop_reg();

        printk(KERN_ERR "watchdog1_timer: counter full irq\n");

        wdt_status = WDT_COMPLETE;
        wake_up_interruptible(&wdt_irq_wq);
    }

    spin_unlock_irqrestore(&lock, flags);

    return IRQ_HANDLED;
}

static int __init jz_watchdog1_timer_init(void)
{
    int ret;

    tcu_clk = clk_get(NULL, "gate_tcu1");
    BUG_ON(IS_ERR(tcu_clk));
    clk_prepare_enable(tcu_clk);

    watchdog1_timer_clean_full_irq_flags();
    watchdog1_timer_enable_full_irq();

    init_waitqueue_head(&wdt_irq_wq);

    ret = request_irq(IRQ_WDT, wdt_irq_handler, 0, "jz_watchdog1_timer_irq", NULL);
    if (ret < 0)
        printk(KERN_ERR "watchdog1_timer: failed to request irq, ret: %d\n", ret);

    enable_irq_wake(IRQ_WDT);

    ret = misc_register(&watchdog1_timer_mdev);
    if (ret < 0)
        panic("watchdog1_timer: %s, watchdog1 timer register misc dev failed !\n", __func__);

    return 0;
}
module_init(jz_watchdog1_timer_init);

static void __exit jz_watchdog1_timer_exit(void)
{
    misc_deregister(&watchdog1_timer_mdev);

    watchdog1_timer_mask_full_irq();
    free_irq(IRQ_WDT, NULL);

    clk_disable_unprepare(tcu_clk);
    clk_put(tcu_clk);
}
module_exit(jz_watchdog1_timer_exit);

MODULE_DESCRIPTION("Ingenic SoC Watchdog1 Timer Driver");
MODULE_LICENSE("GPL");