# 						linux平台下区分系统唤醒源

## 一 背景介绍

​		系统休眠后被唤醒，唤醒源可能是gpio 唤醒、定时唤醒。那么如何区分是哪个唤醒源实现的系统唤醒呢？本文重点介绍如何区分唤醒源，并且如何对应具体的唤醒源。

## 二 X-serial主控芯片的唤醒源及大致功耗

### 2.1 X1000

#### 2.1.1 有32.768K晶振

休眠唤醒源：  

​		A：任意GPIO

​		B：RTC
休眠功耗大致范围 200~300uA@5V

#### 2.1.2 ⽆32.768K晶振

休眠唤醒源： 

​		A:  任意GPIO

​		B：RTC
休眠功耗大致范围  < 5mA@5V

### 2.2 X1600

#### 2.2.1 有32.768K晶振

休眠唤醒源： 

​		A: 任意GPIO

​		B：RTC
休眠功耗大致范围  < 1mA@5V

#### 2.2.2 ⽆32.768K晶振

不⽀持休眠唤醒功能

### 2.3 X2000

#### 2.3.1 有32.768K晶振

休眠唤醒源： 

​		A：任意GPIO

​		B：RTC
休眠功耗大致范围  < 5mA@5V

#### 2.3.2 ⽆32.768K晶振

不⽀持休眠唤醒功能

### 2.4 X2580（只⽀持FastBoot调整电压模式）

#### 2.4.1 有32.768K晶振

休眠唤醒源： 

​		A：指定GPIO 即： 仅WAKEUP管脚（PD31）可唤醒

​		B：RTC
休眠功耗大致范围  < 1.5mA@5V

#### 2.4.2 ⽆32.768K晶振

不⽀持休眠唤醒功能

### 2.5 X2600（standby调整电压模式）

X2600内部没有RTC功能单元，所以不支持配置32.768K的晶振。

X2600 实现休眠请参考：参考⽬录下说明 doc/开发使⽤说明/功耗测试/x2600/《x2600e_evb_v2.0功耗测试说明.pdf》 
休眠唤醒源： 

​		A：任意GPIO

​		B：内部看门狗 Watching1(产⽣中断/不复位) --(设置范围: 0.001s ~ 1431s， 精度:21.33us)

​		C：SADC电压

休眠功耗大致范围  < 10mA@5V

## 三 唤醒方式

### 3.1 GPIO唤醒

本例演示开发板PD_X2000_EVB_JH 设置WKUP_KEY(对应PE31)作为唤醒源的方法。

![1](linux平台下区分系统唤醒源.assets/1.png)

![2](linux平台下区分系统唤醒源.assets/2.png)

配置完成后保存

![3](linux平台下区分系统唤醒源.assets/3.png)

编译

```
zxy@vb:~/job/linux/x2000_sz/build$ make x2000_nand_defconfig
zxy@vb:~/job/linux/x2000_sz/build$ make
```

烧录之后进入休眠：

```
# 
# echo mem > /sys/power/state 
[   29.572771] PM: Syncing filesystems ... done.
[   29.576218] Freezing user space processes ... (elapsed 0.001 seconds) done.
[   29.577777] Freezing remaining freezable tasks ... (elapsed 0.001 seconds) done.
[   29.579346] x2000 suspend begin!
[   29.579564] Suspending console(s) (use no_console_suspend to debug)
DW
```

按键唤醒后有如下log输出：

![4](linux平台下区分系统唤醒源.assets/4.png)

### 3.2 RTC唤醒

kernel中打开rtc配置，并设置60s后自动由RTC唤醒：

![5](linux平台下区分系统唤醒源.assets/5.png)

如上设置之后，需将当前kernel配置拷贝到主配置指定的kernel配置文件中。以烧录配置 x2000_nand_defconfig为例，展示如何确定当前kernel配置，并拷贝生效以上配置。

