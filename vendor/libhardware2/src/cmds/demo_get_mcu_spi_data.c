#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <libhardware2/mcu.h>
#include <libhardware2/fb.h>
#include <pthread.h>

#define SPI_MAX_SIZE 512

#define HOST_START "mcu_spi_ready"
#define MCU_START "mcu_spi_start"
#define HOST_STOP "mcu_spi_end"

int exit_flag;
pthread_cond_t read_cond;
pthread_cond_t write_cond;
pthread_mutex_t mutex;

static struct fb_device_info info;

static void *get_data_thread(void *data)
{
    int mcu_fd = mcu_open();
    if (mcu_fd < 0) {
        fprintf(stderr, "failed to open mcu\n");
        return NULL;
    }

    int ret;
    unsigned char *buf = (unsigned char *)data;

    /* 发送ready信号 */
    ret = mcu_write_data_timeout(mcu_fd, HOST_START, strlen(HOST_START)+1, -1);
    if (ret < 0) {
        printf("failed to write ready signal to mcu\n");
        goto close_mcu;
    }

    /* 接收开始信号 */
    ret = mcu_read_data_timeout(mcu_fd, buf, strlen(MCU_START)+1, -1);
    if (ret < 0 || strcmp(MCU_START, buf)) {
        printf("failed to read start signal from mcu\n");
        goto close_mcu;
    }

    pthread_mutex_lock(&mutex);

    while (!exit_flag) {
        memset(buf, 0, SPI_MAX_SIZE);
        /* 读数据 */
        ret = mcu_read_data_timeout(mcu_fd, buf, SPI_MAX_SIZE, -1);
        if (ret < 0)
            break;

        pthread_cond_signal(&write_cond);
        if (exit_flag)
            break;

        pthread_cond_wait(&read_cond, &mutex);
    }

    pthread_mutex_unlock(&mutex);
    /* 发送结束信号 */
    mcu_write_data_timeout(mcu_fd, HOST_STOP, strlen(HOST_STOP)+1, -1);

close_mcu:
    mcu_close(mcu_fd);

    exit_flag = 1;
    pthread_cond_signal(&write_cond);

    return NULL;
}

int main(void)
{
    unsigned char *fb_data = malloc(SPI_MAX_SIZE);
    unsigned char *spi_data = malloc(SPI_MAX_SIZE);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&read_cond, NULL);
    pthread_cond_init(&write_cond, NULL);

    int fb_fd = fb_open("/dev/fb0", &info);
    int ret = fb_enable(fb_fd);
    if (ret)
        goto close_fb;

    void *fb_base = info.mapped_mem;
    int pos = 0, n = 0;
    int index = 0;
    int size = SPI_MAX_SIZE;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, get_data_thread, spi_data);
    pthread_attr_destroy(&attr);

    pthread_mutex_lock(&mutex);

    while (!exit_flag) {
        pthread_cond_wait(&write_cond, &mutex);
        if (exit_flag)
            break;

        /* 拿数据 */
        memcpy(fb_data, spi_data, size);
        pthread_cond_signal(&read_cond);

        /* 将数据按顺序存放到显示帧缓冲内 */
        if (pos + size <= info.frame_size) {
            memcpy(fb_base + pos, fb_data, size);
            pos += size;
            continue;
        }

        n = info.frame_size - pos;
        memcpy(fb_base + pos, fb_data, n);
        /* 拿到一帧数据后显示 */
        ret = fb_pan_display(fb_fd, &info, index);
        if (ret)
            goto display_err;

        if (info.frame_nums >= 2)
            index = !index;
        fb_base = info.mapped_mem + info.frame_size * index;
        memcpy(fb_base, fb_data + n, size - n);
        pos = size - n;
    }

display_err:
    pthread_mutex_unlock(&mutex);

    exit_flag = 1;
    pthread_cond_signal(&write_cond);
    pthread_cond_signal(&read_cond);
    pthread_join(tid, NULL);

close_fb:
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&write_cond);
    pthread_cond_destroy(&read_cond);

    free(spi_data);
    free(fb_data);
    fb_close(fb_fd, &info);

    return 0;
}