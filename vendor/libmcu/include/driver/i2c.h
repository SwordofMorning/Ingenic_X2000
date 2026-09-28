#ifndef _I2C_H_
#define _I2C_H_

#include <list.h>

enum i2c_addr_type {
    I2C_ADDR_BIT_7,
    I2C_ADDR_BIT_10,
};

#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */

/* 传输标识 */
#define I2C_M_WR        0x0
#define I2C_M_TEN       0x0010
#define I2C_M_RD        0x0001
#define I2C_M_NOSTART   0x4000
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */

struct i2c_msg {
    int len;        /* 数据传输的个数 */
    void *buf;      /* 数据缓冲区 */
    /**
     * flags 传输标识.有以下选择：
     * I2C_M_TEN : 选择 10bit 地址
     * I2C_M_RD  : 发送读操作命令
     * I2C_M_NOSTART ： 传输过程不产生开始信号
     * NOTE： 以上传输标识可组合使用,具体用法可参考i2c_example.c
     */
    int flags;
};


void i2c_init(void);

int i2c_detect_device(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit);
int i2c_transfer(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit, struct i2c_msg *msg, int count);

#endif