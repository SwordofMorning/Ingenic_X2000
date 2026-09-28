#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/input.h>
#include <poll.h>
#include <dirent.h>
#include <sys/ioctl.h>

#define DEV_NUM 50
#define EVENT_NUM 30

struct event_info{
    int fd;
    int num;
    struct input_event event[EVENT_NUM];
};

struct dev_info {
    int fd;
    char name[20];
    char path[30];
};

static char *str_etype(int type)
{
    char *str = NULL;
    switch (type)
    {
    case EV_SYN:
        str = "EV_SYN";
        break;
    case EV_KEY:
        str = "EV_KEY";
        break;
    case EV_ABS:
        str = "EV_ABS";
        break;
    default:
        str = "EV";
        break;
    }

    return str;
}


static void usage(int cmd)
{
    switch (cmd)
    {
    case 0:
        printf("*************************\n");
        printf("listen input dev\n");
        printf("example:\n");
        printf("    default: ./input_listen     (listen /dev/input directory all dev.)\n");
        printf("    path   : ./input_listen  /dev/input/event0   (listen  dev /dev/input/event0.)\n");
        printf("*************************\n");
        exit(0);
        break;

    default:
        break;
    }
}

/*  匹配数组里面的fd, 返回下标
*/
static int find_index(int fd, struct dev_info *dev_set, int n_dev)
{
    int ret = -1;
    for (int i = 0; i < n_dev; i++) {
        if (dev_set[i].fd == fd) {
            ret = i;
            break;
        }
    }
    return ret;
}

/* 判断code的指定值, 判断input_event中哪些是坐标信息
*/
static void get_coordinate(struct input_event *event, int *xy)
{
    if (event->code == 53)
        xy[0] = event->value;

    if (event->code == 54)
        xy[1] = event->value;
}

/*  打印事件
**  e_info[]: 事件数组
**  dev_set : 设备信息数组
**  dev_index : 对应设备信息下标
*/
static void event_buf_handle(struct event_info e_info, struct dev_info *dev_set, int dev_index)
{
    if (dev_index >= DEV_NUM) {
        printf("not find dev info.\n");
        return;
    }

    printf("\ndev: %s, path: %s.\n", dev_set[dev_index].name, dev_set[dev_index].path);
    if (e_info.num <= 0) {
        fprintf(stderr, "no event to printf.\n");
        return;
    }

    int i;
    int xy[2] = {0,0};      //  存储坐标(x, y)
    for (i = 0; i < e_info.num; i++) {   /* 一个设备有多个事件 */
        struct input_event *event = &e_info.event[i];

        printf("%s : %s (code: %d, type: %d, value: %d).\n",
            dev_set[dev_index].name ,str_etype(event->type),
            event->code, event->type, event->value);

        if (event->type == EV_ABS)
            get_coordinate(event, xy);
    }

    if (e_info.event[0].type == EV_ABS)       /* 抽离, 打印坐标信息(x, y) 第5,6个事件是坐标值*/
        printf("%s : (x,y)->(%d, %d).\n", dev_set[dev_index].name, xy[0], xy[1]);
}

/*
**  轮询fd, 判断哪些事件是就绪状态, 将事件信息保存到数组
**  返回值: 事件组数的大小
**
**  p_fd      : poll 结构体数组 包含相关信息 fd revent事件
**  event_buf : 接受完成且有用的event事件
**  n_fd:   : 监测fd的数量
*/
static int poll_read(struct pollfd *p_fd, struct event_info *e_info, int n_fd)
{
    int n_info = 0;                              /* 筛选过后的事件数量, 用于函数返回 */
    struct input_event buf[EVENT_NUM];           /* 数据缓冲区, 用于read接受 */
    for (int i = 0; i < n_fd; i++) {

        if (!(p_fd[i].revents & POLLIN))         /* 只监测pollin事件 */
            continue;

        int n = read(p_fd[i].fd, buf, sizeof(buf));
        if (n < 0) {
            fprintf(stderr, "read fd: %d fail...\n", p_fd[i].fd);
            continue;
        }

        n = n / sizeof(struct input_event);     /* 计算需要处理的事件数量 */
        e_info[n_info].num = n;                 /* 赋值本次接受到的event数量和fd */
        e_info[n_info].fd  = p_fd[i].fd;

        /* 遍历事件信息, 保存.
        */
        for (int j = 0; j < n; j++) {
            e_info[n_info].event[j] = buf[j];
        }
        n_info++;
    }
    return n_info;
}

/*  获取一个设备的名称, 文件路径, 保存到结构体dev_set
*/
static int set_dev_info(char *file_path, struct dev_info *dev_set)
{
    int ret;
    struct stat stat_buf;
    ret = stat(file_path, &stat_buf);
    if (ret < 0) {
        fprintf(stderr, "file stat fail. path : %s\n", file_path);
        return -1;
    }

    /* /dev/input/event0 为字符设备
    */
    if(!S_ISCHR(stat_buf.st_mode)) {
        fprintf(stderr, "no find input event dev.\n");
        return -1;
    }

    int fd = open(file_path, O_RDONLY | O_NDELAY);
    if (fd < 0) {
        fprintf(stderr, "open file fail. path : %s\n", file_path);
        return -1;
    }

    /*  获取设备名称
    */
    char buf[30];
    ret = ioctl(fd, EVIOCGNAME(sizeof(buf)-1), &buf);
    if (ret == -1) {
        fprintf(stderr, "ioctl find name fail. path : %s\n", file_path);
        return -1;       // 允许设备驱动没有名称
    }

    /*  将查询到的设备名称, 路径, 存储到dev_set结构体.
    */
    dev_set->fd = fd;
    strcpy(dev_set->name, buf);
    strcpy(dev_set->path, file_path);
    return 1;
}

