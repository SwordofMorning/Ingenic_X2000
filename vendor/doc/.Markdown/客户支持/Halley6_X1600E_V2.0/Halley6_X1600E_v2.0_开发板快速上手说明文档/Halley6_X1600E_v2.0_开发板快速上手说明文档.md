# **Halley6_X1600E_V2.0 开发板快速上手说明文档**



## 一  开发板介绍

### 1. 开发板如下：

![2022-09-26_16-51](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-26_16-51.png)







![2022-09-26_16-51_1](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-26_16-51_1.png)







### 2. 开发板硬件详细介绍

详细硬件介绍见：

 `doc/FAE文档/Halley6_X1600E_V2.0/Halley6开发套件硬件手册_V2.1.pdf`



## 二 获取源码方法

在获取源码之前，都需要把你开发电脑上的相应的 public key发送给我司进行添加，从而得到获取源码的权限。本地电脑获取 public key的方法如下： 

如果本机还没有 ssh public key,可以通过以下方法生成, 打开一个终端输入下列命令: 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ ssh-keygen
```

//输入 ssh-keygen 后,使用 
默认配置,一路回车直到完成即可 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ cat ~/.ssh/id_rsa.pub
```



Repo 工具下载
`$ mkdir project_workspace`
`$ cd project_workspace`
`$ wget http://git.ingenic.com.cn:8082/bj/repo`
`$ chmod +x repo`

同步代码的地址如下： 

```
./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
```

`./repo sync`

如若报错可使用http同步方式：

```
./repo init -u http://sz_halley2@119.136.25.25:8089/mirror/linux/manifest
```

`./repo sync`

**对于外部客户来说，这里的帐号不用修改，直接使用sz_halley2就可以。** 

**客户下载代码的目录路径要用全英文的，不要带有特殊符号的那种，不然编译可能会有问题**

 同步代码可能遇到的问题如下：

主要是因为系统环境,网络环境不一致导致在同步工程过程中出现的一些报错信息的解决方法。

这里的获取代码是基于Ubuntu14.04  64bit的环境进行的。

**问题1：-bash: ./repo: No such file or directory**

解决方法：

​                系统缺少repo工具或没有添加环境变量

​                wget http://git.ingenic.com.cn:8082/bj/repo  (注:此链接的repo工具为君正修改过后放在服务器上的，也可使用谷歌提供的repo工具)

​                chmod +x repo



**问题2：Permission denied (publickey).**

fatal: Could not read from remote repository.

 解决方法：

请确认是否有权限同步代码，同步代码需提交key于我司开通权限

**问题3：Their offer: diffie-hellman-group1-sha1**

解决方法：

修改~/.ssh/config，加入

Host *

​                KexAlgorithms +diffie-hellman-group1-sha1

 

**问题4：aes128-ctr，aes192-ctr，aes256-ctr**

解决方法：

修改 /etc/ssh/ssh_config 文件

删除Ciphers aes128-ctr，aes192-ctr，aes256-ctr….行前注释符号



**问题5：git config --global user.name "yourname"**

​              git config --global user.email your@email.com

解决方法：

若不需要提交代码至服务器可直接跳过执行后续步骤

若需要提交代码请根据提示命令注册姓名及邮箱地址

git config --global user.name "your name"

git config --global user.email "you email"

 

**问题6：Traceback (most recent call last):**

File "/home/jdai/work/test1/.repo/repo/main.py", line 385, in <module>

  _Main(sys.argv[1:])

File "/home/jdai/work/test1/.repo/repo/main.py", line 365, in _Main

result = repo._Run(argv) or 0

 File "/home/jdai/work/test1/.repo/repo/main.py", line 137, in _Run

解决方法：

rm –rf ***\*.\****repo  (repo前面有***\*.\****)

 ./repo init –u ssh://……….  

使用repo init 同步没有执行完会产生缓存需删除缓存后再次执行。

 

**问题7：Testing colorized output (for 'repo diff', 'repo status'):**

 black   red    green   yellow  blue   magenta  cyan   white 

 bold   dim    ul    reverse 

Enable color display in this user account (y/N)?

解决方法：

直接按回车键

   注意：若在同步过程中长时间卡住不动，有可能是因为库太大，或进程卡死，请ctrl+c 键退出再执行，支持断点续传。



**问题8：Bad owner or permissions on .ssh/config** 

解决方法：

`sudo chmod 600 .ssh/config`

如果已经获取到了源码，可以见详细的源码获取方法：

`doc/FAE文档/获取Linux平台源码的方法.pdf`

doc/FAE文档/获取源码问题总结.pdf

## 三 编译方法

获取源码以后，如果需要修改相应的配置，使用工具：

`tools/iconfigtool/IConfigToolApp$ ./IConfigTool` 



![2022-09-27_11-41](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_11-41.png)





