# Camera_Sensor通用使用手册



## 1.摄像头配置方法



### 配置camera sensor (x2000_darwin gc2053为例子)

１．选择x2000_darwin的config文件, 进行配置

![1](img/1.png)

２．进入模块化驱动

![2](img/2.png)

３．进入外设配置

![3](img/3.png)

４．选择camera设备列表，并且进入配置![4](img/4.png)

５．选择sensor gc2053并进入配置                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                

![5](img/5.png)

６．可通过［单双模组选择］选择摄像头个数，若选择单模组则默认使用sensor0

![6](img/6.png)

![12](img/12.png)

７．进入sensor配置，配置好对应的引脚

![13](img/13.png)

８．紧接着回到模块化驱动，选择x2000驱动列表

![7](img/7.png)

９．进入camera驱动(vic 控制器)

![8](img/8.png)

１０．可选择数据通过isp和数据不通过isp

![9](img/9.png)

１１．如下图所示，根据实际需要进行选择

![10](img/10.png)

![11](img/11.png)

１２．若选择通过ＩＳＰ模块，则需要勾选上libisp

![14](img/14.png)



## 2.摄像头使用方法



### 2.1摄像头不经过ISP（按照上述方法配置好摄像头，选择不通过isp模块）



**特别注意：**

**在x2000系列板级上，如果选择则不通过ISP模块，camera节点为vic0(单摄) vic0 vic1(双摄)**

**双摄为两个节点即可分开控制，相当于两个独立的设备**



**Camera shell 命令：通过该命令操作camere进行工作，具体如下**

```
cmd_camera   power_on   device_path
功能:　使能 camera 设备 , 上电
参数：无
Example: 
				cmd_camera  power_on  dev/vic0
				cmd_camera  power_on  dev/vic1
```

```
cmd_camera  power_off   device_path
功能:　关闭 camera 设备 , 掉电
参数：无
Example: 
				cmd_camera  power_off  dev/vic0
				cmd_camera  power_off  dev/vic1
```

```
cmd_camera   stream_on   device_path
功能:　开始 camera 图像录制
参数：无
Example: 
				cmd_camera  stream_on  /dev/vic0
```

```
cmd_camera   stream off   device_path
功能:　结束 camera 图像录制
参数：无
Example: 
				cmd_camera  stream_off  /dev/vic0
```

```
cmd_camera   info   device_path
功能:　获取 camera 设备信息
参数：无
Example: 
				cmd_camera  info  /dev/vic0
```

```
cmd_camera    get_frame    device_path
功能:　获取一帧的数据到到标准输出
参数：无
Example: 
				cmd_camera  get_frame  /dev/vic0
```

```
cmd_camera    drop_all_frames    device_path
功能:　丢弃已录制的图像数据帧
参数：无
Example: 
				cmd_camera  drop_all_frames  /dev/vic0
```

**使用流程：**

cmd_camera  power_on  dev/vic0            									//使能设备

cmd_camera  stream_on  /dev/vic0				 							//打开图像录制

cmd_camera  get_frame  /dev/vic0	>   /tmp/frame			    //获取图片并保存



**如果想在电脑上查看图片，可以使用adb命令将图片拷贝到电脑，再用专用工具查看**

adb  pull   /tmp/frame    /tmp/picture            //将板子里/tmp/frame里的图片拷贝到电脑/tmp/picture



注意在对应sensor手册查看sensor信息，图片长和高，图片格式

![18](img/18.png)



![17](img/17.png)



使用 7yuv 工具将保存到电脑上的图片打开，并选择图片格式与图片分辨率，出图效果如下：

![20](img/20.png)



![16](img/16.png)



**当使用　cmd_camera_nv12_preview　命令时，则可在屏幕上显示摄像头所拍摄的照片**

```
cmd_camera_nv12_preview     device_path
功能:			使用屏幕预览 camera 录制的图像( nv12 格式 )
参数:			无
Example :
					cmd_camera_nv12_preview			/dev/vic0
```



### 2.2摄像头经过ISP（按照上述方法配置好摄像头，选择通过isp模块）



**特别注意：**

**在x2000系列板级上，如果选择则通过ISP模块，camera节点为mscaler0(单摄) mscaler0 mscaler1(双摄)**



**Camera shell 命令：通过该命令操作camere通过ISP工作，具体如下**

```
cmd_isp   init   device_path   [width=value] [height=value] [frame_nums] [format]
功能:　初始化摄像头为你想要的模式
Example: 
				cmd_isp init   /dev/mscaler0-ch0 
				cmd_isp init   /dev/mscaler0-ch0 width=640 height=480 frame_nums=3 format=NV12

```

```
cmd_isp   power_on   device_path
功能:　使能 camera 设备 , 上电
Example: 
				cmd_isp   power_on   /dev/mscaler0-ch0
```

