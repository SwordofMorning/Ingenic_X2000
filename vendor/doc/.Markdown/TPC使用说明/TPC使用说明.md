# X2600 TPC

# 1 热敏打印机介绍

## 1.1 硬件设备介绍

下面是打印机的硬件设备。

<img src="img/1.png" alt="1" style="zoom:67%;" /> 

下面是连接完成示意图：

<img src="img/2.png" alt="2" style="zoom:67%;" /> 

## 1.2 工作原理

**热敏打印机主要组成部分：**

1. 热敏纸：遇热会变黑。

2. 步进电机：控制进纸和进纸量。

3. 加热元件：用于加热热敏纸，每个加热元件对应一个加热点，所以这个器件决定了打印的最小颗粒, 如下图是加热元件的放大图

    <img src="img/3.png" alt="3" style="zoom:50%;" /> 

4. 加热控制：用于控制加热元件加热，一般有多个通道，分别控制不同区域的加热元件。

5. 移位锁存寄存器：用于存储要加热的数据，寄存器的每个位对应一个加热点。

6. 纸张检测传感器：纸张检测，防止没纸时打印。

7. 热敏传感器：计算加热头的温度，防止温度过高，烧坏打印机。

**热敏打印的流程：**

1. 向移位锁存寄存器发送数据，然后锁存数据。
2. 使能加热信号，寄存器对应的位就会加热，使热敏纸颜色发送变化。
3. 加热完成后，控制步进电机走纸，走纸的距离取决于每个加热点的大小，即一行的大小。
4. 回到步骤 1，直到打印完全部内容。

# 2 X2600 TPC 框架

TPC 为热敏打印机的控制器,热敏打印机主要由热敏头和步进电机组成, 控制器可以控制打印机热敏头的数据传输、加热、锁存等功能,同时控制步进电机的运转, 实现对热敏打印机打印过程的调控。

![4](img/4.png) 

以下是框架图：

![5](img/5.png)   

以上为打印机的总体框架图，分为 4 个部分，为移位，锁存，加热，电机控制。通过有序的控制这四个模块，就可以控制热敏打印机。下面粗略介绍一下这四个部分：

1. 移位：向移位寄存器发送数据。其发送协议为类似 SPI，有 DI 和 clk 线。对于 X2600，DI 通道可以有 1 ~ 8 个，当通道 > 4 时，单次单个通通输出的最大数据量为 128 字节，否则为 256 字节。
2. 锁存：锁存控制信号线。只有一个通道。
3. 加热：加热控制信号线。有 1 ~ 8 个通道。
4. 电机：可作为一个 8 路步进电机单元,可驱动一个 4 相步进电机,也能作为 2 个 4 路步进电机单元,可驱动两个双相步进电机。每个电机的步数最大为 64 步。

这四部分的控制可以分为以下 **3 种模式**：

- 全自动模式：由控制器自动控制 移位，锁存，加热，电机
- 半自动模式：由控制器自动控制 移位，锁存，加热。电机自己控制，所以需要自己对齐
- 手动控制模式：自己控制 移位，锁存，加热，和电机，自己对齐

# 3 运行示例

## 3.1 硬件环境准备

1. 准备热敏打印机

      <img src="img/2.png" alt="2" style="zoom:67%;" />

    介绍：

    1. 加热点有 384 个，一个移位通道
    2. 有一个 2相4线 的步进电机，4 步为一圈
    3. 有两个加热通道

2. 准备开发板

    准备一个 x2600 的针板作为主控板，然后将热敏打印机下面表格与主控板连接：

    <img src="img/6.png" alt="6" style="zoom:80%;" /> 

    连接完成：

    <img src="img/7.png" alt="7" style="zoom:80%;" /> 

## 3.2 软件环境准备

编译固件

### 3.2.1 编译大核固件

打开 x2600e_nand_5.10_defconfig 配置文件

![8](img/8.png) 

勾选 mcu 驱动和命令：

<img src="img/9.png" alt="9" style="zoom: 67%;" /> 

<img src="img/10.png" alt="10" style="zoom: 67%;" /> 

勾选 tpc demo（demo 位于 linux 工程的 libhardware2/src/cmds/demo_tpc/demo_tpc_tx_shift_data.c）：

<img src="img/11.png" alt="11" style="zoom:67%;" /> 

然后编译工程：

![12](img/12.png)  

