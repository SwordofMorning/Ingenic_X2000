# USB的ID引脚和VBUS引脚说明

## 具体功能说用

### 硬件ID引脚（输入）

ID引脚（君正的ID悬空情况下已默认根据DS上拉）用于确定OTG作为Host还是Device。具体的规则如下表：

| ID引脚状态 | USB工作模式 |
| :--------- | :---------- |
| 高电平     | USB设备     |
| 低电平     | USB主机     |

### 硬件VBUS引脚（输入）

当OTG工作在Host或Device模式下，VBUS引脚作用不同。具体模式下的作用见下表格：

| Host模式     | Device模式         |
| ------------ | ------------------ |
| 用于过流检测 | 1.插入上电状态检测 |
|              | 2.断开连接状态检测 |

注意：在Device模式下，需要能检测到插入状态，进而启动BC1.2的探测。

### GPIO模拟的DRVVBUS引脚（输出）

在Host模式下，GPIO模拟的DRVVBUS用于控制外围器件是否输出VBUS，对接入设备的供电进行控制。

## 不同芯片平台差异说明

### 已引出特定硬件ID引脚和VBUS引脚

这里以X2580为例，X2580的DS中关于USB引脚的具体描述如下：

![image-20230728145618606](USB模块的ID引脚和VBUS引脚说明.assets/image-20230728145618606.png)

可知，X2580芯片引出了特定的硬件ID引脚和VBUS引脚，所以芯片USB硬件支持前面功能描述中说明的功能。

硬件上，VBUS接到USB座子的VBUS（注意分压）上，USB0ID接到USB座子的ID（注意根据DS上拉到引脚能承受的电压，下图的ID未接到USB座子的ID上，使用了跳线帽进行调整），具体接法见下：

![image-20230801111842537](USB模块的ID引脚和VBUS引脚说明.assets/image-20230801111842537.png)

需注意，下图的VBUS连接设计存在一定功耗。

![image-20230804115020834](USB模块的ID引脚和VBUS引脚说明.assets/image-20230804115020834.png)

同时软件上，在menuconfig界面进行配置，**不勾选** External Id Pin for OTG（选中则OTG控制器将运行在强制模式下，运行模式仅受软件控制，不受硬件ID和VBUS的状态影响）和External Vbus Detect for OTG（选中则不使用DS中描述的PHY硬件VBUS引脚的功能），意味着启用芯片USB硬件的检测功能。对应配置效果如下图所示：

![image-20230731160256346](USB模块的ID引脚和VBUS引脚说明.assets/image-20230731160256346.png)

即可正常使用USB功能。

### 未引出特定硬件ID引脚和VBUS引脚

这里以X2670为例，X2670的DS中关于USB引脚的具体描述如下：

![image-20230728150743405](USB模块的ID引脚和VBUS引脚说明.assets/image-20230728150743405.png)

可知，X2670芯片没有引出特定的硬件ID引脚和VBUS引脚，故其芯片USB硬件层面不支持前面功能描述中说明的功能。具体存在问题和解决方法如下：

#### 未引出特定的硬件ID引脚，存在问题和解决方法

##### 		问题1：无法自动切换OTG的运行模式。

解决方法有3种：

**方法1. 优先建议使用软件覆盖PHY的ID状态（需要SOC支持），实现OTG运行模式切换**

​		硬件上，使用该方式切换OTG的运行模式，需要该SOC支持。

确认方法：这里以使用的X2670为例，可以通过查阅PM手册的Clock Reset and Power Controller 章节，在寄存器列表找到USB0 Reset Detect Timer Register（USB0RDT,0x40），可以发现存在IDDIG_EN和IDDIG_REG，说明该芯片支持通过软件覆盖PHY的ID状态，进而实现OTG的模式切换。

![image-20230802143845353](USB模块的ID引脚和VBUS引脚说明.assets/image-20230802143845353.png)

​		软件上，在menuconfig界面配置不勾选External Id Pin for OTG，效果如下图所示：

![image-20230802203111831](USB模块的ID引脚和VBUS引脚说明.assets/image-20230802203111831.png)

然后，在开发板启动后，通过下述命令找到覆盖PHY的ID状态的软件控制节点。

```
# find / -name "sw_switch_hsotg"
/sys/devices/platform/apb/10000000.otg_phy/sw_switch_hsotg
```

可以发现存在1个控制节点，是属于OTG的。

接着，可以通过下列命令切换OTG的运行模式，并且在执行后有相关的提示信息。

```
# 切换为Host模式
# echo host > /sys/devices/platform/apb/10000000.otg_phy/sw_switch_hsotg
[ 2638.257007] ---Forced switching Host mode!
# 切换为Device模式
# echo device > /sys/devices/platform/apb/10000000.otg_phy/sw_switch_hsotg
[ 2648.432793] ---Forced switching Device mode!
# 根据实际的ID状态确定运行模式
# echo none > /sys/devices/platform/apb/10000000.otg_phy/sw_switch_hsotg
[ 2659.896582] ---switching mode from pin!
```

**注意：使用该方式，硬件VBUS引脚功能不会失效。**



**方法2.通过额外设计，实现自动切换OTG运行模式。**

***该方法同方法1一样，要求SOC支持通过IDDIG_EN和IDDIG_REG寄存器覆盖ID状态。***

​		在硬件上，可以分配一个IO（注意根据DS上拉）替代USB_ID接入到USB座子的ID上。这里假设从SOC分配一个PC27来替换特定的USB_ID。

​		在软件上，menuconfig配置为**不勾选**External Id Pin for OTG（选中则OTG控制器将运行在强制模式下，运行模式仅受软件控制，不受硬件的ID和VBUS的状态影响），效果如下图示：

