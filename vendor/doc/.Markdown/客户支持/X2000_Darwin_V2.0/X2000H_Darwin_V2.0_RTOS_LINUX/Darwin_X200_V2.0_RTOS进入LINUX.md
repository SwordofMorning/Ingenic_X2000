# Darwin_X2000_V2.0 RTOS进入LINUX

## 1  功能介绍

开机进入uboot,引导启动RTOS系统，在RTOS系统中运行开机动画，然后从RTOS系统中进入到Linux系统。

以下以Darwin_X2000_V2.0开发板为例进行说明。 

RTOS系统使用的配置文件为：x2000_boot_logo_example_defconfig

Linux系统使用的配置文件为：x2000_darwin_factory_defconfig 




## 2  RTOS 配置和编译

### 2.1 RTOS 软件配置

 **IConfigTool配置**



![2023-08-28_15-45](Darwin_X200_V2.0_RTOS进入LINUX.assets/01.png)

选择主配置文件：x2000_boot_logo_example_defconfig

![2023-08-28_15-44](Darwin_X200_V2.0_RTOS进入LINUX.assets/02.png)



![2023-08-28_15-47](Darwin_X200_V2.0_RTOS进入LINUX.assets/03.png)



![2023-09-04_15-28](Darwin_X200_V2.0_RTOS进入LINUX.assets/04.png)

配置lcd 驱动相关的引脚

![2023-09-11_10-36](Darwin_X200_V2.0_RTOS进入LINUX.assets/05.png)



配置fb驱动



![2023-09-04_15-29](Darwin_X200_V2.0_RTOS进入LINUX.assets/06.png)

配置背光相关的引脚

![2023-09-04_15-32](Darwin_X200_V2.0_RTOS进入LINUX.assets/07.png)











![2023-08-28_16-44](Darwin_X200_V2.0_RTOS进入LINUX.assets/08.png)



点击Yes保存配置。



### 2.2  RTOS 修改 vendor.c 文件

rtos系统启动以后，会从vendor.c作为入口运行相关的应用逻辑。vendor.c文件的内容主要是启动一个开机动画，然后退出rtos系统，再进入linux系统。具体vendor.c 的实现如下所示。

修改 freertos/vendor/vendor.c 文件，内容如下：