编译完成：

![13](img/13.png) 

烧录：
配置烧录工具：

<img src="img/14.png" alt="14" style="zoom:67%;" /> 

选择平台和配置文件（nand 为例），选择串口：

![15](img/15.png)   

修改 文件系统 的分区格式：（烧录工具默认文件系统为 UBI mos，需要改为 MTB 模式）

![16](img/16.png) 

选择烧录固件：

![17](img/17.png) 

保存之后开始烧录

### 3.2.2 编译小核固件

打开配置文件：

![18](img/18.png) 

勾选 tpc 驱动：

<img src="img/19.png" alt="19" style="zoom:67%;" /> 

选择电机个数：

<img src="img/20.png" alt="20" style="zoom:67%;" /> 

选择小核 demo：

![21](img/21.png) 

编译小核固件：

```c
# 切换到工程小核libmcu的编译目录
cd libmcu/
# 配置编译器路径
export PATH=../tools/toolchains/riscv-r1.0.0/bin/:$PATH
# 编译对应的配置文件
make x2600e_libcxx_defconfig
# 编译小核(生成libmcu-bare.bin小核镜像文件)
make
# 将libmcu-bare.bin镜像文件adb到开发板的/usr/data或者tmp/可写分区
adb push libmcu-bare.bin /usr/data
```

>使用小核的工具链时，编译时会报错提醒说没有 libmpfr.so.4 库, 软链接到 /usr/lib/x86_64-linux-gnu/libmpfr.so.6.0.1 文件即可。
>仅需链接一次
>sudo ln -s /usr/lib/x86_64-linux-gnu/libmpfr.so.6.0.1 /usr/lib/x86_64-linux-gnu/libmpfr.so.4

## 3.3 运行

![22](img/22.png) 

打印结果：

<img src="img/23.png" alt="23" style="zoom:67%;" /> 

## 3.4 示例讲解

demo 流程图（示例用到的 api 在下面章节有讲解，具体实现请看 demo）：

![24](img/24.png) 

大核 demo（demo 位于 linux 工程的 libhardware2/src/cmds/demo_tpc/demo_tpc_tx_shift_data.c）：

```c
int main(void)
{
    int mcu_fd = mcu_open(); // 打开 MCU 节点
    if (mcu_fd < 0) {
        fprintf(stderr, "failed to open mcu\n");
        return -1;
    }

    int width = (gImage_ingenic_image[2] << 8) + gImage_ingenic_image[3];
    int height = (gImage_ingenic_image[4] << 8) + gImage_ingenic_image[5];
    int ret;

    unsigned char *src_data = &gImage_ingenic_image[6];
    unsigned char *dst_data = malloc(SHIFT_LINES_BYTES);

    int cnt = height;

    int widthbyte = width / 8 + (width % 8 ? 1 : 0);

    while(cnt) {
        /*准备好一行 图片数据*/
        Print_Picture(0, src_data, width, height, dst_data, SHIFT_LINES_BYTES); //处理打印数据
        src_data += widthbyte;

        /*等待小核准备好*/
        ret = wait_mcu_is_ready(mcu_fd);
        if (ret < 0)
            continue;

        /*写一行数据*/
        mcu_write_data(mcu_fd, dst_data, SHIFT_LINES_BYTES);    //将数据发送给小核

        cnt--;
    }
    /*等待小核准备好收数据*/
    wait_mcu_is_ready(mcu_fd);
    /*发送结束信息*/
    mcu_write_data(mcu_fd, "tpc_data_end", strlen("tpc_data_end")); //发送打印结束
    mcu_close(mcu_fd);

    return 0;
}
```

小核 demo （位于 linux 工程的 libmcu/example/tpc_example.c）

