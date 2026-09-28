

# JPEG编解码驱动和应用

## 1  功能介绍

X2600平台上的 JPEG 编解码器是两个单独分开的硬件, 是完全根据JPEG/MJPEG标准设计的编解码器.

编码器 支持的格式（编码前的源图像格式）：
nv12, nv21, yuv422, yuv444, RGB, BGRA, RGBA  

解码器 支持的格式（解码后的目标图像格式）：
nv12, nv21, yuv422, yuv444, RGB, BGRA  

```
x2600设备节点
jpeg encoder 编码的设备节点：/dev/jpegenc
jpeg decoder 解码的设备节点：/dev/jpegdec   
```

以下以 x2600e_vast 开发板进行测试，主配置文件为：x2600e_vast_v20_nand_5.10_factory_defconfig


## 2  软件配置 

### 2.1 IConfigTool配置

![1](x2600e_vast_JPEG编解码驱动和应用.assets/1.png)

主配置选择：x2600e_vast_v20_nand_5.10_factory_defconfig

![2](x2600e_vast_JPEG编解码驱动和应用.assets/2.png)

kernel配置选择：x2600_vast_module_base_linux_sfc_nand_defconfig

![3](x2600e_vast_JPEG编解码驱动和应用.assets/3.png)

选择 libhardware2库

![4](x2600e_vast_JPEG编解码驱动和应用.assets/4.png)

选择 vpu(x2000/x2600)

![5](x2600e_vast_JPEG编解码驱动和应用.assets/5.png)

 选择 x2600 jpeg编码选项 (对应的源文件:`libhardware2/src/lib/jpeg/jpegenc.c`)  
 选择 x2600 jpeg解码选项 (对应的源文件:`libhardware2/src/lib/jpeg/jpegdec.c`)  
 源文件会自动编译进 `libhardware2.so`的库文件里，用户编程时包含头文件即可：    
`#include <libhardware2/jpege_encode.h>`  
`#include <libhardware2/jpegd_decode.h>`


![6](x2600e_vast_JPEG编解码驱动和应用.assets/6.png)

选择 x2600 jpeg编码命令 (对应的源文件:`libhardware2/src/cmds/jpege_encode_main.c`)  
用户即可在命令行使用 `cmd_jpege_encode` 命令进行测试

![7](x2600e_vast_JPEG编解码驱动和应用.assets/7.png)

选择x2600 jpeg解码命令 (对应的源文件:`libhardware2/src/lib/cmds/jpegd_decode_main.c`)  
用户即可在命令行使用 `cmd_jpegd_decode` 命令进行测试

![8](x2600e_vast_JPEG编解码驱动和应用.assets/8.png)

选择 rmem（预留内存管理） (上述的`cmd`命令测试, 会用到 rmem 这个接口)


### 2.2 kernel 配置

kernel 的配置

```c
build$ grep -nr "kernel" configs/x2600e_vast_v20_nand_5.10_factory_defconfig
6:APP_kernel_dir=../kernel/kernel
7:APP_kernel_config=x2600_vast_module_base_linux_sfc_nand_defconfig
```

```c
kernel/kernel$ make menuconfig
```

![9](x2600e_vast_JPEG编解码驱动和应用.assets/9.png)

![10](x2600e_vast_JPEG编解码驱动和应用.assets/10.png)

选择 JZ jpeg unit

![11](x2600e_vast_JPEG编解码驱动和应用.assets/11.png)

dts配置

```c
kernel/kernel$ grep -nr "dts" arch/mips/configs/x2600_vast_module_base_linux_sfc_nand_defconfig
242:CONFIG_DT_X2600_MODULE_BASE_DTS_FILE="x2600_vast_module_base.dts"
```

![12](x2600e_vast_JPEG编解码驱动和应用.assets/12.png)



打开  kernel/kernel$ gedit ./module_drivers/dts/x2600_vast_module_base.dts & 文件，添加如下内容：

