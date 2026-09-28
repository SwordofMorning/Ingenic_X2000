

# SPI使用及说明文档

## 1.SPI_相关配置

### 1.1.IConfigTool 相关配置

打开IConfigTool并且选择对应板级配置，此处以**x2000**为例进行说明。

<img src="img/1.png" />

进入 [模块化驱动] 界面，点击进入 **[驱动]** 并且勾选配置如下配置：

<img src="img/2.png" />



> 用户需要根据设备使用的是spi控制器驱动还是gpio_spi驱动，选择性地进行下列配置：

#### 1.1.1.SPI控制器驱动配置

若新设备使用的是SPI控制器驱动，逐级进入到 **[SPI总线]** 的配置界面，进行相关配置：

<img src="img/3.png" />

#### 1.1.2.GPIO_SPI驱动配置

若新设备使用的是GPIO_SPI驱动，逐级进入到 **[gpio spi 驱动/设备]** 的配置界面，进行相关配置：

> 当不能正常使用SPI控制器驱动时，请使用SPI控制器驱动所使用到的IO引脚来配置GPIO_SPI驱动，从而验证是硬件出现异常还是SPI控制器驱动出现异常。

<img src="img/4.png" />



### 1.2.Kernel 相关配置

1.切换路径到对应版本内核的目录

```shell
cd ~/your_linux_project/kernel/kernel-x2000
```

2.打开menuconfig，**取消**勾选内核的spidev配置，使用模块的spidev驱动

```shell
make menuconfig
#进入到spidev所在路径,取消勾选spidev配置：
Device Drivers  --->
    [*] SPI support  --->
    < > User mode SPI device driver support
#保存配置后，退出menuconfig
```

<img src="img/5.png" width=80% />



## 2.SPI_设备驱动层使用说明

### Q：如何添加新的SPI设备驱动？

> 本章将先介绍设备驱动层相关接口，再通过示例程序说明相关接口的用法

### 2.1.SPI_驱动层接口说明

```C
/*君正封装的接口*/
struct spi_device *spi_register_device(struct spi_board_info *info, int spi_bus_num)
功能：注册spi设备
参数：info									   //SPI设备板级信息结构体
	 spi_bus_num		  					 //SPI总线号
返回值：注册成功返回的spi_device结构体指针		  //非空：注册成功 空：注册失败

/*内核提供的接口*/
int spi_register_driver(struct spi_driver *sdrv)
功能：注册spi设备驱动
参数：sdrv					//SPI设备驱动结构体
返回值：0：注册成功		<0：注册失败

void spi_unregister_device(struct spi_device *spi)
功能：注销spi设备
参数：spi					//注销指定的spi设备
返回值：空

void spi_unregister_driver(struct spi_driver *sdrv)
功能：注销spi设备驱动
参数：sdrv					//注销指定的spi设备驱动
返回值：空

int spi_write_then_read(struct spi_device *spi,const void *txbuf, unsigned n_tx,
		void *rxbuf, unsigned n_rx)
功能：SPI同步写入数据后读取数据
参数：spi					//要进行数据读写的spi设备
	 txbuf				  //数据发送缓冲区
	 rxbuf				  //数据接收缓冲区
	 n_tx				  //数据发送缓冲区大小
	 n_rx				  //数据接收缓冲区大小
返回值：0：成功	<0：失败
```



### 2.2.SPI_设备驱动实例

#### 2.2.1.编写示例程序

> 示例程序 m_spi_dev.c 将演示如何使用第1.2章介绍的接口来编写spi设备驱动程序

