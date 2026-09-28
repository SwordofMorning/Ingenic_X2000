#include "string.h"
#include <malloc.h>
#include <soc/base.h>
#include <driver/clk.h>
#include <driver/irq.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <assert.h>
#include <errno.h>
#include <driver/systick.h>
#include <bit_field2.h>
#include <delay.h>

#include "i2c_regs.h"

#define GPIO_I2C0_SDA        GPIO_PC(14)
#define GPIO_I2C0_SCL        GPIO_PC(13)
#define I2C0_GPIO_FUNC       GPIO_FUNC_3

#ifdef APP_libmcu_x2000_I2C1_PC
#define GPIO_I2C1_SDA        GPIO_PC(24)
#define GPIO_I2C1_SCL        GPIO_PC(23)
#define I2C1_GPIO_FUNC       GPIO_FUNC_2
#else
#define GPIO_I2C1_SDA        GPIO_PD(12)
#define GPIO_I2C1_SCL        GPIO_PD(11)
#define I2C1_GPIO_FUNC       GPIO_FUNC_1
#endif

#ifdef APP_libmcu_x2000_I2C2_PB
#define GPIO_I2C2_SDA        GPIO_PB(23)
#define GPIO_I2C2_SCL        GPIO_PB(22)
#define I2C2_GPIO_FUNC       GPIO_FUNC_2
#elif defined APP_libmcu_x2000_I2C2_PD
#define GPIO_I2C2_SDA        GPIO_PD(21)
#define GPIO_I2C2_SCL        GPIO_PD(20)
#define I2C2_GPIO_FUNC       GPIO_FUNC_2
#else
#define GPIO_I2C2_SDA        GPIO_PE(20)
#define GPIO_I2C2_SCL        GPIO_PE(19)
#define I2C2_GPIO_FUNC       GPIO_FUNC_1
#endif

#ifdef APP_libmcu_x2000_I2C3_PA
#define GPIO_I2C3_SDA        GPIO_PA(17)
#define GPIO_I2C3_SCL        GPIO_PA(16)
#define I2C3_GPIO_FUNC       GPIO_FUNC_0
#else
#define GPIO_I2C3_SDA        GPIO_PD(31)
#define GPIO_I2C3_SCL        GPIO_PD(30)
#define I2C3_GPIO_FUNC       GPIO_FUNC_1
#endif

#ifdef APP_libmcu_x2000_I2C4_PC
#define GPIO_I2C4_SDA        GPIO_PC(26)
#define GPIO_I2C4_SCL        GPIO_PC(25)
#define I2C4_GPIO_FUNC       GPIO_FUNC_1
#else
#define GPIO_I2C4_SDA        GPIO_PD(1)
#define GPIO_I2C4_SCL        GPIO_PD(0)
#define I2C4_GPIO_FUNC       GPIO_FUNC_2
#endif

#ifdef APP_libmcu_x2000_I2C5_PC
#define GPIO_I2C5_SDA        GPIO_PC(28)
#define GPIO_I2C5_SCL        GPIO_PC(27)
#define I2C5_GPIO_FUNC       GPIO_FUNC_1
#else
#define GPIO_I2C5_SDA        GPIO_PD(5)
#define GPIO_I2C5_SCL        GPIO_PD(4)
#define I2C5_GPIO_FUNC       GPIO_FUNC_1
#endif

struct i2c_gpio_def {
    short i2c_sda;
    short i2c_scl;
    short gpio_func;
};

static const unsigned long iobase[] = {
    I2C0_IOBASE,
    I2C1_IOBASE,
    I2C2_IOBASE,
    I2C3_IOBASE,
    I2C4_IOBASE,
    I2C5_IOBASE,
};

#define I2C_ADDR(id, reg) ((volatile unsigned long *)((iobase[id]) + (reg)))

static inline void jz_i2c_write_reg(int id, unsigned int reg, unsigned int value)
{
    *I2C_ADDR(id, reg) = value;
}

static inline unsigned int jz_i2c_read_reg(int id, unsigned int reg)
{
    return *I2C_ADDR(id, reg);
}

