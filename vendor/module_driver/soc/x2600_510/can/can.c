#include <linux/module.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <soc/gpio.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <assert.h>
#include <linux/string.h>
#include <bit_field.h>

#include <utils/clock.h>
#include <utils/gpio.h>
#include <utils/ring_mem.h>
#include "can_regs.h"

#define CAN0_IOBASE     0x13490000
#define CAN1_IOBASE     0x134A0000

static const unsigned long iobase[] = {
    CAN0_IOBASE,
    CAN1_IOBASE,
};

#define CAN_ADDR(id, reg) ((volatile unsigned long *)(KSEG1ADDR(iobase[id]) + reg))

static inline void can_write_reg(int id, unsigned int reg, unsigned int value)
{
    *CAN_ADDR(id, reg) = value;
}

static inline unsigned int can_read_reg(int id, unsigned int reg)
{
    return *CAN_ADDR(id, reg);
}

static inline void can_set_bits(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(CAN_ADDR(id, reg), start, end, val);
}

static inline unsigned int can_get_bits(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(CAN_ADDR(id, reg), start, end);
}

/////////////////////////////////////////////////////////////////////////////////////

/* 若afid设置为该值, 可接收所有帧 */
#define CAN_ALL_ACCEPT_AFID 0xFFFFFFFF

enum can_frame_mode {
    CAN_STANDARD,
    CAN_EXTENDED,
};

enum can_frame_type {
    CAN_REGULAR_DATA,
    CAN_REMOTE_REQUEST,
};

struct can_frame_cfg {
    int frm_id;
    int len;
    unsigned char data[8];
    enum can_frame_mode mode;
    enum can_frame_type type;
};

/////////////////////////////////////////////////////////////////////////////////////

#define IRQ_CAN0    (IRQ_INTC_BASE + 32 + 7)
#define IRQ_CAN1    (IRQ_INTC_BASE + 32 + 6)

#define DMA_REQ_TYPE_CAN0_TX 0x20
#define DMA_REQ_TYPE_CAN0_RX 0x21
#define DMA_REQ_TYPE_CAN1_TX 0x22
#define DMA_REQ_TYPE_CAN1_RX 0x23

static unsigned int can_dma_type[] = {
    /* DMA_REQ_TYPE_CAN0_TX, */
    DMA_REQ_TYPE_CAN0_RX,
    /* DMA_REQ_TYPE_CAN1_TX, */
    DMA_REQ_TYPE_CAN1_RX
};

#define CAN_CGU_CLK_RATE 8000000
#define FRAME_TIMEOUT_MS 10 * HZ

#define CAN_BUFFER_SIZE 256
#define FRAME_SIZE 16

static int can_is_enable_dma = 1;

struct can_baud {
    unsigned int baud;
    unsigned int tseg2;
    unsigned int tseg1;
    unsigned int cancs;
};

static struct can_baud can_bauds[] = {
    {50000,7,10,7},
    {100000,7,10,3},
    {125000,6,7,3},
    {250000,2,3,3},
    {500000,6,7,0},
    {1000000,2,3,0},
};

struct can_filter_cfg {
    int is_enable;
    int afid;
};

struct jz_func_alter {
    short pin;
    unsigned short function;
    const char *name;
};

struct jz_can_pin {
    struct jz_func_alter can_pins[2];
};

struct jz_can_drv {
    int id;
    int irq;
    int is_enable;
    int is_finish;
    struct miscdevice mdev;
    struct device *dev;

    int can_dt;
    int can_dr;
    int alter_num;
    struct jz_can_pin *alter_pin;

    struct clk *clk;
    struct clk *clk_cgu;

    int is_on;
    unsigned int bus_rate;
    struct can_filter_cfg filter[4];

    spinlock_t spinlock;
    struct completion done_tx;
    struct completion done_rx;

    struct ring_mem ring;

    void *rbuff;
    struct dma_chan *rxchan;
    unsigned long dma_pos;
};

static struct jz_can_pin jz_can0_pin[] = {
    {
        {
            {GPIO_PC(7), GPIO_FUNC_2, "can0_tx"},
            {GPIO_PC(8), GPIO_FUNC_2, "can0_rx"}
        }
    },
};

static struct jz_can_pin jz_can1_pin[] = {
    {
        {
            {GPIO_PC(9), GPIO_FUNC_2, "can1_tx"},
            {GPIO_PC(10), GPIO_FUNC_2, "can1_rx"}
        }
    },
};

