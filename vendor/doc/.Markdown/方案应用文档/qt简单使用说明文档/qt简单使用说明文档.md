# qt简单使用说明文档

## 1.Buildroot配置安装Qt5

在buildroot目录下，make menuconfig

<img src="image/1.png" style="zoom:80%;" />

选择target packages项

<img src="image/2.png" style="zoom:80%;" />

找到QT5 勾选

<img src="image/3.png" style="zoom:80%;" />

进入QT5选项，勾选对应需要的选项

<img src="image/4.png" style="zoom:80%;" />

请仔细核对截图中的选项，避免漏选错选！

**保存退出**。

在代码工程build目录下，make buildroot

<img src="image/5.png" style="zoom: 67%;" />

生成**不带字库**的rootfs.squashfs

## 2.Qt5测试用例运行

**/usr/lib/fonts必须有字库，才能正常运行测试用例！**

字库获取途径如下

ubuntu：/usr/share/fonts

windows：C:\WINDOWS\Fonts

把需要的一个TTF字库文件存放到 buildroot/buildroot/output/target/usr/lib/fonts/目录下面，fonts文件夹需要自己创建。

<img src="image/6.png" style="zoom: 80%;" />

**重新回到build目录下面**，执行make buildroot，生成**带字库**的rootfs.squashfs

**qt的测试用例路径：/usr/lib/qt/examples/**

运行触摸测试用例

/usr/lib/qt/examples/widgets/touch/fingerpaint/fingerpaint

## 3.Qt简单的工程示例

首先自行安装qt，下面以Ubuntu举例

找到Qt Creator

<img src="image/7.png" style="zoom:80%;" />

文件->新建文件或项目

<img src="image/8.png" style="zoom: 67%;" />

选择一个窗口类项目，方便看到效果

<img src="image/9.png" style="zoom:80%;" />

创建一个untitled的项目，下一步

<img src="image/10.png" style="zoom:80%;" />

<img src="image/11.png" style="zoom:80%;" />

<img src="image/12.png" style="zoom:80%;" />

<img src="image/13.png" style="zoom:80%;" />

<img src="image/14.png" style="zoom:80%;" />

这个软件开发包很重要，选择对应的构建套件才能在该平台运行

<img src="image/15.png" style="zoom:80%;" />

运行

<img src="image/16.png" style="zoom:80%;" />

此时可以看到一个空白的窗口，即是在Ubuntu系统环境的运行效果：

<img src="image/17.png" style="zoom:80%;" />

此时的utility.pro

<img src="image/18.png" style="zoom:70%;" />

当我们添加所需的功能或者其他需求，pro工程文件会对应更改。

**现在增加对应我们工程的kits**

点击工具——》选项

<img src="image/19.png" style="zoom:70%;" />

<img src="image/20.png" style="zoom:70%;" />

配置编译工具链:

<img src="image/21.png" style="zoom:80%;" />

<img src="image/22.png" style="zoom:80%;" />

配置对应qt版本

<img src="image/23.png" style="zoom:80%;" />

kits添加

<img src="image/24.png" style="zoom:80%;" />

<img src="image/25.png" style="zoom:80%;" />

点击Apply，然后OK。

<img src="image/26.png" style="zoom:80%;" />

选择对应的kits，这里选择mips，然后Build。在对应的构建目录生成Makefile和untitled应用程序！

<img src="image/27.png" style="zoom:80%;" />

adb push  untitled /tmp

然后在串口终端或者adb shell进入tmp目录执行untitled应用，即可得到显示效果！

现在我们测试用命令行去编译生成应用程序

找到我们工程qmake所在的路径:

home/zhouyichun/code/2020_linux_new/buildroot/buildroot/output/host/bin/qmake

<img src="image/28.png" style="zoom:80%;" />

执行/home/zhouyichun/code/2020_linux_new/buildroot/buildroot/output/host/bin/qmake

生成Makefile。

最后make生成最终的应用程序untitled。