选择x1600e_halley6_nand_defconfig配置，路径位于：build$ ls configs/x1600e_halley6_nand_defconfig，点击Open按钮。



![2022-09-27_11-42](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_11-42.png)





按 Ctrl + S 进行配置的保存，点击 Yes保存。



还需要更改配置的内容，参考文档：

`doc/开发使用说明/IConfigTool 使用文档.pdf`



问题：

若打开配置 ./IConfigTool工具后出现闪退现象，将 lib文件夹内的这两个文件删除即可。

`tools/iconfigtool$ cd IConfigToolApp/lib/`

`tools/iconfigtool/IConfigToolApp/lib$ rm libQtCore.so.4 libQtGui.so.4`



Halley6_X1600E_V2.0 整体编译命令如下：

`build$ make clean`

`build$ make x1600e_halley6_nand_defconfig`

`build$ make`



**注意:若 make 失败,可先 make clean 再重新开始本步骤,如若依旧出错,请百度或咨询技术人员。**



编译以后，固件生成目录如下：

`build$ ls output/ -lh`
`total 104M`
`-rw-r--r-- 1 kenny kenny  40M 9月   8 09:54 rootfs.squashfs`
`-rw-r--r-- 1 kenny kenny  50M 9月   8 09:54 rootfs.ubifs`
`-rw-rw-r-- 1 kenny kenny  24K 9月   8 09:54 u-boot-spl-pad.bin`
`-rw-rw-r-- 1 kenny kenny  11M 9月   8 09:54 userdata.ubifs`
`-rw-rw-r-- 1 kenny kenny 3.7M 9月   8 09:54 xImage`

## 四 最新烧录工具的获取

**在ubuntu下执⾏如下命令可以免密下载：**

**ubuntu版本：**

```
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz
```

**windows版本：**  

```
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip
```



## 五 烧录方法

首先进入烧录工具文件夹，打开cloner，然后选中弹出界面上的配置选项

若要使用Windows下的烧录工具需要安装驱动，请参考：

烧录工具目录下的docs/中文/USBCloner 烧录工具说明文档 .pdf 文件内的第二节 “烧录工具驱动的安装”以及相关使用说明

此处以2.5.24 ubuntu版本工具为例

`cloner-2.5.24-ubuntu_alpha$ sudo ./cloner`

![2022-09-27_11-38](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_11-38.png)



![2022-09-27_14-53](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_14-53.png)



![2022-09-28_10-54](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-28_10-54.png)





![2022-09-27_14-59_1](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_14-59_1.png)



![2022-09-27_15-00](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_15-00.png)





![2022-09-27_15-00_1](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_15-00_1.png)





![2022-09-26_16-51](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-26_16-51.png)



烧录操作方法：



点击开始按钮，烧录工具如下所示：

![2022-09-27_15-03](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-27_15-03.png)





然后接上 TYPE_C_USB_Power接口，给开发板供电。然后再使用Micro_USB&Download接口接上usb数据线，按下Boot Key按键，再按下Reset Key按键再释放，最后再释放Boot Key按键，这样进入烧录模式，等待烧录完毕以后即可。

## 六 常用命令使用

### 1. 查看当前安装的驱动

```
lsmod 								   #查看当前已安装的驱动
#insmod [驱动文件]						#安装驱动
#rmmod [驱动文件]						#卸载驱动
```

![2022-09-30_09-22](Halley6_X1600E_v2.0_开发板快速上手说明文档.assets/2022-09-30_09-22.png)



已安装上图所示驱动

### 2.Camera使用

Camera测试命令如下：

`# cmd_camera_software_preview /dev/camera /dev/fb0`  #摄像头预览界面

更多camera相关的文档，请参考：

doc/开发使用说明/Camera使用以及添加说明文档/Camera_Sensor 通用使用手册.pdf

### 3.WIFI和蓝牙

参考相关文档：

doc/开发使用说明/bcm bt&wifi使用说明文档/bcm bt&wifi使用说明文档.pdf

doc/FAE文档/Halley6_X1600E_V2.0/Halley6_X1600E_v2.0_开发板WIFI使用说明.pdf

### 4.LCD

使用LCD之前需要开启屏幕背光，本配置已默认开启背光

```
cmd_fb enable /dev/fb0                                             #使能屏幕设备节点
cmd_fb clear /dev/fb0 color=0xff00ff00                             #清屏：将屏幕上所有像素点颜色均设为绿色 
cmd_fb display /dev/fb0                                            #显色：使颜色在屏幕上显现出来
cmd_fb disable /dev/fb0                                            #失能屏幕：此时屏幕恢复成没有颜色的状态
```



## 七 功能调试介绍

功能调试有adb和串口两种方式，详细见文档：

`doc/FAE文档/Linux平台功能调试.pdf`

## 八 其它功能介绍

其它相关开发文档介绍见：

`doc/开发使用说明/` 和  `doc/FAE文档/`

