# 					GPIO使用说明文档

本文档以X2000_darwin为例子，展示linux代码中gpio使用的两种方法：

1， 以模块驱动的方式封装kernel中gpio的API，配合shell命令来很方便的使用gpio，也可以参考shell命令源码直接调用这些以模块驱动方式加载的封装过的API接口。本文一~三章节详述此种方式。

2， 直接使用kernel中的api相关接口。本文第四章节详述此种方式。

## 一 模块驱动方式GPIO配置方法

### 1 根据对应板级,选择对应配置文件

首先确保工程已经编译过,生成.config.in,具体编译方法请查看<Linux工程编译文档>

打开iconfigtool,选择当前目录下的.config.in

![8](img/8.png)

![9](img/9.png)

### 2 根据需要确定好你要用的gpio,并开始如下配置

#### 2.1配置shell命令

进入libhardware2, 选择gpio并勾选gpio shell命令, 为接下来操作gpio做准备

![2](img/2.png)

#### 2.2 配置gpio辅助驱动

进入模块化驱动,选择x2000驱动列表,勾选gpio辅助驱动,将gpio驱动编译进工程

![6](img/6.png)



## 二 GPIO shell 命令详解

### 2.1 shell命令

```c
cmd_gpio set_func <GPIO> <FUNC>
功能：设定指定IO功能
参数：GPIO //IO⼝的名字
     FUNC //IO的功能
example：cmd_gpio set_func PB28 func0
```

```c
cmd_gpio get_func <GPIO>
功能：获取指定IO功能状态
参数：GPIO //IO的名字
example：cmd_gpio get_func PB28
```

```c
cmd_gpio get_value <GPIO>
功能：获取IO电平
参数：GPIO //IO的名字
example：
cmd_gpio get _value PB28
```



### 2.2 具体例子

本例展示运用shell命令2000_darwin板子关灯。由原理图可知,灯光控制引脚为pc09,默认拉高,人为拉低实现关灯,具体案例如下:

```c
cmd_gpio get_func pc09				  //获取pc09的功能与状态
输出结果:
		output1 pull_up				  //此时状态为上拉,输出高电平
```

```c
cmd_gpio get_value pc09				  //获取pc09电平值
输出结果:
		 1			  				  //证明此刻pc09为高电平,板子上led灯为亮
```

```c
cmd_gpio set_func pc09 output0        //将pc09功能设置为输出低电平,观察板子,led灯灭
输出结果:
		无任何输出
```

```c
cmd_gpio get_func pc09				  //再次获取pc09的功能与状态,与之前设置比较
输出结果:
		output0 pull_up				  //此时输出低电平
```

gpio shell源码位置：

```c
libhardware2\src\cmds\gpio_main.c
```



## 三 模块驱动方式GPIO应用接口分析

### 3.1 API详细介绍

**使用gpio接口需要包含头文件:    #include <libhhardware2/gpio.h>**

```c
int gpio_open(void)
功能：获取gpio设备操作句柄
参数：   
    	无
返回值：
    	成功：gpio设备句柄
		失败：负数
```

```c
int gpio_close(int fd)
功能：关闭gpio设备句柄
参数：
    	fd 				//gpio设备句柄,由gpio_open()函数获得
返回值：
    	成功：0
		失败：负数
```

```c
int gpio_set_func(int fd, const char *gpio, char *funcs[], unsigned int func_count)
功能：设置gpio功能
参数：
     	fd 			    //gpio设备句柄,由gpio_open()函数获得
	 	gpio 			//描述对应的gpio引脚，如"PB08"
	 	funcs 			//描述对应的gpio引进功能
	 	func_count      //funcs 数组的长度，即一次设置的功能个数
返回值：  
    	成功：0
    	失败：负数
```

```c
int gpio_get_func(int fd, const char *gpio, char *buf, int buf_size)
功能：获得gpio当前的功能
参数：
    	fb 				//gpio设备句柄,由gpio_open()函数获得
		gpio 			//描述对应的gpio引脚，如"PB08"
		buf 			//缓冲区，用于获取gpio的功能（一个buf存放所有功能）
		buf_size	    //给定的buf的大小
返回值：
    	成功：0
		失败：负数
```

```c
int gpio_get_func2(int fd, const char *gpio, char *buf[], int buf_count, int buf_size)
功能：获得gpio当前的功能，此函数数与 gpio_set_func 相对应
参数：
    	fd 				//gpio设备句柄,由gpio_open()函数获得
		gpio 			//描述对应的gpio引脚，如"PB08"
		buf 			//buf 数组,每个成员作为一个buf获取gpio的一个功能（与gpio_get_func的区别）
		buf_count 		//数组中成员的个数，即buf的个数
		buf_size 		//buf 数组中每个buf的大小,每个buf大小都一样大
返回值：
    	成功：获取到的个数
		失败：负数
```

