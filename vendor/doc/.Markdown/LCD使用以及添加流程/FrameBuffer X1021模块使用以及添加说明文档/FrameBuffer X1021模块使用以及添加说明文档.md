# FrameBuffer X1021模块使用以及添加说明文档

## 1. LCD配置流程

1.配置“fb驱动”功能

<img src="image/1.png" style="zoom:75%;" />

<img src="image/2.png" style="zoom:75%;" />

<img src="image/3.png" style="zoom:75%;" />

2.配置lcd相关引脚

<img src="image/4.png" style="zoom:75%;" />

<img src="image/5.png" style="zoom:75%;" />

<img src="image/6.png" style="zoom:75%;" />

<img src="image/7.png" style="zoom:75%;" />



## 2. 添加流程

>   如果现有支持的屏幕不适用，可按照下面流程添加自己的屏幕驱动。
>
>   1.添加新的lcd模块
>
>   在device/lcd/x1021/目录下添加新的lcd文件夹，其中包含.c文件、Makefile。
>
>   2.配置struct smart_lcd_data_table结构体数组
>
>   ​         在新添加的lcd代码中定义一个struct smart_lcd_data_table 类型的数组，并进行配置。该结构体数组主要存放初始化lcd寄存器的相应数据及命令,请根据需求以及lcd的数据手册自行配置，结构体类型详细见第3章
>
>   3.配置struct lcdc_data
>
>   ​         在新添加的lcd代码中定义一个struct lcdc_data类型的变量,并进行配置。该结构体类型的相关说明在第3.1章。
>
>   4.实现接口
>
>   ​         在.c文件下实现以下接口：
>
>   ```c
>   power_on()                      //主要实现对lcd上电
>   power_off()                      //主要实现对lcd掉电
>   module_init()                  //驱动的入口函数，主要实现申请相关gpio，调用jzfb_register_lcd()注册lcd
>   module_exit()                 //驱动的出口函数，主要实现释放相关gpio，调用jzfb_unregister_lcd()注销lcd
>   ```
>
>   5.将lcd.c文件加入Iconfig配置界面并且加入编译
>
>   ​          详见第4章



## 3. 类型详解

### 3.1 lcd结构体详解

```c
包含头文件
          #include "soc/x1021/fb/lcdc_data.h"

LCD配置结构体：
struct lcdc_data {
    const char *name;   /*屏幕名字*/
    unsigned int refresh;   /*刷新频率*/
    unsigned int xres;  /*一行像素点的个数*/
    unsigned int yres;  /*一帧图片的行数*/
    unsigned int pixclock;  /*传输帧数据时的时钟频率 */
    unsigned int pixclock_when_init;    /*传输数据或命令时的时钟频率 */
    unsigned int left_margin;  /*行切换，从同步到绘图之间的延迟（vsync的个数）*/
    unsigned int right_margin;  /*行切换，从绘图到同步之间的延迟（vsync的个数）*/
    unsigned int upper_margin;  /*帧切换，从同步到绘图之间的延迟（pixclock的个数）*/
    unsigned int lower_margin;  /*帧切换，从绘图到同步之间的延迟（pixclock的个数）*/
    unsigned int hsync_len;  /*垂直同步脉冲宽度，对应pixclock的个数*/
    unsigned int vsync_len;  /*水平同步脉冲宽度，对应hsync的个数*/

    enum fb_fmt fb_format;  /*数据源的格式，详见3.2*/
    enum lcdc_out_format out_format;    /*图像输出格式，详见3.2*/
    enum lcdc_lcd_mode lcd_mode;    /*数据通讯模式（6800、8080），详见3.2*/

    struct {
                int te_gpio;    /*自定义的te引脚*/
                enum lcdc_out_order out_order;    /*RGB分量的输出顺序，详见3.2*/
                enum lcdc_mcu_data_width mcu_data_width;  /*数据宽度，详见3.2*/
                enum lcdc_mcu_data_width mcu_cmd_width;  /*命令宽度，详见3.2*/
                enum lcdc_data_trans_mode data_trans_mode;  /*数据传输模式，详见3.2*/
                enum lcdc_data_trans_mode cmd_trans_mode;  /*命令传输模式，详见3.2*/

                enum lcdc_signal_polarity wr_data_sample_edge;  /*读写数据时的采样边缘，详见3.2*/
                enum lcdc_dc_pin dc_pin;  /* DC引脚电平功能设置，详见3.2*/

                enum lcdc_signal_polarity te_data_transfered_edge;  /* TE引脚的有效电平，详见3.2*/
                enum lcdc_te_type te_pin_mode;   /* TE使能，详见3.2*/

                enum lcdc_signal_level rdy_cmd_send_level; /*RDY有效电平，详见3.2*/
                int enable_rdy_pin:1;  /*RDY使能（是否等待RDY再发送命令或数据）*/
    }slcd;

    int height;  /*图片高度（mm）*/
    int width;  /*图片宽度（mm）*/

    unsigned int cmd_of_start_frame;  /*发送一帧数据前发送的命令*/
    struct smart_lcd_data_table *slcd_data_table;   /*屏幕的初始化数据，详见3.2章*/
    unsigned int slcd_data_table_length;  /*屏幕初始化数据的大小*/
    int (*power_on)(void);  /*屏幕电源开启函数*/
    int (*power_off)(void);  /*屏幕电源关闭函数*/
};
```

