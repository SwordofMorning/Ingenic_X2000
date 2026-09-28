/**
 * File:   input_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-09-07 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/input.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/thread.h"
#include "base/keys.h"
#include "input_thread.h"
#include "base/custom_keys.inc"

#ifndef EV_SYN
#define EV_SYN 0x00
#endif /*EV_SYN*/

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif /*ABS_MT_SLOT*/

#ifndef ABS_MT_PRESSURE
#define ABS_MT_PRESSURE 0x3a
#endif /*ABS_MT_PRESSURE*/

#define DEV_NUM 50
#define EVENT_NUM 30

typedef struct _event_info{
    int fd;
    int num;
    struct input_event event[EVENT_NUM];
} event_info;
typedef struct _run_info_t
{
    int n_fd;
    int fd[DEV_NUM];
    void *dispatch_ctx;
    char *filename[DEV_NUM];
    input_dispatch_t dispatch;
    struct pollfd poll_fd[DEV_NUM];

    event_queue_req_t req;
} run_info_t;

static int set_dev_info(int index, const char *file_path, run_info_t *info)
{
    int ret;
    struct stat stat_buf;
    ret = stat(file_path, &stat_buf);
    if (ret < 0) {
        log_info("file stat fail. path : %s\n", file_path);
        return -1;
    }

    if(!S_ISCHR(stat_buf.st_mode)) {
        log_info("no find input event dev.\n");
        return -1;
    }

    int fd = open(file_path, O_RDONLY | O_NDELAY);
    if (fd < 0) {
        log_info("open file fail. path : %s\n", file_path);
        return -1;
    }

    char buf[30];
    ret = ioctl(fd, EVIOCGNAME(sizeof(buf)-1), &buf);
    if (ret == -1) {
        log_info("ioctl find name fail. path : %s\n", file_path);
        return -1;
    }

    info->fd[index] = fd;
    info->poll_fd[index].fd = fd;
    info->poll_fd[index].events = POLLIN;
    info->filename[index] = tk_strdup(file_path);
    return 1;
}

static int get_dev_fd_name(const char *path, run_info_t *info)
{
    int i = 0;
    int ret;
    ret = access(path, F_OK);
    if (ret != 0) {
        log_info("file path not exist.\n");
        return RET_FAIL;
    }

    struct stat stat_buf;
    ret = stat(path, &stat_buf);

    if (S_ISCHR(stat_buf.st_mode)) {
        ret = set_dev_info(i, path, info);
        if (ret == -1) {
            log_info("set_dev_info fail.\n");
            return -1;
        }
        return ret;
    }

    if (!S_ISDIR(stat_buf.st_mode)) {
        log_info("path is not a dir or a input dev.\n");
        return -1;
    }

    DIR *dir;
    dir = opendir(path);
    if (!dir) {
        log_info("opendir fail path:%s.\n", path);
        return RET_FAIL;
    }

    struct dirent *file_node = NULL;
    while ((file_node = readdir(dir))) {
        if (strcmp(file_node->d_name,".") == 0 ||
            strcmp(file_node->d_name,"..") == 0)
            continue;

        char file_path[100];
        bzero(file_path, sizeof(file_path));

        strcat(file_path, path);
        strcat(file_path, "/");
        strcat(file_path, file_node->d_name);

        int ret = set_dev_info(i, file_path, info);
        if (ret == -1) {
            log_info("set_dev_info fail.\n");
            continue;
        }

        if ( i >= DEV_NUM - 1) {
            log_info("please set bigger DEV_NUM.\n");
            break;
        }
        i++;
    }
    return i;
}

static int poll_read(struct pollfd *p_fd, event_info *e_info, int n_fd)
{
    int n_info = 0;
    struct input_event buf[EVENT_NUM];
    for (int i = 0; i < n_fd; i++) {

        if (!(p_fd[i].revents & POLLIN))
            continue;

        int n = read(p_fd[i].fd, buf, sizeof(buf));
        if (n < 0) {
            continue;
        }

        n = n / sizeof(struct input_event);
        e_info[n_info].num = n;
        e_info[n_info].fd  = p_fd[i].fd;

        for (int j = 0; j < n; j++) {
            e_info[n_info].event[j] = buf[j];
        }
        n_info++;
    }
    return n_info;
}

