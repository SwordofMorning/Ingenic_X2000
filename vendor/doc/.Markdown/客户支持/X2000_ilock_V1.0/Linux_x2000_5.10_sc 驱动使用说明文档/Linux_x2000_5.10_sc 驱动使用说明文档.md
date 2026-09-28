# Linux_x2000_5.10_sc 驱动使用说明

## 一   硬件平台介绍

​	 本文档使用的硬件平台：PD_x2000_EVB_CORE_V1P1

​	<img src="Linux_x2000_5.10_sc驱动.assets/1.png" alt="1" style="zoom: 80%;" />



## 二  原理介绍

​		为了保护客户厂商的软件程序不被其对手复制，可以使用AES、2Key-3DES加密对代码进行加密，密码可以是用户提供的User-Key0、User-Key1或芯片内部的Chip-Key(随机数密码)。

​		而sc通过调用SC-ROM接口使用密码进行**解密**操作，软件不直接访问密码本身，防止密码泄露。

​		**注意：**要使用sc功能，板子需要提前安全烧录，目前仅支持x2000系列芯片。



## 三 软件实现过程

### 1 uboot 的修改

启用安全启动功能需要修改UBOOT板级配置文件，路径如下：

bootloader/uboot-x2000/include/configs/x2000_base.h

添加如下配置：

```shell
#define CONFIG_JZ_SCBOOT
```



### 2 kernel相关驱动的配置

​		进入对应linux工程目录下kernel/kernel/路径下，敲make menuconfig并回车，进入Ingenic device-drivers Configurations选项，选中Ingenic Security support，完成后保存配置。

<img src="Linux_x2000_5.10_sc驱动.assets/4.png" alt="4" style="zoom:80%;" />

备注：

由于整体编译配置make x2000_nand_5.10_defconfig会覆盖掉kernel的当前配置，所以修改完kernel配置以后，需要手动拷贝一份.config到其实际的kernel配置，当前该板级使用的配置为

x2000_module_base_linux_sfc_nand_defconfig，执行如下拷贝以保证整体编译不会覆盖kernel当前配置：

```shell
kernel/kernel$ cp .config arch/mips/configs/x2000_module_base_linux_sfc_nand_defconfig
```

### 3  iConfigTool的工具的配置

​		打开iConfigTool工具

<img src="Linux_x2000_5.10_sc驱动.assets/2.png" alt="2" style="zoom:80%;" />

 本文使用的配置文件:   x2000_nand_5.10_defconfig		

勾选sc命令，一般默认勾选

<img src="Linux_x2000_5.10_sc驱动.assets/3.png" alt="3" style="zoom:80%;" />

修改完成配置后保存。



编译系统

​		**注意：**IConfig配置好之后就编译配置，而后去配置kernel再编译整个系统。
```shell
make x2000_nand_5.10_defconfig
make
```



## 四  相关签名工具使用

### 1 生成签名需要的文件

Key 生成工具，用于生成 RAS-Key、User-Key0 和 User-Key1,提供给签名工具和烧录工具使用。

运行命令如下：

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool$ sudo ./genkey_64
```

注意:genkey_64 代表 64 位,genkey_32 代表 32 位

![2022-12-15_17-58](Linux_x2000_5.10_sc驱动.assets/7.png)

点击 Generate Key 按钮生成签名需要的文件，文件路径为：



```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool$ ls -lh
total 39M
-rwxrwxr-x 1 root root  23M 9月  17  2021 genkey_32
-rwxrwxr-x 1 root root  16M 9月  17  2021 genkey_64
-rw-rw-r-- 1 root root  588 12月 15 17:55 key.bin
-rw-rw-r-- 1 root root 1.7K 12月 15 17:55 pri_key.pem
-rw-rw-r-- 1 root root   64 12月 15 17:55 user_key.bin
```

生成 Key 后需要把 key.bin 复制到烧录工具的 security 目录下:

```shell
cloner-2.5.28-ubuntu_alpha$ ls security/x2000/key.bin
```

复制命令如下：

```shell
cloner-2.5.28-ubuntu_alpha$ sudo cp securitytool/x2000/keytool/key.bin security/x2000/key.bin -arf
```

### 2 签名工具

签名工具用于签名、加密客户的固件、应用程序、配置文件,以便保护用户核心数据。

签名工具路径，运行签名工具如下: 

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/sigtool$ sudo ./sigtool-64
```

![2023-01-13_15-15](Linux_x2000_5.10_sc驱动.assets/8.png)



首先签名burner bin,该签名文件用于烧录工具第一阶段及第二阶段的固件加密。

![2023-01-13_15-17](Linux_x2000_5.10_sc驱动.assets/9.png)





注： 

RSA KEY： 选择 pri_key.pem

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/pri_key.pem
```



AES KEY：选择 user_key.bin

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/user_key.bin
```



SPL FILE:

```
cloner-2.5.28-ubuntu_alpha/firmwares/x2000/spl.bin
```



Uboot FILE:

```shell
cloner-2.5.28-ubuntu_alpha/firmwares/x2000/uboot.bin
```



点击 sign 签名成功以后，会在源文件的路径下生成 uboot_sec.bin 和 spl_sec.bin 加密后固件。
烧录固件签名完成后在烧录工具的固件路径下文件结构:



```shell
cloner-2.5.28-ubuntu_alpha/firmwares/x2000$ ls -l
total 836
-rw-r--r-- 1 root root   1964 9月  17  2021 config.cfg
-rwxr-xr-x 1 root root   8104 11月 11 16:05 spl.bin
-rw-r--r-- 1 root root   9648 1月  13 14:44 spl_sec.bin
-rwxr-xr-x 1 root root 413260 11月 11 16:05 uboot.bin
-rw-r--r-- 1 root root 415312 1月  13 14:44 uboot_sec.bin
```





签名 u-boot-spl-pad.bin 和 xImage ,签名工具路径，运行签名工具如下: 

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/sigtool$ sudo ./sigtool-64
```

![2022-12-16_09-44](Linux_x2000_5.10_sc驱动.assets/10.png)





​																								u-boot-spl-pad.bin的签名

注： 

RSA KEY： 选择 pri_key.pem

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/pri_key.pem
```

AES KEY：选择 user_key.bin

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/user_key.bin
```

spl bin 选择： u-boot-spl-pad.bin

```
build/output/u-boot-spl-pad.bin
```

点击 sign 按钮进行签名，签名成功会提示： Successful signature! 表示签名成功，成功以后会在目录下生成 u-boot-spl-pad-dst.bin 文件，路径为：

```
build/output/u-boot-spl-pad-dst.bin
```



![2022-12-16_10-05](Linux_x2000_5.10_sc驱动.assets/11.png)



​                                                                                                  xImage 的签名



注： 

RSA KEY： 选择 pri_key.pem

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/pri_key.pem
```

AES KEY：选择 user_key.bin

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/user_key.bin
```

kernel bin 选择： xImage

```
build/output/xImage
```

点击 sign 按钮进行签名，签名成功会提示： Successful signature! 表示签名成功，成功以后会在目录下生成 xImage-dst.bin 文件，路径为：

```
build/output/xImage-dst.bin
```



注意： 

生成的下面三个签名文件，需要保存好，同一个板子一旦签名了，再次烧录需要用同一个签名文件的固件，不然烧录会不成功。

key.bin
pri_key.pem
user_key.bin



## 五  烧录工具配置

​		安全烧录具体操作方法请参考《X2000_SecurityBoot操作指南.pdf》，板极选择x2000e_sfc_nand_lpdr2_linux.cfg。

烧录工具的版本： cloner-2.5.28-ubuntu_alpha

烧录工具的配置如下：



![2022-12-16_10-29](Linux_x2000_5.10_sc驱动.assets/12.png)



![2022-12-16_10-30](Linux_x2000_5.10_sc驱动.assets/13.png)



注：

uboot 选择签名以后的文件： u-boot-spl-pad-dst.bin

kernel选择签名以后的文件： xImage-dst.bin



![2022-12-16_10-28](Linux_x2000_5.10_sc驱动.assets/14.png)

注：

注意Manage mode里面的模式选择



![2022-12-16_10-22](Linux_x2000_5.10_sc驱动.assets/15.png)



选择EFUSE选项，勾选烧录密钥和激活安全模式

EFUSE GPIO:
此配置根据不同的开发板略有不同,例如：本文档测试板 EFUSE_EN_N 为 PD05 配置 101,
如果是 PMU 给 EFUSE 供电保持默认配置-1。



![2022-12-16_10-48](Linux_x2000_5.10_sc驱动.assets/16.png)



点击开始进行烧录即可。

## 六  验证sc加密和解密

### 1 加密流程

加密使用的工具为：进入你当前 linux工程目录下，运行加密工具 sigtool-other-bin 

```shell
 bug_list/sc$ ./sigtool-other-bin
```



![2022-12-16_10-43](Linux_x2000_5.10_sc驱动.assets/17.png)

注：

选择other bin

RSA KEY： 选择 pri_key.pem

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/pri_key.pem
```

AES KEY：选择 user_key.bin

```shell
cloner-2.5.28-ubuntu_alpha/securitytool/x2000/keytool/user_key.bin
```

 文件可以随意使用某个测试文件，本文档使用test.yuv

点击sign，会弹出Successful signature! 对话框，表示签名成功。

会在当前的对应路径生成：test-dst.yuv

把该文件 test-dst.yuv push 到你的板子上面：

adb push  test-dst.yuv /usr/data/

首先确保驱动加载成功，板子上的/dev/路径下有sc设备节点

![5](Linux_x2000_5.10_sc驱动.assets/5.png)



### 2 解密流程

​		将加密过的文件使用adb或者lrzsz或者提前加在buildroot内，将文件复制到板子上的可读写路径下，这里放在/usr/data/。

adb shell

```shell
# cmd_sc src=/usr/data/test-dst.yuv
src_file    : /usr/data/test-dst.yuv
dst_file    : /usr/data/test-dst.yuv_dst
# md5sum /usr/data/test-dst.yuv_dst
5e405a8bbada5df77f668f437b4c112f  /usr/data/test-dst.yuv_dst
```

验证本地的test.yuv的md5sum

```shell
md5sum test.yuv 
5e405a8bbada5df77f668f437b4c112f  test.yuv
```

可以看出加密后放到板子上的文件，通过cmd_sc加密以后，得到的解密文件，跟加密之前的文件的md5sum完全一致，表示整个加密和解密的过程是成功的。