```
cmd_isp   power_off   device_path
功能:　关闭 camera 设备 , 掉电
Example: 
				cmd_isp   power_off   /dev/mscaler0-ch0
```

```
cmd_isp   stream_on    device_path
功能:　开始 camera 图像录制
Example: 
				cmd_isp   stream_on   /dev/mscaler0-ch0
```

```
cmd_isp   stream_off    device_path
功能:　结束 camera 图像录制
Example: 
				cmd_isp   stream_off   /dev/mscaler0-ch0 
```

```
cmd_isp   info    device_path
功能:　获取 camera 设备信息
Example: 
				cmd_isp   info  /dev/mscaler0-ch0
```

```
cmd_isp   get_frame   device_path
功能:　获取一帧的数据到到标准输出
Example: 
				cmd_isp   get_frame   /dev/mscaler0-ch0 
```

```
cmd_isp   drop_all_frames    device_path
功能:　丢弃已录制的图像数据帧
Example: 
				cmd_isp   drop_all_frames   /dev/mscaler0-ch0
```

```
cmd_isp   deinit   device_path
功能:　取消初始化
Example: 
				cmd_isp   deinit    /dev/mscaler0-ch0
```

**使用流程：** 

cmd_isp init   /dev/mscaler0-ch0 width=640 height=480 frame_nums=3 format=NV12

**初始化isp设备　width height 为图像长和高　frame_nums 为mscaler 缓存帧个数**

**format 为设备支持格式　        NV12　NV21  YV12　JZ12　GREY    （仅支持这五种）**

cmd_isp   power_on   /dev/mscaler0-ch0　　			     　　　使能camera设备

cmd_isp   stream_on   /dev/mscaler0-ch0				 			      打开图像录制

cmd_isp   get_frame   /dev/mscaler0-ch0	>  /tmp/frame        获取图片并保存



**当使用demo_isp命令时，将自动截图到　/tmp/frame.nv12**

```
demo_isp  device_path
功能:　自动截图并保存
Example: 
				demo_isp    /dev/mscaler0-ch0
```



**当使用demo_isp_nv12_preview命令时，则可在屏幕上显示摄像头所拍摄的照片，还可延迟拍摄**

```
demo_isp_nv12_preview　device_path   [timeout]
功能:　自动截图并保存
Example: 
				demo_isp_nv12_preview  /dev/mscaler0-ch0
				demo_isp_nv12_preview  /dev/mscaler0-ch0     100
```

​    

**如果想在电脑上查看图片，可以使用adb命令将图片拷贝到电脑，再用专用工具查看**

adb  pull   /tmp/frame    /tmp/picture            //将板子里/tmp/frame里的图片拷贝到电脑/tmp/picture



根据自己设置的图片尺寸与图片格式信息，例如：

​								width=640 height=480 frame_nums=3 format=NV12



使用 7yuv 工具将保存到电脑上的图片打开，并选择图片格式与图片分辨率，出图效果如下：



![15](img/20.png)







## 3.摄像头接口剖析



### 3.1摄像头经过ISP(具体参考libisp下的isp.c)



**接口结构体讲解**

```c
摄像头信息结构体
    
struct camera_info {
    char name[16];																						    //摄像头名称
    unsigned int width;                                                                        			//每行像素数
    unsigned int height;																				//行数
    unsigned int frame_period_us;														   //camera 每帧时长,单位微秒
    camera_data_fmt data_fmt; 																//camera 帧数据格式
    
    unsigned int line_length; 																		//一行的长度,单位字节
    unsigned int frame_size; 																		//一帧数据经过对齐之后的大小
    unsigned int frame_nums;                     												//帧缓冲总数
    unsigned long phys_mem;																	   // 帧缓冲的物理基地址
    void *mapped_mem;																				//mmap 后的帧缓冲基地址
    unsigned int align_size;																		  //帧对齐大小
};
```

```c
摄像头出图属性结构体
    
struct frame_image_format {
    unsigned int width;             														 //ISP MScaler 输出分宽
    unsigned int height;        															 //ISP MScaler 输出分高
    int pixel_format;            																 //ISP MScaler 输出格式
    
    unsigned int frame_size; 															//ISP MScaler 帧大小
    struct channel_scaler scaler; 													 //ISP MScaler 缩放属性
    struct channel_crop crop;														   //ISP MScaler 裁剪属性
    int frame_nums;																				//ISP MScaler 缓存个数
    unsigned char frame_rate_ratio;												//帧率控制: 0% ~ 100%, 精度 10%
};

其中缩放与裁剪属性结构体
    
struct channel_crop {　　　　														//裁剪属性
    int enable;
    unsigned int top;
    unsigned int left;
    unsigned int width;
    unsigned int height;
};

struct channel_scaler {																			//缩放属性
    int enable;
    unsigned int width;
    unsigned int height;
};

```



**接口函数讲解**

