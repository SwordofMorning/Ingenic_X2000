模块编译流程--以dmic为例

1, iConfigTool配置之后，最终体现在配置build/.config.in中的各项。

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ vim .config.in
```

<img src="模块编译流程--以dmic为例.assets/17.png" alt="17" style="zoom:150%;" />

2, 生成soc_dmic.ko 

```
sxyzhang@T430:~/my/work/linux/x2000_sz$ vim module_driver/soc/x2000/dmic/Makefile
```

从而，在module_driver/soc/x2000/dmic/下生成soc_dmic.ko

3, 将build/.config.in中相关的参数写入模块加载的参数文件中

```
sxyzhang@T430:~/my/work/linux/x2000_sz$ vim module_driver/package/soc/x2000/dmic/dmic.mk
```

<img src="模块编译流程--以dmic为例.assets/13.png" alt="13" style="zoom:150%;" />

4, 最终确认

​	![14](模块编译流程--以dmic为例.assets/14.png)

```
sxyzhang@T430:~/my/work/linux/x2000_sz/18module_driver/output$ vim soc_dmic.sh
```

<img src="模块编译流程--以dmic为例.assets/15.png" alt="15" style="zoom:150%;" />

5, buildroot中的加载方式

```
sxyzhang@T430:~/my/work/linux/x2000_sz/module_driver$ vim Makefile 
```

<img src="模块编译流程--以dmic为例.assets/16.png" alt="16" style="zoom:150%;" />

6, 经过以上操作，模块编译的shell文件及ko文件都被拷贝到buildroot/buildroot/output/target/module_driver下了

![18](模块编译流程--以dmic为例.assets/18.png)

重新打包并烧录rootfs就ok了。 其他模块加载都可以参考此方法。

7, 需要注意，上图中driver_default_init_script.sh才是真正系统起来后运行的脚本清单，并不是上图中所有的ko文件都会被加载到。