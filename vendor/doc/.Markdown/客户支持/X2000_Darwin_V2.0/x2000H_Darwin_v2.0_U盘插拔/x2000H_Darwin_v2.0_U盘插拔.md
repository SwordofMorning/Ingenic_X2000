**x2000H Darwin 2.0 U盘插拔的方法**

X2000 Darwin_x2000_v2.0上测试U盘的插拔？

可以先参考文档：doc/开发使用说明/USB使用说明文档/主机/USB_STORAGE.pdf ，然后再结合如下进行配置即可。

使用配置x2000_darwin_nand_defconfig进行编译，然后使用IConfigToolApp 进行如下配置配一下。下面的配置包含adb的调试功能。



![1](x2000H_Darwin_v2.0_U盘插拔.assets/1.png)





![2](x2000H_Darwin_v2.0_U盘插拔.assets/2.png)



kernel/kernel-x2000$ make menuconfig 进行如下配置：

![u_mass_storage01](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage01.png)



![u_mass_storage02](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage02.png)







![usb_mass_storage_support_02](x2000H_Darwin_v2.0_U盘插拔.assets/usb_mass_storage_support_02.png)



![u_mass_storage04](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage04.png)



![u_mass_storage05](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage05.png)





![u_mass_storage06](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage06.png)



![u_mass_storage07](x2000H_Darwin_v2.0_U盘插拔.assets/u_mass_storage07.png)



配置完以后，进入adb 

adb shell

ls  -l  /tmp/mass_storage/

挂载在该目录下。
