# ISP_Camera 使用流程

ISP_Camera 需配合 ISP_demo 使用



## 1.ISP_Camera 配置

### 1.1.选择相应的板级

![1](img/1.png)

![16](img/16.png)

![15](img/15.png)

1.将 uboot 默认配置 x1830_new_xImage_sfc_nand 改成 x1830_new_xImage_video_sfc_nand

2.将 kernel 默认配置 PD_x1830_sfcnand_base_defconfig 改成 PD_x1830_sfcnand_isp_defconfig

`注意：其他板级配置也是这样改`

`uboot 使用 x1xxx_new_xImage_video_sfc_nand 或者 x1xxx_new_xImage_video_sfc_nor`

`kernel 使用 PD_x1xxx_sfcnand_isp_defconfig 或者 PD_x1xxx_sfcnor_isp_defconfig`

### 1.2.配置isp_camera

![2](img/2.png)

![3](img/3.png)

![4](img/4.png)

注意：选择isp camera设备列表，不能选择camera设备列表，camera设备列表是不带isp的驱动，它们无法同时使用

![5](img/5.png)

![6](img/6.png)



## 2.ISP_demo 配置

1.打开 libimpf/src/cmds/sensor_info.h 文件，选择你使用的 sensor的宏，sensor 相对应的宏都在该文件中。

![10](img/10.png)

![11](img/11.png)



2.打开 Iconfigtool 工具，选择使用 isp_demo

![7](img/7.png)

![8](img/8.png)

![9](img/9.png)



## 3.ISP_demo 使用方法

1.进入到开发板终端，运行相应的 demo，demo 生成的文件在 /tmp 目录下

![12](img/12.png)



## 4.相关注意事项

1.进入 isp camera 设备列表配置，不能配置camera设备列表，camera 设备列表是不带 isp 的 camera 设备列表，不能同时配置使用

![14](img/14.png)

2.不能勾选 camera 驱动，这是不带 isp 的 camera 驱动

![13](img/13.png)