```c
/*
    reserved-memory 大小说明：（需要的最小值）
    解码使用的内存大小， 使用libhardware2的解码接口：(felix->max_frames_buffer + 1) * frame_size

	例如：x2600 vast 开发板， 1080p视频的解码，需要预留的内存大小：
	dec_mem:
         frame_size = 1920 * 1080 * 3 / 2 = 3110400;
         felix->max_frames_buffer = 6（默认等于6，可以通过修改模块参数的值来修改,例如：
                                       echo 3 > /sys/module/ingenic_felix/parameters/max_frame_buffers)
         dec_mem = max_frames_buffer + 1 * frame_size
                     = (6 + 1) * 3110400
                     = 20.8M
		同理算出720p需要内存为 (6 + 1) * 1280 * 720 * 3 / 2 = 9.2M
        ps:
          1.计算出来的内存为极限值，需要多预留
          2.此为使用libhardware2 接口内存计算方式，若使用ffmpeg以ffmpeg实际使用为准
*/

/ {
	compatible = "ingenic,x2600";
	/*chosen {
		bootargs = "console=ttyS0,115200 mem=128M@0x0ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw flashtype=nand";
	};*/

    reserved-memory {
		#address-cells = <1>;
		#size-cells = <1>;
		ranges =<>;

		/*
			reserved_mem@0x5000000: 对应预留内存地址起始位置为0x5000000
			reg = <0x05000000 0x2000000>: 分别对应起始位置为0x05000000, 大小为0x2000000
			配置的凭据以此芯片x2600为例, 内存总大小为128M, 即总大小为0x8000000
			x2600通用对应的uboot配置里设置了RMEM_MB=16, 因此总地址的后16M会被RMEM占用, 则剩余内存大小为0x7000000
			由此当我们分配32M预留内存给reserved_memory时, 起始地址则为0x7000000-0x2000000 = 0x5000000, 大小为0x2000000
			如果需要改变此区域大小, 需要同时修改起始地址和大小(reserved_mem@0x5000000 和 reg = <0x05000000 0x2000000> 的地址需要一起修改)
			需要用到该地址申请内存的驱动, 需要加上 memory-region=<&reserved_memory>; 详细参考下面 felix 等的用法
		*/
		reserved_memory: reserved_mem@0x5000000{
			compatible = "shared-dma-pool";
			reg = <0x05000000 0x2000000>;
		};
    };
};

/* 加上 memory-region=<&reserved_memory>; 表示会用上方设置的预留内存地址和大小作为内存申请 */
&jpege {
	status = "okay";
	memory-region=<&reserved_memory>;
};

&jpegd {
	status = "okay";
	memory-region=<&reserved_memory>;
};

&felix {
	status = "okay";
	memory-region=<&reserved_memory>;
};

```

保存kernel配置

```c
kernel/kernel$ cp .config arch/mips/configs/x2600_vast_module_base_linux_sfc_nand_defconfig
```

## 3 编译和烧录

执行上面的操作以后，执行如下命令进行编译

```c
build$ make clean

build$ make x2600e_vast_v20_nand_5.10_factory_defconfig

build$ make 
```

把编译好的文件系统重新烧录到设备.

## 4 API接口详解

### 4.1 jpeg编解码器包含头文件
```c
#include <libhardware2/jpege_encode.h>  编码用到的接口和结构体说明

#include <libhardware2/jpegd_decode.h>  解码用到的接口和结构体说明
```

