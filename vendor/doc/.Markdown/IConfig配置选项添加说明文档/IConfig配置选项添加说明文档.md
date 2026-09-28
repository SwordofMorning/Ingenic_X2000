# IConfig配置选项添加说明文档

## 1.菜单以及菜单选项的定义

### 1.1.config选项

>   config [选项]

使用该关键字后续必须指定该选项类型：

```c
config MD_HELLO_WORLD
    bool "hello world模块"          #选项配置，详见第二章
```

其显示效果如下：

<img src="img/1.png" style="zoom:75%;" />

### 1.2.menuconfig选项

>   menuconfig [菜单选项]
>
>    bool "菜单名字"
>
>   endmenu

menuconfig定义一个可选的二级菜单，需要与"endmenu"一起使用：

```c
menuconfig MD_HELLO_WORLD_MENU
    bool "hello world模块菜单"

config MD_HELLO_WORLD_BUTTON
    bool "hello world选项1"

endmenu
```

其显示效果如下：

<img src="img/2.png" style="zoom:75%;" />

<img src="img/3.png" style="zoom:75%;" />

### 1.3.menu选项

>   menu "菜单名字"
>
>   endmenu

menu用来定义一个菜单：

```makefile
menu "hello world模块菜单2"

endmenu
```

其显示效果如下：

<img src="img/4.png" style="zoom:75%;" />

### 1.4.choice选项

>   choice "选项名字"
>
>   config [选项]
>
>   config [选项] ........
>
>   endchoice

choice 用来定义一个选项列表，若不指定默认选项，其默认选择第一个选项。choice中只支持"bool"类型的config选项。

```c
choice "helloworld选项"

config MD_HELLO_WORLD_BUTTON1
    bool "hello world选项1"

config MD_HELLO_WORLD_BUTTON2
    bool "hello world选项2"

endchoice
```

其显示效果如下：

<img src="img/9.png" style="zoom:75%;" />

<img src="img/10.png" style="zoom:75%;" />

## 2.配置菜单选项

### 2.1.声明菜单选项的类型以及显示名字

>   [类型] [在菜单上显示的名字]

类型有：

1.bool：表示该选项对应的宏为布尔值（y/n)

```c
config MD_HELLO_WORLD
    bool "hello world模块"
```

其显示效果为：<img src="img/1.png" style="zoom:75%;" />



2.int：表示该选项的类型为int值，可配置该选项的值

```c
config MD_HELLO_WORLD_BUTTON
    int "hello world选项1"

endmenu
```

其显示效果为：

<img src="img/5.png" style="zoom:75%;" />

3.string：表示该选项的对应的宏的类型为字符串

```c
config MD_HELLO_WORLD_BUTTON
    string "hello world选项1"
```

其显示效果为：

<img src="img/6.png" style="zoom:75%;" />

### 2.2.depends on关键字

>   depends on [已配置好的目标选项]

depends on 关键字决定当前选项所依赖的选项，只有所依赖的选项被选中，才可以对该选项进行配置，depends on的目标选项只能是"bool"类型或者"menuconfig"类型的。

```c
config MD_HELLO_WORLD_BUTTON1
    bool "hello world选项1"

config MD_HELLO_WORLD_BUTTON2
    bool "hello world选项2"
    depends on MD_HELLO_WORLD_BUTTON1choice
```

### 2.3.select关键字

>   select [已配置好的目标选项]

select关键字配置的选项会在该选项选中的时候，自动选中select修饰的目标选项。select修饰的目标选项只能是"bool"类型或者是"menuconfig"类型的。

```c
config MD_HELLO_WORLD_BUTTON1
    bool "hello world选项1"

config MD_HELLO_WORLD_BUTTON2
    bool "hello world选项2"
    select MD_HELLO_WORLD_BUTTON1
```



### 2.4.choice from关键字

>   choice from [列表]

choice from关键字表示该选项的值从一个列表内选择，列表的定义如下：

>   array [列表]
>
>   [类型] "列表名"
>
>   item "列表选项名" [选项的值]
>
>   item "列表选项名" [选项的值] ........

注意：列表的类型不可为"bool"，选项的值请按照列表类型来定义

如以下例子：

```c
array MD_HELLO_WORLD_LIST
    string "helloworld 选项"
    item "选项1" helloworld1
    item "选项2" helloworld2

config MD_HELLO_WORLD_BUTTON2
    string "hello world选项2"
    choice from MD_HELLO_WORLD_LIST
```

其显示效果为：

<img src="img/7.png" style="zoom:75%;" />

### 2.5.default关键字

>   default [值]

default用来配置选项的默认值。请根据选项的类型来定义相应的默认值

```c
config MD_HELLO_WORLD_BUTTON1
    bool "hello world选项1"
    default n
```

### 2.6.help关键字

>   help（回车）
>
>   [内容]

help关键字用来添加选项的说明信息。

```c
config MD_HELLO_WORLD_BUTTON1
    bool "hello world选项1"
    help
        这是一个测试说明选项
```

其显示效果如下所示：

<img src="img/8.png" style="zoom:75%;" />

## 3. 引用.in配置文件说明

### 3.1 source关键字

>   source [文件路径]

其中文件路径是相对于IConfig打开的.in文件的相对路径，即source的包含路径

source关键字用来引用Iconfig配置文件（.in），如：

```makefile
config MD_HELLO_WORLD_MENU
    bool "hello world模块菜单"

source package/devices/helloworld_button1/Config.in
source package/devices/helloworld_button2/Config.in
```

package/devices/helloworld_button1/Config.in的文件内容为：

```makefile
config MD_HELLO_WORLD_BUTTON1
    bool "helloworld button1"
```

package/devices/helloworld_button1/Config.in的文件内容为：

```makefile
config MD_HELLO_WORLD_BUTTON2
    bool "helloworld button2"
```

其显示效果如下：

<img src="img/11.png" style="zoom:75%;" />

source关键字等价于把source的文件内容，写入到文件的当前位置，如下：

```makefile
config MD_HELLO_WORLD_MENU
    bool "hello world模块菜单"

config MD_HELLO_WORLD_BUTTON1
    bool "helloworld button1"

config MD_HELLO_WORLD_BUTTON2
    bool "helloworld button2"
```