static int find_index(int fd, run_info_t *info, int n_dev)
{
    int ret = -1;
    for (int i = 0; i < n_dev; i++) {
        if (info->fd[i] == fd) {
            ret = i;
            break;
        }
    }
    return ret;
}

static void close_fd(run_info_t *info)
{
    for (int i=0; i<info->n_fd; i++) {
        close(info->fd[i]);
        TKMEM_FREE(info->filename[i]);
    }
    TKMEM_FREE(info);

}

static ret_t input_dispatch(run_info_t *info)
{
    ret_t ret = RET_FAIL;
    char message[MAX_PATH + 1] = {0};
    tk_snprintf(message, sizeof(message) - 1, "input[%s]", info->filename);
    // log_info("intput code: e.type=%d x=%d y=%d, preesed = %d\n",
    //       info->req.event.type, info->req.pointer_event.x, info->req.pointer_event.y, info->req.pointer_event.pressed);

    ret = info->dispatch(info->dispatch_ctx, &(info->req), message);
    info->req.event.type = EVT_NONE;
    return ret;
}

static void event_buf_handle(event_info e_info, run_info_t *info, int index)
{
    int i;
    event_queue_req_t *req = &(info->req);
    if (index >= DEV_NUM) {
        log_info("not find dev info.\n");
        return;
    }

    if (e_info.num <= 0) {
        log_info("no event to printf.\n");
        return;
    }

    for (i = 0; i < e_info.num; i++) {
        struct input_event *event = &e_info.event[i];

        if (event->type == EV_SYN) {
            if (event->code == SYN_REPORT) {
                if (req->event.type == EVT_NONE)
                    req->event.type = EVT_POINTER_MOVE;
                input_dispatch(info);
                break;
            }
        }

        if (event->type == EV_KEY) {
            if (event->code == BTN_TOUCH) {
                if (event->value) {
                    req->pointer_event.pressed = TRUE;
                    req->event.type = EVT_POINTER_DOWN;
                } else {
                    req->pointer_event.pressed = FALSE;
                    req->event.type = EVT_POINTER_UP;
                }
            }
        }

        if (event->type == EV_ABS) {
            if (event->code == ABS_MT_POSITION_X)
                req->pointer_event.x = event->value;
            if (event->code == ABS_MT_POSITION_Y)
                req->pointer_event.y = event->value;
        }
    }
}

static ret_t input_dispatch_one_event(run_info_t *info)
{
    event_info event_buf[info->n_fd];
    int ret;
    int fd_index;

    int nready = poll(info->poll_fd, info->n_fd, -1);
    if (nready <= 0 || nready > info->n_fd) {
        log_info("poll error.\n");
        return RET_OK;
    }

    bzero(event_buf, sizeof(event_buf));
    ret = poll_read(info->poll_fd, event_buf, info->n_fd);

    if (ret != nready) {
        log_info("event omit.\n");
    }

    for (int i = 0; i < nready; i++) {
        fd_index = find_index(event_buf[i].fd, info, info->n_fd);
        if (fd_index < 0) {
            log_info("find fd index fail.\n");
            continue;
        }
        event_buf_handle(event_buf[i], info, fd_index);
    }

    return RET_OK;
}

static void *input_run(void *ctx)
{
    run_info_t *info = (run_info_t *)ctx;

    while (input_dispatch_one_event(info) == RET_OK)
    {
    };
    close_fd(info);

    return NULL;
}

tk_thread_t *input_thread_run(const char *filename, input_dispatch_t dispatch, void *ctx,
                              int32_t max_x, int32_t max_y)
{
    run_info_t *info = TKMEM_ZALLOC(run_info_t);
    tk_thread_t *thread = NULL;
    return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

    memset(info, 0x00, sizeof(run_info_t));

    info->n_fd = get_dev_fd_name(filename, info);
    info->dispatch_ctx = ctx;
    info->dispatch = dispatch;

    thread = tk_thread_create(input_run, info);
    if (thread != NULL) {
        tk_thread_start(thread);
    }
    else {
        close_fd(info);
    }

    return thread;
}

ret_t input_thread_global_init(void)
{
    custom_keys_init(FALSE);
    return RET_OK;
}

ret_t input_thread_global_deinit(void)
{
    custom_keys_deinit(FALSE);
    return RET_OK;
}
