#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <common.h>
#include <bit_field.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <asm/device.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>

#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/clk.h>

#include <soc/base.h>
#include "nemc_gpio.c"
#include "nemc.h"

#define NEMC_MAX_addr_width  13
#define NEMC_MAX_buswidth  16

#define NEMC_MAX_SIZE   (1 << NEMC_MAX_addr_width) * NEMC_MAX_buswidth / 8

#define SMCR1 0x0014
#define SMCR2 0x0018

#define SARC1 0x0034
#define SARC2 0x0038

#define STRV 24, 29
#define TAW  20, 23
#define TBP  16, 19
#define TAH  12, 15
#define TAS  8, 11
#define BW   6, 7
#define BL   1, 2
#define SMT  0, 0

#define NEMC_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(NEMC_IOBASE + reg))


#define CMD_NEMC_SET_TIME                _IOWR('S', 120, struct nemc_timing *)
#define CMD_NEMC_GET_TIME                 _IOWR('S', 122, struct nemc_timing *)


static inline void nemc_write(unsigned int reg, unsigned int val)
{
    *NEMC_ADDR(reg) = val;
}

static inline int nemc_read(unsigned int reg)
{
    return *NEMC_ADDR(reg);
}

static void nemc_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(NEMC_ADDR(reg), start, end, val);
}

static unsigned int nemc_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(NEMC_ADDR(reg), start, end);
}

static void nemc_set_tas(int id, unsigned int tas)
{
    nemc_set_bit(SMCR1 + id * 0x0004, TAS, tas);
}

static void nemc_set_taw(int id, unsigned int taw)
{
    nemc_set_bit(SMCR1 + id * 0x0004, TAW, taw);
}

static void nemc_set_tbp(int id, unsigned int tbp)
{
    nemc_set_bit(SMCR1 + id * 0x0004, TBP, tbp);
}

void nemc_set_tah(int id, unsigned int tah)
{
    nemc_set_bit(SMCR1 + id * 0x0004, TAH, tah);
}

static void nemc_set_strv(int id, unsigned int strv)
{
    nemc_set_bit(SMCR1 + id * 0x0004, STRV, strv);
}

static void nemc_set_buswidth(int id, unsigned int buswidth)
{
    nemc_set_bit(SMCR1 + id * 0x0004, BW, buswidth == 16);
}


static unsigned int nemc_get_tas(int id)
{
    return nemc_get_bit(SMCR1 + id * 0x0004, TAS);
}

static unsigned int nemc_get_taw(int id)
{
    return nemc_get_bit(SMCR1 + id * 0x0004, TAW);
}

static unsigned int nemc_get_tbp(int id)
{
    return nemc_get_bit(SMCR1 + id * 0x0004, TBP);
}

static unsigned int nemc_get_tah(int id)
{
    return nemc_get_bit(SMCR1 + id * 0x0004, TAH);
}

static unsigned int nemc_get_strv(int id)
{
    return nemc_get_bit(SMCR1 + id * 0x0004, STRV);
}

/*----------------------------------------------------------------------------------------------------------------*/

enum nemc_mode {
    NORMAL_MODE,
    BURST_MODE,           //暂不支持
};

struct clk *clk;

struct nemc_dev_data {
    int id;
    int is_enable;
    int bus_width;
    unsigned long addr_start;
    unsigned long addr_start_vitual;
    struct nemc_timing timing;
    enum nemc_mode mode;
};

struct nemc_dev_data nemc_dev[2] = {
    {
        .id = 0,
        .addr_start = 0x1b000000,
        .addr_start_vitual = CKSEG1ADDR(0x1b000000),
        .mode = NORMAL_MODE,
    },
    {
        .id = 1,
        .addr_start = 0x1a000000,
        .addr_start_vitual = CKSEG1ADDR(0x1a000000),
        .mode = NORMAL_MODE,
    },
};

static void nemc_set_mode(int id, enum nemc_mode mode)
{
    nemc_set_bit(SMCR1 + id * 0x0004, SMT, mode);

    nemc_dev[id].mode = mode;
}

void nemc_set_timing(int id, struct nemc_timing *timing)
{
    nemc_set_tas(id, timing->tas);
    nemc_set_taw(id, timing->taw);
    nemc_set_tbp(id, timing->tbp);
    nemc_set_tah(id, timing->tah);
    nemc_set_strv(id, timing->strv);

    nemc_dev[id].timing = *timing;
}

void nemc_get_timing(int id, struct nemc_timing *timing)
{
    *timing = nemc_dev[id].timing;
}

