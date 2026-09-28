
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <soc/base.h>
#include <assert.h>
#include "bit_field.h"
#include <utils/gpio.h>
#include <utils/clock.h>
#include <linux/string.h>

#define RSAC    0x0
#define RSAE    0x4
#define RSAN    0x8
#define RSAM    0xC
#define RSAP    0x10

#define RSAC_RSA_INT_M      17, 17
#define RSAC_PER_INT_M      16, 16
#define RSAC_Asfifo_empty   9, 9
#define RSAC_RSA_SEL        7, 7
#define RSAC_RSAC           6, 6
#define RSAC_RSAD           5, 5
#define RSAC_RSAS           4, 4
#define RSAC_PERC           3, 3
#define RSAC_PERD           2, 2
#define RSAC_PERS           1, 1
#define RSAC_EN             0, 0

#define RSA_REG_BASE    0x134C0000

#define RSA_ADDR(reg)   ((volatile unsigned long *)((KSEG1ADDR(RSA_REG_BASE)) + (reg)))

static inline void rsa_write_reg(unsigned int reg, unsigned int value)
{
    *RSA_ADDR(reg) = value;
}

static inline unsigned int rsa_read_reg(unsigned int reg)
{
    return *RSA_ADDR(reg);
}

static inline void rsa_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(RSA_ADDR(reg), start, end, val);
}

static inline unsigned int rsa_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(RSA_ADDR(reg), start, end);
}

static inline void dump_reg(void)
{
    printk(KERN_ERR "debug dump reg:\n");
    printk(KERN_ERR "RSAC = 0x%08x\n", rsa_read_reg(RSAC));
    // printk(KERN_ERR "RSAE = 0x%08x\n", rsa_read_reg(RSAE));
    // printk(KERN_ERR "RSAN = 0x%08x\n", rsa_read_reg(RSAN));
    // printk(KERN_ERR "RSAM = 0x%08x\n", rsa_read_reg(RSAM));
    printk(KERN_ERR "RSAP = 0x%08x\n", rsa_read_reg(RSAP));
}

#define RSA_CLK 400000000  //<=500M

#define CMD_RSA_PREPARE_KEY _IOWR('r', 0, void *)
#define CMD_RSA_DO_CRYPT    _IOWR('r', 1, void *)

#define RSA_MAX_KEYL 64

enum rsa_mode {
    RSA_1024,
    RSA_2048,
};

struct rsa_key {
    int is_pub_use;
    enum rsa_mode mode;
    unsigned int n[RSA_MAX_KEYL];
    unsigned int e[RSA_MAX_KEYL];
    unsigned int d[RSA_MAX_KEYL];
};

struct rsa_data {
    int len;
    unsigned int *src;
    unsigned int *dst;
};

struct jz_rsa_drv {
    struct clk *clk;
    struct clk *pclk;
    struct miscdevice mdev;
    struct mutex lock;

    int busy;
    unsigned int keyl;
    struct rsa_key key;
    struct rsa_data data;
};

static struct jz_rsa_drv rsa_drv;

static inline int rsa_get_keyl(enum rsa_mode mode)
{
    switch (mode)
    {
    case RSA_1024:
        return 32;

    case RSA_2048:
        return 64;

    default:
        printk(KERN_ERR "rsa: do not support this mode: %d\n", mode);
        break;
    }

    return 0;
}

static inline void rsa_write_data(unsigned int reg, unsigned int *src, int keyl)
{
    int i;
    for (i = 0; i < keyl; i++)
        rsa_write_reg(reg, src[i]);
}

static inline void rsa_read_data(unsigned int *dst, int keyl)
{
    int i;
    for (i = 0; i < keyl; i++) {
        /* wait until RSAC_Asfifo_empty become 0, then rsap can be read */
        while (rsa_get_bit(RSAC, RSAC_Asfifo_empty));
        dst[(keyl - 1) - i] = rsa_read_reg(RSAP);
    }
}

