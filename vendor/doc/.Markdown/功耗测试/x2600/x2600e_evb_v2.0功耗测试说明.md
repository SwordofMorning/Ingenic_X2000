# x2600e_evb_v2.0 功耗测试说明

## 	一、测试环境

### 		1.软件环境

#### 				1>uboot

<img src="img/1.png" style="zoom:95%;" />

#### 				2>kernel

![](img/2.png)

#### 				3>buildroot

<img src="img/3.png" style="zoom: 80%;" />

### 			2.硬件测试环境

​			测试使用的是x2600e_evb_v2.0平台，测试功耗的仪器是：mPower1203 series

## 二、测试流程

​	注：x2600e_evb_v2.0平台的VDDIO_CIM的电压硬件配置的是3.3v,在kernel中的配置需要与其对应，否则易损坏芯片。在kernel中对应板极的dts文件的修改如下：

![](img/4.png)

### 	1.idle模式

​			使用的配置文件为：x2600e_nand_5.10_defconfig

### 	2.sleep模式

​			1> 使用的配置文件为：x2600e_nand_5.10_defconfig

​			2> 对GPIO口的状态进行的调整：PD00-PD05设置为OUTPUT1

​			3>对core和ddr进行降压

​				硬件修改：

​				左侧是对DDR电压的修改电路，右侧是对CORE电压的修改电路。

![](img/5.png)

​				软件修改： 修改 arch/mips/xburst2/soc-x2600/pm.c文件，详细修改流程参照doc/x2600休眠调压.pdf

![](img/7.png)

若对DDR的部分的电压有硬件修改，需要加上下图中的更改：![](img/9.png)

![](img/10.png)

![](img/11.png)

​			4>使用的休眠命令为：echo mem > /sys/power/state

### 	3.fullspeed模式

​			使用的配置文件为：x2600e_vast_v20_nand_5.10_defconfig

​			测试使用的命令是：memtester和whetstone

 

### 4.standby模式

​			1> 使用的配置文件为：x2600e_nand_5.10_defconfig

​			2> 对GPIO口的状态进行的调整：PD00-PD05设置为OUTPUT1

​			3> 测试环境：

​			测试standby模式需要更新到最新的kernel，保证pm.c中有**LOW_POWER_IDLE**这个宏定义。

​			测试standby模式也需要打开kernel中的相关的宏定义，对于core电压的调整，需要硬件和软件一致，硬件的修改方式和sleep模式相同。如下图中软件中打开了standby模式对core的电压的控制，硬件对于core电压也应该修改成0.8V时的状态。	![](img/8.png)

测试时候使用的命令是：echo standby > /sys/power/state

休眠唤醒方式设置：**adc休眠唤醒 or timer_watchdog1(硬件定时器)休眠唤醒**

adc唤醒：
![](img/17.png)
![](img/18.png)
以下示例为，使能adc的awd模式，当采样值大于3000或低于400时触发中断，即唤醒休眠，在进入板子端串口后，依次输入：
```c
/* 使能adc的awd模式，当采样值大于1000时触发中断，即唤醒休眠 */
cmd_adc awd_enable 0 low_threshold=-1 high_threshold=1000
/* 使能adc采样 */
cmd_adc seq1_enable channels=0 enable_continue=1 is_enable_irq=0 continus_clk_div=120000 delay_clk_div=24000 &
```

硬件定时器唤醒(目前是依赖于watchdog1唤醒)：
![](img/15.png)
![](img/16.png)
```c
cmd_hw_timer start 30000000
```
注意后面的单位为us

## 三、测试结果

| 状态                              | 整机功耗 |
| --------------------------------- | -------- |
| idle模式                          | 60mA@5V  |
| memtest+whetstone                 | 168mA@5V |
| sleep模式（降低core电压为0.8V）   | 8mA@5V   |
| standby模式（降低core电压为0.8V） | 8mA@5V   |