/*  获取文件目录下, 普通文件节点, open文件节点, 返回fd集合.
**  返回值     : 设备的数量
**
**  char *path : 文件目录
**  struct dev_info *dev_set: 设备相关信息缓冲区
*/
static int get_dev_fd_name(char *path, struct dev_info *dev_set)
{
    /* 判断文件是节点 还是目录, 如果是目录将符合条件的节点, */
    int i = 0;                 /* 存储fd_set的数组下标, 用于函数返回 */
    int ret;
    ret = access(path, F_OK);
    if (ret != 0) {
        fprintf(stderr, "file path not exist.\n");
        usage(0);
    }

    struct stat stat_buf;
    ret = stat(path, &stat_buf);

    /* 指定文件路径是设备 */
    if (S_ISCHR(stat_buf.st_mode)) {
        ret = set_dev_info(path, &dev_set[i]);
        if (ret == -1) {
            fprintf(stderr, "set_dev_info fail.\n");
            return -1;
        }
        return ret;
    }

    /* 指定文件路径是目录 */
    if (!S_ISDIR(stat_buf.st_mode)) {
        fprintf(stderr, "path is not a dir or a input dev.\n");
        return -1;
    }

    /*  遍历文件节点
    */
    if (strcmp(path, "/dev/input")) {
        fprintf(stderr, "directory only support /dev/input.\n");
        return -1;
    }

    DIR *dir;
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "opendir fail path:%s.\n", path);
        return -1;
    }

    struct dirent *file_node = NULL;
    while ((file_node = readdir(dir))) {
        /*  过滤两个文件 */
        if (strcmp(file_node->d_name,".") == 0 ||
            strcmp(file_node->d_name,"..") == 0)
            continue;

        /*  组成文件节点路径
        */
        char file_path[100];
        bzero(file_path, sizeof(file_path));

        strcat(file_path, path);
        strcat(file_path, "/");
        strcat(file_path, file_node->d_name);

         /*  查询文件节点属性, 且保存信息
         */
        int ret = set_dev_info(file_path, &dev_set[i]);
        if (ret == -1) {
            fprintf(stderr, "set_dev_info fail.\n");
            continue;
        }

        if ( i >= DEV_NUM - 1) {        /* 添加的数量到达最大值 */
            fprintf(stderr, "please set bigger DEV_NUM.\n");
            break;
        }
        i++;
    }
    return i;
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "please set correct parameter.\n");
        usage(0);
    }

    char *path = "/dev/input";      //  默认路径
    if (argc == 2)
        path = argv[1];

    if (!strcmp(path, "-h") ||
        !strcmp(path, "-H") ||
        !strcmp(path, "-help"))
        usage(0);

    /*  通过给定的路径, 获取设备信息
    */
    struct dev_info dev_set[DEV_NUM];
    int n_fd = get_dev_fd_name(path, dev_set);
    if (n_fd < 1) {
        fprintf(stderr, "find dev node error.\n");
        return -1;
    }

    for (int i = 0; i < n_fd; i++) {
        printf("listen device: %s path: %s.\n",
            dev_set[i].name, dev_set[i].path);
    }

    /* 赋值操作, 填充poll_fd结构体.
    */
    struct pollfd p_fd[n_fd];
    bzero(p_fd, sizeof(p_fd));
    for (int i = 0; i < n_fd; i++) {
        p_fd[i].fd = dev_set[i].fd;
        p_fd[i].events = POLLIN;        /* POLLIN: 可读事件 */
    }

    struct event_info event_buf[n_fd];     /* poll 事件, 有多少个fd同时到达 */
    while (1) {
        /* poll监测, 有可读事件, 函数有返回值.
        ** -1 : 一直等待; nready:当前就绪fd数量.
        */
        int nready = poll(p_fd, n_fd, -1);
        if (nready <= 0 || nready > n_fd) {
            fprintf(stderr, "poll error.\n");
            continue;
        }

        /* 监测到有事件, 轮询fd, 获取event_buffer
        ** 获取到信息后进行处理, 打印.
        */
        bzero(event_buf, sizeof(event_buf));
        int ret = poll_read(p_fd, event_buf, n_fd);
        if (ret != nready)
            fprintf(stderr, "event omit.\n");

        for (int i = 0; i < nready; i++) {
            int fd_index = find_index(event_buf[i].fd, dev_set, n_fd);
            if (fd_index < 0) {
                fprintf(stderr, "find fd index fail.\n");
                continue;
            }
            event_buf_handle(event_buf[i], dev_set, fd_index);      /* 处理一个设备的input_event */
        }
    }

    return 0;
}