static int rsa_prepare_key(struct jz_rsa_drv *drv, struct rsa_key *key)
{
    if (drv == NULL || key == NULL) {
        printk(KERN_ERR "RSA: prepare_key get drv/key failed!\n");
        return -EFAULT;
    }

    int mode = key->mode;
    if (!(mode == RSA_1024 || mode == RSA_2048)) {
        printk(KERN_ERR "RSA: prepare_key do not support this mode: %d!\n", mode);
        return -EPERM;
    }

    drv->keyl = rsa_get_keyl(mode);
    if (!drv->keyl) {
        printk(KERN_ERR "RSA: prepare_key get_keyl failed!\n");
        return -EINVAL;
    }

    rsa_set_bit(RSAC, RSAC_RSA_SEL, mode);

    unsigned int *key_e_or_d = key->is_pub_use ? key->e : key->d;
    rsa_write_data(RSAE, key_e_or_d, drv->keyl);
    rsa_write_data(RSAN, key->n, drv->keyl);

    rsa_set_bit(RSAC, RSAC_PERS, 1);
    rsa_set_bit(RSAC, RSAC_PERC, 1);
    rsa_set_bit(RSAC, RSAC_PERC, 0);

    int ret = -ETIMEDOUT;
    int i = 1000;
    while (i--) {
        /* wait for per-process done */
        if (rsa_get_bit(RSAC, RSAC_PERD)) {
            ret = 0;
            break;
        }
    }

    rsa_set_bit(RSAC, RSAC_PERS, 0);

    return ret;
}

static inline int rsa_do_wait_crypt_done(void)
{
    int i = 500000;
    while (i--) {
        /* wait for main-process done */
        if (rsa_get_bit(RSAC, RSAC_RSAD))
            break;
        else if (i == 0)
            return -ETIMEDOUT;
    }

    return 0;
}

static int rsa_do_crypt(struct jz_rsa_drv *drv, struct rsa_data *data)
{
    if (drv == NULL || data == NULL) {
        printk(KERN_ERR "RSA: do_crypt get drv/data failed!\n");
        return -EFAULT;
    }

    unsigned int *src = data->src;
    unsigned int *dst = data->dst;
    int len = data->len;
    int keyl = drv->keyl;

    if (keyl == 0 || src == NULL || dst == NULL || len == 0) {
        printk(KERN_ERR "RSA: do_crypt get keyl/input/output/len failed!\n");
        return -EFAULT;
    }

    if (len % keyl) {
        printk(KERN_ERR "RSA: do_crypt len(%d) should be aligned with keyl(%d)!\n", len, keyl);
        return -EFAULT;
    }

    int ret = 0;
    unsigned int din[RSA_MAX_KEYL];
    unsigned int dout[RSA_MAX_KEYL];
    while (len > 0) {
        if (copy_from_user(din, src, keyl * 4)) {
            printk(KERN_ERR "RSA: data_in copy_from_user error!!");
            ret = -EFAULT;
            break;
        }

        rsa_write_data(RSAM, din, keyl);

        rsa_set_bit(RSAC, RSAC_RSAS, 1);
        rsa_set_bit(RSAC, RSAC_RSAC, 1);
        rsa_set_bit(RSAC, RSAC_RSAC, 0);

        if (rsa_do_wait_crypt_done()) {
            ret = -ETIMEDOUT;
            break;
        }

        rsa_read_data(dout, keyl);
        if (copy_to_user(dst, dout, keyl * 4)) {
            printk(KERN_ERR "RSA: data_out copy_to_user error!!");
            ret = -EFAULT;
            break;
        }

        rsa_set_bit(RSAC, RSAC_RSAS, 0);

        len -= keyl;
        src += keyl;
        dst += keyl;
    }

    return ret;
}

