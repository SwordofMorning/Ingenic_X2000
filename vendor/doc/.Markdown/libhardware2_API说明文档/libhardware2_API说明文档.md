# libhardware2_API说明文档

## 1.链接libhardware2.so动态库编译说明

1.  若自主添加的应用程序需要用到libhardware2.so的接口，可在代码前面include相应的模块如：

    ```c
    #include <libhardware2/adc.h>
    ```

2.  编译必须使用buildroot生成的工具链，其目录在："工程路径/buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc"：例如

```shell
/buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc test.c -lhardware2 -o test
#-l  指定库的名字，libhardware2.so对应名字为：hardware2
```

test.c代码如下：

```c
#include <stdio.h>
#include <libhardware2/adc.h>

int main(void)
{
    printf("test start!!!!\n");
    adc_enable(0);
    printf("test stop!!\n");

    return 0;
}
```

## 2. GPIO

包含头文件：

>   #include <libhhardware2/gpio.h>

gpio使用流程：

>   1.  gpio_open                                                       获取gpio设备操作句柄
>   2.  gpio_set_func/gpio_get_func...             gpio的操作

api详解：

```c
int gpio_open(void)
功能：获取gpio设备操作句柄
参数：无
返回值：成功：gpio设备句柄
                  失败：负数
```

```c
int gpio_close(int fd)
功能：关闭gpio设备句柄
参数：fd                                //gpio设备句柄,由gpio_open()函数获得
返回值：成功：0
                  失败：负数
```

```c
int gpio_set_func(int fd, const char *gpio, char *funcs[], unsigned int func_count)
功能：设置gpio功能
参数：fd                               //gpio设备句柄,由gpio_open()函数获得
              gpio                         //描述对应的gpio引脚，如"PB08"
              funcs                       /*描述对应的gpio引进功能，如char *func[] = {"input", "pull_up"}。
                                                   通过gpio_get_help获取相关信息*/
              func_count          //funcs 数组的长度，即一次设置的功能个数
返回值：成功：0
                  失败：负数

//char funcs[]的值可参考以下内容：
/*common_func:
        func0: device func0
        func1: device func0
        func2: device func0
        func3: device func0
        output0: output 0
        output1: output 1
        input: input mode
        int_lo: interrupt by low level
        int_hi: interrupt by high level
        int_fe: interrupt by falling edge
        int_re: interrupt by rising edge
        int_lo_m: interrupt by low level, interrupt is masked
        int_hi_m: interrupt by high level, interrupt is masked
        int_fe_m: interrupt by falling edge, interrupt is masked
        int_re_m: interrupt by rising edge, interrupt is masked
 pull_func:
        pull_hiz: no pull, keep hiz
        pull_up: pull up
        pull_down: pull down
*/
```

```c
int gpio_get_func(int fd, const char *gpio, char *buf, int buf_size)
功能：获得gpio当前的功能
参数：fb                              //gpio设备句柄,由gpio_open()函数获得
              gpio                        //描述对应的gpio引脚，如"PB08"
              buf                          //缓冲区，用于获取gpio的功能（一个buf存放所有功能）
              buf_size               //给定的buf的大小
返回值：成功：0
                  失败：负数
```

```c
int gpio_get_func2(int fd, const char *gpio, char *buf[], int buf_count, int buf_size)
功能：获得gpio当前的功能，此函数数与 gpio_set_func 相对应
参数：fd                            //gpio设备句柄,由gpio_open()函数获得
              gpio                      //描述对应的gpio引脚，如"PB08"
              buf                        //buf 数组,每个成员作为一个buf获取gpio的一个功能（与gpio_get_func的区别）
              buf_count         //数组中成员的个数，即buf的个数
              buf_size             //buf 数组中每个buf的大小,每个buf大小都一样大
返回值：成功：获取到的个数
                  失败：负数
```

注意：gpio_get_func与gpio_get_func2，一般用gpio_get_func

```c
int gpio_get_value(int fd, const char *gpio)
功能：获得gpio输入的值
参数：fd                             //gpio设备句柄,由gpio_open()函数获得
              gpio                       //描述对应的gpio引脚，如"PB08"
返回值：成功：0或1
                  失败：负数
```

```c
int gpio_get_help(int fd, char *buf, unsigned int buf_size)
功能：获得gpio的帮助信息
参数：fd                             //gpio设备句柄,由gpio_open()函数获得
              buf                         //用于获取gpio的帮助信息
              buf_size              //给定的buf的大小
返回值：成功：0或1
                  失败：负数
```

## 3. SPI

包含头文件：

>   #include <libhardware/spi.h>

使用流程：