```c
void tpc_example(void)
{
    printf("%s is called\n", __func__);

#ifdef APP_libmcu_driver_irq
    host_cpu_set_irq_callback(host_irq_cb);
#endif

    tpc_init();//初始化 tpc 控制器

    int ret;
    //初始化半自动模式：自动控制移位，锁存，加热，电机自己控制
    ret = tpc_sync_mode_init(&shift_cfg, &heat_cfg, &lat_cfg, NULL, -1);
    if (ret < 0)
        return;

    ret = tpc_motor_init(MOTOR_ID, &motor_cfg_0);//初始化电机
    if (ret < 0)
        return;

    unsigned char shift_data[2048];
    int shift_size = 0;


    int heat_size = sizeof(struct heat_data);
    struct heat_data *heat_data = malloc(heat_size);

    int motor_size = MOTOR_REPEAT_STEP * sizeof(int);
    unsigned int *motor_data = malloc(motor_size);

    int heat_clkcnt = TPC_US_CLKCNT(10 * 1000);

    int cnt = 0;
    while (1) {
        /*通知大核 准备好接收数据*/
        mcu_tpc_is_ready();

        memset(shift_data, 0, sizeof(shift_data));
        //接收大核传过来的打印数据
        shift_size =  mcu_read_host_tpc_data(shift_data, sizeof(shift_data));

        motor_data[0] = TPC_US_CLKCNT(10 * 1000);
        motor_data[1] = TPC_US_CLKCNT(10 * 1000);
        motor_data[2] = TPC_US_CLKCNT(10 * 1000);
        motor_data[3] = TPC_US_CLKCNT(10 * 1000);

        /*与全对齐模式相比，电机不对齐的话，得手动计算加热到下一行的锁存的延时时间*/
        uint64_t heat_delay_clkcnt = motor_data[0] + motor_data[1] + motor_data[2] + motor_data[3] - heat_clkcnt;
        heat_data[0].nxt_trigger_type = TPC_HEAT_TRIGGER_BY_LATCH | heat_delay_clkcnt;
        heat_data[0].channels[0] = TPC_HEAT_ENABLE | heat_clkcnt;
        heat_data[0].channels[1] = TPC_HEAT_ENABLE | heat_clkcnt;


        if (strcmp("tpc_data_end", (char *)shift_data) == 0) {

            tpc_sync_mode_data_stream_end();//结束半自动模式数据搬运，会将 dma 缓存的数据全部搬完后停止
            tpc_motor_data_stream_end(MOTOR_ID);//结束电机的数据搬运，会将 dma 缓存的数据全部搬完后停止

            tpc_sync_mode_stop(0);//停止半自动模式，不立刻停止
            tpc_motor_stop(MOTOR_ID, 0);//停止电机，不立刻停止
            cnt = 0;
            continue;
        }
        //将数据写入 dma 缓存
        write_sync_mode_data(shift_data, (void *)heat_data, (void *)motor_data, shift_size, heat_size, motor_size);

        /*收到32行数据后(内部默认申请32行 buffer大小)开始输出*/
        cnt++;
        if (cnt == 32) {
            tpc_sync_mode_start();//开始半自动模式
            tpc_motor_start(MOTOR_ID);//开始电机
        }
    }
}
```

# 4 代码实现（重要）

## 4.1 TPC 四个结构体讲解

以下代码均在小核工程的 `libmcu/soc/x2600/include/soc/tpc_data.h` 文件中。

这四个结构体用于配置 tpc 的 4 个模块，分别是 移位，加热，锁存，电机。

### 4.1.1 移位结构体

配置结构体：

```c
struct tpc_shift_cfg {
    uint8_t idle_level; /* 空闲时的电平状态 */
    uint8_t big_end;   /* 大小端 */
    /**
     * 极性 spi_pol :
     * 当spi_pol=0，在时钟空闲即无数据传输时,clk电平为低电平
     * 当spi_pol=1，在时钟空闲即无数据传输时,clk电平为高电平
     * 相位 spi_pha :
     * 当spi_pha=0，表示在第一个跳变沿开始传输数据，下一个跳变沿完成传输
     * 当spi_pha=1，表示在第二个跳变沿开始传输数据，下一个跳变沿完成传输
     */
    uint8_t spi_pol;
    uint8_t spi_pha;

    uint8_t channels;       /* 数据输出通道数1 ~ 8*/
    uint8_t data_invert;    /* 移位数据取反*/
    uint8_t line_bytes;    /* 数据输出总长度 （通道数大于4时每个通道单次输出最大是128（字节），小于4通道单次输出最大是256(字节)，
    						* 每次发送数据时，各个通道的发送的数据量都相同，例如：
   							* 一行打印数据为 1024bit，5 数据通道打印头，设通道 0~通道 3 为 208bit, 通道 4 为 192bit，每个通道
   							* 的数据存储单位为字节, 输出数据的长度相等。那么通道 0 ~ 通道 4 数据输出长度为 26 字节，通道 0 ~ 通道 3 有
   							* 效数据长度为 26 字节，通道 4 有效数据长度为 24 字节，通道 4 中无效数据先行输出。
   							*/

    uint64_t out_clk_rate; /* spi clk */
    tpc_irq_callback irq_callback; /* 中断函数回调指针 */
};

```

