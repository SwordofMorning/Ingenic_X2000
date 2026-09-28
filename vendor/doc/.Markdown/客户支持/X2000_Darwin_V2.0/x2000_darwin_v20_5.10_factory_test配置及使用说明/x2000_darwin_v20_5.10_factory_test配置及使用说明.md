x2000_darwin_v20 5.10_factory_test配置及使用说明

一 软硬件环境

硬件板：Darwin_X2000_V2.0 开发板

软件版本：![1](x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/1.png)

串口：默认UART2_PD, 3000000  8N1。

二 factory_test 模式的作用

​	可以实现在当前配置x2000_darwin_v20_5.10_nand_defconfig基础上默认启动模块驱动的工厂测试程序，包括：

PWM， fb， keyboard， camera， MAC， amic， dmic， speaker。 当前库版本的5.10代码的wifi和BT驱动还有点问题，晚点更新了会提交到库里。

方便查看底层设备驱动或者设备是否工作，是否有硬件问题。

**特别说明：**本配置不包括可见光人脸识别算法，只是底层模块驱动的支持和测试，如果需要添加可见光人脸识别算法，则需要等后续施广华添加相应的配置到代码中。

三 编译方法

```
cd build
make clean
make clean_apps
make x2000_darwin_v20_5.10_nand_defconfig
make
```

生成文件目录：

<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/2.png" alt="2" style="zoom:150%;" />

四 系统启动

<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/3.png" alt="3" style="zoom:150%;" />

五 启动流程及代码追踪<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/4.png" alt="4" style="zoom:150%;" />

<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/5.png" alt="5" style="zoom:150%;" />

上述/etc/factory_test/factory_test_conf对应build下tools/iconfigtool/IConfigToolApp/IConfigTool工具的配置：

![6](x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/6.png)



![7](x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/7.png)

![8](x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/8.png)

修改上述配置后，可以增加或减少开机自启动的测试脚本。

五  代码追踪

<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/4.png" alt="4" style="zoom:150%;" />

板子中上述S99factory_test_shell来自于代码：

buildroot/buildroot/output/target/etc/init.d/S99factory_test_shell。

其中有各个模块的测试脚本函数。对应每个模块的脚本函数，所使用的命令一目了然，以camera模块测试为例：

<img src="x2000_darwin_v20 5.10_factory_test配置及使用说明.assets/9.png" alt="9" style="zoom:150%;" />

以上红框中的测试命令camera_cmd 对应的源码是：

libhardware2/src/cmds/camera_main.c





















