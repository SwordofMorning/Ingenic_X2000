#### **x2000H Darwin X2000 V2.0 开发板快速上手说明**



X2000H Darwin_x2000_v2.0 开发板如下：

![2022-09-16_11-39](x2000H_Darwin_v2.0_开发板快速上手说明.assets/11.png)

![10](x2000H_Darwin_v2.0_开发板快速上手说明.assets/10.jpeg)

#### 一   硬件详细介绍

详细硬件介绍见：

 `doc/FAE文档/X2000H_Darwin_X2000_V2.0/Darwin_X2000_V2.0开发套件硬件手册_V2.0.pdf`



#### 二  获取源码方法

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

##### **客户下载代码的目录路径要用全英文的，不要带有特殊符号的那种，不然编译可能会有问题**



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

#### 三   编译方法

Darwin_X2000_V2.0开发板使用的工程配置文件为：x2000_darwin_factory_defconfig

如果需要更改默认配置，可以使用工具 IConfigTool 进行配置，详细使用方法见文档：

`doc/开发使用说明/IConfigTool 使用文档.pdf`



整体编译命令如下：

`build$ make clean`

`build$ make x2000_darwin_factory_defconfig

`build$ make`

编译以后，固件生成目录如下：

`build$ ls output/ -lh`
`total 104M`
`-rw-r--r-- 1 kenny kenny  40M 9月   8 09:54 rootfs.squashfs`
`-rw-r--r-- 1 kenny kenny  50M 9月   8 09:54 rootfs.ubifs`
`-rw-rw-r-- 1 kenny kenny  24K 9月   8 09:54 u-boot-spl-pad.bin`
`-rw-rw-r-- 1 kenny kenny  11M 9月   8 09:54 userdata.ubifs`
`-rw-rw-r-- 1 kenny kenny 3.7M 9月   8 09:54 xImage`

#### 四  最新烧录工具获取

在ubuntu下执行如下命令可以免密下载：

##### **ubuntu版本：**

wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz

##### windows版本：

wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip

#### 五  烧录方法

![2022-09-15_11-54](x2000H_Darwin_v2.0_开发板快速上手说明.assets/2.png)





上 TYPE_C_USB_Power接口，给开发板供电。然后再使用Micro_USB&Download接口接上usb数据线进行烧录使用。烧录工具配置如下：



![2023-02-17_17-52](x2000H_Darwin_v2.0_开发板快速上手说明.assets/2023-02-17_17-52.png)



![2022-09-15_11-59_1](x2000H_Darwin_v2.0_开发板快速上手说明.assets/4.png)



![2022-09-15_11-59_2](x2000H_Darwin_v2.0_开发板快速上手说明.assets/5.png)

![2022-09-15_11-59_3](x2000H_Darwin_v2.0_开发板快速上手说明.assets/6.png)

![2022-09-15_11-59_4](x2000H_Darwin_v2.0_开发板快速上手说明.assets/7.png)

![2022-09-15_12-00](x2000H_Darwin_v2.0_开发板快速上手说明.assets/8.png)

点击开始以后，按住BOOT_KEY键不放，再按下RST_KEY以后松手，就可以进入烧录模式，然后再把BOOT_KEY键松开，等待烧录完毕以后即可。

#### 六 常用命令使用

##### 1  查看当前安装的驱动

```shell
lsmod 								   #查看当前已安装的驱动
#insmod [驱动文件]						#安装驱动
#rmmod [驱动文件]						#卸载驱动
```

![2022-09-30_09-22](x2000H_Darwin_v2.0_开发板快速上手说明.assets/2023-03-10_10-26.png)

已安装上图所示驱动

##### 2 Camera使用

Camera测试命令如下：

`# cmd_camera_software_preview /dev/camera /dev/fb0`  #摄像头预览界面

更多camera相关的文档，请参考：

doc/开发使用说明/Camera使用以及添加说明文档/Camera_Sensor 通用使用手册.pdf

##### 3 WIFI和蓝牙

参考相关文档：

doc/开发使用说明/bcm bt&wifi使用说明文档/bcm bt&wifi使用说明文档.pdf

##### 4  LCD

使用LCD之前需要开启屏幕背光，本配置已默认开启背光

```shell
cmd_fb enable /dev/fb0                                             #使能屏幕设备节点
cmd_fb clear /dev/fb0 color=0xff00ff00                             #清屏：将屏幕上所有像素点颜色均设为绿色 
cmd_fb display /dev/fb0                                            #显色：使颜色在屏幕上显现出来
cmd_fb disable /dev/fb0                                            #失能屏幕：此时屏幕恢复成没有颜色的状态
```



#### 七  功能调试介绍

功能调试有adb和串口两种方式，详细见文档：

`doc/FAE文档/Linux平台功能调试.pdf`

#### 八  其它功能介绍

其它相关开发文档介绍见：

`doc/开发使用说明/` 和  `doc/FAE文档/`

