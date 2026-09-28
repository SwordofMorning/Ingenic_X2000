# I2C使用说明文档



## 1.I2C 配置方法,本文档以x2000_darwin为例子

### 1.1根据对应板级,选择对应配置⽂件

**1.首先确保工程已经编译过,生成.config.in,具体编译方法参考<Linux工程编译文档>**



**2.打开iconfigtool,选择当前目录下的.config.in**

![13](img/13.png)

![14](img/14.png)

### 1.2根据需要确定好你要⽤的i2c,并开始如下配置

**1.进⼊libhardware2, 选择I2C并勾选I2C shell命令, 为接下来操作I2C做准备**

![2](img/2.png)

![3](img/3.png)

**2.进⼊模块化驱动,选择x2000驱动列表,勾选I2控制器驱动,将I2C驱动编译进⼯程**

![4](img/4.png)

**根据需求配置正确的I2C,选择需要的总线号,对应GPIO口及I2C速度,一般为400000**

![5](img/5.png)

**每个总线号可能有不同的GPIO口,根据需要选择**

![6](img/6.png)



### 1.3GPIO模拟I2C使用

**若I2C总线不够用,或者怀疑i2c驱动异常,还可以使用GPIO口模拟I2C,以便检测,具体配置如下:**

![10](img/10.png)

**这里选择使用的引脚和速度:**

![11](img/11.png)



## 2.I2C shell命令详解



### 2.1 shell命令:

```c
cmd_i2c detect <busnum>
功能：探测i2c设备
参数：
    	busnum 		//探测的i2c总线号
example：
		cmd_i2c detect 5
```

```c
cmd_i2c read <busnum> <dev_addr> <size>
功能：从指定的i2c总线下的设备接收数据
参数：
    	busnum 		//指定的i2c总线号
		dev_addr    //指定设备相对应的设备地址
		size	    //接收的⼤⼩
example:
		cmd_i2c read 5 0x5d 8
```

```c
cmd_i2c write <busnum> <dev_addr> <data0> [data...]
功能：往指定的i2c总线下的设备发送数据
参数：
    	busnum 			 //指定的i2c总线号
		dev_addr 		 //指定设备对应的设备地址
		<data0>[data...] //发送的数据（16进制）
example：
		cmd_i2c write 5 0x5d 0xaa 0xbb
```

```c
cmd_i2c read_reg <busnum> <dev_addr> <reg_addr> <size>
功能：从指定的i2c总线下的设备的寄存器地址读取数据（8位寄存器）
参数：
    	busnum 			//指定的i2c总线号
		dev_addr 		//指定设备对应的设备地址
		reg_addr 		//指定设备的寄存器地址
		size 			//读取的⼤⼩
example：
		read_reg 5 0x5d 0x00 8
```

```c
cmd_i2c wrtie_reg <busnum> <dev_addr> <reg_addr> <data0> [data...]
功能：往指定的i2c总线下的设备的寄存器写⼊数据（8位寄存器）
参数：
    	busnum 			//指定的i2c总线号
		dev_addr 		//指定设备对应的设备地址
		reg_addr 		//指定设备对应的寄存器地址
		<data0>[data...]//写⼊的数据（16进制）
example：
		write_reg 5 0x5d 0x00 0xaa 0xbb
```

```c
cmd_i2c read_reg_16 <busnum> <dev_addr> <reg_addr_16> <size>
功能：从指定的i2c总线下的设备的寄存器地址读取数据（16位寄存器）
参数：
    	busnum 			 //指定的i2c总线号
		dev_addr		 //指定设备对应的设备地址
		reg_addr_16 	 //指定设备的寄存器地址
		size 		   	 //读取的⼤⼩
example：
		read_reg_16 5 0x5d 0x3010 2
```

```c
cmd_i2c wrtie_reg_16 <busnum> <dev_addr> <reg_addr_16> <data0> [data...]
功能：往指定的i2c总线下的设备的寄存器写⼊数据（16位寄存器）
参数：
    	busnum 			 //指定的i2c总线号
		dev_addr 		 //指定设备对应的设备地址
		reg_addr_16 	 //指定设备对应的寄存器地址
		<data0>[data...] //写⼊的数据（16进制）
example：
		write_reg_16 5 0x5d 0x3010 0x55 0xaa
```