### 3.1.2 加热结构体

配置结构体：

```c
struct tpc_heat_cfg {
    int channels;                          /* 热信号通道1 ~ 8 */
    int idle_level;                        /* 热信号空闲时电平 */
    uint64_t max_heat_clkcnt;                   /* 最大加热时间(clk cnt)，当加热时间超过时会报错 */
    tpc_irq_callback irq_callback; 			/* 中断函数回调指针 */
};

```

### 3.1.3 锁存结构体

配置结构体：

```c
struct tpc_lat_cfg {
    int idle_level;                         /* 锁存的空闲时电平 */
    int active_clkcnt;                      /* 锁存有效持续时间 */
    uint64_t wait_clkcnt_out;                    /* 触发输出锁存信号之后，等待多少个时钟才开始输出(clk cnt) */
    uint64_t wait_clkcnt_out_shift;              /* 锁存输出完后，等待等待多少个时钟触发移位数据, 仅全自动/半自动模式有效  (clk cnt)*/
    tpc_irq_callback irq_callback;				/* 中断函数回调指针 */
};

```

### 3.1.4 电机结构体

配置结构体：

``` c
/*电机控制器支持1个 8路电机或者 两个4路的电机*/
struct tpc_motor_cfg {
    int channels;                                       /* id0 通道数为 1 ～ 8，当id0 大于4通道时， id1不可用 */

    uint8_t idle_level;                                 /* 电机io idle 时的电平 例如：
    													 * 0b0101 代表4个channels， 从1通道开始依次是 1 0 1 0
    													 */
    uint8_t start_step_io;                              /* 电机io开始步时的电平 */
    uint8_t stop_step_io;                               /* 电机io结束步时的电平 */

    uint64_t start_step_clkcnt;                              /* 开始步步长（clk cnt）*/
    uint64_t stop_step_clkcnt;                               /* 结束步步长（clk cnt）*/

    int repeat_step;                                    /* 电机循环步数,最大64步 */
    unsigned long long repeat_step_io[MAX_MOTO_IO];		/* 每步对应的 io 电平，例如：一个 4 路，8 步电机
    													 *     .repeat_step_io = {
        												 *			[0] = 0b11001100,
        												 *			[1] = 0b00110011,
        												 *			[2] = 0b01100110,
        												 * 			[3] = 0b10011001,
    													 *		}
    													 *	则  第一步时，各路电平为 0：0	1：1		2：0		3：1
    													 *  	第二步时，各路电平为：0：0	1：1		2：1		3:0
    													 *    	······
    													 */

    int align_step;                                     /* 在周期内的第几步触发加热，具体请看 重要提示 中的对 align_step 的解释 */
    uint64_t max_step_clkcnt;                           /* 电机步的最大步长(clk cnt) */
    tpc_irq_callback irq_callback;						/* 中断函数回调指针 */
};
```

## 4.2 数据流图

![25](img/25.png)  

这三个缓存的数据格式如下：

1. 移位

    ![26](img/26.png) 

    针对这个格式，demo 声明了一个数据结构：

    ```c
    unsigned char shift_data[sum];
    /*
    	使用示例：384 个打印点，共 48 个字节，然后打印虚线
    	第一行数据：
    	shift_data[0]=0b10101010;
    	······
    	shift_data[47]=0b10101010;
        第二行数据：
    	shift_data[48]=0b10101010;
    	······
    	shift_data[95]=0b10101010;
    	······
    */
    ```

2. 加热：

    <img src="img/27.png" alt="27" style="zoom:80%;" /> 

    针对这个格式，demo 声明了一个数据结构：

    ```c
    struct heat_data {
        unsigned int nxt_trigger_type;
        unsigned int channels[2];
    }heat_data[sum];
    /*
    	使用示例：两个加热通道
    	第一次加热：
        heat_data[0].nxt_trigger_type = 1 << 24 | TPC_US_CLKCNT(2*1000); 
        锁存触发加热，加热完成后延时 2ms 触发下一次锁存，仅全自动、半自动模式有效
        heat_data[0].channels[0] = 1 | TPC_US_CLKCNT(2*1000); 使能加热，加热时间为 2ms
        heat_data[0].channels[1] = 1 | TPC_US_CLKCNT(2*1000); 使能加热，加热时间为 2ms
    */
    ```