```c
int gpio_get_value(int fd, const char *gpio)
功能：获得gpio输入的值
参数：
    	fd 				//gpio设备句柄,由gpio_open()函数获得
		gpio			//描述对应的gpio引脚，如"PB08"
返回值：
    	成功：0或1
		失败：负数
```

```c
int gpio_get_help(int fd, char *buf, unsigned int buf_size)
功能：获得gpio的帮助信息
参数：
    	fd 				//gpio设备句柄,由gpio_open()函数获得
		buf 			//用于获取gpio的帮助信息
		buf_size 		//给定的buf的大小
返回值：
    	成功：0或1
		失败：负数
```



### 3.2 具体例子

本例展示运用接口实现x2000_darwin板子关灯。由原理图可知,灯光控制引脚为pc09,默认拉高,人为拉低实现关灯,具体案例如下:

```c
int main(int argc, char *argv[])
{
    int fd, ret;
    char *func[] = {"output0", "pull_up"};                  //要设置的功能名
    char *gpio = "pc09";					   				//要设置的引脚名
    char buf[256];

    fd = gpio_open();						  			  	//打开gpio
    if (fd < 0) {
        fprintf(stderr, "failed to open device\n");
        exit(-1);
    }

    ret = gpio_get_value(fd, gpio);			    			 //得到gpio的电平值
    fprintf(stderr, "the value of %s is %d\n", gpio, ret);

    ret = gpio_set_func(fd, gpio, func, 2);	//设置gpio功能,这里设置为输出0,已达到关灯的目的
    if (ret)
        fprintf(stderr, "failed to set func\n");

    ret = gpio_get_func(fd, gpio, buf, sizeof(buf));//得到gpio功能,可以比较一下是否与设置一致
    if (!ret)
        fprintf(stderr, "%s\n", buf);

    gpio_close(fd);									//关闭gpio
}
```



## 四 在kernel中使用GPIO

### 4.1 kernel中的GPIO相关API详细介绍

#### **4.1.1 申请与释放gpio**

```c
int gpio_request(unsigned gpio, const char *label)
功能：申请gpio
参数：
    	gpio 	//gpio引脚
		label   //描述gpio引脚
返回值：
    	成功：0
		失败：负数
```

```c
void gpio_free(unsigned gpio)
功能：注销gpio
参数：
    	gpio 	//gpio引脚
返回值： 
    	无
```

#### 4.1.2 获取与设置gpio端口电平

```c
int gpio_get_value(unsigned gpio)
功能：获取gpio电平大小
参数：
    	gpio    //gpio引脚
返回值：
    	成功：0
		失败：负数
```

```c
void gpio_set_value(unsigned gpio, int value)
功能：设置gpio电平大小
参数：
    	gpio   //gpio引脚
        value  //引脚电平大小
返回值：
    	无
```

#### **4.1.3 设置gpio端口方向与电平**

```c
int gpio_direction_output(unsigned gpio, int value)
功能：设置gpio输出电平大小
参数：
    	gpio   //gpio引脚
        value  //引脚电平大小
返回值：
    	成功：0
		失败：负数
```

```c
int gpio_direction_input(unsigned gpio)
功能：设置gpio为输入
参数：
    	gpio   //gpio引脚
返回值：
    	成功：0
		失败：负数
```

#### **4.1.4 gpio功能设置**

```c
#include<soc/gpio.h>         //包含此结构体
enum gpio_function {         //gpio功能枚举,提供给gpio功能设置函数使用
    GPIO_FUNC_0    = 0x10,
    GPIO_FUNC_1    = 0x11,
    GPIO_FUNC_2    = 0x12,
    GPIO_FUNC_3    = 0x13,
    GPIO_OUTPUT0   = 0x14,
    GPIO_OUTPUT1   = 0x15,
    GPIO_INPUT     = 0x16,
    GPIO_INT_LO    = 0x18,
    GPIO_INT_HI    = 0x19,
    GPIO_INT_FE    = 0x1a,
    GPIO_INT_RE    = 0x1b,
    GPIO_INT_MASK_LO   = 0x1c,
    GPIO_INT_MASK_HI   = 0x1d,
    GPIO_INT_MASK_FE   = 0x1e,
    GPIO_INT_MASK_RE   = 0x1f,

    GPIO_PULL_HIZ  = 0x80,
    GPIO_PULL_UP   = 0xa0,
    GPIO_PULL_DOWN = 0xc0,
};
```

```c
int jzgpio_set_func(int port, enum gpio_function func, unsigned long pins)
功能：设置gpio功能
参数：
    	port 	//gpio引脚组别
		func    //描述gpio引脚功能
    	pins    //gpio引脚
返回值：
    	成功： 0
		失败：负数
```