```c
//示例代码 m_spi_dev.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <soc/gpio.h>
#include <linux/spi/spi.h>

#define BUF_LEN 80

static int spi_bus_num = 0;         // 设置spi总线号
static struct spi_device *m_dev;

struct spi_device *spi_register_device(struct spi_board_info *info, int spi_bus_num)
{
    struct spi_master *master = spi_busnum_to_master(spi_bus_num);
    struct spi_device *dev;

    if (!master) {
        printk(KERN_ERR "error:failed to get spi master %d\n",spi_bus_num);
        return NULL;
    }

    info->bus_num = spi_bus_num;
    dev = spi_new_device(master, info);

    if (!dev) {
        printk(KERN_ERR "can not register spi device to %d busnum!",
               spi_bus_num);
    }

    spi_master_put(master);

	return dev;
}

static int m_spi_probe(struct spi_device *pdev)
{
    /* 设备与驱动匹配成功后在此处进行SPI传输测试 */

	int i,ret;
    unsigned char txbuf[BUF_LEN];
	unsigned char rxbuf[BUF_LEN];

    for (i = 0; i < BUF_LEN; i++) {
        txbuf[i] = i;
    }

    ret = spi_write_then_read(pdev,&txbuf[0], BUF_LEN, &rxbuf[0], BUF_LEN);

    if (ret < 0) {
        printk(KERN_ERR "m_new_spi_device:spi_write_then_read failed\n");
        return ret;
    }

    for (i = 0; i < BUF_LEN; i+=8) {
        printk(KERN_ERR "%02x %02x %02x %02x %02x %02x %02x %02x\n",
			rxbuf[i],rxbuf[i+1],rxbuf[i+2],rxbuf[i+3],
               rxbuf[i+4],rxbuf[i+5],rxbuf[i+6],rxbuf[i+7]);
    }

    printk(KERN_ERR "\n%d Bytes spi data transfer finished...\n",BUF_LEN);

    return 0;
}

static int m_spi_remove(struct spi_device *pdev)
{
    printk(KERN_ERR "m_new_spi_device is removed...\n");

    return 0;
}

static struct spi_device_id id_table[] = {
    {
        .name = "m_new_spi_device",
    }
};

//配置SPI设备驱动结构体
static struct spi_driver m_spi_driver = {
    .driver = {
        .name   = "m_new_spi_device",	// 设备驱动名称
        .bus    = &spi_bus_type,		// 驱动总线类型
        .owner  = THIS_MODULE,
    },
    .id_table   = id_table,				// 用于匹配设备与驱动
    .probe      = m_spi_probe,
    .remove     = m_spi_remove,
};

//配置SPI设备板级信息结构体
static struct spi_board_info m_spi_device = {
    .modalias           =   "m_new_spi_device", // SPI设备名称：m_new_spi_device
    .max_speed_hz       =   10 * 1000 * 1000,   // SPI最高速率：10Mhz
    .mode               =   SPI_MODE_3,         // SPI传输模式：模式3

    .controller_data    =   (void *)(GPIO_PC(16)),  // 设置片选引脚为GPIO_PC(16)
                                                    // 若不使用片选，设置为-1

    .chip_select        =   0,              // 没有使用到的片选设置
                                            // 在同一SPI总线上，可能有多个SPI设备
                                            // 该值要保持唯一性，大小不能超过num_chipselect
};

static int __init m_spi_init(void)
{
    // 注册spi设备驱动
    int ret = spi_register_driver(&m_spi_driver);

    if (ret) {
        printk(KERN_ERR "m_new_spi_device: failed to register spi driver\n");
        ret = -1;
        goto err_register_driver;
    }

    // 注册spi设备
    m_dev = spi_register_device(&m_spi_device, spi_bus_num);
    if (m_dev == NULL) {
        printk(KERN_ERR "m_new_spi_device: failed to register spi device\n");
        ret = -1;
        goto err_register_device;
    }

    return 0;

err_register_device:
    spi_unregister_driver(&m_spi_driver);
err_register_driver:

    return ret;
}

static void __exit m_spi_exit(void)
{
    printk(KERN_ERR "spi device driver exit...\n");

    // 注销spi设备
    spi_unregister_device(m_dev);

    // 注销spi设备驱动
    spi_unregister_driver(&m_spi_driver);
}

module_init(m_spi_init);
module_exit(m_spi_exit);

MODULE_DESCRIPTION("Add new spi device driver demo");
MODULE_LICENSE("GPL");
```