void *nemc_get_addr(int id)
{
    struct nemc_dev_data *nemc = &nemc_dev[id];
    if (!nemc->is_enable) {
        printk(KERN_ERR "nemc not enable\n");
        return NULL;
    }

    return (void *)nemc->addr_start_vitual;
}

/*----------------------------------------------------------------------------------------*/

module_param_named(nemc0_enable, nemc_dev[0].is_enable, int, 0644);
module_param_named(nemc0_buswidth, nemc_dev[0].bus_width, int, 0644);

module_param_named(nemc1_enable, nemc_dev[1].is_enable, int, 0644);
module_param_named(nemc1_buswidth, nemc_dev[1].bus_width, int, 0644);

static int nemc_open(struct inode *inode, struct file *filp);
static int nemc_close(struct inode *inode, struct file *filp);
static long nemc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nemc_mmap(struct file *file, struct vm_area_struct *vma);

static struct file_operations nemc_misc_fops = {
    .open = nemc_open,
    .release = nemc_close,
    .unlocked_ioctl = nemc_ioctl,
    .mmap = nemc_mmap,
};

static struct miscdevice mdev[2] = {
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "nemc0",
        .fops = &nemc_misc_fops,
    },
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "nemc1",
        .fops = &nemc_misc_fops,
    },
};

static int nemc_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int nemc_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long nemc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct miscdevice *dev = filp->private_data;
    int id = dev - mdev;

    struct nemc_dev_data *nemc = &nemc_dev[id];
    struct nemc_timing *timing;

    switch (cmd)
    {
    case CMD_NEMC_SET_TIME:
        timing = (void *)arg;
        nemc_set_timing(id, timing);
        break;

    case CMD_NEMC_GET_TIME:
        timing = (void *)arg;
        copy_to_user(timing, &nemc->timing, sizeof(*timing));
        break;
    default:
        printk(KERN_ERR "Nemc %d: not support this cmd = %d\n", id, cmd);
        return -1;
    }

    return 0;

}

static int nemc_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct miscdevice *dev = filp->private_data;
    int id = dev - mdev;

    struct nemc_dev_data *nemc = &nemc_dev[id];

    unsigned long start;
    phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

    u32 len = NEMC_MAX_SIZE;
    len = ALIGN(NEMC_MAX_SIZE, PAGE_SIZE);

    if ((vma->vm_end - vma->vm_start + offset) > len)
        return -EINVAL;

    start = nemc->addr_start;
    start &= PAGE_MASK;
    offset += start;

    vma->vm_pgoff = offset >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED;

    if (io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int bus_width;

#ifndef MAX
#define	MAX(a, b)		(((a) > (b)) ? (a) : (b))
#endif /* MAX */

static void init_nemc_dev(int id)
{
    struct nemc_dev_data *nemc = &nemc_dev[id];
    struct nemc_timing *timing = &nemc->timing;

    if (!nemc->is_enable)
        return;

    if (nemc->bus_width != 8 && nemc->bus_width != 16) {
        printk(KERN_ERR "nemc%d not support this buswidth %d\n",id, nemc->bus_width);
        nemc->is_enable = 0;
        return;
    }

    int ret = misc_register(&mdev[id]);
    if (ret < 0)
        return;

    nemc_set_mode(id, nemc->mode);
    nemc_set_buswidth(id, nemc->bus_width);

    timing->tas = nemc_get_tas(id);
    timing->taw = nemc_get_taw(id);
    timing->tbp = nemc_get_tbp(id);
    timing->tah = nemc_get_tah(id);
    timing->strv = nemc_get_strv(id);

    bus_width = MAX(bus_width, nemc->bus_width);
}


static int nemc_init(void)
{

    clk = clk_get(NULL, "gate_nemc");

    clk_prepare_enable(clk);

    init_nemc_dev(0);

    init_nemc_dev(1);

    if (nemc_dev[0].is_enable || nemc_dev[1].is_enable)
        nemc_init_gpio(bus_width, nemc_dev[0].is_enable, nemc_dev[1].is_enable);

    return 0;
}

static void nemc_exit(void)
{
    nemc_deinit_gpio();

    if (nemc_dev[0].is_enable)
        misc_deregister(&mdev[0]);

    if (nemc_dev[1].is_enable)
        misc_deregister(&mdev[1]);

    clk_put(clk);

    return;
}

module_init(nemc_init);
module_exit(nemc_exit);

MODULE_DESCRIPTION("necm module");
MODULE_LICENSE("GPL");