3. 电机

    <img src="img/28.png" alt="28" style="zoom:80%;" /> 

    针对这个格式，demo 声明一个数据结构：

    ```c
    unsigned int motor_data[sum];
    /*
    	使用示例：电机为 4 步一圈，然后逐步减速
    	第一圈的数据：
    	motor_data[0] = TPC_US_CLKCNT(2*1000); 2ms
    	motor_data[1] = TPC_US_CLKCNT(3*1000); 3ms
    	motor_data[2] = TPC_US_CLKCNT(4*1000); 4ms
    	motor_data[3] = TPC_US_CLKCNT(5*1000); 5ms
    	第二圈的数据：
    	motor_data[4] = TPC_US_CLKCNT(6*1000); 6ms
    	motor_data[5] = TPC_US_CLKCNT(7*1000); 7ms
    	motor_data[6] = TPC_US_CLKCNT(8*1000); 8ms
    	motor_data[7] = TPC_US_CLKCNT(9*1000); 9ms
    	······
    */
    ```

## 4.3 API 接口

以下接口需要包含 `libmcu/soc/x2600/include/soc/tpc.h` 文件

```c
/*
 * @brief 初始化 tpc 控制器
 * @param 无参数
 * @retval 无
 */
void tpc_init(void);

unsigned int tpc_get_one_us_clks(void);

/*-----------------------------shift lat heat设备单步接口， 用的是普通dma---------------------------------------------------------------------*/

/*
 * @brief 初始化移位控制器
 * @param config : 移位配置结构体
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_shift_init(struct tpc_shift_cfg *config);

/*
 * @brief 初始化加热控制器
 * @param config : 加热配置结构体
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_heat_init(struct tpc_heat_cfg *config);

/*
 * @brief 初始化锁存控制器
 * @param config : 锁存配置结构体
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_latch_init(struct tpc_lat_cfg *config);

/*
 * @brief 将移位数据写入 dma 缓存
 * @param data : 移位数据存放位置的指针
 * @param len ： 数据长度
 * @retval 返回写入 dma 缓存的数据的长度
 */
int tpc_shift_write_data(unsigned char *data, int len);

/*
 * @brief 将加热数据写入 dma 缓存
 * @param data : 加热数据存放位置的指针
 * @param len ： 数据长度
 * @retval 返回写入 dma 缓存的数据的长度
 */
int tpc_heat_write_data(unsigned int *data, int len);

/*
 * @brief 触发移位，阻塞等待 dma 搬运数据后，开始输出
 * @param 无
 * @retval 成功返回 0，失败返回错误代码
 */
int tpc_shift_start(void);

/*
 * @brief 触发加热，阻塞等待 dma 搬运数据后，开始输出
 * @param 无
 * @retval 成功返回 0，失败返回错误代码
 */
int tpc_heat_start(void);

/*
 * @brief 触发锁存，阻塞等待 dma 搬运数据后，开始输出
 * @param 无
 * @retval 成功返回 0，失败返回错误代码
 */
int tpc_latch_start(void);

/*
 * @brief 用于马上停止移位设备，一般不会用到
 * @param 无
 * @retval 无
 */
void tpc_shift_quick_stop(void);

/*
 * @brief 用于马上停止加热设备，一般不会用到
 * @param 无
 * @retval 无
 */
void tpc_heat_quick_stop(void);

/*
 * @brief 用于马上停止锁存设备，一般不会用到
 * @param 无
 * @retval 无
 */
void tpc_latch_quick_stop(void);

/*
 * @brief 用于注销移位设备
 * @param 无
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_shift_deinit(void);

/*
 * @brief 用于注销加热设备
 * @param 无
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_heat_deinit(void);

/*
 * @brief 用于注销锁存设备
 * @param 无
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_latch_deinit(void);

/*-----------------------------------------电机接口， 用的是loop dma---------------------------------------------------*/
/*
 * loop dma 模式：使用 dma 去循环搬运指定范围数据
 * 当采用 loop dma 模式时，所以必须及时喂数据给 dma 缓存，防止喂数据不及时时，打印到错误的数据
 */

/*
 * @brief 初始化电机控制器
 * @param id ： 第几个电机（0，1）
 * @param motor : 电机配置结构体
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_motor_init(int id, struct tpc_motor_cfg *motor);

/*
 * @brief 将电机数据写入 dma 缓存
 * @param id : 第几个电机（0，1）
 * @param data : 电机数据存放位置的指针
 * @param len ： 数据长度
 * @retval 返回写入 dma 缓存的数据的长度
 */
int tpc_motor_write_data(int id, unsigned int *data, int len);

/*
 * @brief 开始电机，阻塞等待 dma 搬运数据后，开始输出
 * @param id ： 第几个电机（0，1）
 * @retval 无
 */
void tpc_motor_start(int id);

/*
 * @brief 停止 dma 的数据搬运，会将 dma 缓存中的数据搬完后停止
 * @param id ： 第几个电机（0，1）
 * @retval 无
 */
void tpc_motor_data_stream_end(int id);

/*
 * @brief 停止电机
 * @param id : 第几个电机（0，1）
 * @param quick_stop：是否立刻停止电机
 *      quick_stop = 0：不是，等 dma 数据搬完再停止，
 *      quick_stop = 1： 是，立刻结束 dma 搬运，然后停止电机
 * @retval 成功返回 0，失败返回错误代码
 */
int tpc_motor_stop(int id, int quick_stop);

/*
 * @brief 注销电机设备
 * @param id : 第几个电机（0，1）
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_motor_deinit(int id);


/*-----------------------------------------同步模式接口, 用的是 loop dma------------------------------------------------*/

/*
 * 当为全自动模式时，下面接口会同时控制移位，锁存，加热，电机运作
 * 当为半自动模式时，下面接口会同时控制移位，锁存，加热，电机需要使用上面的接口自己控制
 */

/*
 * @brief 初始化自动模式
 * @param shift : 移位控制结构体
 * @param heat ： 加热控制结构体
 * @param lat ： 锁存控制结构
 * @param motor ： 电机控制结构
 * @param motor_id ： 第几个电机（0，1）
 *      当 motor != NULL 时为全自自动模式
 *      当 motor == NULL 时为半自动模式
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_sync_mode_init(struct tpc_shift_cfg *shift, struct tpc_heat_cfg *heat, struct tpc_lat_cfg *lat,
                       struct tpc_motor_cfg *motor, int motor_id);

/*
 * @brief 开始半自动模式，会搬运 shift dma 缓存和 heat dma 缓存中的数据，当为全自动模式时，同时也会搬运
 *        motor dma 缓存中的数据
 * @param 无
 * @retval 无
 */
void tpc_sync_mode_start(void);

/*
 * @brief 停止 dma 的数据搬运，会将 dma 缓存中的数据搬完后停止
 * @param 无
 * @retval 无
 */
void tpc_sync_mode_data_stream_end(void);

/*
 * @brief 停止打印， 如果为全自动模式，也会停止电机
 * @param quick_stop：是否立刻停止打印
 *          quick_stop = 0：不是，等 dma 数据搬完再停止，
 *          quick_stop = 1： 是，立刻结束 dma 搬运，然后停止打印
 * @retval 成功返回 0，失败返回错误代码
 */
int tpc_sync_mode_stop(int quick_stop);

/*
 * @brief 注销设备接口, 如果为全自动模式，也会注销电机
 * @param 无
 * @retval 成功返回 0，失败返回其他值
 */
int tpc_sync_mode_deinit(void);

```