#### **4.1.5 gpio当作中断口使用**

```c
int gpio_to_irq(unsigned int gpio)
功能：将gpio端口号转换为相印的中断值
参数：
		gpio    //gpio号
返回值：
    	成功：相应的中断值
		失败：负数
```

```c
int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
    			const char *name, void *dev)
功能：申请中断
参数：
		irq			//中断号,由gpio_to_irq函数获得
    	handler		//中断调用的回调函数
    	frags		//中断类型,决定中断出发方式
    	name		//设备驱动程序名称
    	dev			//中断名称,方便区分中断
返回值：
    	成功: 0
		失败：负数
```

```c
void free_irq(unsigned int irq, void *dev_id)
功能：注销中断
参数：
		irq			//中断号,由gpio_to_irq函数获得
    	dev_id		//中断名称,方便区分中断
返回值：
		无
```



### 4.2 具体例子

以下为在kernel中使用gpio,通过观察led灯光情况,从而理解gpio的使用:

#### **4.2.1 使用gpio拉高拉低**

```c
#include <linux/module.h>
#include <linux/init.h>
#include <soc/gpio.h>       		//其包含将GPIO_PC(9)转换为数字'73'的封装函数
#include <linux/delay.h>			//包含延时函数
#include <linux/gpio.h>				//包含gpio相关API

void gpio_test(void)		//初始情况为开灯,我们将引脚拉低,关灯,等待两秒,在将引脚拉高开灯
{
    int val;
    int test_gpio = GPIO_PC(9);							//led灯控制引脚为pc09
    printk(KERN_ERR "start test\n");   					//开始测试

    gpio_request(test_gpio, "test gpio");    			//申请gpio
    gpio_direction_output(test_gpio, 0); 				//设置gpio为输出,且输出为0,关灯
    msleep(2000);     									//延迟2s,以便观察效果
    
    val = gpio_get_value(test_gpio);		//得到led引脚电平值
    printk(KERN_ERR "the value of %d is %d\n",test_gpio, val);//将电平值打印
    gpio_set_value(test_gpio, 1);			//设置led引脚电平值为1,开灯
    msleep(2000);							//延迟2s,以便观察效果
    
    gpio_set_value(test_gpio, 0); 			//设置gpio为输出,且输出为0,关灯
    gpio_free(test_gpio); 					//释放申请的gpio
}

static int __init vendor_init(void)
{
    printk(KERN_INFO "vendor_init test init\n");
	gpio_test();
    return 0;
}

static void __exit vendor_exit(void)
{
    printk(KERN_INFO "vendor_exit test exit\n");
}

module_init(vendor_init);
module_exit(vendor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhongwan");
MODULE_DESCRIPTION("vendor test");
```

#### **4.2.2 将gpio口做中断口使用:**

当按下boot按键时,进入中断,可以观察到串口打印,表示按键次数的奇偶性

```c
#include <linux/module.h>
#include <linux/init.h>
#include <soc/gpio.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>		//中断相关的头文件
#include <linux/irq.h>				//中断相关的头文件


static int test_gpio = GPIO_PE(31);	//中断触发引脚为PE31,该脚为wake_up按键引脚
struct delayed_work gpio_delay_work;

void gpio_work_func (struct work_struct *p_work)    //按键处理函数
{
    static int k = 0;
    k++;

    if (k%2 != 0)
    printk(KERN_ERR "An odd number of irq\n");		//奇数次中断
    else
    printk(KERN_ERR "An even number of irq\n");		//偶数次中断
}

irqreturn_t test_gpio_handle(int irq, void *dev)
{
    schedule_delayed_work(&gpio_delay_work, msecs_to_jiffies(0));//将按键函数延时执行,消抖处理

    return 0;
}

void gpio_irq_test(void)
{
    char *dev = "gpio";								//程序名称
    char *name = "gpio_driver";						//中断名称,方便区分中断
    int irq;										//irq为中断标号
    printk(KERN_ERR "start test\n");				//开始测试

    irq = gpio_to_irq(test_gpio);					//将gpio端口号转换为相印的中断值
    request_irq(irq, test_gpio_handle, IRQF_TRIGGER_RISING, name, dev);//申请中断
}

static int __init vendor_init(void)
{
    printk(KERN_INFO "vendor_init test init\n");
    INIT_DELAYED_WORK(&gpio_delay_work, gpio_work_func);//初始化工作队列
    gpio_irq_test();
    return 0;
}

static void __exit vendor_exit(void)
{
    printk(KERN_INFO "vendor_exit test exit\n");
}

module_init(vendor_init);
module_exit(vendor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhongwan");
MODULE_DESCRIPTION("vendor test");
```