### 3.2 结构体成员详解

```c
包含头文件
           #include "soc/x1021/fb/lcdc_data.h"

图像数据源格式流枚举：
enum fb_fmt {
             fb_fmt_RGB555,             /*RGB555，一个像素2个字节，15bpp*/
             fb_fmt_RGB565,             /*RGB565，一个像素2个字节，16bpp*/
             fb_fmt_RGB888,             /*RGB888，一个像素4个字节，24bpp*/
             fb_fmt_ARGB8888,       /*ARGB888，一个像素4个字节，24bpp*/
             fb_fmt_NV12,                  /*NV12，一个像素2个字节，16bpp*/
             fb_fmt_NV21,                  /*NV21，一个像素2个字节，16bpp*/
};


输出到屏幕的RGB格式枚举：
enum lcdc_out_format {
             OUT_FORMAT_565,        /* 输出格式为RGB565 */
             OUT_FORMAT_666,        /* 输出格式为RGB666 */
             OUT_FORMAT_888,        /* 输出格式为RGB888 */
};

数据通讯接口模式枚举：
enum lcdc_lcd_mode {
                 SLCD_6800 = 1,     /*6800接口*/
                 SLCD_8080,            /*8080接口*/
}

RGB分量的排列顺序枚举：
enum lcdc_out_order {
            ORDER_RGB,  /*RGB*/
            ORDER_RBG,  /*RBG*/
            ORDER_GRB,  /*GRB*/
            ORDER_BRG,  /*BRG*/
            ORDER_GBR,  /*GBR*/
            ORDER_BGR,  /*BGR*/
};

数据宽度枚举
enum lcdc_mcu_data_width {
             MCU_WIDTH_8BITS,        /* 一个时钟节拍传8位数据 */
             MCU_WIDTH_9BITS,        /* 一个时钟节拍传9位数据 */
             MCU_WIDTH_16BITS,      /* 一个时钟节拍传16位数据 */
             MCU_WIDTH_18BITS,      /* 一个时钟节拍传18位数据 */
             MCU_WIDTH_24BITS,       /* 一个时钟节拍传24位数据 */
};

数据发送模式枚举
enum lcdc_data_trans_mode {
             LCD_PARALLEL_MODE,    /* PARALLEL模式 */
             LCD_SERIAL_MODE,          /* SERIAL模式 */
};

采样边缘枚举
enum lcdc_signal_polarity {
             AT_FALLING_EDGE,            /* 下降沿采样 */
             AT_RISING_EDGE,               /* 上升沿采样 */
};

DC引脚电平功能枚举
enum lcdc_dc_pin {
             CMD_LOW_DATA_HIGH,     /* dc为高时发送data，为低发送cmd */
             CMD_HIGH_DATA_LOW,      /* dc为高时发送cmd，为低发送data */
};

有效电平枚举
enum lcdc_signal_level {
             AT_LOW_LEVEL,       /* 低电平有效 */
             AT_HIGH_LEVEL,      /* 高电平有效 */
};

TE引脚配置模式枚举
enum lcdc_te_type {
             TE_NOT_EANBLE,                /* TE不使能 */
             TE_GPIO_IRQ_TRIGGER,   /* TE gpio中断模式 */
             TE_LCDC_TRIGGER,             /* TE寄存器使能模式 */
};
```