>   1.  spi_add_device                                            添加spi设备
>   2.  spi_open                                                         获取spi设备句柄
>   3.  spi_set_mode/spi_set_bits/....              设置spi传输信息
>   4.  传输

api详解：

```c
int spi_open(char *spi_dev_path)
功能：获取spi设备句柄
参数：spi_dev_path                //spi设备路径,如/dev/spidev0.0
返回值：成功：spi设备句柄
                  失败：-1
```

```c
void spi_close(int spi_fd)
功能：关闭spi设备
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
返回值：无
```

```c
int spi_transfer(int spi_fd, struct spi_ioc_transfer *spi_tr, int num)
功能：spi设备数据传输（同时发送和接收）
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              spi_tr                                 //spi设备传输需要配置的结构体，详见中间参数详解
              num                                   //传输结构体的个数
返回值：成功：0
                  失败：-1
```

```c
int spi_write(int spi_fd, char *tx_buf, int len)
功能：spi设备数据传输（只发送不接收）
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              tx_buf                               //要发送数据的缓冲区
              len                                      //发送数据的长度（单位：字节）
返回值：成功：0
                  失败：-1
```

```c
int spi_read(int spi_fd, char *rx_buf, int len)
功能：spi设备数据传输（只接收不发送）
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              rx_buf                                //接收数据的缓冲区
              len                                       //接收的长度（单位：字节）
返回值：成功：0
                  失败：-1
```

```c
int spi_set_mode(int spi_fd, int mode)
功能：设置spi传输时的模式
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              mode                                 //传输时的模式
返回值：成功：0
                  失败：-1
```

```c
int spi_set_speed(int spi_fd, int speed)
功能：设置spi传输时的模式
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              speed                                 //传输时的速度
返回值：成功：0
                  失败：-1
```

```c
int spi_set_bits(int spi_fd, int bits_per_word)
功能：设置spi传输时的位数
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              bits_per_word               //传输时的位数
返回值：成功：0
                  失败：-1
```

```c
int spi_set_lsb(int spi_fd, int lsb_first)
功能：设置spi传输时数据的高低位发送顺序
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              lsb_first                            //1表示低位在前，0表示高位在前
返回值：成功：0
                  失败：-1
```

```c
void spi_get_info(int spi_fd, struct spi_info *spi)
功能：获取spi传输的配置信息
参数：spi_fd                                 //spi设备句柄，通过spi_open获得
              spi                                       //存放信息的结构体，详见中间参数详解
返回值：成功：0
                  失败：-1
```

```c
int spi_add_device(struct spidev_register_data *data)
功能：添加一个spi设备
参数：data                                      //注册添加一个spi设备需要准备的结构体，详见中间参数详解
返回值：成功：0
                  失败：-1
```

```c
int spi_del_device(char *spidev_path)
功能：删除一个spi设备
参数：spidev_path                       //设备节点的路径,如：/dev/spidev0.0
返回值：成功：0
                  失败：-1
```

中间参数详解：

```c
/*linux kernel定义的spi传输结构体*/
struct spi_ioc_transfer {
          __u64    tx_buf;               //unsigned long型，写数据缓冲区
          __u64    rx_buf;              //unsigned long型，读数据缓冲区
          __u32    len;                     //unsigned int 型，读写长度
          __u32    speed_hz;       //unsigned int 型，通信时的时钟频率

          __u16    delay_usecs;        //unsigned short型，发生在一个spi_ioc_transfer之后的修正延时
          __u8    bits_per_word;      //char型，字长（比特数）
          __u8    cs_change;              /*char型，该transfer传输完毕后是否要非使能cs引脚。
                                                                通常在该设备的最后一个transfer置位*/
          __u8    tx_nbits;                   //定义几线发送（1、2、4线可选）。0默认单线传输
          __u8    rx_nbits;                   //定义几线接收（1、2、 4线可选）。0默认单线传输
          __u16    pad;
};
```

```c
/*用来存放spi信息结构体*/
struct spi_info {
                  unsigned char spi_mode;                           //spi传输模式
                  unsigned int spi_speed;                            //spi传输速度
                  unsigned char spi_bits;                              //spi传输位数
                  unsigned char spi_lsb;                                //spi传输时数据的高低位发送顺序
};

/*spi_mode在linux kernel内每一位的定义如下：*/
#define SPI_CPHA    0x01            /* 第0位，采样的相位，0为第一个边沿，1为第二个边沿 */
#define SPI_CPOL    0x02            /* 第1位，spi空闲时时钟的相位，0为低电平，1为高电平 */
#define SPI_CS_HIGH 0x04            /* 第2位，置1片选的有效信号是高电平*/
#define SPI_LSB_FIRST   0x08            /*第3位，置1发送时低比特在前*/
#define SPI_3WIRE   0x10            /* 第4位，输入输出信号使用同一根信号线 */
#define SPI_LOOP    0x20            /* 第5位，回环模式  */
#define SPI_NO_CS   0x40            /* 第6位，单个设备占用一个spi总线，无片选，spi传输可只用3根线 */
#define SPI_READY   0x80            /* 第7位，spi传输多一根控制线，从机拉低停止传输 */
```