static inline void jz_i2c_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field_v(I2C_ADDR(id, reg), start, end, val);
}

static inline unsigned int jz_i2c_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field_v(I2C_ADDR(id, reg), start, end);
}

static void jz_i2c_hal_disable_smb(int id)
{
    jz_i2c_set_bit(id, SMB_ENABLE, SMBENABLE_SMBENB, 0);
}

static void jz_i2c_hal_enable_smb(int id)
{
    jz_i2c_set_bit(id, SMB_ENABLE, SMBENABLE_SMBENB, 1);
}

static unsigned int jz_i2c_hal_wait_smb_enable_status(int id)
{
    return jz_i2c_get_bit(id, SMB_ENBST, SMBENBST_SMBEN);
}

static void jz_i2c_hal_set_speed_standard(int id)
{
    jz_i2c_set_bit(id, SMB_CON, SMBCON_SPEED, 1);
}

static void jz_i2c_hal_set_speed_fast(int id)
{
    jz_i2c_set_bit(id, SMB_CON, SMBCON_SPEED, 2);
}

void jz_i2c_hal_set_smb_shcnt(int id, int val)
{
    if (val < 6)
        val = 6;
    jz_i2c_set_bit(id, SMB_SHCNT, SMBSHCNT, val);
}

void jz_i2c_hal_set_smb_slcnt(int id, int val)
{
    if (val < 8)
        val = 8;
    jz_i2c_set_bit(id, SMB_SLCNT, SMBSLCNT, val);
}

void jz_i2c_hal_set_smb_fhcnt(int id, int val)
{
    if (val < 6)
        val = 6;
    jz_i2c_set_bit(id, SMB_FHCNT, SMBFHCNT, val);
}

void jz_i2c_hal_set_smb_flcnt(int id, int val)
{
    if (val < 8)
        val = 8;
    jz_i2c_set_bit(id, SMB_FLCNT, SMBFLCNT, val);
}

void jz_i2c_hal_set_setup_time(int id, int val)
{
    jz_i2c_set_bit(id, SMB_SDASU, SMBSDASU, val);
}

void jz_i2c_hal_set_hold_time(int id, int val)
{
    jz_i2c_set_bit(id, SMB_SDAHD, SMBSDAHD, val);
}

static void jz_i2c_hal_set_addressing(int id, enum i2c_addr_type addr_bit)
{
    if (addr_bit == I2C_ADDR_BIT_10)
        jz_i2c_set_bit(id, SMB_TAR, SMBTAR_MATP, 1);
    else
        jz_i2c_set_bit(id, SMB_TAR, SMBTAR_MATP, 0);
}

/*-----------------------------------------------------------------------------------------------*/

#define MAX_FIFO_LEN    64
#define TX_TL           32
#define RX_TL           (32 - 1)

#define READ_CMD    0x100
#define STOP_CMD    0x200
#define RESTART_CMD 0x400

#define ABRT_7B_ADDR_NOACK  1
#define ABRT_10B_ADDR_NOACK 2

#define I2C_START 1
#define I2C_RE_START 2
#define I2C_STOP 4

#define MAX_SETUP_TIME 255
#define MAX_HOLD_TIME  65535

struct jz_i2c_drv {
    int id;
    unsigned long rate;

    int abtsrc;

    int len;
    int cmd;
    int rd_num;
    char *tx_buf;
    char *rx_buf;

    enum clk_type clk;

    short i2c_sda;
    short i2c_scl;
    short gpio_func;

};

static struct jz_i2c_drv *jz_i2c_dev[6] = {
    #ifdef APP_libmcu_x2000_I2C0_BUS
    [0] = &(struct jz_i2c_drv) {
        .id = 0,
        .rate = APP_libmcu_x2000_I2C0_RATE,
        .clk = CLK_GATE_I2C0,
        .i2c_sda = GPIO_I2C0_SDA,
        .i2c_scl = GPIO_I2C0_SCL,
        .gpio_func = I2C0_GPIO_FUNC
    },
    #endif

