如果同时修改了iConfigTool 和kernel配置，那么之后的编译应该怎么做呢？

1, 应该先在对应的kernel中make menuconfig之后，将.config 拷贝为当前的内核配置XXX_defconfig文件。

```
cp .config arch/mips/configs/x1600_halley6_module_base_linux_sfc_nand_defconfig
```

2, iConfigTool进行配置  ---〉save 到对应的build编译配置文件，比如x1600_nand_defconfig

3，在build 下执行:

```
make x1600_nand_defconfig
make
```

4，重新烧录kernel和rootfs镜像。