```c
/*注册添加一个spi设备准备的结构体*/
struct spidev_register_data {
                   int busnum;                               //要挂载的spi总线号
                   char *cs_gpio;                          //spi设备使用的片选引脚
                   char spidev_path[20];          //不需要用户配置，用来存放注册成功后，spi设备节点的路径
};
```

## 4. I2C

包含头文件：

>   #include <libhardware/I2c.h>

i2c使用流程：

>   1.  i2c_open                                        获取i2c设备句柄
>   2.  传输（多个功能接口，详见api详解）

api详解：

```c
int i2c_open(int bus_num)
功能：打开i2c设备
参数：bus_num                                           //i2c总线号
返回值：成功：i2c设备句柄
                  失败：负数
```

```c
void i2c_close(int i2c_fd)
功能：关闭I2C设备
参数：i2c_fd                                                   //i2c设备句柄，通过i2c_open获得
返回值：无
```

```c
int i2c_read(int i2c_fd, uint16_t device_addr, void *buffer, int size)
功能：i2c接收数据
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             buffer                                                  //存放接收数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

```c
i2c_write_reg_16int i2c_write(int i2c_fd, uint16_t device_addr, void *buffer, int size)
功能：i2c发送数据
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             buffer                                                  //存放发送数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

```c
int i2c_read_reg(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
功能：i2c读寄存器
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             reg_addr                                           //寄存器地址(8bit)
             buffer                                                  //存放读取数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

```c
int i2c_write_reg(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
功能：i2c写寄存器
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             reg_addr                                           //寄存器地址(8bit)
             buffer                                                  //存放写入数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

```c
int i2c_read_reg_16(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
功能：i2c读寄存器
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             reg_addr                                           //寄存器地址(16bit)
             buffer                                                  //存放读取数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

```c
int i2c_write_reg_16i2c_write_reg_16(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
功能：i2c写寄存器
参数：i2c_fd                                                  //i2c设备句柄，通过i2c_open获得
             device_addr                                     //i2c设备地址
             reg_addr                                           //寄存器地址(16bit)
             buffer                                                  //存放写入数据的buffer
             size                                                      //buffer的大小
返回值：成功：0
                  失败：负数
```

## 5. ADC

包含头文件：

>   #include <libhardware2/adc.h>

adc使用流程：

>   1.  adc_enable                                                使能指定的adc通道，获取adc设备句柄
>   2.  adc_set_vref                                             设置基准电压
>   3.  adc_get_value/adc_get_voltage     获取adc原始值或电压值

api详解：

```c
long adc_enable(unsigned int channle_id)
功能：使能指定的adc通道
参数：channle_id                 //adc的通道号
返回值：成功：ADC操作句柄
                  失败：-1
```

```c
void adc_disable(long handle)
功能：失能对应的adc通道
参数：handle                          //adc操作句柄
返回值：无
```

```c
int adc_set_vref(long handle, unsigned int vref)
功能：设置 adc基准电压
参数：handle                          //adc操作句柄
              vref                                //adc基准电压
返回值：成功：0
                  失败：负数
```

```c
int adc_get_vref(long handle)
功能：获取 adc 基准电压
参数：handle                           //adc操作句柄
返回值：成功：adc基准电压
                  失败：负数
```

```c
int adc_get_value(long handle)
功能：获取adc通道采样后的原始值
参数：handle                           //adc操作句柄
返回值：成功：adc原始值
                  失败：负数
```

```c
int adc_get_voltage(long handle)
功能：获取adc通道采样后的电压值，由采样后的原始值和基准电压计算得到
参数：handle                            //adc操作句柄
返回值：成功：电压值
                  失败：负数
```

## 6. PWM

包含头文件：

>   #include <libhardware2/pwm.h>

pwm使用流程：

>   1.  pwm_request                    申请指定 PWM 通道
>   2.  pwm_config                       设置 pwm 参数
>   3.  pwm_set_level                  设置 PWM 调制的级数，并输出PWM

api详解：

```c
long pwm_request(const char *gpio_name)
功能：申请指定 PWM 通道
参数：gpio_name                                      //pwm对应的gpio引脚，如，"pb17"、"pc11"
返回值：成功：pwm句柄
                  失败：-1
```

```c
void pwm_release(long handle);
功能：释放指定的 PWM 通道
参数：handle                                                //pwm句柄，由pwm_request()获取
返回值：无
```

```c
int pwm_config(long handle, struct pwm_config_data *cfg);
功能：设置 pwm 参数，参数由 pwm_config_data 结构体传入
参数：handle                                                //pwm句柄，由pwm_request()获取
              cfg                                                       //存放设置pwm参数的信息结构体，详见中间参数详解
返回值：成功：0
                  失败：负数
```

```c
int pwm_set_level(long handle, unsigned int level)
功能：设置 PWM 调制的级数，并输出PWM
参数：handle                                                //pwm句柄，由pwm_request()获取
             level                                                     //pwm调制的级数,即一个周期内非空闲电平的长度
返回值：成功：0
                  失败：负数
```

中间参数详解

```c
/*设置pwm参数的信息结构体*/
struct pwm_config_data {
                       enum pwm_shutdown_mode shutdown_mode;                              //设置pwm结束时的模式
                       enum pwm_idle_level idle_level;                                                            //设置pwm空闲时的电平
                       enum pwm_accuracy_priority accuracy_priority;                           //设置pwm的精度优先
                       unsigned long freq;                                                                                        //设置pwm的频率
                       unsigned long levels;                                                                                     //设置最大级数
                       unsigned int id;                                                                                                //申请pwm成功后的id号
};
```

```c
/*设置pwm结束时的模式枚举*/
enum pwm_shutdown_mode {
    /**
     * pwm停止输出时,尽量保证pwm的信号结尾是一个完整的周期
     */
    PWM_graceful_shutdown,
    /**
     * pwm停止输出时,立刻将pwm设置成空闲时电平
     */
    PWM_abrupt_shutdown,
};
```

```c
/*设置pwm空闲时的电平枚举*/
enum pwm_idle_level {
              /*pwm 空闲时电平为低*/
              PWM_idle_low,

              /* pwm 空闲时电平为高 */
              PWM_idle_high,
};
```

```c
/*设置pwm的精度优先*/
enum pwm_accuracy_priority {
              /* 优先满足pwm的目标频率的精度,级数可能不准确*/
              PWM_accuracy_freq_first,

              /* 优先满足pwm的级数设置,pwm频率可能不准确 */
              PWM_accuracy_levels_first,
};
```

## 7.EFUSE

包含头文件：

>   #include <libhardware2/efuse.h>

efuse使用流程：

>   1.  efuse_get_segment_information                   获取efuse每个段的信息
>   2.  efuse_write/read                                                    efuse读或写
>   3.  efuse_free_information                                       释放空间

api详解：

```c
int efuse_write(char *name, unsigned char *wr_buf, int start, int size)
功能：写efuse
参数：name                                       //efuse段的名字
             wr_buf                                    //写入的数据
             start                                         //写入的起始位
             size                                          //写入数据的大小
 返回值：成功：0
                   失败：-1
```

```c
int efuse_read(char *name, unsigned char *buf, int start, int size)
功能：读efuse
参数：name                                       //efuse段的名字
              buf                                           //读取数据存放的位置
              start                                        //读取的起始位
              size                                         //读取数据的大小
 返回值：成功：0
                   失败：-1
```

```c
int efuse_read_seg_size(char *name)
功能：读efuse某一段的大小
参数：name                                       //efuse段的名字
返回值：成功：0
                  失败：-1
```

```c
struct efuse_segment_info *efuse_get_segment_information(void)
功能：获取efuse每个段的信息
参数：无
返回值：成功：存放efuse每个段信息的结构体数组指针
                            （数组最后一个结构体的segment_name成员为NULL）
                               结构体成员详见中间参数详解
                  失败：NULL
```

```c
void efuse_free_information(struct efuse_segment_info *info)
功能：释放存放efuse每个段信息时所申请的空间
参数：info                                         //存放efuse每个段信息的结构体数组指针(结构体成员详见中间参数详解)
返回值：无
```

中间参数详解

```c
/*efuse段信息结构体*/
struct efuse_segment_info {
                unsigned int seg_start;                          //段的起始位
                unsigned int seg_size;                           //段的大小
                char *segment_name;                          //段的名字
};
```

## 8.WATCHDOG

包含头文件：

>   #include <libhardware2/watchdog.h>

watchdog使用流程：

>   1.  watchdog_start                            启动看门狗
>   2.  watchdog_feed                             喂狗
>   3.  watchdog_stop                            关闭看门狗

api详解：

```c
int watchdog_start(unsigned long ms)
功能：启动看门狗，同时设置最迟喂狗时间
参数：ms                                                      //最迟喂狗时间 单位 ms
返回值：成功：0
                  失败：负数
```

```c
int watchdog_stop(void)
功能：停止看门狗计数
参数：无
返回值：成功：0
                  失败：负数
```

```c
int watchdog_feed(void)
功能：喂狗
参数：无
返回值：成功：0
                  失败：负数
```

```c
int watchdog_reset(void)
功能：cpu复位重启
参数：无
返回值：成功：0
                  失败：负数
```



## 9. DTRNG

包含头文件：

>   #include <libhardware2/dtrng>

api详解：

```c
int dtrng_get_random_number(unsigned int *value)
功能：获取DTRNG生成的随机数
参数：value                                                  //获取随机数的值
返回值：成功：0
                  失败：负数
```

## 10. FrameBuffer

包含头文件：

>   #include <libhardware2/fb.h>

framebuffer使用流程：

>   1.  fb_open                     获取fb设备句柄
>   2.  fb_enable                  使能fb设备，上电
>   3.  fb_pan_display       显示/刷新fb指定的帧

api详解：

```c
int fb_open(const char *dev_path, struct fb_device_info *info)
功能：打开fb设备驱动，获取fb操作句柄
参数：const char *dev_path                            //fb设备路径  如：/dev/fb0
              struct fb_device_info *info               //需要初始化的info结构体，详见中间参数详解
返回值：成功：fb操作句柄
                  失败：-1
```

```c
int fb_close(int fd, struct fb_device_info *info)
功能：关闭fb设备句柄，释放内存资源
参数：int fd                                                            //fb设备操作句柄
              struct fb_device_info *info               //需要被释放的fb info结构体，详见中间参数详解
返回值：成功：0
                  失败：负数
```

```c
int fb_enable(int fd);
功能：使能fb设备，上电
参数：int fd                                                            //fb设备操作句柄
返回值：成功：0
                  失败：负数
```

```c
int fb_disable(int fd)
功能：关闭fb设备，掉电
参数：int fd                                                           //fb设备操作句柄
返回值：成功：0
                  失败：负数
```

```c
int fb_pan_display(int fd, struct fb_device_info *info, unsigned int frame_index);
功能：显示/刷新fb指定的帧
参数：int fd                                                           //fb设备操作句柄
             struct fb_device_info *info               //fb设备的相关信息的结构体，详见中间参数详解
             unsigned int frame_index                 //要显示帧的序号
返回值：成功：0
                  失败：负数
```

x1830 、x2000提供layer叠加的功能,以及NV12格式的显示，以下接口仅x1830、x2000支持：

```c
int fb_pan_display_enable_user_cfg(int fd);
功能：pan display时选择用户的配置
参数：int fd                                                           //fb设备操作句柄
返回值：成功：0
                  失败：负数
```

```c
int fb_pan_display_disable_user_cfg(int fd);
功能：pan display 时选择驱动的配置
参数：int fd                                                          //fb设备操作句柄
返回值：成功：0
                  失败：负数
```

```c
int fb_pan_display_disable_set_cfg(int fd, struct lcdc_layer *cfg);
功能：设置用户的配置
参数：int fd                                                         //fb设备操作句柄
             struct lcdc_layer *cfg                         //lcd控制器layer的设置，详见中间参数详解
返回值：成功：0
                  失败：负数
```

中间参数详解：

```c
/*fb设备信息结构体*/
struct fb_device_info {
    unsigned int xres;                            //水平像素个数
    unsigned int yres;                            //垂直像素个数
    unsigned int line_length;             //一行所占字节大小
    unsigned int bits_per_pixel;       //一个像素所占的位数
    unsigned int frame_size;              //一帧大小（单位：字节）
    unsigned int frame_nums;          //帧个数
    void *mapped_mem;                     //映射的地址

    struct fb_fix_screeninfo fix;         //记录用户不可以修改的控制器的参数
    struct fb_var_screeninfo var;      //记录用户可以修改的控制器的参数
};
```

```c
/*fb数据格式*/
enum fb_fmt {
    fb_fmt_RGB555,                              //RGB555格式
    fb_fmt_RGB565,                              //RGB565格式
    fb_fmt_RGB888,                              //RGB888格式
    fb_fmt_ARGB8888,                         //ARGB888格式
    fb_fmt_NV12,                                    //NV12格式
    fb_fmt_NV21,                                    //NV21格式
};
```

```c
/*x1830图层输出层级枚举*/
enum lcdc_layer_order {
    lcdc_layer_top,                                 //输出到顶层
    lcdc_layer_bottom,                        //输出到底层
};
```

```c
/*x1830 lcd 控制器的设置结构体*/
struct lcdc_layer {

    enum fb_fmt fb_fmt;                      /*layer 的数据格式。
                                                                       内部会使用硬件模块转换成LCD实际需要的数据格式*/

    unsigned int xres;                              //layter的宽度
    unsigned int yres;                              // layer的长度

    unsigned int xpos;                              //layer在屏幕上的水平偏移
    unsigned int ypos;                              // layer 在屏幕上的垂直偏移

    enum lcdc_layer_order layer_order;              //layer 的所在的层级

    int layer_enable;                         //是否使能当前 layer

    struct {
        void *mem;                              //rgb数据所在内存地址
        unsigned int stride;              //一行的大小，单位： 字节
    } rgb;                                                //rgb 格式时使用


    struct {
        void *mem;                              //y数据所在内存地址
        unsigned int stride;              // 一行的大小，单位： 字节
    } y;                                                    // NV12,NV21 格式时使用

    struct {
        void *mem;                                 //uv数据所在内存地址
        unsigned int stride;                // 一行的大小，单位： 字节
    } uv;                                                   // NV12,NV21 格式时使用

    struct {
        unsigned char enable;         //透明度使能标志位
        unsigned char value;            //图层透明度数值(0~255)
    } alpha;                                           //设置透明度，enable为1时有效
};
```

## 11. MSCALER

包含头文件：

>   #include <libhardware2/mscaler.h>

mscaler使用流程：

>   1.  mscaler_open                       获取mscaler设备句柄
>   2.  mscaler_alloc_mem           申请空间
>   3.  mscaler_convert                   图像缩放

api详解：

```c
int mscaler_open(struct mscaler_device_info *info)
功能：获得mscaler设备句柄
参数：info                        /*需要初始化的mscaler信息结构体,不能为NULL。
                                                 如果成功,则stride_align，frame_align被赋值，结构体详见中间参数详解*/
返回值：成功：mscaler设备句柄
                  失败：-1
```

```c
int mscaler_close(int fd)
功能：关闭mscaler设备句柄
参数：fd                                                 //mscaler设备句柄,由mscaler_open()函数获得
返回值：成功：0
                  失败：-1
```

```c
int mscaler_alloc_mem(int fd, struct mscaler_device_info *info, int mem_size)
功能：由mscaler申请需要的空间大小（如果图像数据有自己的内存空间，无需申请）
参数：fd                              //mscaler设备句柄,由mscaler_open()函数获得
              info                         /*mscaler的信息,如果内存申请成功
                                                 则phys_addr，mem_align_size，mapped_mem被赋值*/
              mem_size           //需要申请内存空间的大小
返回值：成功：0
                  失败：-1

```

```c
int mscaler_free_mem(int fd, struct mscaler_device_info *info)
功能：释放mscaler_alloc_mem申请的内存空间
参数：fd                                                 //mscaler设备句柄,由mscaler_open()函数获得
             info                                             //mscaler的信息,释放指定的内存空间
返回值：成功：0
                  失败：-1
```

```c
int mscaler_convert(int fd, struct mscaler_frame *src, struct mscaler_frame *dst)
功能：图像缩放和格式转换
参数：fd                                                 //mscaler设备句柄,由mscaler_open()函数获得
             src                                               //图像数据源参数，结构体详见中间参数详解
             dst                                               //图像数据目标参数，结构体详见中间参数详解
返回值：成功：0
                  失败：-1
```

中间参数详解：

```c
/*mscaler信息结构体*/
struct mscaler_device_info
{
              /* 一行数据需要对齐的大小，单位： 字节 */
              unsigned int stride_align;

              /* 一帧数据需要对齐的大小，单位： 字节 */
              unsigned int frame_align;

              /* 申请的物理内存地址 */
              unsigned long phys_addr;

              /* 申请的内存空间对齐之后的大小 */
              unsigned int mem_align_size;

              /* 申请的用户空间内存地址 */
              void *mapped_mem;
};
```

```c
/*图像数据参数结构体*/
struct mscaler_frame
{
              /* 输入图像格式只支持 nv12/nv21 输出图像格式全支持mscaler_fmt */
              enum mscaler_fmt            fmt;

              /* 图像宽 */
              unsigned int                xres;

              /* 图像高 */
              unsigned int                yres;

              /* mscaler_fmt全格式使用，rgb/rgba等格式只需要使用y分量即可 */
              struct mscaler_frame_part   y;

              /* NV12,NV21 格式时使用,uv分量 */
              struct mscaler_frame_part   uv;
};
```

```c
struct mscaler_frame_part
{
              /* 用户空间虚拟地址 */
              void                        *mem;

              /* 分量大小 cache_line对齐 */
              unsigned int                mem_size;

              /* 实际物理地址,必须 cache_line 对齐       */
              unsigned long               phys_addr;

              /* 行字节对齐要求，必须同时满足MSCALER_ALIGN对齐 */
              unsigned int                stride;
};
```



```c
/*输出数据格式*/
enum mscaler_fmt{
              MSCALER_FORMAT_NV12         = 0,
              MSCALER_FORMAT_NV21         = 1,

              MSCALER_FORMAT_BGRA_8888    = (0<<2)+2,
              MSCALER_FORMAT_GBRA_8888    = (1<<2)+2,
              MSCALER_FORMAT_RBGA_8888    = (2<<2)+2,
              MSCALER_FORMAT_BRGA_8888    = (3<<2)+2,
              MSCALER_FORMAT_GRBA_8888    = (4<<2)+2,
              MSCALER_FORMAT_RGBA_8888    = (5<<2)+2,

              MSCALER_FORMAT_ABGR_8888    = (8<<2)+2,
              MSCALER_FORMAT_AGBR_8888    = (9<<2)+2,
              MSCALER_FORMAT_ARBG_8888    = (10<<2)+2,
              MSCALER_FORMAT_ABRG_8888    = (11<<2)+2,
              MSCALER_FORMAT_AGRB_8888    = (12<<2)+2,
              MSCALER_FORMAT_ARGB_8888    = (13<<2)+2,

              MSCALER_FORMAT_BGR_565      = (0<<2)+3,
              MSCALER_FORMAT_GBR_565      = (1<<2)+3,
              MSCALER_FORMAT_RBG_565      = (2<<2)+3,
              MSCALER_FORMAT_BRG_565      = (3<<2)+3,
              MSCALER_FORMAT_GRB_565      = (4<<2)+3,
              MSCALER_FORMAT_RGB_565      = (5<<2)+3,
};
```



## 12. KEYBOARD

包含头文件：

>   #include <libhardware2/mscaler.h>

api详解：

```c
long keys_open(void)
功能：查找并打开多个按键的设备
参数：无
返回值：成功：按键设备句柄
                  失败：-1
```

```c
void keys_close(long handle)
功能：关闭按键设备
参数：handle                                 //按键设备句柄，由keys_open()获得
返回值：无
```

```c
int read_key_event(long handle, struct key_event *key_event, int timeout)
功能：读取按键事件
参数：handle                                 //按键设备句柄，由keys_open()获得
             key_event                          //按键值和状态的结构体，结构体成员详见中间参数详解
             timeout                               //阻塞时间（-1：一直阻塞）（单位：ms）
返回值：成功：1
                  超时：0
                  失败：负数
```

中间参数详解：

```c
/*按键值和状态结构体*/
struct key_event {
              int key_type;                         //按键值
              int is_press;                           //状态（是否按下）
};
```



## 13. CAMERA

包含头文件：

>   #include <libhardware2/camera.h>

使用流程：

>   1.  camera_open                    获取camera设备句柄
>   2.  camera_power_on         使能camera设备，上电
>   3.  camera_stream_on        开始录制图像
>   4.  camera_wait_frame       获取一帧图像数据

api详解：

```c
int camera_open(struct camera_info *info, const char* device_path)
功能：获得camera设备句柄
参数：info                                              //用于存放camera的信息,不能为NULL，结构体成员详见中间参数详解
              device_path                            //设备节点路径
返回值：成功：camera设备句柄
                  失败：-1
```

```c
int camera_close(int fd, struct camera_info *info)
功能：关闭camera设备句柄
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
              info                                            //camera的信息,由camera_open()函数初始化，详见中间参数详解
 返回值：成功：0
                   失败：-1
```

```c
int camera_power_on(int fd)
功能：使能camera设备,上电
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：0
                  失败：负数
```

```c
int camera_power_off(int fd)
功能：关闭camera设备,掉电
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：0
                  失败：负数
```

```c
int camera_stream_on(int fd)
功能：开始camera图像录制
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：0
                  失败：负数
```

```c
int camera_stream_off(int fd)
功能：结束camera图像录制
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：0
                  失败：负数
```

```c
void *camera_wait_frame(int fd)
功能：获取一帧录制的图像数据
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：图像缓冲区指针
                  失败：NULL
```

```c
int camera_put_frame(int fd, void *mem)
功能：释放一帧图像缓冲区
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
              mem                                          //将要释放的缓冲区指针,由camera_wait_frame()函数获得
返回值：成功：0
                  失败：负数
```

```c
int camera_get_avaliable_frame_count(int fd)
功能：获取已录制的图像数据帧数
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
返回值：成功：帧数
                  失败：负数
```

```c
int camera_drop_frames(int fd, unsigned int frames)
功能：丢弃已录制的图像数据帧
参数：fd                                                 //camera设备句柄,由camera_open()函数获得
              frame                                        //需要丢弃的帧数
返回值：成功：帧数
                  失败：负数
```

中间参数详解：

```c
/*存放camera信息的结构体*/
struct camera_info {
              char name[16];

              /* 每行像素数 */
              unsigned int width;

              /* 行数 */
              unsigned int height;

              /* camera 每帧时长,单位微秒 */
              unsigned int frame_period_us;

              /* camera 帧数据格式 */
              unsigned int data_fmt;

             /* 一行的长度,单位字节
               * 对于 nv12,nv21, 表示y数据一行的长度
               * 另外由此可以算出uv数据偏移 line_length*height
               */
              unsigned int line_length;

              /* 一帧数据经过对齐之后的大小 */
              unsigned int frame_size;

              /* 帧缓冲总数 */
              unsigned int frame_nums;

              /* 帧缓冲的物理基地址 */
              unsigned long phys_mem;

              /* mmap 后的帧缓冲基地址 */
              void *mapped_mem;

              /*帧对齐大小 */
              unsigned int frame_align_size;
};
```

```c
   /* camera帧数据 部分常用 格式枚举 */
/* Grey formats */
#define V4L2_PIX_FMT_GREY    v4l2_fourcc('G', 'R', 'E', 'Y') /*  8  Greyscale     */
#define V4L2_PIX_FMT_Y16     v4l2_fourcc('Y', '1', '6', ' ') /* 16  Greyscale     */
/* Luminance+Chrominance formats */
#define V4L2_PIX_FMT_YVU420  v4l2_fourcc('Y', 'V', '1', '2') /* 12  YVU 4:2:0     */
#define V4L2_PIX_FMT_YUYV    v4l2_fourcc('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_YYUV    v4l2_fourcc('Y', 'Y', 'U', 'V') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_YVYU    v4l2_fourcc('Y', 'V', 'Y', 'U') /* 16 YVU 4:2:2 */
#define V4L2_PIX_FMT_UYVY    v4l2_fourcc('U', 'Y', 'V', 'Y') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_VYUY    v4l2_fourcc('V', 'Y', 'U', 'Y') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_YUV422P v4l2_fourcc('4', '2', '2', 'P') /* 16  YVU422 planar */
/* two planes -- one Y, one Cr + Cb interleaved  */
#define V4L2_PIX_FMT_NV12    v4l2_fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
#define V4L2_PIX_FMT_NV21    v4l2_fourcc('N', 'V', '2', '1') /* 12  Y/CrCb 4:2:0  */
/* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
#define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B', 'A', '8', '1') /*  8  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG8  v4l2_fourcc('G', 'B', 'R', 'G') /*  8  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG8  v4l2_fourcc('G', 'R', 'B', 'G') /*  8  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB8  v4l2_fourcc('R', 'G', 'G', 'B') /*  8  RGRG.. GBGB.. */
#define V4L2_PIX_FMT_SBGGR10 v4l2_fourcc('B', 'G', '1', '0') /* 10  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG10 v4l2_fourcc('G', 'B', '1', '0') /* 10  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG10 v4l2_fourcc('B', 'A', '1', '0') /* 10  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB10 v4l2_fourcc('R', 'G', '1', '0') /* 10  RGRG.. GBGB.. */
#define V4L2_PIX_FMT_SBGGR12 v4l2_fourcc('B', 'G', '1', '2') /* 12  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG12 v4l2_fourcc('G', 'B', '1', '2') /* 12  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG12 v4l2_fourcc('B', 'A', '1', '2') /* 12  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB12 v4l2_fourcc('R', 'G', '1', '2') /* 12  RGRG.. GBGB.. */
#define V4L2_PIX_FMT_SBGGR16 v4l2_fourcc('B', 'Y', 'R', '2') /* 16  BGBG.. GRGR.. */

/* Add New Item */
#define V4L2_PIX_FMT_SGBRG16 v4l2_fourcc('Y', 'B', 'R', '2') /* 16  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG16 v4l2_fourcc('Y', 'R', 'B', '2') /* 16  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB16 v4l2_fourcc('R', 'Y', 'B', '2') /* 16  RGRG.. GBGB.. */
```