```c
int isp_open(const char* device_path);
功能：获得camera设备句柄
参数：
    			device_path 设备节点
返回值：
    			成功返回设备句柄,失败返回-1
```

```c
int isp_mmap(int fd, struct camera_info *info);
功能：获得输出camera信息,映射地址等
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			info camera的信息,由isp_open()函数初始化
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_close(int fd, struct camera_info *info);
功能：关闭camera设备句柄
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			info camera的信息,由isp_open()函数初始化
返回值：
    			成功返回0,失败返回-1
```

```c
int isp_power_on(int fd);
功能：使能camera设备,上电
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_power_off(int fd);
功能：关闭camera设备,掉电
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_stream_on(int fd);
功能：开始camera图像录制
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_stream_off(int fd);
功能：结束camera图像录制
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
void *isp_wait_frame(int fd);
功能：获取一帧录制的图像数据(未获取有效数据继续等待3s)
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回图像缓冲区指针,失败返回NULL
```

```c
void *isp_get_frame(int fd);
功能：获取一帧录制的图像数据(不等待)
参数：
    			fd camera设备句柄,由isp_open()函数获得
返回值：
    			成功返回图像缓冲区指针,失败返回NULL
```

```c
int isp_put_frame(int fd, void *mem);
功能：释放一帧图像缓冲区
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			mem 缓冲区指针,由isp_wait_frame()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_drop_frames(int fd, unsigned int frames);
功能：丢弃已录制的图像数据帧
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			frames 需要丢弃的帧数
返回值：
    			成功返回帧数,失败返回负数
```

```c
int isp_get_info(int fd, struct camera_info *info);
功能：获得输出camera信息,映射地址等
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			info 用于存放camera的信息,不能为NULL
返回值：
    			成功返回0,失败返回负数
```

```c
int isp_get_sensor_info(int fd, struct camera_info *info);
功能：获得输出camera未经过ISP的原始sensor信息等
参数：
    			fd camera设备句柄,由isp_open()函数获得
    			info 用于存放camera的信息,不能为NULL
返回值：
    			成功返回0,失败返回负数
```





### 3.２摄像头不经过ISP(具体参考libhardware2下的camera.c)



**接口结构体讲解**

```c
摄像头信息结构体
    
struct camera_info {
    char name[16];																						    //摄像头名称
    unsigned int width;                                                                        			//每行像素数
    unsigned int height;																				//行数
    unsigned int frame_period_us;														   //camera 每帧时长,单位微秒
    camera_data_fmt data_fmt; 																//camera 帧数据格式
    
    unsigned int line_length; 																		//一行的长度,单位字节
    unsigned int frame_size; 																		//一帧数据经过对齐之后的大小
    unsigned int frame_nums;                     												//帧缓冲总数
    unsigned long phys_mem;																	   // 帧缓冲的物理基地址
    void *mapped_mem;																				//mmap 后的帧缓冲基地址
    unsigned int align_size;																		  //帧对齐大小
};
```



**接口函数讲解**

```c
int camera_open(struct camera_info *info, const char* device_path);
功能：获得camera设备句柄
参数：
    			info 用于存放camera的信息,不能为NULL
    			device_path 设备节点
返回值：
    			成功返回设备句柄,失败返回-1
```

```c
int camera_close(int fd, struct camera_info *info);
功能：关闭camera设备句柄
参数：
    			fd camera设备句柄,由camera_open()函数获得
    			info camera的信息,由camera_open()函数初始化
返回值：
    			成功返回0,失败返回-1
```

```c
int camera_power_on(int fd);
功能：使能camera设备,上电
参数：
    			fd camera设备句柄,由camera_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int camera_power_off(int fd);
功能：关闭camera设备,掉电
参数：
    			fd camera设备句柄,由camera_open()函数获得	
返回值：
    			成功返回0,失败返回负数
```

```c
int camera_stream_on(int fd);
功能：开始camera图像录制
参数：
    			fd camera设备句柄,由camera_open()函数获得		
返回值：
    			成功返回0,失败返回负数
```

```c
int camera_stream_off(int fd);
功能：结束camera图像录制
参数：
    			fd camera设备句柄,由camera_open()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
void *camera_wait_frame(int fd);
功能：获取一帧录制的图像数据
参数：
    			fd camera设备句柄,由camera_open()函数获得		
返回值：
    			成功返回图像缓冲区指针,失败返回NULL
```

```c
int camera_put_frame(int fd, void *mem);
功能：释放一帧图像缓冲区
参数：
    			fd camera设备句柄,由camera_open()函数获得
    			mem 缓冲区指针,由camera_wait_frame()函数获得
返回值：
    			成功返回0,失败返回负数
```

```c
int camera_drop_frames(int fd, unsigned int frames);
功能：丢弃已录制的图像数据帧
参数：
    			fd camera设备句柄,由camera_open()函数获得
    			frames 需要丢弃的帧数
返回值：
    			成功返回帧数,失败返回负数
```