static long rsa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct jz_rsa_drv *drv = container_of(filp->private_data,
            struct jz_rsa_drv, mdev);

    mutex_lock(&drv->lock);
    switch (cmd) {
    case CMD_RSA_PREPARE_KEY: {
        struct rsa_key *tmp = (struct rsa_key *)arg;
        drv->key.is_pub_use = tmp->is_pub_use;
        drv->key.mode = tmp->mode;

        if (copy_from_user(drv->key.n, tmp->n, RSA_MAX_KEYL * 4)) {
            printk(KERN_ERR "RSA: key_n copy_from_user error!!");
            ret = -EFAULT;
            break;
        }

        if (copy_from_user(drv->key.e, tmp->e, RSA_MAX_KEYL * 4)) {
            printk(KERN_ERR "RSA: key_e copy_from_user error!!");
            ret = -EFAULT;
            break;
        }

        if (copy_from_user(drv->key.d, tmp->d, RSA_MAX_KEYL * 4)) {
            printk(KERN_ERR "RSA: key_d copy_from_user error!!");
            ret = -EFAULT;
            break;
        }

        ret = rsa_prepare_key(drv, &drv->key);
        break;
    }

    case CMD_RSA_DO_CRYPT: {
        struct rsa_data *tmp = (struct rsa_data *)arg;

        if (copy_from_user(&drv->data, tmp, sizeof(struct rsa_data))) {
            printk(KERN_ERR "RSA: rsa_data copy_from_user error!!");
            ret = -EFAULT;
            break;
        }

        ret = rsa_do_crypt(drv, &drv->data);
        break;
    }

    default: {
        printk(KERN_ERR "RSA: do not support this cmd: %x\n", cmd);
        ret = -EINVAL;
    }
    }
    mutex_unlock(&drv->lock);

    return ret;
}

static int rsa_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    struct jz_rsa_drv *drv = container_of(filp->private_data,
            struct jz_rsa_drv, mdev);

    mutex_lock(&drv->lock);

    if (drv->busy) {
        printk(KERN_ERR "RSA: failed to open, busy!\n");
        ret = -EBUSY;
        goto err;
    }

    drv->busy = 1;
    rsa_set_bit(RSAC, RSAC_EN, 1);

    mutex_unlock(&drv->lock);

err:
    return ret;
}

static int rsa_release(struct inode *inode, struct file *filp)
{
    struct jz_rsa_drv *drv = container_of(filp->private_data,
            struct jz_rsa_drv, mdev);

    mutex_lock(&drv->lock);

    drv->busy = 0;
    rsa_set_bit(RSAC, RSAC_EN, 0);

    mutex_unlock(&drv->lock);

    return 0;
}

static struct file_operations rsa_fops = {
    .owner = THIS_MODULE,
    .open = rsa_open,
    .release = rsa_release,
    .unlocked_ioctl = rsa_ioctl,
};

static int __init jz_rsa_init(void)
{
    int ret;
    struct jz_rsa_drv *drv = &rsa_drv;

    drv->clk = clk_get(NULL, "gate_rsa");
    assert(!IS_ERR(drv->clk));

    drv->pclk = clk_get(NULL, "div_rsa");
    assert(!IS_ERR(drv->pclk));

    struct clk *clk = clk_get(NULL, "mux_rsa");
    assert(!IS_ERR(clk));

    clk_set_parent(clk, clk_get(NULL, "mpll"));
    clk_set_rate(drv->pclk, RSA_CLK);

    clk_prepare_enable(drv->clk);
    clk_prepare_enable(drv->pclk);

    mutex_init(&drv->lock);
    drv->mdev.minor = MISC_DYNAMIC_MINOR;
    drv->mdev.name = "jz_rsa";
    drv->mdev.fops = &rsa_fops;

    ret = misc_register(&drv->mdev);
    BUG_ON(ret < 0);

    return 0;
}
module_init(jz_rsa_init);

static void __exit jz_rsa_exit(void)
{
    struct jz_rsa_drv *drv = &rsa_drv;
    misc_deregister(&drv->mdev);
    clk_disable_unprepare(drv->clk);
    clk_disable_unprepare(drv->pclk);
    clk_put(drv->clk);
    clk_put(drv->pclk);
    return;
}
module_exit(jz_rsa_exit);

MODULE_DESCRIPTION("Ingenic Soc RSA driver");
MODULE_LICENSE("GPL");