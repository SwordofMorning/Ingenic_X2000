# MONITOR_USB_STATE

## 1. 监听usb状态

1.编译监听usb状态文件

> cd doc/开发使用说明/USB使用说明文档/USB状态监听/
>
> ../../../../buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc usb_state.c -o  usb_state

> usb_state 命令介绍
>
> usb 状态：not attached、attached、powered、reconnecting、unauthenticated、default、addresssed、configured、suspended。
>
> ./usb_state <udc_path> <状态>
>
> ./usb_state  /sys/class/udc/13500000.otg "suspended"
>
> 注意:udc_path 路径不同平台会有差异,x1000平台udc_path路径为/sys/class/udc/dwc2/,x2000平台udc_path路径为/sys/class/udc/13500000.otg

```c
/*
 功能：监听usb状态
*/
int main(int argc, char *argv[])
{
    int fd;
    fd_set efds;
    int ret, len;
    char buf[128];
    char *usb_state = NULL;
    const char *udc_path = NULL;

    char state_buf[128];

    prg_name = argv[0];

    if (argc != 3)
        usage(-1);

    udc_path = argv[1];
    usb_state = argv[2];

    memset(state_buf, 0, sizeof(state_buf));
    sprintf(state_buf, "%s/state", udc_path);
    fd = open(state_buf, O_RDONLY);
    if (fd < 0) {
        printf("open %s fail\n", state_buf);
        return -1;
    }

    while (1) {
        FD_ZERO(&efds);
        FD_SET(fd, &efds);

        ret = select(fd + 1, NULL, NULL, &efds, NULL);
        if (ret == -1) {
            fprintf(stderr, "select error\n");
            break;
        }

        if (FD_ISSET(fd, &efds)) {
            memset(buf, 0, sizeof(buf));
            len = read(fd, buf, sizeof(buf));
            ret = strncmp(buf, usb_state, len - 1);
            if (ret == 0) {
                printf("usb state : %s\n", usb_state);
                break;
            }

            lseek(fd, 0, SEEK_SET);
        }
    }

    close(fd);

    return 0;
}
```