static void m_release(struct device *dev){}

struct platform_device jz_can_device[] = {
    {
        .name = "jz-can0",
        .id = 0,
        .dev = {
            .release = m_release,
        },
    },
    {
        .name = "jz-can1",
        .id = 1,
        .dev = {
            .release = m_release,
        },
    },
};

struct jz_can_drv jzcan_dev[2] = {
    {
        .id = 0,
        .irq = IRQ_CAN0,
        .alter_pin = jz_can0_pin,
        .alter_num = ARRAY_SIZE(jz_can0_pin),
    },
    {
        .id = 1,
        .irq = IRQ_CAN1,
        .alter_pin = jz_can1_pin,
        .alter_num = ARRAY_SIZE(jz_can1_pin),
    }
};

module_param_named(can0_is_enable, jzcan_dev[0].is_enable, int, 0644);
module_param_gpio_named(can0_dt, jzcan_dev[0].can_dt, 0644);
module_param_gpio_named(can0_dr, jzcan_dev[0].can_dr, 0644);
module_param_named(can1_is_enable, jzcan_dev[1].is_enable, int, 0644);
module_param_gpio_named(can1_dt, jzcan_dev[1].can_dt, 0644);
module_param_gpio_named(can1_dr, jzcan_dev[1].can_dr, 0644);

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;
    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio, func);

    return 0;
}

static int m_gpio_request(int gpio, struct jz_can_drv *drv, int id)
{
    int i;
    int ret = 0;
    char buf[10];
    int num = drv->alter_num;
    struct jz_can_pin *can_pin_alter = drv->alter_pin;
    struct jz_func_alter *can_pin;

    if (gpio < 0)
        return 0;

    for (i = 0; i < num; i++) {
        can_pin = can_pin_alter[i].can_pins;
        if (gpio == can_pin[id].pin) {
            ret = gpio_init(gpio, can_pin[id].function, can_pin[id].name);
            if (ret < 0) {
                printk(KERN_ERR "CAN%d: failed to request %s: %s!\n", id, can_pin[id].name, gpio_to_str(gpio, buf));
                return -EINVAL;
            }

            return 0;
        }
    }

    printk(KERN_ERR "CAN%d: %s(%s) is invaild!\n", id, can_pin[id].name, gpio_to_str(gpio, buf));
    return -EINVAL;
}

static int can_gpio_request(struct jz_can_drv *drv)
{
    int ret = 0;
    int id = drv->id;

    if (drv->can_dt < 0) {
        printk(KERN_ERR "CAN%d: can_dt must be set.\n", id);
        return -EINVAL;
    }

    if (drv->can_dr < 0) {
        printk(KERN_ERR "CAN%d: can_dr must be set.\n", id);
        return -EINVAL;
    }

    ret = m_gpio_request(drv->can_dt, drv, 0);
    if (ret)
        return -EINVAL;

    ret = m_gpio_request(drv->can_dr, drv, 1);
    if (ret)
        goto err_dt;

    return 0;

err_dt:
    gpio_free(drv->can_dt);

    return -EINVAL;
}

static void can_gpio_release(int id)
{
    if (jzcan_dev[id].can_dt >= 0 )
        gpio_free(jzcan_dev[id].can_dt);
    if (jzcan_dev[id].can_dr >= 0)
        gpio_free(jzcan_dev[id].can_dr);
}