    #ifdef APP_libmcu_x2000_I2C1_BUS
    [1] = &(struct jz_i2c_drv){
        .id = 1,
        .rate = APP_libmcu_x2000_I2C1_RATE,
        .clk = CLK_GATE_I2C1,
        .i2c_sda = GPIO_I2C1_SDA,
        .i2c_scl = GPIO_I2C1_SCL,
        .gpio_func = I2C1_GPIO_FUNC
    },
    #endif

    #ifdef APP_libmcu_x2000_I2C2_BUS
    [2] = &(struct jz_i2c_drv) {
        .id = 2,
        .rate = APP_libmcu_x2000_I2C2_RATE,
        .clk = CLK_GATE_I2C2,
        .i2c_sda = GPIO_I2C2_SDA,
        .i2c_scl = GPIO_I2C2_SCL,
        .gpio_func = I2C2_GPIO_FUNC
    },
    #endif

    #ifdef APP_libmcu_x2000_I2C3_BUS
    [3] = &(struct jz_i2c_drv) {
        .id = 3,
        .rate = APP_libmcu_x2000_I2C3_RATE,
        .clk = CLK_GATE_I2C3,
        .i2c_sda = GPIO_I2C3_SDA,
        .i2c_scl = GPIO_I2C3_SCL,
        .gpio_func = I2C3_GPIO_FUNC
    },
    #endif

    #ifdef APP_libmcu_x2000_I2C4_BUS
    [4] = &(struct jz_i2c_drv) {
        .id = 4,
        .rate = APP_libmcu_x2000_I2C4_RATE,
        .clk = CLK_GATE_I2C4,
        .i2c_sda = GPIO_I2C4_SDA,
        .i2c_scl = GPIO_I2C4_SCL,
        .gpio_func = I2C4_GPIO_FUNC
    },
    #endif

    #ifdef APP_libmcu_x2000_I2C5_BUS
    [5] = &(struct jz_i2c_drv) {
        .id = 5,
        .rate = APP_libmcu_x2000_I2C5_RATE,
        .clk = CLK_GATE_I2C5
        .i2c_sda = GPIO_I2C5_SDA,
        .i2c_scl = GPIO_I2C5_SCL,
        .gpio_func = I2C5_GPIO_FUNC
    }
    #endif
};

static inline void jz_i2c_set_rxfifo_threshold(int id, int len)
{
    if (len > MAX_FIFO_LEN)
        jz_i2c_set_bit(id, SMB_RXTL, SMBRXTL_RXTL, RX_TL);
    else
        jz_i2c_set_bit(id, SMB_RXTL, SMBRXTL_RXTL, len - 1);
}

static inline void jz_i2c_set_txfifo_threshold(int id, int len)
{
    if (len > MAX_FIFO_LEN)
        jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, TX_TL);
    else
        jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, len);
}

static void jz_i2c_read_rx_fifo(struct jz_i2c_drv *drv, int len)
{
    unsigned short tmp;
    int id = drv->id;
    drv->len -= len;

    while(len > 0) {
        tmp = jz_i2c_get_bit(id, SMB_DC, SMBDC_DAT);
        *drv->rx_buf++ = tmp;
        len--;
    }
}

/*
 * NOTE:发送读数据的个数是通过写RX FIFO 的次数来确定的
 */
static void jz_i2c_write_rx_fifo(struct jz_i2c_drv *drv, int len, int first)
{
    unsigned short tmp = 0;
    int id = drv->id;
    int cmd = drv->cmd;

    len = len > drv->rd_num ? drv->rd_num : len;/* 本次最多可以发送的读操作个数 */
    drv->rd_num -= len;

    while (len > 0) {
        tmp = READ_CMD;
        if (first && (cmd & I2C_RE_START)) {
            tmp |= RESTART_CMD;
            first = 0;
        }

        if (len == 1 && drv->rd_num == 0) {
            if (cmd & I2C_STOP)
                tmp |= STOP_CMD;
        }
        len--;
        jz_i2c_write_reg(id, SMB_DC, tmp);
    }
}