```c
#include <stdio.h>
#include <common.h>
#include <driver/fb.h>
#include <driver/backlight.h>
#include <os.h>


#include <stdio.h>
#include <cmyk_to_rgb.h>
#include <string.h>
#include <stdlib.h>
#include "jpeglib.h"
#include "jerror.h"

#include <driver/fb.h>
#include <driver/irq.h>
#include <cpu/cpu.h>
#include <driver/backlight.h>
#include <os.h>

#include <include_bin.h>
INCBIN(jpeg_display, "example/resource/test.jpeg");  //显示的jpeg图片路径

static struct fb_info info;
static struct fb_handle *fb0_handle;

/* 初始化fb */
static void enbale_fb_thread(void *data)
{
    fb_enable(fb0_handle);
}

//显示jpeg图片到lcd屏幕上
int jpeg_display_to_fb(void)
{
    int brightness;
    struct backlight *lcd_pwm;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;
    
    fb0_handle = fb_open("fb0");
    if(fb0_handle == NULL)
        printf("fb0 = NULL\n");
    
    fb_get_info(fb0_handle, &info);
    if (info.fb_fmt != fb_fmt_RGB888 && info.fb_fmt != fb_fmt_ARGB8888 ) {
        printf("jpeg_display_to_fb: this just support rgb888! you should change this demo\n");
        return -1;
    }
    
    // 开启线程
    thread_create("enbale_fb_thread", 1024, enbale_fb_thread, NULL);
    
    // 绑定标准错误处理结构
    cinfo.err = jpeg_std_error(&jerr);
    
    // 初始化JPEG对象
    jpeg_create_decompress(&cinfo);
    
    jpeg_mem_src(&cinfo,jpeg_displayData, jpeg_displaySize);
    
    // 读取图像信息
    (void) jpeg_read_header(&cinfo, TRUE);
    
    // 设定解压缩参数，此处我们将图像长宽缩小为原图的1/2，目前支持1/1,1/2,1/4,1/8
    cinfo.scale_num=1;
    cinfo.scale_denom=1;
    
    // 开始解压缩图像
    (void) jpeg_start_decompress(&cinfo);
    
    // 分配缓冲区空间
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    
    int display_width = min(cinfo.output_width, info.xres);
    int display_height = min(cinfo.output_height, info.yres);
    
    int fb_x_offset = (info.xres - display_width) / 2;
    int fb_y_offset = (info.yres - display_height) / 2;
    
    int jpeg_x_offset = (cinfo.output_width - display_width) / 2;
    int jpeg_y_offset = (cinfo.output_height - display_height) / 2;
    
    unsigned char *jpeg_rgb_buffer = NULL;
    if (cinfo.out_color_space == JCS_CMYK) {
        jpeg_rgb_buffer = (unsigned char *)malloc(row_stride * sizeof(unsigned char));
    }
    
    // 定位到fb应该显示的第一行
    unsigned int *fb_mem = info.fb_mem + info.bytes_per_line * fb_y_offset;
    
    // 定位到jpeg应该显示的第一行（即跳过图片不显示的上半部分）
    int w,h;
    for (h = 0; h < jpeg_y_offset; h++) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
    }
    
    // 逐行解码jpeg图片，并拷贝到fb
    for (h = 0; h < display_height; h++) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        unsigned char *jpeg_rgb = buffer[0];
    
        if (cinfo.out_color_space == JCS_CMYK) {
            cmyk_to_rgb24((unsigned int *)buffer[0], jpeg_rgb_buffer, row_stride, row_stride, cinfo.output_width, 1);
            jpeg_rgb = jpeg_rgb_buffer;
        }
    
        unsigned int *fb_rgb = fb_mem;
        fb_rgb += fb_x_offset;
        jpeg_rgb += jpeg_x_offset * 3;
    
        for (w = 0; w < display_width; w++) {
            unsigned int r = jpeg_rgb[w*3 + 0];
            unsigned int g = jpeg_rgb[w*3 + 1];
            unsigned int b = jpeg_rgb[w*3 + 2];
    
            fb_rgb[w] = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
        }
    
        fb_mem = (void *)fb_mem + info.bytes_per_line;
    }
    
    // 将jpeg解码的行定位到最后，否则jpeg库会报错退出
    cinfo.output_scanline = cinfo.output_height;
    
    // 等待 fb 开启
    while (!fb_is_enable(fb0_handle)) {
        usleep(300);
    }
    
    // 显示
    fb_pan_display(fb0_handle, 0);
    
    // 开启屏幕背光
    lcd_pwm = backlight_open("backlight_gpio0");
    if (lcd_pwm == NULL) {
        printf("backlight_open fail.\n");
    }
    
    brightness = backlight_get_maxbrightness(lcd_pwm);
    printf("max brightness = %d\n", brightness);
    
    backlight_set_brightness(lcd_pwm, brightness);
    
    brightness = backlight_get_brightness(lcd_pwm);
    printf("brightness = %d\n", brightness);
    
    // 结束解压缩操作
    (void) jpeg_finish_decompress(&cinfo);
    
    // 释放资源
    if (cinfo.out_color_space == JCS_CMYK) {
        free(jpeg_rgb_buffer);
    }
    jpeg_destroy_decompress(&cinfo);
    
    return 0;

}


/*************************************************************/
//开机log显示完成以后，释放所有的rtos资源
static void release_all_resources(void)
{
    // lcd
    disable_irq(IRQ_LCD);
    release_irq(IRQ_LCD);

    // intc
    arch_deinit_cpu();
    disable_irq(IRQ_V_IP2);
    release_irq(IRQ_V_IP2);
    
    // ost
    disable_irq(IRQ_V_IP4);
    release_irq(IRQ_V_IP4);

}

#ifdef CONFIG_XBURST
extern void jump_to_uboot(void);
#endif

//显示开机logo
static void show_logo(void *data)
{
    jpeg_display_to_fb();

    release_all_resources();

#ifdef CONFIG_XBURST
    jump_to_uboot(); //xbust1 系列 rtos for uboot 需调用该函数
#endif

#ifdef CONFIG_XBURST2   //x2000 系列是xburst2 ,调用这个函数 arch_shutdow_current_cpu()
    arch_shutdow_current_cpu();
#endif
}


void quick_boot_logo_for_kernel(void)
{
    //启动线程显示开机logo
    thread_create("load kernel thread", 8192, show_logo, NULL);
}


//程序执行的入口
void vendor_init(void)
{
    //fb_test();   //fb_test()  该函数是为了测试lcd显示的简单逻辑，为的是保证lcd显示正常，这样可以保证开机动画的显示硬件是正常的
    printf("vendor init...\n");
    
    //该函数是在rtos中启动开机动画，然后再进入到linux系统
    quick_boot_logo_for_kernel();

}
//测试lcd屏幕，比如写颜色彩条，保证lcd硬件没问题
void fb_test(void)
{
    struct fb_info fb_info;
    struct fb_handle *fb;
    struct backlight *lcd_pwm;
    lcd_pwm = backlight_open("backlight_gpio0");
    if (lcd_pwm == NULL)
        printf("backlight_open fail.\n");
    else
        backlight_set_brightness(lcd_pwm, lcd_pwm->max_brightness);

    fb = fb_open("fb0");
    
    if (fb == NULL) {
        printf("open fb0 error!\n");
        return;
    }
    
    fb_enable(fb);
    fb_get_info(fb, &fb_info);
    
    if (fb_info.fb_fmt != fb_fmt_RGB888 && fb_info.fb_fmt != fb_fmt_ARGB8888 ) {
        printf("fb_test: this just support rgb888! you should change this demo\n");
        return;
    }
    
    int i, j;
    unsigned int *p = fb_info.fb_mem;
    
    for (j = 0; j < fb_info.yres; j++) {
        for (i = 0; i < fb_info.xres; i++) {
            *p++ = 0xffff0000;
        }
    }
    
    fb_pan_display(fb, 0);
    msleep(300);
    
    p = fb_info.fb_mem;
    for (j = 0; j < fb_info.yres; j++) {
        for (i = 0; i < fb_info.xres; i++) {
            *p++ = 0xff00ff00;
        }
    }
    
    fb_pan_display(fb, 0);
    msleep(300);
    
    p = fb_info.fb_mem;
    for (j = 0; j < fb_info.yres; j++) {
        for (i = 0; i < fb_info.xres; i++) {
            *p++ = 0xff0000ff;
        }
    }
    
    fb_pan_display(fb, 0);
    msleep(300);

}
```



