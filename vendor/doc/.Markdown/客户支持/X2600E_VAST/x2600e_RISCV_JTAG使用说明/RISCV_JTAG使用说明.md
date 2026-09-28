# RISCV_JTAG

## 一 简介

- 君正平台下jtag 的调试依赖于君正的jtag调试器, 调试的板⼦必须有jtag 接⼝
- x2600 小核使用jtag 调试, 必须在大核正常运行的情况下进行
- 君正仅提供vscode 的调试环境

## 二 硬件以及vscode环境搭建

以PD_X2600_EVB_V1.0开发板为例, 将君正jtag盒子与板子相接, 注意接的是riscv 小核的jtag 接口

![12](RISCV_JTAG使用说明.assets/001.png)

然后在pc端输入：

```c
bhu@bhu-PC:~/work$ adb devices
List of devices attached
ingenic_dev     device                       
ingenic,jdi     device                          //若出现ingenic,jdi则说明连接成功

```

小核的工程目录在: <工程目录>/libmcu ,用vscode 打开libmcu文件夹

确认硬件jtag io电压多少, 如下图所示, 这里以君正x2600_evb_1.0 的针板为例, 根据原理图所示是3.3V：

![-1](RISCV_JTAG使用说明.assets/002.png)

将libmcu/example/jtag_riscv_x2600_debug/profile_io_3.3v 目录下的所有文件复制到.vscode 目录下（若原理图接的是1.8v 就接复制profile_io_1.8v的）

![15](RISCV_JTAG使用说明.assets/003.png)

## 三 配置流程

### 3.1 xburst2 大核配置以及编译

打开iconfig 工具,选择配置文件 , 这里以x2600e_nand_5.10_defconfig为例 , xburst2 大核配置流程如下：

![0](RISCV_JTAG使用说明.assets/004.png)

![1](RISCV_JTAG使用说明.assets/005.png)

![2](RISCV_JTAG使用说明.assets/006.png)

![3](RISCV_JTAG使用说明.assets/007.png)

![4](RISCV_JTAG使用说明.assets/008.png)

> **注意：大核使能riscv 小核jtag需要用到如下io脚，请注意预留，不要使用别的功能**

![5](RISCV_JTAG使用说明.assets/009.png)

保存配置

![13](RISCV_JTAG使用说明.assets/010.png)

回到build 目录下编译：

```shell
make x2600e_nand_5.10_defconfig
make 
```

编译出来的固件在ouput/目录下，将固件烧录到板子上

### 3.2 riscv 小核代码的配置

小核的代码工程目录为：<君正工程目录>/libmcu 文件夹, 重新打开iconfig配置小核, 这里以x2600_test_defconfig 为例子：

![6](RISCV_JTAG使用说明.assets/011.png)

![7](RISCV_JTAG使用说明.assets/012.png)

![8](RISCV_JTAG使用说明.assets/013.png)

![9](RISCV_JTAG使用说明.assets/014.png)

![10](RISCV_JTAG使用说明.assets/015.png)

保存配置

![14](RISCV_JTAG使用说明.assets/016.png)

进入到libmcu目录编译配置：

```shell
export PATH=$PWD/../tools/toolchains/riscv-r1.0.0/bin:$PATH   #将riscv 工具链添加到环境变量
make x2600_test_defconfig								      #编译配置
```

**当x2600_test_defconfig配置⽂件有改动时, 需要再make x2600_test_defconfig, 在vscode 调试才能⽣效**

若编译出现如下报错

![16](RISCV_JTAG使用说明.assets/017.png)

修改链接的库:

```c
bhu@bhu-PC:/usr$ find -name  'libmpfr.so.*'                     //查看libmpfr目录
find: ‘./share/uadp’: 权限不够
./lib/x86_64-linux-gnu/libmpfr.so.6
./lib/x86_64-linux-gnu/libmpfr.so.6.0.2
./lib/x86_64-linux-gnu/libmpfr.so.4
bhu@bhu-PC:/usr/lib/x86_64-linux-gnu$ sudo ln -s libmpfr.so.6 libmpfr.so.4          //链接到libmpfr.so.4 

```

## 四 运行

按F5 vscode 会去运行.vscode 下的launch.json 脚本.

主要做了编译程序、初始化jtag 盒子以及运行一系列gdb命令, 使程序加载到目标板子上,详见补充说明

![11](RISCV_JTAG使用说明.assets/018.png)

> **当大核断电或者复位之后, jtag 盒子也需要重新插拔断电一下, 然后重新开始调试**

## 五 补充说明

tasks.json文件说明如下：