static void jz_i2c_write_tx_fifo(struct jz_i2c_drv* drv, int len, int first)
{
    unsigned short tmp = 0;
    int id = drv->id;
    int cmd = drv->cmd;

    len = len > drv->len ? drv->len : len;/* 本次最多可以发送的数据个数 */
    drv->len -= len;

    while (len > 0) {
        tmp = *(drv->tx_buf)++;
        tmp &= 0xff;
        if (first && (cmd & I2C_RE_START)) {
            tmp |= RESTART_CMD;
            first = 0;
        }

        if (len == 1 && drv->len == 0) {
            if (cmd & I2C_STOP)
                tmp |= STOP_CMD;
        }
        len--;
        jz_i2c_write_reg(id, SMB_DC, tmp);
    }
}

static void jz_i2c_hal_set_speed(int id)
{
    int high_cnt = 0;
    int low_cnt = 0;
    int setup_time = 0;
    int hold_time = 0;
    unsigned long rate = jz_i2c_dev[id]->rate;
    unsigned long i2c_clk_rate = cpccr_get_rate(CLK_ID_PCLK);
    int mode = jz_i2c_dev[id]->rate <= 100000 ? 1 : 0;

    /*          high
     *          _____       _____
     *  clk  __|     |_____|     |____ ...
     *                 low
     *         |<-单个周期->|
     *
     *  在这里 high_cnt和low_cnt各占单个周期的二分之一.
     */
    high_cnt = i2c_clk_rate / (rate * 2);
    low_cnt = i2c_clk_rate / (rate * 2);

    /*           high
     *          _____       _____      _____
     *  clk  __|  |  |_____|     |____|     |____ ...
     *            |  |  |
     *            |  |  |
     *            |__|__|     _____
     *  data _____/  |  |\___/      \____ ...
     *            |  |  |
     *     setup->|  |< |
     *             ->|  |<-hold
     *
     *  在这里 setup time和hold time各占单个周期的四分之一.
     */
    setup_time = i2c_clk_rate / (rate * 4);
    if (setup_time > 1)
        setup_time -= 1;

    hold_time = i2c_clk_rate / (rate * 4);

    if (setup_time > MAX_SETUP_TIME)
        setup_time = MAX_SETUP_TIME;
    if (setup_time < 1)
        setup_time = 1;
    if (hold_time > MAX_HOLD_TIME)
        hold_time = MAX_HOLD_TIME;

    /* 若没有配置，默认使用高速模式 */
    if (mode) {
        /* standard 标准模式 */
        jz_i2c_hal_set_speed_standard(id);
        jz_i2c_hal_set_smb_shcnt(id, high_cnt);
        jz_i2c_hal_set_smb_slcnt(id, low_cnt);
    } else {
        /* fast 高速模式 */
        jz_i2c_hal_set_speed_fast(id);
        jz_i2c_hal_set_smb_fhcnt(id, high_cnt);
        jz_i2c_hal_set_smb_flcnt(id, low_cnt);
    }

    jz_i2c_hal_set_setup_time(id, setup_time);
    jz_i2c_hal_set_hold_time(id, hold_time);
}

static void i2c_init_setting(struct jz_i2c_drv *drv)
{
    int id = drv->id;

    /* 开时钟 */
    clk_enable(drv->clk, 1);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    jz_i2c_set_bit(id, SMB_CON, SMBCON_RESTART, 1);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 设置I2C的地址类型 */
    jz_i2c_hal_set_addressing(id, I2C_ADDR_BIT_7);

    /* 设置I2C的速度*/
    jz_i2c_hal_set_speed(id);

    /* 设置I2C 时序滤波寄存器 单位:APB clock cycles */
    jz_i2c_write_reg(id, SMB_FSPKLEN, 0x0F);

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 关时钟 */
    clk_enable(drv->clk, 0);
}

static void jz_i2c_enable(int id)
{
    /* 开时钟 */
    clk_enable(jz_i2c_dev[id]->clk, 1);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 设置I2C的速度 */
    jz_i2c_hal_set_speed(id);

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 开控制器 */
    jz_i2c_hal_enable_smb(id);
}