# 5 工作时序讲解

## 5.1 全自动模式

以下使用的电机为 4 步一个周期，对齐步配置为 4 步。在全自动模式下，加热的触发由电机触发，即电机的第一步开始时和对齐步结束时，会触发加热。所以在全自动模式下，一个电机周期可以有 2 个触发加热的时刻。以下是第一步和对齐步结束为同一时刻，所以只触发了一次加热。

![29](img/29.png) 

时序讲解：

| 符号   | 含义                                                         |
| ------ | ------------------------------------------------------------ |
| t1     | 移位数据输出的 clk，频率可配置，可配置到 50M                 |
| t2     | 锁存有效时间，可配置                                         |
| t3     | 加热有效时间，这个决定了可颜色深浅，可配置                   |
| t4     | 步进电机每步时间，这个决定了电机的速度，可配置               |
| a -> b | 开始步时间，可配置。对于全自动模式，这个时间需要大于 spi 发送数据的时间 + t2 |
| c -> d | 在全自动模式下，加热完成后就会触发锁存。                     |

在全自动模式下，开始发送移位数据时，就进行开始步。移位数据发送完成，锁存后就会发送下一次移位数据，然后等待锁存。

## 5.2 半自动模式

以下使用的电机为 4 步一个周期。在半自动模式下，加热的触发由锁存触发，即锁存结束后就会触发加热。所以在半自动模式下，需要自己对齐一行数据要走多少步之后才加热。