```json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "make_project",      //定义编译任务
            "type": "shell",
            "command":"export PATH=${workspaceFolder}/../tools/toolchains/riscv-r1.0.0/bin:$PATH;make clean;make -j8"        //配置工具链环境变量，并编译
        },
        {
            "label": "ingenic jtag init",	//定义初始化君正jtag盒子任务																							
            "type": "shell",
            "command": [
            	//初始化并使能jtag 盒子的指示灯
                "adb -s ingenic,jdi shell \"/led_status.sh 0\";" 
            	//初始化jtag盒子的引脚(可选1.8v 、3.3v)
                "adb -s ingenic,jdi shell \"jtag_en.sh 3.3V\";",	
            	//运行openocd应用，指定需要调试的板子为x2600的riscv 小核
                "adb -s ingenic,jdi shell \"nohup openocd -f /openocd/interface/x2100-jdi-v1_1.cfg -f /openocd/target/x2600-risc.cfg > /dev/null\";", 																			//建⽴转发，为gdb 的连接做准备																																																
                "adb -s ingenic,jdi forward tcp:3333 tcp:3333;",
            ]
        },
        {
            "label":"build",             //定义工作任务，由launch.json 调用。其依赖上述两个任务
            "dependsOn":[
                "ingenic jtag init",
                "make_project",
            ]
        },
    ]
}
```

launch.json 说明如下:

```json
{
    "version": "0.2.0",
    "configurations": [

       // GDB Debugging:
       {
          "program": "${workspaceFolder}/libmcu-bare.elf",                               //编译⽣成的 .elf ⽂件
          "name": "Launch (gdb)",
          "request": "launch",
          "args": [],
          "stopAtEntry": false,
          "cwd": "${workspaceFolder}",																		//⼯程路径
          "externalConsole": false,
          "internalConsoleOptions": "openOnSessionStart",
          "type": "cppdbg",
          "MIMode": "gdb",
          "miDebuggerPath": "${workspaceFolder}/../tools/toolchains/riscv-r1.0.0/bin/riscv32-ingenicv0-elf-gdb",     //riscv 工具链下的gdb工具
          "miDebuggerArgs": "",
          "miDebuggerServerAddress": "localhost:3333",
          "targetArchitecture": "mips",
          "preLaunchTask": "build",																															// 与task.json 的"label" 字段做匹配
          "customLaunchSetupCommands": [																									//gdb默认启动时，默认运⾏的命令，可根据需要⾃⾏添加
             {
                "description": "gdb 启用整齐打印",
                "text": "-enable-pretty-printing",
                "ignoreFailures": true
             },
             {
                "text":"file ${workspaceFolder}/libmcu-bare.elf",															//gdb 加载调试文件
                "ignoreFailures": false
             },
             {
                "text": "target remote localhost:3333",																				  //链接调试器
                "ignoreFailures": false
             },
             {
                "text": "monitor reset halt",
                "ignoreFailures": false
             },
             {
                "text": "load",																																  //将文件下载到目标设备上
                "ignoreFailures": false
             },
             {
                "text": "source  ${workspaceFolder}/.vscode/connect_with_xburst2.gdb",   //运行的gdb 脚本，详见下面
                "ignoreFailures": false
             },
          ],
          "logging": {
             "engineLogging": true,
          }
       }
    ]
 }
```

connect_with_xburst2.gdb脚本，主要是通知大核riscv 小核将运行在ddr的哪个地址上，说明如下:

```c
set $cnt=5                                                                             					//定义超时时间 5s
set $entry_point=(unsigned int)&_start                                  //获取加载程序的_start 段，既程序入口地址

set $ccu_to_host = 0x12a0000c												    //定义变量ccu_to_host, 大核寄存器，写该寄存器会触发大核中断
set $reset_pc_value = 0x12a00004										    //定义变量reset_pc_value，大核寄存器，大核配置的小核复位入口地址，这里复用为通知该脚本

set {int}$reset_pc_value = 0														  //设置resert_pc_value 为0
set {int}$ccu_to_host = $entry_point | 0x80000000           //写入口地址通知触发大核中断

//轮询reset_pc_value 和 ccu_to_host 寄存器的值，大核触发中断后会将将ccu_to_host 清0 并将reset_pc_value 置成 程序入口地址，超时5s退出
while ($cnt)
    print "riscv reset pc value is: "
    print *(int*)$reset_pc_value
    if (*(int*)$ccu_to_host == 0 && *(int*)$reset_pc_value == $entry_point)
        set $cnt = 0
    else
        set $cnt=$cnt-1
        shell sleep 1
        if ($cnt==0)
            print "wait xburst2 ack timeout\n"
            quit -1
        end
    end
end
```