static void jz_i2c_disable(int id)
{
    int tmp;
    unsigned long timeout = 1000000;

    /* 判断I2C状态 */
    tmp = jz_i2c_get_bit(id, SMB_ST, SMBST_MSTACT);
    while (tmp && (--timeout > 0)) {
        udelay(10);     /* 经测试，在传输数据量大的情况下，建议延时10us及以上 */
        tmp = jz_i2c_get_bit(id, SMB_ST, SMBST_MSTACT);
    }
    if (timeout <= 0)
        panic("I2C%d disable timeout!\n", id);

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 关时钟 */
    clk_enable(jz_i2c_dev[id]->clk, 0);
}

static void jz_i2c_reset(int id)
{
    jz_i2c_disable(id);
    udelay(10);
    jz_i2c_enable(id);
}

static int get_i2c_status(int id)
{
    int rx_valid;
    int tx_valid;

    struct jz_i2c_drv *drv = jz_i2c_dev[id];

    unsigned long smb_intst = jz_i2c_read_reg(id, SMB_INTST);

    /* TXABT 中断被触发 */
    if (get_bit_field(smb_intst, SMBINTST_TXABT)) {
        drv->abtsrc = jz_i2c_read_reg(id, SMB_ABTSRC);

        //清除 TXABT 中断
        jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);
        return 0;
    }

    /* TX FIFO 空中断被触发 */
    if (get_bit_field(smb_intst, SMBINTST_TXEMP)) {
        if (drv->len == 0) {
            /* 关 FIFO空 中断 */
            jz_i2c_set_bit(id, SMB_INTM, SMBINTM_MTXEMP, 0);

            /* 如果MSG没有STOP信号，需要在这里唤醒 */
            if (!(drv->cmd & I2C_STOP)) {
                jz_i2c_set_bit(id, SMB_INTM, SMBINTM_MISTP, 0);
                return 0;
            }
        } else {
            tx_valid = MAX_FIFO_LEN - jz_i2c_read_reg(id, SMB_TXFLR);

            jz_i2c_set_txfifo_threshold(id, drv->len);

            jz_i2c_write_tx_fifo(drv, tx_valid, 0);

            return 1;
        }
    }

    /* RX FIFO 满中断被触发 */
    if (get_bit_field(smb_intst, SMBINTST_RXFL)) {

        rx_valid = jz_i2c_read_reg(id, SMB_RXFLR);
        jz_i2c_read_rx_fifo(drv, rx_valid);

        if (drv->len > 0) {
            jz_i2c_set_rxfifo_threshold(id, drv->len);
            jz_i2c_write_rx_fifo(drv, rx_valid, 0);
            return 1;
        } else {
            return 0;
        }
    }

    /* 停止信号中断被触发 */
    if (get_bit_field(smb_intst, SMBINTST_ISTP)) {
        jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
        return 0;
    }

    return 1;
}

static inline unsigned int timeout_us(int id, int len)
{
    unsigned long rate;

    if (!len)
        return 0;

    rate = jz_i2c_dev[id]->rate / 1000;

    return (len + 1) * 1000 * 1000 * 9 * 2 / rate;
}

