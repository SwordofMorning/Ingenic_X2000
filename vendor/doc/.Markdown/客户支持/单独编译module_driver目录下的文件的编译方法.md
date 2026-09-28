**单独编译module_driver目录下的文件的编译方法**

因为目前外设代码采用模块驱动加载方式，那么当我们整体编译完成之后，基本上修改的就是module_driver目录下的内容。修改之后的编译方式有两种：

1, 整体编译

在build目录下执行如下命令：

```
make
```

然后重新烧录rootfs的镜像。

[^此种方法缺点是需要整体编译和烧录，比较耗时，可以作为最终版本使用，调试阶段建议采用第二种方法来调试。]: 

2, 模块编译

准备环境，默认不加载依赖库及驱动脚本，并烧录

```
chmod -x ../buildroot/buildroot/output/target/module_driver/driver_default_init_script.sh   // 默认不加载此文件
make buildroot --->烧录rootfs
```

修改module_driver目录下文件内容后的编译：

```
make app_module_driver 
adb push  ../module_drivers/output/  /usr/data/
```

在开发板端：

```
cd /usr/data/
chmod 777  *.sh
./driver_default_init_script.sh  // 每次系统重启之后要执行一次，后面调试就不需要了
```

至此，环境搭建好了。之后再有修改比如：../module_driver/soc/x1600/mac/的内容，就可以单独执行以下操作：

```
make app_module_driver 
adb push ../module_driver/output/soc_mac_driver.ko /usr/data/
```

在开发板端：

```
cd /usr/data/

rmmod soc_mac_driver

./soc_mac_driver.sh
```

[^不需要修改module_driver之后每次都重新编译和烧录，大大节省了调试时间，提高效率。]: 

调试完成后，需要系统启动后自动加载依赖库及模块驱动：

```
chmod 777 ../buildroot/buildroot/output/target/module_driver/driver_default_init_script.sh
make buildroot --->烧录rootfs
```