```c
struct smart_lcd_data_table {
             enum smart_config_type type;           /* 发送数据的类型 */
             unsigned int value;                                    /* 数据 */
};

enum smart_config_type {
             SMART_CONFIG_DATA,               /* 数据 */
             SMART_CONFIG_CMD,                /* 命令 */
             SMART_CONFIG_UDELAY,          /* 延时时间（us） */
};
```

### 3.3 lcd注册注销函数详解

```c
int jzfb_register_lcd(struct lcdc_data *pdata);
功能：将lcd的相关数据注册到fb驱动下
参数：pdata                                                    //lcd的数据结构体，详见lcd结构体详解
返回值：成功：0
                  失败：负数
```

```c
void jzfb_unregister_lcd(struct lcdc_data *pdata)
功能：将lcd的数据从fb驱动下注销         ///lcd的数据结构体，详见lcd结构体详解
参数：pdata
返回值：无
```



## 4. 加入IConfig配置界面以及加入编译

>   在package/devices/ lcd/x1021/目录下，添加新的lcd模块文件夹，文件夹包含Config.in文件、.mk文件

### 4.1 加入IConfig配置界面

1.Config.in文件可参考以下实例

```c
menuconfig MD_X1021_SLCD_KWH035
    bool "SLCD KWH035 (320x480) (X1021)"
    select MD_X1021_FB
    depends on MD_SOC_X1021

config MD_X1021_KWH035_TE
    string "gpio tearing effect(lcd 图像防裂屏引脚)"
    choice from SOC_GPIO_PINS
    default -1


config MD_X1021_KWH035_CS
    string "gpio chip select(lcd 片选引脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1021_KWH035_RD
    string "gpio lcd read signal(lcd 读引脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1021_KWH035_RST
    string "gpio reset(lcd 复位引脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1021_KWH035_POWER_EN
    string "gpio power enable(lcd power使能引脚)"
    choice from SOC_GPIO_PINS
    default -1

endmenu # MD_X1021_LCD_KWH035
```

2.在package/device/lcd/Config.in文件末尾添加：

source package/devices/lcd/x1021/"添加的lcd模块文件夹"/Config.in

>   IConfig详细说明请参考《IConfig说明文档》。

### 4.2 加入编译

.mk文件可参考以下实例：

```makefile
#-------------------------------------------------------
package_name = lcd_kwh035
package_depends = utils soc_fb
package_module_src = devices/lcd/x1021/lcd_kwh035
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_kwh035_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_kwh035_init_file = output/lcd_kwh035.sh

define lcd_kwh035_finalize_hook
    $(Q)cp devices/lcd/x1021/lcd_kwh035/lcd_kwh035.ko output/
    $(Q)echo -n 'insmod lcd_kwh035.ko' > $(lcd_kwh035_init_file)
    $(Q)echo -n ' gpio_lcd_te=$(MD_X1021_KWH035_TE)' >> $(lcd_kwh035_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X1021_KWH035_POWER_EN)' >> $(lcd_kwh035_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X1021_KWH035_RST)' >> $(lcd_kwh035_init_file)
    $(Q)echo -n ' gpio_lcd_cs=$(MD_X1021_KWH035_CS)' >> $(lcd_kwh035_init_file)
    $(Q)echo -n ' gpio_lcd_rd=$(MD_X1021_KWH035_RD)' >> $(lcd_kwh035_init_file)
    $(Q)echo  >> $(lcd_kwh035_init_file)
endef
```

在package/device/lcd/lcd.mk文件末尾添加：

package-$("新添加lcd模块对应的宏控") += package/devices/lcd/x1021/"新添加lcd模块文件夹"/".mk文件"

>   .mk文件的详细说明请参考《模块驱动添加流程》