static inline void *m_dma_alloc_coherent(struct device *dev, int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(struct device *dev, void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(dev, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

/////////////////////////////////////////////////////////////////////////////////////

static void jz_can_init_setting(int id)
{
    /* enter reset mode */
    can_set_bits(id, CANMODE, CANMODE_RSTM, 1);

    unsigned long cancmd = 0;
    set_bit_field(&cancmd, CANCMD_CTB, 1);/* Clear Transmit Buffer */
    set_bit_field(&cancmd, CANCMD_RRB, 1);/* Release Receive Buffer */
    can_write_reg(id, CANCMD, cancmd);

    /* disable interrupt */
    can_write_reg(id, CANINT, 0);

    /* enable acceptance filter */
    can_set_bits(id, CANFLT, CANFLT_FTER, 0x1);

    unsigned long canerr = 0;
    set_bit_field(&canerr, CANERR_CANTEC, 0);
    set_bit_field(&canerr, CANERR_CANREC, 0);
    set_bit_field(&canerr, CANERR_CANEWLR, 0x60);
    can_write_reg(id, CANERR, canerr);

    /* Setting of Acceptance Filter */
    can_write_reg(id, CANAFID0, 0);
    can_write_reg(id, CANAFMK0, 0x1fffffff);
    can_write_reg(id, CANAFID1, 0);
    can_write_reg(id, CANAFMK1, 0x1fffffff);
    can_write_reg(id, CANAFID2, 0);
    can_write_reg(id, CANAFMK2, 0x1fffffff);
    can_write_reg(id, CANAFID3, 0);
    can_write_reg(id, CANAFMK3, 0x1fffffff);
    jzcan_dev[id].filter[0].is_enable = 1;
    jzcan_dev[id].filter[0].afid = CAN_ALL_ACCEPT_AFID;
}

static void jz_can_set_bauds(int id, int baud)
{
    if (baud <= 0) {
        baud = 500000;
        printk(KERN_DEBUG "CAN%d: bitrate change to %d\n", id, baud);
    }

    struct can_baud *b = NULL;
    int i, array_size = ARRAY_SIZE(can_bauds);
    for (i = 0; i < array_size; i++)
        if (can_bauds[i].baud == baud)
            b = &can_bauds[i];

    if (b == NULL) {
        printk(KERN_ERR "CAN%d: cgu_clk_rate(%d) can't divide to %d\n", id, CAN_CGU_CLK_RATE, baud);
        return;
    }

    unsigned long canbtr = 0;
    if (baud < 500000)
        set_bit_field(&canbtr, CANBTR_SAW, 1);

    set_bit_field(&canbtr, CANBTR_TSEG2, b->tseg2);
    set_bit_field(&canbtr, CANBTR_TSEG1, b->tseg1);
    set_bit_field(&canbtr, CANBTR_SJW, 3);
    set_bit_field(&canbtr, CANBTR_CANCS, b->cancs);

    can_set_bits(id, CANMODE, CANMODE_RSTM, 1);
    can_write_reg(id, CANBTR, canbtr);
    can_set_bits(id, CANMODE, CANMODE_RSTM, 0);
    jzcan_dev[id].bus_rate = baud;
}

static dma_addr_t get_rdma_addr(int id)
{
    struct jz_can_drv *drv = &jzcan_dev[id];
    int start = virt_to_phys(drv->rbuff);
    struct dma_chan *chan = drv->rxchan;
    dma_addr_t can_dst;

    int count = 1;

    do {
        can_dst = chan->device->get_current_trans_addr(
            chan, NULL, NULL, DMA_DEV_TO_MEM);
        if (can_dst >= start && can_dst < start + CAN_BUFFER_SIZE)
            return can_dst;
    } while (count--);

    printk(KERN_ERR "CAN%d: DMA address[0x%08x] illegal!\n", id, can_dst);

    if (!can_dst)
        can_dst = start;

    return 0;
}

static int can_dma_config(int id)
{
    struct dma_slave_config rx_config;
    struct jz_can_drv *drv = &jzcan_dev[id];
    struct dma_chan *chan = drv->rxchan;

    rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    rx_config.src_maxburst = 16;
    rx_config.dst_maxburst = 16;
    rx_config.slave_id = can_dma_type[id];
    rx_config.direction = DMA_DEV_TO_MEM;
    rx_config.src_addr = iobase[id] + CANRFIR;

    int ret = dmaengine_slave_config(chan, &rx_config);
    if (ret) {
        printk(KERN_ERR "CAN%d: Failed to config dma chan\n", id);
        return -1;
    }

    struct dma_async_tx_descriptor *desc;
    desc = chan->device->device_prep_dma_cyclic(chan,
            (unsigned long)virt_to_phys(drv->rbuff),
            CAN_BUFFER_SIZE,
            CAN_BUFFER_SIZE,
            DMA_DEV_TO_MEM,
            DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
    if (!desc) {
        printk(KERN_ERR "CAN%d: Failed to prepare dma desc\n", id);
        return -1;
    }

    desc->callback = NULL;
    desc->callback_param = NULL;

    dmaengine_submit(desc);
    dma_async_issue_pending(chan);
    drv->dma_pos = get_rdma_addr(id);

    return 0;
}

static void jz_can_enable(int id)
{
    struct jz_can_drv *drv = &jzcan_dev[id];
    if (drv->is_on)
        return ;

    spin_lock_init(&drv->spinlock);
    init_completion(&drv->done_tx);
    init_completion(&drv->done_rx);
    memset(drv->rbuff, 0, CAN_BUFFER_SIZE);

    if (can_is_enable_dma)
        can_dma_config(id);
    else
        ring_mem_init(&drv->ring, drv->rbuff, CAN_BUFFER_SIZE);

    can_set_bits(id, CANMODE, CANMODE_RSTM, 1);
    can_set_bits(id, CANMODE, CANMODE_RXCNT4, 1);
    can_set_bits(id, CANMODE, CANMODE_DMAEN, can_is_enable_dma);
    can_set_bits(id, CANMODE, CANMODE_RSTM, 0);

    can_set_bits(id, CANINT, CANINT_ALL, 0xfff);/* Interrupt Enable */
    drv->is_on = 1;
}

static void jz_can_disable(int id)
{
    if (!jzcan_dev[id].is_on)
        return ;

    can_write_reg(id, CANINT, 0);
    can_set_bits(id, CANMODE, CANMODE_RSTM, 1);
    can_set_bits(id, CANERR, CANERR_CANTEC, 0);
    can_set_bits(id, CANERR, CANERR_CANREC, 0);
    jzcan_dev[id].bus_rate = 0;
    jzcan_dev[id].is_on = 0;
}

static void jz_can_set_acceptance_filter(int id)
{
    int i;
    struct jz_can_drv *drv = &jzcan_dev[id];

    can_set_bits(id, CANMODE, CANMODE_RSTM, 1);

    for (i = 0; i < 4; i++) {
        struct can_filter_cfg *tmp = &drv->filter[i];

        can_set_bits(id, CANFLT, i, i, !!(tmp->is_enable));
        if (!tmp->is_enable)
            continue;

        unsigned long canafidx = CANAFID0 + i * 8;
        unsigned long canafmkx = CANAFMK0 + i * 8;
        can_write_reg(id, canafidx, tmp->afid);
        can_write_reg(id, canafmkx, tmp->afid == CAN_ALL_ACCEPT_AFID ? 0x1FFFFFFF : 0);
    }

    can_set_bits(id, CANMODE, CANMODE_RSTM, 0);
}

static void jz_can_dump_acceptance_filter(int id)
{
    int i;
    struct jz_can_drv *drv = &jzcan_dev[id];

    for (i = 0; i < 4; i++) {
        struct can_filter_cfg *tmp = &drv->filter[i];
        if (!tmp->is_enable) {
            printk(KERN_ERR "CAN%d: filter%d unused\n", id, i);
            continue;
        }

        if (tmp->afid == CAN_ALL_ACCEPT_AFID)
            printk(KERN_ERR "CAN%d: filter%d accept all id\n", id, i);
        else
            printk(KERN_ERR "CAN%d: filter%d accept id 0x%x\n", id, i, tmp->afid);
    }
}

static void can_irq_error(int id, unsigned long flag)
{
    unsigned long status = flag;
    if (get_bit_field(&status, CANINT_BOI)) {
        can_write_reg(id, CANINT, 0);
        can_set_bits(id, CANMODE, CANMODE_RSTM, 1);
        printk(KERN_ERR "CAN%d: err, disable all irq and get into reset mode!", id);
    }
}

static irqreturn_t can_irq_handler(int irq, void *data)
{
    struct jz_can_drv *drv = (struct jz_can_drv *)data;
    int id = drv->id;
    unsigned long status = can_read_reg(id, CANINT);
    status = ((status >> 16) & 0xfff) & status;

    if (get_bit_field(&status, CANINT_DMAI))
        complete(&drv->done_rx);

    if (get_bit_field(&status, CANINT_RI)) {
        unsigned int p[4];
        p[0] = can_read_reg(id, CANRFIR);
        p[1] = can_read_reg(id, CANRXID);
        p[2] = can_read_reg(id, CANRXDATA0);
        p[3] = can_read_reg(id, CANRXDATA1);
        ring_mem_write(&drv->ring, p, FRAME_SIZE);
        complete(&drv->done_rx);
    }

    if (!can_is_enable_dma)
        can_set_bits(id, CANCMD, CANCMD_RRB, 1);

    if (get_bit_field(&status, CANINT_TI))
        complete(&drv->done_tx);

    can_irq_error(id, status);

    return IRQ_HANDLED;
}

static int can_transmit_frame_inr(int id, struct can_frame_cfg *cfg)
{
    struct jz_can_drv *drv = &jzcan_dev[id];
    int len = cfg->len;
    unsigned int p[2] = {0};

    unsigned long flags;
    spin_lock_irqsave(&jzcan_dev[id].spinlock, flags);

    if (cfg->mode == CAN_STANDARD) {
        if (cfg->frm_id > 0x7FF)
            printk(KERN_DEBUG "CAN%d: frame ID change to 0x%03x(0x%x & 0x7FF)\n", id, cfg->frm_id & 0x7FF, cfg->frm_id);
    } else if (cfg->frm_id > 0x1FFFFFFF) {
            printk(KERN_DEBUG "CAN%d: frame ID change to 0x%08x(0x%x & 0x1FFFFFFF)\n", id, cfg->frm_id & 0x1FFFFFFF, cfg->frm_id);
    }

    if (len > 8) {
        printk(KERN_DEBUG "CAN%d: len(%d) too large change to 8\n", id, len);
        len = 8;
    }

    memcpy(p, cfg->data, len);

    can_set_bits(id, CANTFIR, CANTFIR_FF, cfg->mode == CAN_EXTENDED);/* Standard mode frame */
    can_set_bits(id, CANTFIR, CANTFIR_RTR, cfg->type == CAN_REMOTE_REQUEST);/* regular data frame */
    can_set_bits(id, CANTFIR, CANTFIR_DLC, len);/* set data len */
    can_write_reg(id, CANTXID, cfg->frm_id);/* set id */
    can_write_reg(id, CANTXDATA0, p[0]);/* set data0 */
    can_write_reg(id, CANTXDATA1, p[1]);/* set data1 */

    can_set_bits(id, CANCMD, CANCMD_TR, 1);

    spin_unlock_irqrestore(&drv->spinlock, flags);

    int ret = wait_for_completion_interruptible_timeout(&drv->done_tx, FRAME_TIMEOUT_MS);
    if (ret <= 0) {
        printk(KERN_ERR "CAN%d: send timeout\n", id);
        return -ETIMEDOUT;
    }

    return len;
}

static int jz_can_transmit_frame(int id, struct can_frame_cfg *cfg)
{
    if (!jzcan_dev[id].is_on) {
        printk(KERN_ERR "CAN%d: please enable before write\n", id);
        return -1;
    }

    assert(cfg);
    return can_transmit_frame_inr(id, cfg);
}

static int can_receive_frame_dma(int id, struct can_frame_cfg *cfg)
{
    struct jz_can_drv *drv = &jzcan_dev[id];
    unsigned int p[4] = {0};
    int ret = 0, n, n0;
    unsigned long dst_now_addr, dst_old_addr;
    unsigned int buffer_size = CAN_BUFFER_SIZE;
    unsigned long start_addr = virt_to_phys(drv->rbuff);
    unsigned long end_addr = start_addr + buffer_size;

    while (get_rdma_addr(id) == drv->dma_pos) {
        ret = wait_for_completion_interruptible_timeout(&drv->done_rx, FRAME_TIMEOUT_MS);
        if (ret <= 0) {
            printk(KERN_DEBUG "CAN%d: receive timeout\n", id);
            return -ETIMEDOUT;
        }
    }

    unsigned long flags;
    spin_lock_irqsave(&jzcan_dev[id].spinlock, flags);

    dst_now_addr = get_rdma_addr(id);
    dst_old_addr = (unsigned long)phys_to_virt(drv->dma_pos);
    if (dst_now_addr > drv->dma_pos) {
        n = dst_now_addr - drv->dma_pos;
        if (n > FRAME_SIZE)
            n = FRAME_SIZE;

        dma_cache_inv(dst_old_addr, n);
        memcpy(p, (void *)dst_old_addr, n);
    } else {
        n = buffer_size - (drv->dma_pos - dst_now_addr);
        n0 = end_addr - drv->dma_pos;
        if (n0 > FRAME_SIZE) {
            n = FRAME_SIZE;

            dma_cache_inv(dst_old_addr, n);
            memcpy(p, (void *)dst_old_addr, n);
        } else {
            if (n > FRAME_SIZE)
                n = FRAME_SIZE;

            dma_cache_inv(dst_old_addr, n0);
            memcpy(p, (void *)dst_old_addr, n0);

            dma_cache_inv((unsigned long)start_addr, n - n0);
            memcpy((unsigned char*)p + n0, (void *)start_addr, n - n0);
        }
    }

    drv->dma_pos = start_addr + (drv->dma_pos - start_addr + n) % buffer_size;

    if (get_bit_field((unsigned long *)&p[0], CANRFIR_FF))
        cfg->mode = CAN_EXTENDED;
    if (get_bit_field((unsigned long *)&p[0], CANRFIR_RTR))
        cfg->type = CAN_REMOTE_REQUEST;

    cfg->frm_id = p[1];
    cfg->len = get_bit_field((unsigned long *)&p[0], CANRFIR_DLC);
    if (cfg->len > 8)
        cfg->len = 8;

    memcpy(cfg->data, &p[2], cfg->len);

    spin_unlock_irqrestore(&drv->spinlock, flags);

    return cfg->len;
}

static int can_receive_frame_inr(int id, struct can_frame_cfg *cfg)
{
    struct jz_can_drv *drv = &jzcan_dev[id];
    unsigned int p[4] = {0};

    int ret = 0;
    while (ring_mem_readable_size(&drv->ring) < FRAME_SIZE) {
        ret = wait_for_completion_interruptible_timeout(&drv->done_rx, FRAME_TIMEOUT_MS);
        if (ret <= 0) {
            printk(KERN_DEBUG "CAN%d: receive timeout\n", id);
            return -ETIMEDOUT;
        }
    }

    unsigned long flags;
    spin_lock_irqsave(&jzcan_dev[id].spinlock, flags);

    ring_mem_read(&drv->ring, p, FRAME_SIZE);

    if (get_bit_field((unsigned long *)&p[0], CANRFIR_FF))
        cfg->mode = CAN_EXTENDED;
    if (get_bit_field((unsigned long *)&p[0], CANRFIR_RTR))
        cfg->type = CAN_REMOTE_REQUEST;

    cfg->frm_id = p[1];
    cfg->len = get_bit_field((unsigned long *)&p[0], CANRFIR_DLC);
    if (cfg->len > 8)
        cfg->len = 8;

    memcpy(cfg->data, &p[2], cfg->len);

    spin_unlock_irqrestore(&drv->spinlock, flags);

    return cfg->len;
}

static int jz_can_receive_frame(int id, struct can_frame_cfg *cfg)
{
    if (!jzcan_dev[id].is_on) {
        printk(KERN_ERR "CAN%d: please enable before read\n", id);
        return -1;
    }

    assert(cfg);
    if (can_is_enable_dma)
        return can_receive_frame_dma(id, cfg);
    else
        return can_receive_frame_inr(id, cfg);
}

#define CMD_can_set_rate                _IOWR('c', 11, int *)
#define CMD_can_set_filter              _IOWR('c', 12, void *)
#define CMD_can_put_filter              _IOWR('c', 13, int *)
#define CMD_can_get_filter              _IO('c', 14)
#define CMD_can_enable                  _IO('c', 15)
#define CMD_can_write                   _IOWR('c', 16, struct can_frame_cfg *)
#define CMD_can_read                    _IOWR('c', 17, struct can_frame_cfg *)
#define CMD_can_disable                 _IO('c', 18)

static long can_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct jz_can_drv *drv = (struct jz_can_drv *)filp->private_data;
    int id = drv->id;

    switch (cmd) {
        case CMD_can_set_rate: {
            int bus_rate = *(int *)arg;
            jz_can_set_bauds(id, bus_rate);
            break;
        }
        case CMD_can_set_filter: {
            unsigned long *array = (void *)arg;
            int num = array[0];
            drv->filter[num].is_enable = 1;
            drv->filter[num].afid = array[1];
            jz_can_set_acceptance_filter(id);
            break;
        }
        case CMD_can_put_filter: {
            int num = *(int *)arg;
            drv->filter[num].is_enable = 0;
            jz_can_set_acceptance_filter(id);
            break;
        }
        case CMD_can_get_filter:
            jz_can_dump_acceptance_filter(id);
            break;
        case CMD_can_enable:
            jz_can_enable(id);
            break;
        case CMD_can_write: {
            struct can_frame_cfg *cfg = (struct can_frame_cfg *)arg;
            ret = jz_can_transmit_frame(id, cfg);
            break;
        }
        case CMD_can_read: {
            struct can_frame_cfg *cfg = (struct can_frame_cfg *)arg;
            ret = jz_can_receive_frame(id, cfg);
            break;
        }
        case CMD_can_disable:
            jz_can_disable(id);
            break;
        default:
            printk(KERN_ERR "CAN%d: do not support this cmd: %x\n", id, cmd);
            return -EINVAL;
    }

    return ret;
}

static int can_open(struct inode *inode, struct file *filp)
{
    struct jz_can_drv *drv = container_of(filp->private_data,
            struct jz_can_drv, mdev);

    filp->private_data = drv;

    return 0;
}

static int can_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations can_misc_fops = {
    .owner          = THIS_MODULE,
    .open           = can_open,
    .release        = can_release,
    .unlocked_ioctl = can_ioctl,
};

static void can_dma_init(struct jz_can_drv *drv)
{
    int id = drv->id;
    char tmp_name[128];
    struct device_node *np;

    sprintf(tmp_name, "can%d: can@0x%lx", id, iobase[id]);
    np = of_find_node_by_path(tmp_name);
    if (!np) {
        printk(KERN_ERR "CAN%d: %s of_find_node_opts_by_path failed\n", id, tmp_name);
        return ;
    }

    drv->dev->of_node = np;

    drv->rxchan = dma_request_slave_channel(drv->dev, "rx");
    if (!drv->rxchan) {
        printk(KERN_ERR "CAN%d: dma rxchan requested failed\n", id);
        return ;
    }
}

static void can_init(int id)
{
    int ret;
    struct jz_can_drv *drv = &jzcan_dev[id];

    char tmp_name[128];
    sprintf(tmp_name, "div_can%d", id);
    drv->clk_cgu = clk_get(NULL, tmp_name);
    BUG_ON(IS_ERR(drv->clk_cgu));

    sprintf(tmp_name, "gate_can%d", id);
    drv->clk = clk_get(NULL, tmp_name);
    BUG_ON(IS_ERR(drv->clk));

    clk_prepare_enable(drv->clk_cgu);
    clk_set_rate(drv->clk_cgu, CAN_CGU_CLK_RATE);

    ret = can_gpio_request(drv);
    if (ret < 0) {
        printk(KERN_ERR "CAN%d: gpio requeset failed\n", id);
        return;
    }

    ret = platform_device_register(&jz_can_device[id]);
    BUG_ON(ret);

    clk_prepare_enable(drv->clk);
    jz_can_init_setting(id);
    jz_can_set_bauds(id, 500000);

    drv->dev = &jz_can_device[id].dev;

    sprintf(tmp_name, "can%d", id);
    memset(&drv->mdev, 0, sizeof(struct miscdevice));
    drv->mdev.minor = MISC_DYNAMIC_MINOR;
    drv->mdev.name = tmp_name;
    drv->mdev.fops = &can_misc_fops;

    ret = misc_register(&drv->mdev);
    if (ret < 0) {
        printk(KERN_ERR "CAN%d: %s register failed\n", id, tmp_name);
        return;
    }

    drv->rbuff = m_dma_alloc_coherent(drv->dev, CAN_BUFFER_SIZE);
    BUG_ON(!drv->rbuff);

    if (can_is_enable_dma)
        can_dma_init(drv);

    sprintf(tmp_name, "CAN%d", id);
    ret = request_irq(drv->irq, can_irq_handler, 0, tmp_name, (void *)drv);
    BUG_ON(ret);

    drv->is_finish = 1;
}

static void can_exit(int id)
{
    struct jz_can_drv *drv = &jzcan_dev[id];

    free_irq(drv->irq, drv);
    m_dma_free_coherent(drv->dev, drv->rbuff, CAN_BUFFER_SIZE);
    clk_disable_unprepare(drv->clk);
    clk_disable_unprepare(drv->clk_cgu);
    clk_put(drv->clk);
    clk_put(drv->clk_cgu);
    can_gpio_release(id);
    misc_deregister(&drv->mdev);
    platform_device_unregister(&jz_can_device[id]);
}

static int __init jz_can_init(void)
{
    if (jzcan_dev[0].is_enable)
        can_init(0);
    if (jzcan_dev[1].is_enable)
        can_init(1);

    return 0;
}

static void __exit jz_can_exit(void)
{
    if (jzcan_dev[0].is_finish)
        can_exit(0);
    if (jzcan_dev[1].is_finish)
        can_exit(1);
}

module_init(jz_can_init);
module_exit(jz_can_exit);

MODULE_DESCRIPTION("JZ x2600_510 CAN Driver");
MODULE_LICENSE("GPL");