```c
zxy@vb:~/job/linux/x2000_sz/build$ cat configs/x2000_nand_defconfig | grep kernel
APP_kernel_dir=../kernel/kernel-x2000      //当前kernel路径
APP_kernel_config=x2000_module_base_linux_sfc_nand_defconfig  //当前kernel配置文件
```

```c
zxy@vb:~/job/linux/x2000_sz/build$ cd ../kernel/kernel-x2000/
zxy@vb:~/job/linux/x2000_sz/kernel/kernel-x2000$ cp .config arch/mips/configs/x2000_module_base_linux_sfc_nand_defconfig 
```

编译

```c
zxy@vb:~/job/linux/x2000_sz/build$ make x2000_nand_defconfig
zxy@vb:~/job/linux/x2000_sz/build$ make
```

烧录之后进入休眠：

```c
# 
# echo mem > /sys/power/state 
[   29.572771] PM: Syncing filesystems ... done.
[   29.576218] Freezing user space processes ... (elapsed 0.001 seconds) done.
[   29.577777] Freezing remaining freezable tasks ... (elapsed 0.001 seconds) done.
[   29.579346] x2000 suspend begin!
[   29.579564] Suspending console(s) (use no_console_suspend to debug)
DW
```

60s后RTC自动唤醒：

![6](linux平台下区分系统唤醒源.assets/6.png)

### 3.3 内部看门狗和SADC电压唤醒 

X2600可以支持内部看门狗唤醒和SADC电压唤醒，具体配置及代码修改，请参考 doc/开发使⽤说明/功耗测试/x2600/《x2600e_evb_v2.0功耗测试说明.pdf》。

## 四  代码获取中断唤醒源的API

### 4.1  V4.4 kernel中获取中断唤醒源的API

获取中断唤醒源的API位于：drivers/base/power/wakeup.c 

```c
unsigned int pm_get_wakeup_irq(void)
{
	return pm_wakeup_irq;
} 
```

获取的是中断唤醒源的逻辑中断号。具体用法如下：

arch/mips/xburst2/soc-x2000/pm.c

```c
static void x2000_pm_end(void)
{
	printk("x2000 pm end irq_num=%d!\n", pm_get_wakeup_irq() );
	pm_clear_sleep_level();
}
```

不同的芯片，pm.c的路径会有差别。

### 4.2  V5.10 kernel中获取中断唤醒源的API

获取中断唤醒源的API位于：drivers/base/power/wakeup.c

```c
unsigned int pm_wakeup_irq(void)
{
	return wakeup_irq[0];
}
```

获取的是中断唤醒源的逻辑中断号。具体用法如下：

arch/mips/xburst2/soc-x2000/pm.c

```c
static void x2000_pm_end(void)
{
	printk("x2000 pm end irq_num=%d!\n", pm_wakeup_irq() );
	pm_clear_sleep_level();
}
```

不同的芯片，pm.c的路径会有差别。

### 4.3 逻辑中断号的对应方式

请参考下图：

![7](linux平台下区分系统唤醒源.assets/7.png)

由上图可知，3.1章节中设置的gpio  KEY_MENU对应逻辑中断号72，与3.1中的log输出对应。

3.2章节中设置的RTC唤醒对应逻辑中断号40，与3.2中的log输出对应。

## 五 应用层获取中断唤醒源

配置kernel 

![8](linux平台下区分系统唤醒源.assets/8.png)

参照3.2中方法来将当前kernel配置拷贝到主配置指定的kernel配置文件中，并整体编译和烧录。

上层应用可以通过 cat /sys/power/pm_wakeup_irq 来获取当前唤醒源的逻辑中断号。

如果是RTC唤醒，则执行结果如下：

![9](linux平台下区分系统唤醒源.assets/9.png)

如果是PE31唤醒，则执行结果如下：

![10](linux平台下区分系统唤醒源.assets/10.png)

上面两示例都说明了代码API 获取的逻辑中断号与系统中设备节点/sys/power/pm_wakeup_irq的一致，也间接证明了两种方法的正确性。