### 4.2  结构体、函数详解
jpege_encode.h
```c
/* 编码前传入的相关参数 */
struct jpeg_encoder_config {
    unsigned int file_size;  // 图像的源文件大小
    int qa;                  // 压缩等级
    int width;               // 图像的宽
    int height;              // 图像的高
    IMPP_PIX_FMT in_fmt;     // 编码前图像源格式
    void *input_mem;         // 待编码的源数据地址
};

/* 编码后传出的相关数据 */
struct jpeg_encoder_output_data{
    void *mem;               // 编码后的目标数据地址
    int image_size;          // 编码后的目标数据大小
    unsigned long phy_mem;   // dst_buf物理地址
    int buff_size;           // dst_buff的大小

};


/* 将jpeg encoder作为设备打开，返回设备描述符 */ 
struct jpeg_encoder *jpeg_encoder_open(void);
/* 关闭jpeg encoder */
void jpeg_encoder_close(struct jpeg_encoder *encoder);

/**
 * @brief 获取编码产生的数据
 * @param encoder encoder设备描述符
 * @param input   编码前传入的相关参数
 * @param output  编码后传出的相关数据
 * @return 成功返回0,失败返回-1
 */
int jpeg_encoder_get(struct jpeg_encoder *encoder, struct jpeg_encoder_config *input, struct jpeg_encoder_output_data *output);
/**
 * @brief 释放编码产生的数据
 * @param encoder encoder设备描述符
 * @param output  编码后传出的相关数据
 * @return 无返回值
 */
void jpeg_encoder_put(struct jpeg_encoder *encoder , struct jpeg_encoder_output_data *output);
```
jpegd_decode.h
```c
/* 解码前传入的相关参数 */
struct jpeg_decoder_config {
    unsigned int file_size;  // jpg图片文件大小
    int width;               // jpg图片的宽
    int height;              // jpg图片的高
    IMPP_PIX_FMT out_fmt;    // 解码后图像目标格式
    void *input_mem;         // 待解码的源数据地址
};

/* 解码后传出的相关数据 */
struct jpeg_decoder_output_data{
    void *mem;               // 解码后的目标数据地址
    int image_size;          // 解码后的目标数据大小
    int image_width;         // 解码后图像的宽
    int image_height;        // 解码后图像的高
    unsigned long phy_mem;   // dst_buf物理地址
    int buff_size;           // dst_buff的大小
};



/* 将jpeg decoder作为设备打开，返回设备描述符 */
struct jpeg_decoder *jpeg_decoder_open(void);
/* 关闭jpeg decoder */
void jpeg_decoder_close(struct jpeg_decoder *decoder);

/**
 * @brief 获取解码产生的数据
 * @param decoder decoder设备描述符
 * @param input   解码前传入的相关参数
 * @param output  解码后传出的相关数据
 * @return 成功返回0,失败返回-1
 */
int jpeg_decoder_get(struct jpeg_decoder *decoder, struct jpeg_decoder_config *input, struct jpeg_decoder_output_data *output);
/**
 * @brief 释放解码产生的数据
 * @param decoder decoder设备描述符
 * @param output  解码后传出的相关数据
 * @return 无返回值
 */
void jpeg_decoder_put(struct jpeg_decoder *decoder , struct jpeg_decoder_output_data *output);
```



## 5 命令测试

### 5.1 使用`cmd_jpege_encode`测试编码功能（此命令只能测试源图像格式为nv12的）
#### 5.1.1 命令使用说明
```shell
#命令使用
cmd_jpege_encode <file_path> <width> <height>

#参数说明
    file_path          : #待编码图像
    width              : #待编码图像的宽
    height             : #待编码图像的高

#Example: 
cmd_jpege_encode picture 1280 720
```
#### 5.1.2 测试
可以将原图 logo_496_280_nv12.yuv(nv12格式)  通过adb push 到设备的/usr/data/目录下  
在/usr/data/下执行

```shell
cmd_jpege_encode logo_496_280_nv12.yuv 496 280
```
编码生成的文件保存在 /usr/data/test.jpg, 可以通过adb pull下来后直接打开查看
#### 5.1.3 效果图
原图 (使用7yuv软件查看)
![13](x2600e_vast_JPEG编解码驱动和应用.assets/13.png)
效果图
![14](x2600e_vast_JPEG编解码驱动和应用.assets/14.png)

### 5.2 使用`cmd_jpegd_decode`测试解码功能（此命令只能测试目标图像格式为nv12的）
#### 5.2.1 命令使用说明
```shell
#命令使用
cmd_jpegd_decode <file_path> <width> <height>

#参数说明
    file_path           : #待解码图像
    width               : #待解码图像的宽
    height              : #待解码图像的高

#Example: 
cmd_jpegd_decode test.jpg 1280 720
```
#### 5.2.2 测试
 可以将文件 logo.jpg 通过adb push到设备的/usr/data/目录下  
*在/usr/data/下执行*

```shell
cmd_jpegd_decode logo.jpg 496 280
```
解码生成的原图保存在 /usr/data/picture, 可以通过adb pull下来后用7yuv软件打开查看

#### 5.2.3 效果图
原图：

![15](x2600e_vast_JPEG编解码驱动和应用.assets/15.png)



效果图 (使用7yuv软件查看)：

![16](x2600e_vast_JPEG编解码驱动和应用.assets/16.png)