### 2.3  RTOS 编译

```c
cd freertos
    
source build/envsetup.sh

make x2000_boot_logo_example_defconfig

make
```



生成固件：freertos$ ls zero.bin




## 3  Linux 配置和编译

### 3.1 Linux 软件配置

![2023-08-28_17-53](Darwin_X200_V2.0_RTOS进入LINUX.assets/09.png)

主配置选择：x2000_darwin_factory_defconfig



![2023-08-28_17-53_1](Darwin_X200_V2.0_RTOS进入LINUX.assets/10.png)

uboot配置选择：x2000_base_rtos_xImage_sfc_nand ，添加 SPL_RTOS_BOOT   RTOS_BOOT_ON_SECOND_CPU 标志，内容如下：

```c
bootloader/uboot-x2000$ grep -nr "x2000_base_rtos_xImage_sfc_nand" boards.cfg
708:x2000_base_rtos_xImage_sfc_nand     mips        xburst2    x2000_base   ingenic    x2000_v12   x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_RTOS_BOOT,SPL_OS_BOOT,RTOS_BOOT_ON_SECOND_CPU,RMEM_MB=32,SPL_PARAMS_FIXER
```



![2023-08-29_09-13](Darwin_X200_V2.0_RTOS进入LINUX.assets/11.png)

按 Ctrl + S 进行配置保存，点击Yes保存配置。



### 3.2  Linux  编译

编译命令如下：



```c
build$ make clean

build$ make x2000_darwin_factory_defconfig

build$ make
```

生成的固件路下：

```c
build$ ls output/ -l
total 144216
-rw-r--r-- 1 kenny kenny 58253312 8月  25 14:57 rootfs.squashfs
-rw-r--r-- 1 kenny kenny 74407936 8月  25 14:57 rootfs.ubifs
-rw-rw-r-- 1 kenny kenny    24576 8月  25 17:10 u-boot-spl-pad.bin
-rw-rw-r-- 1 kenny kenny 11173888 8月  25 14:57 userdata.ubifs
-rw-rw-r-- 1 kenny kenny  3813440 8月  25 14:57 xImage
```



## 4 烧录固件

执行上面的操作以后，进行烧录。烧录工具配置如下：

![2023-08-29_15-28](Darwin_X200_V2.0_RTOS进入LINUX.assets/12.png)

Darwin_X2000_V2.0开发板选择 x2000h_sfc_nand_lpddr3_linux.cfg 板级



![2023-09-05_17-35](Darwin_X200_V2.0_RTOS进入LINUX.assets/13.png)

标志1中 label不能随意修改，标志2中 offset要跟SFC中分区信息保持一致

attribute中选择的文件如下：

uboot选择linux的 u-boot-spl-pad.bin

kernel选择linux的xImage

rootfs选择linux的rootfs.squashfs

rots选择rtos的zero.bin

userdata选择linux的userdata.ubifs (userdata.ubifs 这个根据情况可以选择烧录或者不烧录)

![2023-09-05_17-41](Darwin_X200_V2.0_RTOS进入LINUX.assets/14.png)

类型和配置默认选择即可。



![2023-09-05_17-45](Darwin_X200_V2.0_RTOS进入LINUX.assets/15.png)

第一次烧录需要勾选全部擦除。

![2023-08-29_16-09](Darwin_X200_V2.0_RTOS进入LINUX.assets/16.png)

Partition name不能修改。Manage mode 注意 userdata可读可写选择UBI_MANAGER,其它只读分区选择MTD_MODE

注意：rtos的分区信息要放在rootfs后面，不然会导致无法进入到linux系统。



## 5 RTOS进入Linux系统

烧录固件以后，启动系统。

通过uboot引导，先启动rtos系统，退出rtos系统以后，进入Linux系统。具体串口信息如下。在rtos系统中显示开机动画。

