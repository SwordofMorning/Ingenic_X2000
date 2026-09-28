# USB_HID

## 1. menuconfig 配置流程

```c
Device Drivers  --->
         [*] USB support  --->
                  <*>   USB Gadget Support  --->
                             <*>   USB Gadget Drivers
                                         (USB functions configurable through configfs)  --->
                             [*]       HID function
```

## 2. keyboard设备使用流程

1. 在终端执行keyboard设备参数配置脚本 （文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_HID/hid_device_config.sh ）

   > 命令：./hid_device_config.sh start keyboard

2. 在终端执行HID应用程序（文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_HID/gadget_hid.c）

   > 命令：./gadget_hid keyboard /dev/hidg0

```c
准备windows电脑打开设备管理器 -> 键盘，查询是否存在新插入的键盘设备

打开任意一个记事本，等待linux端输入
a b c d e --left-shit （现象：ABCDE）
--return （现象：回车）
a b c d e --hold （现象：一直输出abcde）
a b c d e f （现象：abcdef）
--return （现象：回车）
--caps-lock （现象：锁定大写）
a b c d e （现象：ABCDE）
--quit （现象：退出）
注意： 单次输入的最大命令长度为6个字节，每个命令之间空格分割
```

## 3. keyboard设备和按键示例

1.编译keyboard设备和按键示例文件gadget_hid_wakeup_keyboard.c

> ../../../../../buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc gadget_hid_wakeup_keyboard.c -lhardware2 -lpthread  -o  gadget_hid_wakeup_keyboard

> gadget_hid_wakeup_keyboard命令介绍
>
> ./gadget_hid_wakeup_keyboard <udc_path>  <hid设备节点>
>
> ./gadget_hid_wakeup_keyboard /sys/class/udc/13500000.otg  /dev/hidg0
>
> 注意:udc_path 路径不同平台会有差异,x1000平台udc_path路径为/sys/class/udc/dwc2/,x2000平台udc_path路径为/sys/class/udc/13500000.otg

```c
/*
 功能：读取按键的键值，通过hid上报给PC键值，当PC端休眠时，则唤醒PC端
*/
int main(int argc, char **argv)
{
    int fd;
    int state;
    int len, i, ret;
    char report[8];
    int timeout = -1;
    pthread_t tid;
    const char *udc_path = NULL;
    const char *device_name = NULL;
    struct key_event event;

    char srp_path[128];
    char state_path[128];

    unsigned char hid_key, f_hid_key;

    prg_name = argv[0];


    if (argc != 3)
        usage(-1);

    udc_path = argv[1];
    device_name = argv[2];

    hid_fd = open(device_name, O_RDWR | O_NONBLOCK, 0666);
    if (hid_fd < 0) {
        fprintf(stderr, "open %s fail\n", device_name);
        usage(-1);
        return -1;
    }

    memset(state_path, 0, sizeof(state_path));
    sprintf(state_path, "%s/state", udc_path);

    usb_state_fd = open(state_path, O_RDONLY);
    if (usb_state_fd < 0) {
        close(hid_fd);
        printf("open %s fail\n", state_path);
        return -1;
    }

    /* 创建线程监听PC端下发给键盘的LED的状态 */
    pthread_create(&tid, NULL, read_hid_led_state_thread, NULL);

    memset(report, 0x0, sizeof(report));

    fd = keys_open();
    if (fd < 0)
        return -1;

    /* 监听按键事件 */
    while ((ret = read_key_event(fd, &event, timeout)) > 0) {

        hid_key = key_val[event.key_type];
        f_hid_key = f_key_val[event.key_type];

        /* 按下的是普通按键或者功能按键 */
        if (hid_key || f_hid_key) {
            if (event.is_press) {
                /* 填充功能按键键值 */
                report[0] |= f_key_val[event.key_type];
                /* 填充普通按键的键值 */
                fill_keyboard_value(report, hid_key);

                /* 向hid上报键值 */
                len = write(hid_fd, report, ARRAY_SIZE(report));
                if (len < 0) {
                    /* 上报hid值失败时，返回EAGAIN错误(hid 的端点不是DWC2_L0 打开状态)  */
                    if (errno == EAGAIN) {
                        usleep(50000);
                        memset(srp_path, 0, sizeof(srp_path));
                        sprintf(srp_path, "echo 1 > %s/srp", udc_path);
                        /* 唤醒PC */
                        system(srp_path);
                    }

                    /* 唤醒PC后重新上报数据 */
                    if (write(hid_fd, report, ARRAY_SIZE(report)) != ARRAY_SIZE(report))
                        fprintf(stderr, "write %s fail\n", device_name);
                }
            } else if (!event.is_press) {
                /* 释放功能按键和普通按键 */
                report[0] &= ~f_key_val[event.key_type];
                clear_keyboard_value(report, hid_key);
                if (write(hid_fd, report, ARRAY_SIZE(report)) != ARRAY_SIZE(report)) {
                    fprintf(stderr, "write %s fail\n", device_name);
                }
            }
        }
    }

    keys_close(fd);

    close(hid_fd);
    close(usb_state_fd);

    return 0;
}
```

## 4. mouse设备使用流程

1. 在终端执行keyboard设备参数配置脚本 （文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_HID/hid_device_config.sh ）

   > 命令：./hid_device_config.sh start mouse

2. 在终端执行HID应用程序（文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_HID/gadget_hid.c）

   > 命令：./gadget_hid mouse /dev/hidg0

```c
准备windows电脑打开设备管理器 -> 鼠标，查询是否存在新插入的鼠标设备

在linux端可以直接进行测试
100 （现象：鼠标右移）
-100 （现象：鼠标左移）
0 100 （现象：鼠标下移）
0 -100 （现象：鼠标上移）
--b1 （现象：鼠标左键）
--b2 （现象：鼠标右建）
--b3 （现象：鼠标中键）
--quit （现象：退出）
```