### 2.2具体例⼦:

```c
cmd_gpio set_func pb24 output1  //这是gtx99的上电流程,必须先上电才可以进行如下操作
```

```c
cmd_i2c detect 5  //探测i2c 5号总线,发现0x5d地址有设备,就是我们的gt9xx
    
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- UU -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
```

```c
cmd_i2c read_reg 5 0x5d 0x80 1				//找一个可读写的寄存器

Dump Recv Buffer:							//读到的值
ff:	
```



## 3.I2C 应用接口分析



### 3.1API详细介绍:

```c
int i2c_open(int bus_num)
功能：打开i2c设备
参数：
    	bus_num 	//i2c总线号
返回值：
    	成功：i2c设备句柄
		失败：负数
```

```c
void i2c_close(int i2c_fd)
功能：关闭I2C设备
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
返回值：
    	无
```

```c
int i2c_read(int i2c_fd, uint16_t device_addr, void *buffer, int size)
功能：i2c接收数据
参数：
    	i2c_fd		 //i2c设备句柄，通过i2c_open获得
		device_addr  //i2c设备地址
		buffer 		 //存放接收数据的buffer
		size 		 //buffer的大小
返回值：
    	成功：0
		失败：负数
```

```c
int i2c_write(int i2c_fd, uint16_t device_addr, void *buffer,int size)
功能：i2c发送数据
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
		device_addr //i2c设备地址
		buffer 		//存放发送数据的buffer
    	size 		//buffer的大小
返回值：
    	成功：0
		失败：负数
```

```c
int i2c_read_reg(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void*buffer, int size)
功能：i2c读寄存器
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
		device_addr //i2c设备地址
		reg_addr 	//寄存器地址(8bit)
		buffer 		//存放读取数据的buffer
		size 		//buffer的大小
返回值：
    	成功：0
		失败：负数
```

```c
int i2c_write_reg(int i2c_fd,uint16_t device_addr, uint8_t reg_addr,void*buffer, int size)
功能：i2c写寄存器
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
		device_addr //i2c设备地址
		reg_addr    //寄存器地址(8bit)
		buffer      //存放写入数据的buffer
		size 		//buffer的
大小
返回值：
    	成功：0
		失败：负数
```

```c
int i2c_read_reg_16(int i2c_fd, uint16_t device_addr, uint8_t reg_addr, void*buffer, int size)
功能：i2c读寄存器
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
		device_addr //i2c设备地址
		reg_addr 	//寄存器地址(16bit)
		buffer 		//存放读取数据的buffer
		size 		//buffer的
大小
返回值：
    	成功：0
		失败：负数
```

```c
int i2c_write_reg_16(int i2c_fd, uint16_t device_addr, uint8_t reg_addr, void *buffer, int size)
功能：i2c写寄存器
参数：
    	i2c_fd 		//i2c设备句柄，通过i2c_open获得
		device_addr //i2c设备地址
		reg_addr 	//寄存器地址(16bit)
		buffer 		//存放写入数据的buffer
		size 		//buffer的大小
返回值：
    	成功：0
		失败：负数
```



### 3.2具体例⼦:

```c
#include <stdio.h>
#include <libhardware2/i2c.h>									//包含i2c相关api
#include <stdint.h>												

int main(void)										//i2c总线读取与写入,寄存器读写与写入例子
{
    int read_size = 4, write_size = 4;
    int bus_num = 5;
    uint16_t device_addr = 0x5d;								//gt9xx设备地址
    uint8_t reg_addr = 0x80;									//可读写寄存器地址

    uint8_t buffer_read[read_size];

    int fd = i2c_open(bus_num);									//打开总线设备

    printf("read reg :       ");
    i2c_read_reg(fd, device_addr, reg_addr, buffer_read, read_size);	//读取寄存器数据
    i2c_read_print(buffer_read, read_size);

    i2c_close(fd);												//关闭总线设备

    return 0;
}
```





## 4.在Kernel中使用I2C 



### 4.1kernel中的I2C相关API详细介绍:

```c
struct i2c_client *i2c_register_device(struct i2c_board_info *info, int i2c_bus_num)
功能:注册设备
参数:
	info        //设备信息结构体
    i2c_bus_num	//i2c总线号
```

```c
int i2c_register_driver(struct module *owner, struct i2c_driver *driver)
功能:注册驱动
参数:
	owner	//模块所有者,一般为 THIS_MODULE
    driver  //驱动配置结构体
```

```c
void i2c_unregister_device(struct i2c_client *client)
功能:注销设备
参数:
	client	//i2c设备,由i2c_register_device获得
```

```c
void i2c_del_driver(struct i2c_driver *driver)
功能:注销驱动
参数:
	driver	//驱动配置结构体,由i2c_register_driver获得
```

```c
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
功能:传输核心函数
参数:
	adap   //总线句柄
    msgs   //消息结构体
    num    //要执行的消息数
```



### 4.2具体例⼦:

```c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <soc/gpio.h>
#include <linux/delay.h>

#define GT_DEVICE_NAME "gtx99_test"					//设备名
#define GT_DEVICE_ADDR 0x5d					    	//设备地址,用于总线找设备

static int i2c_bus_num   = 5;						//所用总线号
static struct i2c_client *i2c_dev;					//i2c设备
static int power_gpio    = GPIO_PB(24);     		//gtx99电源管脚

unsigned char read_value;
unsigned char buf_reg_addr = 0x80;					//从0x80读数据

static int gt_test_probe(struct i2c_client *client, const struct i2c_device_id *id)		//匹配函数
{
    struct i2c_msg read_msg[2] = {					//读数据结构体
        [0] = {
            .addr  = client->addr,
            .flags = 0,
            .len   = 1,
            .buf   = &buf_reg_addr,
        },
        [1] = {
            .addr  = client->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = &read_value,
        }
    };

    printk(KERN_ERR "start probe the test gtx99\n");

    gpio_request(power_gpio, "gt9xx");

    gpio_direction_output(power_gpio, 1);
    msleep(10);

    i2c_transfer(client->adapter, read_msg, 2);
    printk(KERN_ERR "read buff : %x\n", read_value);

    return 0;
}


static int gt_test_remove(struct i2c_client *client)		//模块移除函数
{
    printk(KERN_ERR "gtx99 is removed\n");
    return 0;
}

static const struct i2c_device_id gt_test_id[] = {
    { GT_DEVICE_NAME, 0},
};

static struct i2c_driver gt_test_driver = {
    .probe      = gt_test_probe,
    .remove     = gt_test_remove,
    .id_table   = gt_test_id,
    .driver = {
        .name   = GT_DEVICE_NAME,
        .owner  = THIS_MODULE,
    }
};

static struct i2c_board_info gt_test_info = {
    .type = GT_DEVICE_NAME,
    .addr = GT_DEVICE_ADDR,
};

struct i2c_client *i2c_register_device(struct i2c_board_info *info, int i2c_bus_num)		//注册设备函数定义
{
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    adapter = i2c_get_adapter(i2c_bus_num);
    if (!adapter) {
        printk(KERN_ERR "error: failed to get i2c adapter %d\n", i2c_bus_num);
        return NULL;
    }

    client = i2c_new_device(adapter, info);

    i2c_put_adapter(adapter);

    return client;
}

static int __init test_init(void)
{
    int ret;
    printk(KERN_INFO "i2c_init test init\n");

    ret = i2c_register_driver(THIS_MODULE, &gt_test_driver);//注册驱动
    if (ret) {
        printk(KERN_ERR "failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&gt_test_info, i2c_bus_num);//注册设备
    if (!i2c_dev) {
        printk(KERN_ERR "failed to register i2c device\n");
        i2c_del_driver(&gt_test_driver);
        return -EINVAL;
    }

    return 0;
}

static void __exit test_exit(void)
{
    printk(KERN_INFO "i2c_exit test exit\n");

    i2c_unregister_device(i2c_dev);     //注销设备
    i2c_del_driver(&gt_test_driver);	//注销驱动
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhongwan");
MODULE_DESCRIPTION("i2c test");
```

**将代码编译并运行,观察结果,具体编译流程参考<<kernel模块添加手册>>**

**运行结果:**

![12](img/12.png)