#### 2.2.2.如何使用

> 本章中的测试为SPI的自发自收传输实验，在进行测试前确保SSI0_DT(发送)和SSI0_DR(接收)引脚已经用杜邦线进行连接

1.编译编写的新spi设备驱动并使用adb工具下载编译好的驱动到开发板

```shell
adb push m_spi_dev.ko /tmp
```

2.进入tmp目录并且加载新的spi设备驱动

```shell
insmod m_spi_dev.ko
```

3.加载新的spi设备驱动后，缓冲区数据如下图所示，说明spi数据传输成功

<img src="img/6.png" width="65%"/>

4.验证新增spi设备驱动程序能正常加载后，若想添加此设备驱动模块到工程代码中，请参考文档《模块驱动添加流程》



## 3.SPI_应用层使用说明

> 本章将先介绍应用层相关接口，再通过示例程序说明相关接口的用法

### 3.1.SPI_应用层结构体及接口说明

#### 3.1.1.包含头文件

```c
#include <libhardware2/spi.h>
```

#### 3.1.2.SPI_应用层结构体说明

```c
SPI信息结构体配置
struct spi_info {
    unsigned char spi_mode;				//SPI传输模式
    unsigned int spi_speed;				//SPI传输速率
    unsigned char spi_bits;				//SPI传输每个字的位数
    unsigned char spi_lsb;				//SPI传输数据低位的顺序
};
```

```c
SPI数据传输结构体配置
struct spi_ioc_transfer {
	__u64		tx_buf;					//指向用户空间数据发送缓冲区的指针
	__u64		rx_buf;					//指向用户空间数据接收缓冲区的指针

	__u32		len;					//缓冲区长度，单位为字节
	__u32		speed_hz;				//设备比特率

	__u16		delay_usecs;			//传输中最后一位传输后延迟的时间
	__u8		bits_per_word;			//每个字的位数
	__u8		cs_change;				//在开始下一次传输之前取消选择设备
	__u8		tx_nbits;				//定义几线发送(1、2、4线可选) 0默认单线传输
	__u8		rx_nbits;				//定义几线接收(1、2、4线可选) 0默认单线传输
	__u8		word_delay_usecs;		//每传输完毕一个字，开始传输下一个字延迟的时间
	__u8		pad;
};
```



#### 3.1.3.SPI_应用层接口功能说明

> 详细的接口介绍请查阅《libhardware2_API说明文档》
>
> 接口具体实现查阅工程代码目录 libhardware2/src/lib/spi/spi.c

```C
int spi_open(char *spidev_path) //功能：打开指定的spi设备

void spi_close(int spi_fd) //功能：关闭指定的spi设备

int spi_transfer(int spi_fd, struct spi_ioc_transfer *spi_tr, int num) //功能：SPI传输(收发)

int spi_read(int spi_fd, char *rx_buf, int len) //功能：SPI数据接收

int spi_write(int spi_fd, char *tx_buf, int len) //功能：SPI数据发送

int spi_set_mode(int spi_fd, int mode) //功能：设置SPI传输模式

int spi_set_speed(int spi_fd, int speed) //功能：设置SPI传输速率

int spi_set_bits(int spi_fd, int bits_per_word) //功能：设置spi传输时的位数

int spi_set_lsb(int spi_fd, int lsb_first) //功能：设置spi传输时数据的高低位发送顺序

void spi_get_info(int spi_fd, struct spi_info *spi) //功能：获取SPI的配置信息

int spi_add_device(struct spidev_register_data *data) //功能：添加spi设备

int spi_del_device(char *spidev_path) //功能：删除spi设备
```

> 具体的SPI接口使用示例在第3.2.2章



### 3.2.测试方法

> 本章中的测试为SPI的自发自收传输实验，在进行测试前确保SSI0_DT(发送)和SSI0_DR(接收)引脚已经用杜邦线进行连接

#### 3.2.1.使用SPI辅助命令

