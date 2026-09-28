#include <stdio.h>
#include <string.h>

#include <soc/base.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <cpu/host_cpu.h>

#include <driver/gpio.h>
#include <driver/i2c.h>


/*＊
 * 具体设备的写函数,需要根据具体的设备去实现
 * bus_num: i2c 总线号
 * addr: 从设备地址
 * addr_bit: 从设备地址位宽
 * reg: 从设备具体的寄存器地址
 * buf: 发送缓冲区
 * len: 发送数据的个数
 */
static int xx_reg_write(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit, unsigned char reg, char *buf, int len)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .flags = I2C_M_WR,
            .len = 1,
            .buf = &reg,
        },
        [1] = {
            .flags  = I2C_M_WR | I2C_M_NOSTART,
            .len    = len,
            .buf    = buf,
        }
    };

    int ret = i2c_transfer(bus_num, addr, addr_bit, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

/*＊
 * 具体设备的读函数,需要根据具体的设备去实现
 * bus_num: i2c 总线号
 * addr: 从设备地址
 * addr_bit: 从设备地址位宽
 * reg: 从设备具体的寄存器地址
 * buf: 接收缓冲区
 * len: 接收数据的个数
 */
static int xx_reg_read(int bus_num, unsigned short addr, enum i2c_addr_type addr_bit, unsigned char reg, char *buf, int len)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .flags  = I2C_M_WR,
            .len    = 1,
            .buf    = &reg,
        },
        [1] = {
            .flags  = I2C_M_RD,
            .len    = len,
            .buf    = buf,
        }
    };

    int ret = i2c_transfer(bus_num, addr, addr_bit, msg, 2);
    if (ret > 0)
        ret = 0;

    return 0;
}


void i2c_example(void)
{
    char tbuf = 0x8e;
    char rbuf = 1;
    int ret;

    ret = xx_reg_read(2, 0x37, I2C_ADDR_BIT_7, 0xfc, &rbuf, 1); /* 0xfc是i2c外设具体的寄存器地址 */
    if (ret < 0)
        printf("i2c read error %d\n", ret);

    printf("i2c: rx_buf = 0x%x\n", rbuf);

    /* 具体设备的读写函数需要自己实现，这里只给出简单的示例 */
    ret = xx_reg_write(2, 0x37, I2C_ADDR_BIT_7, 0xfc, &tbuf, 1);
    if (ret < 0)
        printf("i2c write error %d\n", ret);

    ret = xx_reg_read(2, 0x37, I2C_ADDR_BIT_7, 0xfc, &rbuf, 1);
    if (ret < 0)
        printf("i2c read error %d\n", ret);

    printf("i2c: rx_buf = 0x%x\n", rbuf);
}
