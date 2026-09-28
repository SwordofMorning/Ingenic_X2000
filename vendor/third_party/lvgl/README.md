## lvgl 注意事项
请在`iconfigtool`中选中`APP_lvgl`,即可在`build`目录中编译本工程

0. 下载 `lvgl` 源码
    当前目录中没有`lvgl`目录时,`Makefile`会在编译时自动从`github`下载`lvgl`源码
    当前下载的版本是`v8.2.0`  
    用户也可以把自己下载的`lvgl`源码放到此目录使用

1. `lv_conf.h` 文件\
    `lv_conf.h` 是`lvgl`的配置文件,请自行查看`lvgl`的说明文档  
    如果当前目录中没有 `lv_conf.h` 文件,那么会从自带`lv_conf_default.h`复制  
    用户也可以把自己下载的`lv_conf.h`放到此目录使用

## 如何使用
1. 假设工程目录源文件`vendor/main_lv_demo_widgets.c`
    ```c
    /* file: vendor/main_lv_demo_widgets.c
     */
    #include <stdio.h>
    #include <assert.h>
    #include "lvgl/lvgl.h"
    #include "lvgl/examples/lv_examples.h"
    #include "lvgl/demos/lv_demos.h"

    #include "lvgl_ingenic_support.h"

    int main(int argc, char *argv[])
    {
        const char *fb_path = "/dev/fb0";
        const char *tp_path = "/dev/input/event0";

        lv_init();

        int ret;
        ret = lvgl_init_fb_display(fb_path);
        assert(!ret);

        ret = lvgl_init_tp_input(tp_path);
        // assert(!ret);

        lv_demo_widgets();

        lvgl_usleep_loop(10*1000);

        return 0;
    }
    ```
2. 编译`lvgl`
    在`iconfigtool`中选中`APP_lvgl`
    然后编译
    ```shell
    cd build
    make app_third_party/lvgl
    cd ..
    ```
3. 编译`vendor/main_lv_demo_widgets.c`
    ```shell
    cd vendor
    # 使用buildroot中的工具链,方便自动找到对应的lib
    CC=../buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc

    CFLAGS=-DLV_CONF_INCLUDE_SIMPLE -I../third_party/lvgl/
    LDFLAGS=-lhardware2 -lutils2 -llvgl -Wl,--gc-sections
    
    $CC main_lv_demo_widgets.c $CFLAGS $LDFLAGS -o lv_demo_widgets
    ```
    |编译选项                    | 功能                                 |
    |:---                       |:---                                 |
    |`-DLV_CONF_INCLUDE_SIMPLE` | 确保`lv_conf.h`被正确`include`
    |`-I../third_party/lvgl/`   | 将`lvgl`加入`include`搜索路径
    |`-llvgl`                   | 使用 `liblvgl.a`
    |`-lhardware2 -lutils2`     | `liblvgl.a` 中有用到相关接口
    |`-Wl,--gc-sections`        | 清除未使用到的`lvgl`模块,减少生成文件大小