![image-20230802203111831](USB模块的ID引脚和VBUS引脚说明.assets/image-20230802203111831.png)

同时，需要在使用的PHY节点添加如下描述信息

```
ingenic,id-dete-gpio = <&gpc 27 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
```

GPIO_ACTIVE_HIGH意味着PC27上的电平为低电平时，OTG为Host模式；当PC27上电平为高时，OTG为device模式。

设备树配置效果如下图所示：

![image-20230818150147774](USB模块的ID引脚和VBUS引脚说明.assets/image-20230818150147774.png)

这样，就实现了当PC27为低电平时，OTG运行模式为Host；当PC27为高电平时，OTG运行模式为Device。



**方法3. 通过软件配置OTG控制器的强制运行模式，实现OTG运行模式切换（不建议）**

​		软件上，配置页面上**勾选** External Id Pin for OTG选项，禁用特定硬件ID引脚功能，配置效果如下：

![image-20230731162038101](USB模块的ID引脚和VBUS引脚说明.assets/image-20230731162038101.png)

**注意：在勾选 External Id Pin for OTG 选项后，将导致OTG控制器运行在强制模式模式下，ID和VBUS信息由软件配置值覆盖，不会受相关硬件的ID和VBUS引脚状态的影响，因此特定的硬件ID和VBUS引脚的功能将失效。**

勾选 External Id Pin for OTG选项后，将生成 **/sys/class/usb_role/具体DWC2控制器节点名/role** 节点，可以通过下面命令切换OTG运行模式：

```
# 切换到Host模式
echo host > /sys/class/usb_role/具体DWC2控制器节点名/role
# 切换到Device模式
echo device > /sys/class/usb_role/具体DWC2控制器节点名/role
# 清除
echo none > /sys/class/usb_role/具体DWC2控制器节点名/role
```



#### 未引出特定的硬件VBUS引脚，存在问题和解决方法

##### 		问题1：在Host模式下，过流检测异常

在SOC未引出特定的硬件VBUS引脚的情况下，必须勾选 External Vbus Detect for OTG 项来屏蔽PHY的硬件VBUS功能。若不勾选的情况下，插入设备会一直触发过流检测，host模式无法正常使用。具体配置效果如下图：

![image-20230731163752968](USB模块的ID引脚和VBUS引脚说明.assets/image-20230731163752968.png)

##### 		问题2：在Device模式下，BC1.2探测异常

在Device模式下，要正常使用BC1.2，SOC要有特定的硬件VBUS引脚（目前软件仅支持1600系列）。

##### 		问题3：在Device模式下，无法检测USB的插入拔出状态

解决方法分2种：

**方法1. 不需要检测USB的插入拔出状态**

​		这种情况下，仅需软件上需要屏蔽PHY的硬件VBUS功能。配置方法为通过menuconfig，开启External Vbus Detect for OTG选项即可。配置效果如下图：

![image-20230731163752968](USB模块的ID引脚和VBUS引脚说明.assets/image-20230731163752968.png)

**方法2. 需要检测USB插入拔出状态**

​		硬件上：需要从SOC的引脚里面分配1个IO，替代VBUS（注意根据DS确保分压到IO能接受的电平，避免损坏SOC）连接到USB接口的VBUS上。硬件设计可参考下图：

![image-20230803103438807](USB模块的ID引脚和VBUS引脚说明.assets/image-20230803103438807.png)

​		软件上：首先在menuconfig配置页面，开启External Vbus Detect for OTG选项，屏蔽PHY的硬件VBUS功能。然后，根据实际分配的IO来配置设备树中的对应PHY属性。

这里假设分配PC00替代VBUS连接到USB接口的VBUS上，那么需要在使用的对应PHY设备节点中添加如下内容：

```
ingenic,vbus-dete-gpio = <&gpc 00 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
```

GPIO_ACTIVE_HIGH 意味当PC00的电平为高，则检测到接入；若PC00为低电平时，则检测到连接断开。

phy设备树节点配置效果类似下图所示：

![image-20230818134424155](USB模块的ID引脚和VBUS引脚说明.assets/image-20230818134424155.png)

即可通过其他IO替代特定的VBUS引脚，实现USB的插入拔出检测。



#### 无特定的硬件DRVVBUS引脚，存在问题和解决方法

##### 问题1：在Host模式下，无法控制VBUS的输出状态

解决方法分2种：

**方法1. 不需要控制VBUS输出状态，一直输出VBUS。**

**方法2. 通过特殊设计，实现对VBUS输出状态控制，具体方法如下：**

​		硬件上，可以根据实际情况，任意分配一个IO接入到类似WS4612EAA-5/TR芯片的VBUS输出控制芯片EN上，在满足某些条件后，再通过IO使能类似WS4612EAA-5/TR的芯片输出VBUS给USB设备供电。具体参考下图设计：

![image-20230801114137857](USB模块的ID引脚和VBUS引脚说明.assets/image-20230801114137857.png)

这里假设分配PB28接入VBUS输出控制芯片WS4612EAA-5/TR的EN上，VBUS输出控制芯片WS4612EAA-5/TR输出条件为PB28为高电平。

​		在软件上，需要在使用的PHY设备节点下添加如下内容：

```
ingenic,drvvbus-gpio = <&gpb 28 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
```

GPIO_ACTIVE_HIGH 说明当PB28输出高电平时，使能输出VBUS；当PB28输出低电平时，停止输出VBUS。

设备树配置效果图如下：

![image-20230731191517313](USB模块的ID引脚和VBUS引脚说明.assets/image-20230731191517313.png)

即可实现当ID为低电平（作为Host）时，PB28会对应输出高电平，进而控制VBUS输出。

