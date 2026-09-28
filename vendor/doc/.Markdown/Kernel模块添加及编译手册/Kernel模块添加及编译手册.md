# 模块添加及编译手册



## 1.驱动模块独立编译方法

**1.创建文件夹vendor,其中包含vendor.c与编译规则Makefile文件:**

![10](img/10.png)

**2.在vendor.c中编写:**

```c
#include <linux/module.h>		//包含模块加载卸载函数
#include <linux/init.h>			//包含模块加载所要执行的函数
#include <linux/kernel.h>		//包含kenel中某些api接口

static int __init vendor_init(void)			//模块加载调用函数
{
    printk(KERN_ERR "vendor_init test init\n");

    return 0;
}

static void __exit vendor_exit(void)   		//模块卸载调用函数
{
    printk(KERN_ERR "vendor_exit test exit\n");
}

module_init(vendor_init);
module_exit(vendor_exit);

MODULE_LICENSE("GPL");						//模块包含协议
MODULE_AUTHOR("zhongwan");					//作者
MODULE_DESCRIPTION("vendor test");			//模块描述
```

**3.在Makefile中编写:**

**KERN_DIR为自己的板子的kernel路径,这里是Xburst2,所以选择kernel-x2000,而如果是Xburst1就必须根据板子选择对应的kernel:**

**x1000,x1520,x1830为kernel-x1000,而x1021选择kernel-x1021**

```makefile
KERN_DIR = /home/your_linux/kernel/kernel-x2000 #基于自己使用的kernel添加路径

obj-m += vendor.o								#我们需要编译源文件,将其编译成模块

all:
	make -C $(KERN_DIR) M=`pwd` modules			#编译

.PHONY: clean									#将clean声明为伪目标

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean	#清除编译生成的文件
```

**4.进入到vendor目录下,打开终端,并将编译器添加进环境变量,可以生成模块vendor.ko文件:**

**环境变量指定为对应的编译器,这里为x2000_darwin,用Xburst2的编译器**

**Xburst1: mips-gcc520-glibc222/bin**

**Xburst2: mips-gcc720-glibc226/bin**

```makefile
cd vendor
export PATH=/home/your_linux/tools/toolchains/mips-gcc720-glibc226/bin:$PATH  #配置环境变量,注意这里是使用你存放编译器的目录,请勿直接复制,而是根据实际情况进行配置
make 
```

![11](img/11.png)

**5.将生成的vendor.ko推入板子观察效果:**

```shell
adb push vendor.ko /usr/data   #将生成的模块推入板子/usr/data目录,
adb shell					   #进入板子
cd /usr/data				   #进入/usr/data查看
ls   
```

![12](img/12.png)

**6.将模块手动加载进开发板,并用dmesg查看加载模块时的打印:**

![13](img/13.png)

![14](img/14.png)

**7.将模块卸载,并查看卸载打印:**

![15](img/15.png)

![16](img/16.png)

**8.如有需要,请在模块中添加自己想要的操作,按照上述方法编译加载,即可使用模块**



## 2.驱动模块添加方法

### 2.1驱动模块在module_driver中的添加方法

**1.进入到module_driver目录下:** 

![1](img/1.png)

**2.在其下devices目录下添加要添加的外设模块文件夹,这里以vendor_test为例子,**

**并在vendor_test文件夹下添加vendor_test.c文件与对应Makefile文件:**

![2](img/2.png)

**3.在其Makefile中编写如下内容:**

**模块名称不要和生成依赖的名称一致,obj-m则表示将设备编译生模块,若不想编译成模块则obj-y**

```makefile
include $(DRIVERS_DIR)/tools/common_module.mk  #包含编译模块所用到的工具

MODULE_NAME := test 						   #模块名称

module-y += vendor_test.o					   #生成模块所依赖的文件

obj-m := $(MODULE_NAME).o					   #编译生成模块

$(MODULE_NAME)-y = $(module-y)
```

**4.在vendor_test.c文件中编写如下内容:**

```c
#include <linux/module.h>                  //模块加载对应头文件

static __init int vendor_init(void)	  	   //模块加载调用的函数
{
    printk(KERN_ERR "vendor module init\n");
    return 0;
}

static __exit void vendor_exit(void)	   //模块卸载调用函数
{
    printk(KERN_ERR "vendor module exit\n");
}

module_init(vendor_init);				   //模块加载函数
module_exit(vendor_exit);				   //模块卸载函数

MODULE_DESCRIPTION("test vendor");		   //模块描述
MODULE_LICENSE("GPL");					   //模块所用协议
```



### 2.2.驱动模块在Iconfig和Makefile中的配置方法

**1.进入到module_driver下的package目录:**

![3](img/3.png)

**2.在devices里的other目录下创建vendor_test文件夹和其下目录Config.in和vendor_test.mk:**

![4](img/4.png)

**3.在Config.in文件中编写菜单,并在Iconfig中选择将其编译进工程:**

**在vendor_test目录下的Config.in中添加:**

```makefile
config MD_VENDOR_TEST					#模块宏的名称
    bool "vendor test module"			#模块描述
    default n							#默认选项,不选择,需要在IConfing中勾选
```

**并在上一级Config.in中添加索引,已将该宏添加进来:**

```makefile
source /package/devices/other/vendor_test/Config.in
```

**在iconfig中将宏选中并保存:**

![8](img/8.png)

**4.在vendor_test文件夹中创建vendor_test.mk文件,并按照如下方式编写:**

```makefile
package_name = test               #模块包名称
package_depends =			      #模块依赖包名称,若赋值则优先编译
package_module_src = devices/vendor_test #需要编译的文件目录,该目录下包含编译模块的makefile文件
package_finalize_hook = vendor_test_finalize_hook  #定义在模块编译之前的操作
package_make_hook =     	 	  #定义模块的编译方式
package_init_hook =			 	  #定义在模块编译之前的操作
package_clean_hook =			  #定义用来清理文件

test_init_file = output/test.sh   #初始化将要执行的脚本文件

define vendor_test_finalize_hook
	$(Q)cp devices/vendor_test/test.ko output/
	$(Q)echo -n 'insmod test.ko ' > $(test_init_file)
	$(Q)echo >> $(test_init_file)
endef
```

**5.同时,在other.mk文件中添加对于编译和规则,通过之前Iconfig设置的宏决定是否加入编译:**

```makefile
package-$(MD_VENDOR_TEST) += package/devices/other/vendor_test/vendor_test.mk
```



### 2.3驱动模块使用方法

**1.当执行玩上述添加流程,就可以开始编译模块了**

```makefile
cd linux/build                      #进入到你的linux目录下的build目录
make x2000_darwin_nand_defconfig	#编译刚才保存的defconfig文件
make                                #编译
```

**之后若之该动module_driver目录下的内容时候,则只需要按照下面流程操作**

```makefile
make app_module_driver				#编译module_driver
make buildroot						#编译文件系统
```

**当我们编译module_driver时,可以观察串口打印,可以发现生成了test.ko文件,说明模块编译成功:**
![7](img/7.png)

**2.当我们编译完模块,就可以烧入进板子,进行使用,如何烧录请参考<<Linux工程烧入介绍>>**

**此时打开串口,输入lsmod命令,看到test,说明模块加载成功:**

![9](img/9.png)