![30](img/30.png)  

| 符号   | 含义                                                         |
| ------ | ------------------------------------------------------------ |
| t1     | 移位数据输出的 clk，频率可配置，可配置到 50M                 |
| t2     | 锁存有效时间，可配置                                         |
| t3     | 加热有效时间，这个决定了可颜色深浅，可配置                   |
| t4     | 步进电机每步时间，这个决定了电机的速度，可配置               |
| a -> b | 开始步时间，可配置。对于半自动对齐模式，这个时间需要<br />为 spi 发送数据的时间 + t2，这样可以与第一次加热对齐。<br />因为在在半自动模式下，加热的触发由锁存触发。 |
| c -> d | 对于半自动模式，可配置加热后触发锁存时间，用来手动对<br />齐电机，即对齐一行的数据。上面配置为电机走过一行的时<br />间（4步一行）- t2 - t3。即当下一次锁存完成后，触发加热，<br />时间刚好为电机这边开始第一步的时间。即新的一行的时间。 |

在半自动模式下，电机自己控制什么时候开始。

## 5.3 手动控制模式

以下使用的电机为 4 步一个周期。在手动控制模式下，所有动作均为自己触发。但是电机在第一次触发后，直到打印结束才能停。电机可以设置在电机周期内的第几步产生中断，然后在中断回调函数里面触发移位，在移位完成中断回调里面触发锁存，在锁存完成中断回调里面触发加热，如此完成一行数据的打印。

![31](img/31.png) 

| 符号   | 含义                                           |
| ------ | ---------------------------------------------- |
| t1     | 移位数据输出的 clk，频率可配置，可配置到 50M   |
| t2     | 锁存有效时间，可配置                           |
| t3     | 加热有效时间，这个决定了可颜色深浅，可配置     |
| t4     | 步进电机每步时间，这个决定了电机的速度，可配置 |
| a -> b | 移位完成中断，触发锁存                         |
| c -> d | 锁存完成，触发加热                             |
| e -> f | 加热完成，触发电机开始步，开始步时间可配       |
| g -> h | 产生中断，触发移位，开始新的一行数据发送       |

# 6 重要提示

## 6.1 多通道移位数据

设一点行有 1024 个打印点，以下为不同通道数的数据传输示意图。

![32](img/32.png) 

8 数据通道打印头，每个移位寄存器的位数为 128bit，每个通道的数据存储单位为字节,输出数据的长度相等。每个通道的数据输出长度为 16 字节,有效数据长度为 16 字节。

![33](img/33.png) 

5 数据通道打印头，设通道 0~通道 3 为 208bit, 通道 4 为 192bit，每个通道的数据存储单位为字节, 输出数据的长度相等。那么通道 0 ~ 通道 4 数据输出长度为 26 字节，通道 0~ 通道 3 有效数据长度为 26 字节，通道 4 有效数据长度为 24 字节，通道 4 中无效数据先行输出。

## 6.2 多通道加热，但是打印机的功率只支持同一时间只有一个加热通道

采用假加热模式。

在加热的 dma 数据格式里面，可以设置本次加热是否使能加热信号，如下：

![34](img/34.png) 

所以我们可以将同一行数据进行多次传输，每次只加热一个通道。例如：

当打印机是 2 个加热通道，4 步一行。则可以设置为 2 步一行，将一行数据进行两次传输，第一次加热左通道，第二次加热右通道。

## 6.3 align_step 讲解

在全自动模式下，电机配置结构体中的 align_step 成员就起效果了，它可以让全自动模式下的一个电机周期内有两个加热时刻，也就是一个周期内可以打印两行。例如：

一个 8 步（0 ~ 7）为一周期的电机，在全自动模式，将 align_step 设置为 4，则电机会在第 0 步打印一行数据，在第 4 步打印一行数据。