```shell
#使用cmd_spi add_dev命令进行spi设备的添加
1.cmd_spi add_dev 0 pc16		#0：spi总线号		pc16：片选引脚
/dev/spidev0.0

#使用cmd_spi transfer进行5个字节的数据传输实验
2.cmd_spi transfer /dev/spidev0.0  0x00 0x01 0x02 0x03 0x04
receive[0]: 00
receive[1]: 01
receive[2]: 02
receive[3]: 03
receive[4]: 04
#数据接收缓冲区能正确接收到发送的数据，说明spi数据传输成功

#使用cmd_spi set对spi设备属性设置，并且用cmd_spi info查看是否设置成功
3.cmd_spi set /dev/spidev0.0 mode=0x03 speed=500000 bits=8 lsb=0

4.cmd_spi info /dev/spidev0.0
mode	speed	bits	lsb
03 		500000 	8 		0

#使用cmd_spi del_dev删除指定spi设备
5.cmd_spi del_dev /dev/spidev0.0
```

#### 3.2.2.编写示例程序

> 参照第3.1章的接口说明，编写简单的spi示例程序demo_spi.c进行测试
>
> 编译示例程序，使用adb工具下载到/tmp目录下并运行（应用编译方法参考《客户应用编译说明文档.pdf》）

```C
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libhardware2/spi.h>

#define BUF_LEN		  	240

int main()
{
    int spi_fd = -1;
	char cs_gpio[5] = "pc09";

    /* 1.spi初始化 */
    struct spidev_register_data register_data = {0};
    register_data.cs_gpio = cs_gpio;
    register_data.busnum = 0;

    // 1.1.注册spi设备
	if(spi_add_device(&register_data) < 0) {
        printf("spi register fail\n");
        return -1;
    }

    // 延时等待设备节点生成
    usleep(1000*1000);

    // 1.2.打开spi设备
    spi_fd = spi_open(register_data.spidev_path);
    if (spi_fd < 0) {
        printf("spi open fail,spi_fd=%d\n",spi_fd);
		return -1;
    }

    // 1.3.设置spi传输模式 SPI_MODE_3
    if (spi_set_mode(spi_fd, SPI_MODE_3) < 0) {
        printf("spi set mode fail \n");
        return -1;
    }

    // 1.4.设置spi传输速率
    if (spi_set_speed(spi_fd, 10 * 100000) < 0) {
        printf("spi set speed fail\n");
        return -1;
    }

    // 1.5.设置spi传输的数据位数
    if (spi_set_bits(spi_fd, 8) < 0) {
        printf("spi set bits fail\n");
        return -1;
    }

    // 1.6.设置spi传输数据高低位发送顺序
    if (spi_set_lsb(spi_fd, 0) < 0) {
        printf("spi set lsb fail \n");
        return -1;
    }

    printf("spi init ok...\n");

    /* 2.spi数据传输 */
    unsigned char rx_buf[BUF_LEN] = {0};
    unsigned char tx_buf[BUF_LEN] = {0};

    for (int i = 0; i < BUF_LEN; i++) {
        tx_buf[i] = i;
    }

    // 2.1.配置spi传输结构体
    struct spi_ioc_transfer spi_msg  = {
        .len = BUF_LEN,
        .rx_buf 	= (unsigned long)rx_buf,
        .tx_buf 	= (unsigned long)tx_buf,
        .speed_hz 	= 0,
        .bits_per_word = 8,
    };

    // 2.2.spi收发数据
    if (spi_transfer(spi_fd, &spi_msg, 1) < 0) {
        printf("spi transfer fail\n");
        return -1;
    }

    for (int i = 0; i < BUF_LEN; i++) {
        if (i % 24 == 0) {
            printf("\n");
        }
        printf(" %02x ",rx_buf[i]);
    }

    // 2.3.注销spi设备
    if (spi_del_device(register_data.spidev_path) < 0) {
        printf("\nspi unregister fail\n");
    }

    printf("\nspi unregister ok...\n");

    return 0;
}
```

> 运行 demo_spi.c 程序，缓冲区数据如下图所示，说明spi数据传输成功

<img src="img/7.png" />