static int jz_i2c_read(int id, unsigned short addr, char *buf, int len, int cmd)
{
    int ret = 0;
    unsigned int timeout;
    unsigned long smb_intm = jz_i2c_read_reg(id, SMB_INTM);

    if (len <= 0)
        return 0;

    timeout = timeout_us(id, len) + 1000;

    jz_i2c_dev[id]->rd_num = len;
    jz_i2c_dev[id]->len = len;
    jz_i2c_dev[id]->rx_buf = buf;
    jz_i2c_dev[id]->cmd = cmd;

    /* 清除STOP TXABT中断 */
    jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
    jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);

    /* 设置 RX FIFO 阈值 */
    jz_i2c_set_rxfifo_threshold(id, len);

    if (cmd & I2C_START)
        jz_i2c_write_reg(id, SMB_TAR, addr);

    jz_i2c_write_rx_fifo(jz_i2c_dev[id], MAX_FIFO_LEN, 1);

    /* 开中断 */
    smb_intm = set_bit_field(smb_intm, SMBINTM_MRXFL, 1);
    smb_intm = set_bit_field(smb_intm, SMBINTM_MTXABT, 1);

    if (cmd & I2C_STOP)
        smb_intm = set_bit_field(smb_intm, SMBINTM_MISTP, 1);

    jz_i2c_write_reg(id, SMB_INTM, smb_intm);

    uint64_t start = systick_get_time_usec();
    while (get_i2c_status(id)) {
        if (systick_get_time_usec() - start > timeout) {
            printf("I2c%d, read timeout\n", id);

            jz_i2c_reset(id);
            ret = -ETIMEDOUT;
            break;
        }
    }

    if (jz_i2c_dev[id]->abtsrc) {

        int src = jz_i2c_dev[id]->abtsrc;

        printf("I2C%d, TXABRT ERR 0x%x\n", id, src);

        if (src & ABRT_7B_ADDR_NOACK || src & ABRT_10B_ADDR_NOACK) {

            if (cmd & I2C_RE_START) {
                printf("I2C :DEVICE 0x%x MAYBE NO SUPPORT RESTART CMD\n", addr);
            }

            ret = -ENXIO;

        } else {
            ret = -EIO;
        }

        jz_i2c_dev[id]->abtsrc = 0;
    }
    return ret;
}

static int jz_i2c_detect_device(int id, unsigned short addr, enum i2c_addr_type addr_bit)
{
    int ret = 0;
    unsigned int timeout;
    unsigned short tmp;
    unsigned long smb_intm;

    tmp = READ_CMD | STOP_CMD;

    timeout = timeout_us(id, 1) + 1000;

    jz_i2c_enable(id);

    jz_i2c_hal_set_addressing(id, addr_bit);

    jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
    jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);

    jz_i2c_write_reg(id, SMB_TAR, addr);
    jz_i2c_write_reg(id, SMB_DC, tmp);


    smb_intm = jz_i2c_read_reg(id, SMB_INTM);

    smb_intm = set_bit_field(smb_intm, SMBINTM_MISTP, 1);
    smb_intm = set_bit_field(smb_intm, SMBINTM_MTXABT, 1);

    jz_i2c_write_reg(id, SMB_INTM, smb_intm);

    uint64_t start = systick_get_time_usec();
    while (get_i2c_status(id)) {
        if (systick_get_time_usec() - start > timeout) {
            printf("I2c%d, detect timeout\n", id);

            jz_i2c_reset(id);
            ret = -ETIMEDOUT;
            break;
        }
    }

    if (jz_i2c_dev[id]->abtsrc) {
        printf("I2C%d: device: 0x%x, detect err %x\n", id, addr, jz_i2c_dev[id]->abtsrc);
        jz_i2c_dev[id]->abtsrc = 0;
        ret = -ENXIO;
    }

    jz_i2c_disable(id);

    return ret;
}