```c
U-Boot SPL 2013.07-00032-ga0524e43b-dirty (Aug 25 2023 - 17:10:20)
ERROR EPC 92403594
Current Version: V2
CPA_CPAPCR:0310086d
CPM_CPMPCR:07c0484d
CPM_CPEPCR:0310186d
CPM_CPCCR:9a094410
DDR: NK6CL256M16DKX-H1 type is : LPDDR3
DDRP_INNOPHY_CALIB_DELAY_AL:00000051
DDRP_INNOPHY_CALIB_DELAY_AH:00000051
-----ddr_readl(DDRP_INNOPHY_CALIB_DONE): 00000003
358, VID=0x0000009b, PID=0x00000012
[0.000000] xburst2 rtos @ Aug 25 2023 16:59:50, epc: 8600414c
[0.044012] vendor init...
[0.612413] max brightness = 1
[0.612612] brightness = 1
Uncompressing Linux...
Ok, booting the kernel.
[    0.000000] Linux version 4.4.94+ (kenny@kenny-computer) (gcc version 7.2.0 (Ingenic Linux-Release5.1.8-Default_xburst2_glibc2.29 Optimize: jr base on 5.1.6 2023.07-18 08:46:58) ) #628 SMP PREEMPT Fri Aug 25 14:57:00 CST 2023
[    0.000000] CPU0 RESET ERROR PC:92403594
[    0.000000] bootconsole [early0] enabled
[    0.000000] CPU0 revision is: 00132000 (Ingenic XBurst@II.V2)
[    0.000000] FPU revision is: 00f32000
[    0.000000] MSA revision is: 00002000
[    0.000000] MIPS: machine is ingenic,x2000_module_base
[    0.000000] Determined physical RAM map:
[    0.000000]  memory: 007aa000 @ 00010000 (usable)
[    0.000000]  memory: 00046000 @ 007ba000 (usable after init)
[    0.000000] User-defined physical RAM map:
[    0.000000]  memory: 0e000000 @ 00000000 (usable)
[    0.000000]  memory: 10000000 @ 30000000 (usable)
[    0.000000] Zone ranges:
[    0.000000]   Normal   [mem 0x0000000000000000-0x000000001fffffff]
[    0.000000]   HighMem  [mem 0x0000000020000000-0x000000003fffffff]
[    0.000000] Movable zone start for each node
[    0.000000] Early memory node ranges
[    0.000000]   node   0: [mem 0x0000000000000000-0x000000000dffffff]
[    0.000000]   node   0: [mem 0x0000000030000000-0x000000003fffffff]
[    0.000000] Initmem setup node 0 [mem 0x0000000000000000-0x000000003fffffff]
[    0.000000] [SMP] Slave CPU(s) 1 available.
[    0.000000] Primary instruction cache 32kB, VIPT, 8-way, linesize 32 bytes.
[    0.000000] Primary data cache 32kB, 8-way, VIPT, no aliases, linesize 32 bytes
[    0.000000] =======found ...... ingenic sc cache ops ...!, found: 1
[    0.000000] 
[    0.000000] Unified secondary cache 512kB 16-way, linesize 64 bytes.
[    0.000000] PERCPU: Embedded 10 pages/cpu @81826000 s8464 r8192 d24304 u40960
[    0.000000] Built 1 zonelists in Zone order, mobility grouping on.  Total pages: 121856
[    0.000000] Kernel command line: console=ttyS2,3000000n8  mem=224M@0x0 rmem=32M@0xe000000 mem=256M@0x30000000  init=/linuxrc flashtype=nand  root=/dev/mtdblock_bbt_ro2 rootfstype=squashfs ro 
[    0.000000] PID hash table entries: 1024 (order: 0, 4096 bytes)
[    0.000000] Dentry cache hash table entries: 32768 (order: 5, 131072 bytes)
[    0.000000] Inode-cache hash table entries: 16384 (order: 4, 65536 bytes)
[    0.000000] Memory: 474328K/491520K available (5820K kernel code, 314K rwdata, 1704K rodata, 280K init, 371K bss, 17192K reserved, 0K cma-reserved, 262144K highmem)
[    0.000000] SLUB: HWalign=32, Order=0-3, MinObjects=0, CPUs=2, Nodes=1
[    0.000000] Preemptible hierarchical RCU implementation.
[    0.000000] 	Build-time adjustment of leaf fanout to 32.
[    0.000000] NR_IRQS:411
[    0.000000] parse cpu-intc-iomap, intc define in dt is too large!
[    0.000000] core irq setup finished
[    0.000000] percpu irq inited.
[    0.000000] x2000 Clock Power Management Unit init!
[    0.000000] =========== x2000 clocks: =============
[    0.000000] 	apll     = 1200000000 , mpll     = 1500000000, ddr = 750000000
[    0.000000] 	cpu_clk  = 1200000000 , l2c_clk  = 600000000
[    0.000000] 	ahb0_clk = 300000000 , ahb2_clk = 300000000
[    0.000000] 	apb_clk  = 150000000 , ext_clk  = 24000000
[    0.000000] 
[    0.000000] parse cpu-ost-iomap, ost number define in dt is too large!
[    0.000000] percpu cpu_num:0 timerevent init
[    0.000000] clockevents_config_and_register success.
[    0.000000] clocksource: jz_clocksource: mask: 0x7fffffffffffffff max_cycles: 0x588fe9dc0, max_idle_ns: 440795202592 ns
[    0.000000] sched_clock: 64 bits at 24MHz, resolution 41ns, wraps every 4398046511097ns
[    0.000000] Console: colour dummy device 80x25
[    0.000000] Calibrating delay loop... 2387.14 BogoMIPS (lpj=11935744)
[    0.000000] pid_max: default: 32768 minimum: 301
[    0.000000] Mount-cache hash table entries: 1024 (order: 0, 4096 bytes)
[    0.000000] Mountpoint-cache hash table entries: 1024 (order: 0, 4096 bytes)
[    0.000000] [SMP] Prepare 2 cores., cpu: 0
[    0.000000] [SMP] Booting CPU1 ...
[    0.000000] CPU1 RESET ERROR PC:86000E1C
[    0.000000] Primary instruction cache 32kB, VIPT, 8-way, linesize 32 bytes.
[    0.000000] Primary data cache 32kB, 8-way, VIPT, no aliases, linesize 32 bytes
[    0.000000] =======found ...... ingenic sc cache ops ...!, found: 1
[    0.000000] 
[    0.000000] Unified secondary cache 512kB 16-way, linesize 64 bytes.
[    0.000000] #### now starting init for cpu : 1
[    0.000000] percpu irq inited.
[    0.000000] percpu cpu_num:1 timerevent init
[    0.000000] clockevents_config_and_register success.
[    0.000000] CPU1 revision is: 00132000 (Ingenic XBurst@II.V2)
[    0.000000] FPU revision is: 00f32000
[    0.000000] MSA revision is: 00002000
[    0.000000] Brought up 2 CPUs
[    0.000000] [SMP] slave cpu1 start up finished.
[    0.000000] devtmpfs: initialized
[    0.000000] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 19112604462750000 ns
[    0.000000] futex hash table entries: 512 (order: 2, 16384 bytes)
[    0.000000] pinctrl core: initialized pinctrl subsystem
[    0.000000] NET: Registered protocol family 16
[    0.000000] ingenic pinctrl 10010000.pinctrl: 5 gpio chip add success, pins 160
[    0.000000] ingenic pinctrl 10010000.pinctrl: ingenic pinctrl probe success
[    0.000000] usbcore: registered new interface driver usbfs
[    0.000000] usbcore: registered new interface driver hub
[    0.000000] usbcore: registered new device driver usb
[    0.000000] media: Linux media interface: v0.10
[    0.000000] Linux video capture interface: v2.00
[    0.000000] pps_core: LinuxPPS API ver. 1 registered
[    0.000000] pps_core: Software ver. 5.3.6 - Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>
[    0.000000] PTP clock support registered
[    0.000000] Advanced Linux Sound Architecture Driver Initialized.
[    0.000000] ingenic-dma 13420000.dma: INGENIC SoC DMA initialized
[    0.000000] Bluetooth: Core ver 2.21
[    0.000000] NET: Registered protocol family 31
[    0.000000] Bluetooth: HCI device and connection manager initialized
[    0.000000] Bluetooth: HCI socket layer initialized
[    0.000000] Bluetooth: L2CAP socket layer initialized
[    0.000000] Bluetooth: SCO socket layer initialized
[    0.000030] clocksource: Switched to clocksource jz_clocksource
[    0.007944] NET: Registered protocol family 2
[    0.008489] TCP established hash table entries: 2048 (order: 1, 8192 bytes)
[    0.008802] TCP bind hash table entries: 2048 (order: 2, 16384 bytes)
[    0.009107] TCP: Hash tables configured (established 2048 bind 2048)
[    0.009415] UDP hash table entries: 256 (order: 1, 8192 bytes)
[    0.009679] UDP-Lite hash table entries: 256 (order: 1, 8192 bytes)
[    0.010130] NET: Registered protocol family 1
[    0.010652] RPC: Registered named UNIX socket transport module.
[    0.010903] RPC: Registered udp transport module.
[    0.011098] RPC: Registered tcp transport module.
[    0.011293] RPC: Registered tcp NFSv4.1 backchannel transport module.
[    0.027337] squashfs: version 4.0 (2009/01/31) Phillip Lougher
[    0.043904] jitterentropy: Initialization failed with host not compliant with requirements: 2
[    0.044724] bounce: pool size: 64 pages
[    0.044892] io scheduler noop registered
[    0.045059] io scheduler deadline registered
[    0.045264] io scheduler cfq registered (default)
[    0.046167] PPP generic driver version 2.4.2
[    0.046493] PPP BSD Compression module registered
[    0.046698] PPP Deflate Compression module registered
[    0.046923] PPP MPPE Compression module registered
[    0.047130] NET: Registered protocol family 24
[    0.360070] dwc2 13500000.otg_new: EPs: 9, dedicated fifos, 3576 entries in SPRAM
[    0.360606] dwc2 13500000.otg_new: DWC OTG Controller
[    0.360835] dwc2 13500000.otg_new: new USB bus registered, assigned bus number 1
[    0.361164] dwc2 13500000.otg_new: irq 9, io mem 0x13500000
[    0.361506] usb usb1: New USB device found, idVendor=1d6b, idProduct=0002
[    0.361794] usb usb1: New USB device strings: Mfr=3, Product=2, SerialNumber=1
[    0.362096] usb usb1: Product: DWC OTG Controller
[    0.362293] usb usb1: Manufacturer: Linux 4.4.94+ dwc2_hsotg
[    0.362529] usb usb1: SerialNumber: 13500000.otg_new
[    0.363239] hub 1-0:1.0: USB hub found
[    0.363427] hub 1-0:1.0: 1 port detected
[    0.364149] i2c /dev entries driver
[    0.364435] sdhci: Secure Digital Host Controller Interface driver
[    0.364697] sdhci: Copyright(c) Pierre Ossman
[    0.365144] usbcore: registered new interface driver usbhid
[    0.365379] usbhid: USB HID core driver
[    0.366450] jz-rot 13070000.rotate: device registered as /dev/video0
[    0.366726] jz-rot 13070000.rotate: Could not get reserved memory
[    0.367411] helix-venc 13200000.helix: failed to init reserved mem
[    0.367691] helix-venc 13200000.helix: encoder(helix) registered as /dev/video1
[    0.368425] felix-vdec 13300000.felix: failed to init reserved mem
[    0.368691] felix-vdec 13300000.felix: h264decoder(felix) registered as /dev/video2
[    0.369313] Enter 'CDT' mode.
[    0.369439] Enter 'DMA Descriptor chain' mode.
[    0.369634] create CDT index: 0 ~ 4,  index number:5.
[    0.369916] ingenic-sfc 13440000.sfc: id_manufactory = ff, id_device 9b
[    0.370264] ingenic-sfc 13440000.sfc: id_manufactory = 9b, id_device 12
[    0.370545] ingenic-sfc 13440000.sfc: Found Supported device, id_manufactory = 0x9b, id_device = 0x12
[    0.370928] use nand common get feature interface!
[    0.371134] create CDT index: 5 ~ 22,  index number:18.
[    0.371364] Scanning device for bad blocks
[    0.373288] random: nonblocking pool is initialized
[    0.414757] Creating 5 MTD partitions on "sfc_nand":
[    0.414970] 0x000000000000-0x000000100000 : "uboot"
[    0.426398] 0x000000100000-0x000000900000 : "kernel"
[    0.427726] 0x000000900000-0x000006900000 : "rootfs"
[    0.429092] 0x000006900000-0x000006a00000 : "rtos"
[    0.440546] 0x000006a00000-0x000007d00000 : "userdata"
[    0.452127] 10030000.serial: ttyS0 at MMIO 0x10030000 (irq = 55, base_baud = 9375000) is a uart0
[    0.452801] 10031000.serial: ttyS1 at MMIO 0x10031000 (irq = 54, base_baud = 9375000) is a uart1
[    0.453429] 10032000.serial: ttyS2 at MMIO 0x10032000 (irq = 53, base_baud = 9375000) is a uart2
[    0.453858] console [ttyS2] enabled
[    0.453858] console [ttyS2] enabled
[    0.454231] bootconsole [early0] disabled
[    0.454231] bootconsole [early0] disabled
[    0.454930] 10033000.serial: ttyS3 at MMIO 0x10033000 (irq = 52, base_baud = 9375000) is a uart3
[    0.456068] rtc-ingenic 10003000.rtc: rtc core: registered 10003000.rtc as rtc0
[    0.457643] ingenic RTC probe success 
[    0.457999] Error: Driver 'rtc-ingenic' is already registered, aborting...
[    0.458779] ingenic watchdog probe success
[    0.459202] Netfilter messages via NETLINK v0.30.
[    0.459574] ip_set: protocol 6
[    0.459921] ip_tables: (C) 2000-2006 Netfilter Core Team
[    0.460506] NET: Registered protocol family 17
[    0.460856] bridge: automatic filtering via arp/ip/ip6tables has been deprecated. Update your scripts to load br_netfilter if you need this.
[    0.461707] Bridge firewalling registered
[    0.473335] rtc-ingenic 10003000.rtc: setting system clock to 2020-03-01 18:17:08 UTC (1583086628)
[    0.474246] v4l2loopback driver version 0.12.5 loaded
[    0.477854] ALSA device list:
[    0.478056]   No soundcards found.
[    0.478704] mtd: rootfs 100663296 131072 0
[    0.480319] VFS: Mounted root (squashfs filesystem) readonly on device 50:2.
[    0.481732] devtmpfs: mounted
[    0.482068] Freeing unused kernel memory: 280K
Starting mdev... OK
[    0.700996] init enable rtc32k out
[    0.724762] md_i2c_gpio md_i2c_gpio.6: using pins 92 (SDA) and 91 (SCL)
[    0.896336] mmc0: Unknown controller version (5). You may experience problems.
[    0.896847] md_ingenic,sdhci md_ingenic,sdhci.1: No vmmc regulator found
[    0.897299] md_ingenic,sdhci md_ingenic,sdhci.1: No vqmmc regulator found
[    0.930092] mmc0: SDHCI controller on ingenic-sdhci [md_ingenic,sdhci.1] using ADMA
[    0.945580] mmc0: queuing unknown CIS tuple 0x80 (2 bytes)
[    0.947796] mmc0: queuing unknown CIS tuple 0x80 (3 bytes)
[    0.950410] mmc0: queuing unknown CIS tuple 0x80 (3 bytes)
[    0.954997] mmc0: queuing unknown CIS tuple 0x80 (7 bytes)
[    0.956584] mmc0: queuing unknown CIS tuple 0x81 (1 bytes)
[    0.968930] gt9xx_touch: unknown parameter 'gtp_version' ignored
[    0.969772] <<-GTP-INFO->> GTP driver installing....
[    0.980654] <<-GTP-INFO->> GTP Driver Version: V2.4.0.1<2016/10/26>
[    0.981066] <<-GTP-INFO->> GTP Driver Built@11:53:02, Aug 25 2023
[    0.981478] <<-GTP-INFO->> GTP I2C Address: 0x14
[    0.981851] <<-GTP-INFO->> Guitar reset
[    1.047887] mmc0: new high speed SDIO card at address 0001
[    1.166012] <<-GTP-INFO->> Chip Type: GOODIX_GT9
[    1.169596] <<-GTP-INFO->> IC Version: 911_1060
[    1.169900] <<-GTP-DEBUG->> [1379]Config Groups' Lengths: 186, 0, 0, 0, 0, 0
[    1.232299] <<-GTP-INFO->> Sensor_ID: 0
[    1.232568] <<-GTP-DEBUG->> [1428]Get config data from header file.
[    1.232983] <<-GTP-INFO->> Config group0 used,length: 186
[    1.235588] <<-GTP-DEBUG->> [1453]Config Version: 0, 0x00; IC Config Version: 65, 0x41
[    1.236110] <<-GTP-INFO->> Driver send config.
[    1.291113] <<-GTP-INFO->> X_MAX: 720, Y_MAX: 1280, TRIGGER: 0x01
[    1.310082] <<-GTP-INFO->> create proc entry gt9xx_config success
[    1.310680] input: goodix-ts as /devices/virtual/input/input0
[    1.311287] <<-GTP-INFO->> Request input device for pen/stylus.
[    1.311877] input: goodix-pen as /devices/virtual/input/input1
[    1.312461] <<-GTP-DEBUG->> [1759]INT trigger type:1
[    1.312821] <<-GTP-INFO->> GTP works in interrupt mode.
[    1.319728] Bluetooth: HCI UART driver ver 2.3
[    1.319940] <<-GTP-DEBUG->> [819]pre_touch:00, finger:80.
[    1.320451] Bluetooth: HCI UART protocol (null) registered
[    1.320810] Bluetooth: HCI Realtek H5 protocol initialized
[    1.321157] rtk_btcoex: rtk_btcoex_init: version: 1.2
[    1.321493] rtk_btcoex: create workqueue
[    1.330967] <<-GTP-DEBUG->> [819]pre_touch:00, finger:80.
[    1.341191] <<-GTP-DEBUG->> [819]pre_touch:00, finger:80.
[    1.342092] rtk_btcoex: alloc buffers 1408, 2240 for ev and l2
[    1.348502] input: jz adc keyboard as /devices/virtual/input/input2
[    1.359343] jz adc keyboard driver has been initialized successfully!
insmod: can't insert 'rtl8723ds.ko': Operation not permitted
/
Saving random seed: SKIP (read-only file system detected)
UBI device number 1, total 152 LEBs (19300352 bytes, 18.4 MiB), available 0 LEBs (0 bytes), LEB size 126976 bytes (124.0 KiB)
Starting system message bus: done
Starting network: OK
Starting bluetooth ...
killall: rtk_hciattach: no process killed
Realtek Bluetooth init uart with init speed:115200, final_speed:115200, type:HCI UART H5
[    1.547621] ERROR: wl_dis_n must set
[    1.547627] RTW: ERROR ERROR: rtk power init failed !!
[    1.638447] ERROR: wl_dis_n must set
[    1.638451] RTW: ERROR ERROR: rtk power init failed !!
[    1.737685] stmmac - user ID: 0x10, Synopsys ID: 0x37
[    1.737687]  Ring mode enabled
[    1.737692]  DMA HW capability register supported
[    1.737693]  Enhanced/Alternate descriptors
[    1.737694] 	Enabled extended descriptors
[    1.737697]  RX Checksum Offload Engine supported (type 2)
[    1.737698]  TX Checksum insertion supported
[    1.737699]  Wake-Up On Lan supported
[    1.737738]  Enable RX Mitigation via HW Watchdog Timer
[    1.753857] cmd fifo full!
[    1.755653] cmd fifo full!
[    1.759808] libphy: stmmac: probed
[    1.759815] eth%d: PHY ID 001cc816 at 0 IRQ POLL (stmmac-0:00) active
[    1.771329] cmd fifo full!
[    1.773151] cmd fifo full!
[    1.775682] ingenic-icodec-board ingenic-icodec-board.0: internal-codec <-> ingenic-aic.0 mapping ok
[    1.788433] cmd fifo full!
[    1.790364] cmd fifo full!
[    1.822022] cmd fifo full!
[    1.823825] cmd fifo full!
[    1.825890] ubi1: attaching mtd4
[    1.838974] cmd fifo full!
[    1.840818] cmd fifo full!
[    1.849138] ubi1: scanning is finished
[    1.851912] ubi1 warning: print_rsvd_warning: cannot reserve enough PEBs for bad PEB handling, reserved 2, need 20
[    1.852722] ubi1: attached mtd4 (name "userdata", size 19 MiB)
[    1.852726] ubi1: PEB size: 131072 bytes (128 KiB), LEB size: 126976 bytes
[    1.852730] ubi1: min./max. I/O unit sizes: 2048/2048, sub-page size 2048
[    1.852733] ubi1: VID header offset: 2048 (aligned 2048), data offset: 4096
[    1.852736] ubi1: good PEBs: 152, bad PEBs: 0, corrupted PEBs: 0
[    1.852739] ubi1: user volume: 1, internal volumes: 1, max. volumes count: 128
[    1.852742] ubi1: max/mean erase counter: 4/1, WL threshold: 4096, image sequence number: 0
[    1.852745] ubi1: available PEBs: 0, total reserved PEBs: 152, PEBs reserved for bad PEB handling: 2
[    1.854397] ubi1: background thread "ubi_bgt1d" started, PID 852
[    1.855341] cmd fifo full!
[    1.857164] cmd fifo full!
[    1.872142] cmd fifo full!
[    1.873961] cmd fifo full!
[    1.876185] UBIFS (ubi1:0): background thread "ubifs_bgt1_0" started, PID 859
[    1.889101] cmd fifo full!
[    1.889221] UBIFS (ubi1:0): recovery needed
[    1.890942] cmd fifo full!
[    1.905757] cmd fifo full!
[    1.907570] cmd fifo full!
[    1.918456] UBIFS (ubi1:0): recovery completed
[    1.918513] UBIFS (ubi1:0): UBIFS: mounted UBI device 1, volume 0, name "userdata"
[    1.918518] UBIFS (ubi1:0): LEB size: 126976 bytes (124 KiB), min./max. I/O unit sizes: 2048 bytes/2048 bytes
[    1.918523] UBIFS (ubi1:0): FS size: 17141760 bytes (16 MiB, 135 LEBs), journal size 9023488 bytes (8 MiB, 72 LEBs)
[    1.918526] UBIFS (ubi1:0): reserved for root: 0 bytes (0 KiB)
[    1.918533] UBIFS (ubi1:0): media format: w4/r0 (latest is w4/r0), UUID 03FDB177-65C5-4282-B7D6-F1DCCF9F9E51, small LPT model
[    1.920981] UBIFS (ubi1:0): full atime support is enabled.
[    1.922999] cmd fifo full!
[    1.924822] cmd fifo full!
[    1.940324] cmd fifo full!
[    1.942139] cmd fifo full!
[    1.956986] cmd fifo full!
[    1.958795] cmd fifo full!
bluetoothd[914]: Bluetooth daemon 5.54
bluetoothd[914]: Starting SDP server
bluetoothd[914]: kernel lacks bnep-protocol support
bluetoothd[914]: System does not support network plugin
bluetoothd[914]: Bluetooth management interface 1.10 initialized
ifconfig: SIOCGIFFLAGS: No such device
killall: adbd: no process killed
[    5.335993] file system registered
Starting adb ...
max_len = 1048576
[    5.383582] drivers/char/jz_spinand_firmware.c license_read_config 228: flash don't save license value!
[    5.384222] drivers/char/jz_spinand_firmware.c fmw_ioctl 369:license get config failed, ret = 0
pMacMgr->data_get_length(), len - prefix_len: -20
running autorun.sh
autorun.sh end.
install_listener('tcp:5037','*smartsocket*')
[    5.423962] read descriptors
[    5.424170] read strings
[    5.432838] dwc2 13500000.otg_new: bound driver configfs-gadget
[    5.908681] dwc2 13500000.otg_new: new address 4
[    5.935370] configfs-gadget gadget: high-speed config #1: c
Realtek Bluetooth ERROR: H5 sync timed out
```

