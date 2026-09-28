# IConfigTool 使用文档

本文档详细介绍 IConfigTool 的使用方法

## 1.IConfigTool 工具介绍

>   IConfigTool 是基于 Qt 的 Gui 界面
>
>   IConfigTool 不包含 uboot、kernel、buildroot 的相关配置，kernel、bulidroot 的相关配置可通过 menuconfig 配置
>
>   IConfigTool 主要用于模块化驱动，无线设备的配置
>
>   IConfigTool 还可配置一些常用工具及应用，如（出厂）外设测试脚本， ota 升级（包括本地升级和网络升级），人脸应用，isp_camera 应用，片上外设的接口/命令（可通过 shell 脚本命令直接对外设进行操作），(根文件系统) rootfs 相关设置，包括 USB/sd_card 的自动挂载，userdata 分区的挂载，adb 服务的开启关闭等
>

## 2.IConfigTool 配置流程

1.解压并打开配置工具

>   *IConfigTool 配置工具在"tools/iconfigtool" 目录下，解压后直接运行*

![22](img/22.png)

>   *如果IConfigTool 出现闪退时，删除工具 lib/ 目录下 libQtCore.so.4 与 libQtGui.so.4文件*

![18](img/18.png)

>   *重新打开 IConfigTool 配置工具*

![21](img/21.png)



2.选择需要使用的配置文件，这里以 x2000_darwin_nand_defconfig 为例

![1](img/1.png)

Config.in 是生成配置界面的文件

>   *build/Config.in*

Config 是需要修改的配置文件

>   *build/configs/x2000_darwin_nand_defconfig*



3.配置实例，以 x2000 efuse 为例

**若不知道需要使用的配置的具体路径，可通过 Ctrl+F 打开查找窗口查找相关配置，例如想要使用 efuse，可通过 Ctrl+F 打开查找窗口，输入 efuse，点击 Search 搜索，则可以得到相关路径**

![3](img/3.png)

**勾选 efuse 驱动并配置**

![4](img/4.png)

**勾选 efuse 的接口命令，完成配置后 Ctrl+S 保存**

![19](img/19.png)

**保存后，相应文件内的宏会被选择上，可通过 grep 查看，现在该文件可以用于编译了**

![23](img/23.png)

**IConfigTool 配置工具只是改变配置文件的内容，要想配置生效，还需应用配置，即 make 配置文件**



4.**进入编译目录，重新 make 相关配置，然后编译**

![5](img/5.png)



5.烧录至开发板后，因为勾选了 efuse 的接口命令，我们可以在命令行使用 efuse 相关命令直接对其操作

![20](img/20.png)