static int jz_i2c_write(int id, unsigned short addr, char *buf, int len, int cmd)
{
    int ret = 0;
    unsigned int timeout;
    unsigned long smb_intm = jz_i2c_read_reg(id, SMB_INTM);

    if (len <= 0)
        return 0;

    timeout = timeout_us(id, len) + 1000;

    jz_i2c_dev[id]->tx_buf = buf;
    jz_i2c_dev[id]->len    = len;
    jz_i2c_dev[id]->cmd    = cmd;

    /* 清除STOP TXABT中断 */
    jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
    jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);

    /* 设置TX触发中断的阈值 */
    jz_i2c_set_txfifo_threshold(id, len);

    if (cmd & I2C_START)
        jz_i2c_write_reg(id, SMB_TAR, addr);

    /* 执行第一次写操作 */
    jz_i2c_write_tx_fifo(jz_i2c_dev[id], MAX_FIFO_LEN, 1);

    if (jz_i2c_dev[id]->len == 0)
        jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, 0);

    /* 开中断 */
    smb_intm = set_bit_field(smb_intm, SMBINTM_MTXEMP, 1);
    smb_intm = set_bit_field(smb_intm, SMBINTM_MTXABT, 1);

    if (jz_i2c_dev[id]->cmd & I2C_STOP)
       smb_intm = set_bit_field(smb_intm, SMBINTM_MISTP, 1);

    jz_i2c_write_reg(id, SMB_INTM, smb_intm);


    uint64_t start = systick_get_time_usec();
    while (get_i2c_status(id)) {
        if (systick_get_time_usec() - start > timeout) {
            printf("I2c%d, write timeout\n", id);

            jz_i2c_reset(id);
            ret = -ETIMEDOUT;
            break;
        }
    }

    if (jz_i2c_dev[id]->abtsrc) {

        int src = jz_i2c_dev[id]->abtsrc;

        printf("I2C%d, TXABRT ERR 0x%x\n", id, src);

        if (src & ABRT_7B_ADDR_NOACK || src & ABRT_10B_ADDR_NOACK) {

            if (cmd & I2C_RE_START) {
                printf("I2C :DEVICE 0x%x MAYBE NO SUPPORT RESTART CMD\n", addr);
            }

            ret = -ENXIO;

        } else {
            ret = -EIO;
        }

        jz_i2c_dev[id]->abtsrc = 0;
    }
    return ret;
}

static int jz_i2c_transfer(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit, struct i2c_msg *msg, int count)
{
    int i;
    int cmd;
    int ret = 0;

    jz_i2c_enable(bus_num);

    /**
     * 当count大于1,即一个msg有多次传输时，开始传输时产生一个开始信号，传输结束时产生一个停止信号.
     * 传输过程只考虑需不需要restart信号.
    */
    for (i = 0; i < count; i++) {

        struct i2c_msg *m = msg + i;

        cmd = 0;

        if (i == 0) {
            cmd = I2C_START;/* 第一个msg产生开始信号 I2C_START */
        } else {
            /* 没有I2C_M_NOSTART代表传输过程需要产生restart信号 */
            if (!(m->flags & I2C_M_NOSTART))
                cmd = I2C_RE_START;
        }

        if (i == (count - 1))
            cmd |= I2C_STOP;/* 最后一个msg传输结束产生停止信号 */

        jz_i2c_hal_set_addressing(bus_num, addr_bit);

        if (m->flags & I2C_M_RD) {
            ret = jz_i2c_read(bus_num, addr, m->buf, m->len, cmd);
        } else {
            ret = jz_i2c_write(bus_num, addr, m->buf, m->len, cmd);
        }

        if (ret != 0)
            break;
    }

    jz_i2c_disable(bus_num);

    return ret ? : i;
}

static void i2c_gpio_init(struct jz_i2c_drv *drv)
{
    gpio_set_func(drv->i2c_sda, drv->gpio_func);
    gpio_set_func(drv->i2c_scl, drv->gpio_func);
}

static void jz_i2c_init(int id)
{
    struct jz_i2c_drv *drv = jz_i2c_dev[id];

    i2c_gpio_init(drv);

    i2c_init_setting(drv);
}


/*-----------------------------------------------------------------------------------------------*/

int i2c_transfer(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit, struct i2c_msg *msg, int count)
{
    if (!jz_i2c_dev[bus_num]) {
        printf("I2C%d:not enable\n", bus_num);
        return -1;
    }

    return jz_i2c_transfer(bus_num, addr, addr_bit, msg, count);
}

int i2c_detect_device(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit)
{
    if (!jz_i2c_dev[bus_num]) {
        printf("I2C%d:not enable\n", bus_num);
        return -1;
    }

    return jz_i2c_detect_device(bus_num, addr, addr_bit);
}

void i2c_init(void)
{
    if (jz_i2c_dev[0])
        jz_i2c_init(0);

    if (jz_i2c_dev[1])
        jz_i2c_init(1);

    if (jz_i2c_dev[2])
        jz_i2c_init(2);

    if (jz_i2c_dev[3])
        jz_i2c_init(3);

    if (jz_i2c_dev[4])
        jz_i2c_init(4);

    if (jz_i2c_dev[5])
        jz_i2c_init(5);
}