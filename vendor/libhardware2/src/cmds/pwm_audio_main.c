#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <libhardware2/pwm.h>

#define PWM_DUTY_MAX_COUNT   (0xFFFF)

struct pwm_audio_dev {
    long pwm;

    int exit_flag;

    unsigned int pwm_full_num;
    unsigned int buf_size;
    unsigned int buf_offset;

    unsigned int write_pos;
    unsigned int read_pos;

    pthread_t tid;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
    pthread_mutex_t read_mutex;
    pthread_mutex_t write_mutex;

    unsigned int base_freq;
    unsigned int unit_time;
    unsigned int buf_count;
    struct pwm_data *data;
};

static void *pwm_audio_thread(void *data)
{
    struct pwm_audio_dev *dev = data;

    pthread_mutex_lock(&dev->read_mutex);

    while (!dev->exit_flag) {
        while ((dev->write_pos - dev->read_pos) == 0) {
            pthread_cond_wait(&dev->read_cond, &dev->read_mutex);

            if (dev->exit_flag)
                goto pwm_audio_thread_exit;
        }

        pwm_dma_update(dev->pwm, &dev->data[(dev->read_pos % dev->buf_count) * dev->buf_size], dev->buf_size);
        dev->read_pos++;
        pthread_cond_signal(&dev->write_cond);
    }

pwm_audio_thread_exit:

    pthread_mutex_unlock(&dev->read_mutex);

    return NULL;
}

void pwm_audio_data_flush(struct pwm_audio_dev *dev)
{
    int i;
    unsigned short low;
    unsigned short high;
    struct pwm_data *data;

    if (!dev) {
        printf("%s: dev == NULL\n", __func__);
        return;
    }

    pthread_mutex_lock(&dev->write_mutex);

    if (dev->buf_offset) {
        data = &dev->data[(dev->write_pos % dev->buf_count) * dev->buf_size];
        high = dev->pwm_full_num / 2;
        low = dev->pwm_full_num - high;
        for(i = dev->buf_offset; i < dev->buf_size; i++) {
            data[i].high = high;
            data[i].low = low;
        }

        dev->buf_offset = 0;
        dev->write_pos++;
        pthread_cond_signal(&dev->read_cond);
    }

    pthread_mutex_unlock(&dev->write_mutex);
}

int pwm_audio_mono_s16le_write(struct pwm_audio_dev *dev, unsigned int sampling_rate, const short *data, unsigned int data_count)
{
    int i, j;
    short diff;
    unsigned int data_mul;
    struct pwm_data *pwm_data;

    if (!dev) {
        printf("%s: dev == NULL\n", __func__);
        return -1;
    }

    if (dev->base_freq % sampling_rate) {
        printf("%s: pwm audio sampling rate %d not support\n", __func__, sampling_rate);
        return -1;
    }

    data_mul = dev->base_freq / sampling_rate;
    if (dev->buf_size % data_mul) {
        printf("%s: pwm audio unit time %dms not support\n", __func__, dev->unit_time);
        return -1;
    }

    pthread_mutex_lock(&dev->write_mutex);

    while (data_count)
    {
        if (dev->buf_offset == 0) {
            while ((dev->buf_count - (dev->write_pos - dev->read_pos)) == 0) {
                pthread_cond_wait(&dev->write_cond, &dev->write_mutex);
                if (dev->exit_flag)
                    goto pwm_audio_write_exit;
            }
        }

        pwm_data = &dev->data[(dev->write_pos % dev->buf_count) * dev->buf_size];

        for (i = dev->buf_offset; i < dev->buf_size; i += data_mul) {
            if (!data_count)
                break;

            diff = data[0] * dev->pwm_full_num / PWM_DUTY_MAX_COUNT;
            pwm_data[i].high = dev->pwm_full_num / 2 + diff;
            if (pwm_data[i].high >= dev->pwm_full_num)
                pwm_data[i].high = dev->pwm_full_num - 1;
            if (pwm_data[i].high == 0)
                pwm_data[i].high = 1;
            pwm_data[i].low = dev->pwm_full_num - pwm_data[i].high;

            for (j = 1; j < data_mul; j++) {
                pwm_data[i + j] = pwm_data[i];
            }

            data++;
            data_count--;
        }

        if (i < dev->buf_size) {
            dev->buf_offset = i;
        } else {
            dev->buf_offset = 0;
            dev->write_pos++;
            pthread_cond_signal(&dev->read_cond);
        }
    }

pwm_audio_write_exit:
    pthread_mutex_unlock(&dev->write_mutex);
    return 0;
}

struct pwm_audio_dev *pwm_audio_init(const char *gpio_name, unsigned int base_freq, unsigned int unit_time, unsigned int buf_count)
{
    int err;
    int rate;
    struct pwm_audio_dev *dev;
    enum pwm_idle_level idle_level = PWM_idle_low;
    enum pwm_dma_start_level start_level = PWM_start_high;

    dev = malloc(sizeof(struct pwm_audio_dev));
    if (!dev) {
        printf("pwm_audio_dev malloc fail\n");
        return NULL;
    }

    dev->exit_flag = 0;
    dev->read_pos = 0;
    dev->write_pos = 0;

    dev->base_freq = base_freq;
    dev->unit_time = unit_time;
    dev->buf_count = buf_count;

    dev->buf_size = base_freq * unit_time / 1000;
    dev->pwm = pwm_request(gpio_name);
    if (dev->pwm < 0) {
        printf("pwm_request %s fail\n", gpio_name);
        goto err_free_dev;
    }

    rate = pwm_dma_init(dev->pwm, idle_level, start_level);
    if(rate <= 0) {
        printf("pwm_dma_init %s fail\n", gpio_name);
        goto err_pwm_release;
    }

    if (rate % base_freq) {
        printf("pwm not support base freq %d, rate %d\n", base_freq, rate);
        goto err_pwm_release;
    }

    dev->pwm_full_num = rate / base_freq;

    dev->data = malloc(dev->buf_count * dev->buf_size * sizeof(struct pwm_data));
    if (!dev->data) {
        printf("malloc pwm dma data fail\n");
        goto err_pwm_release;
    }

    pthread_mutex_init(&dev->read_mutex, NULL);
    pthread_mutex_init(&dev->write_mutex, NULL);
    pthread_cond_init(&dev->read_cond, NULL);
    pthread_cond_init(&dev->write_cond, NULL);
    err = pthread_create(&dev->tid, NULL, pwm_audio_thread, dev);
    if (err) {
        printf("create pwm_audio_thread fail\n");
        goto err_free_dma_data;
    }

    return dev;

err_free_dma_data:
    free(dev->data);
err_pwm_release:
    pwm_release(dev->pwm);
err_free_dev:
    free(dev);
    return NULL;
}

void pwm_audio_exit(struct pwm_audio_dev *dev)
{
    if (!dev) {
        printf("%s: dev == NULL\n", __func__);
        return;
    }

    dev->exit_flag = 1;
    pthread_cond_signal(&dev->read_cond);
    pthread_cond_signal(&dev->write_cond);

    pthread_join(dev->tid,NULL);
    pthread_mutex_destroy(&dev->read_mutex);
    pthread_mutex_destroy(&dev->write_mutex);
    pthread_cond_destroy(&dev->read_cond);
    pthread_cond_destroy(&dev->write_cond);

    pwm_release(dev->pwm);
    free(dev->data);
    free(dev);
}

/* ---------------------------------------------------------------*/

#define AUDIO_DATA_BUF_SIZE  4096

#define PWM_AUDIO_BASE_FREQ     48000
#define PWM_AUDIO_UNIT_TIME     50
#define PWM_AUDIO_BUF_COUNT     2

static int usage(char *app_name)
{
    printf("Usage1:%s config <gpio> <audio_sample_rate> <audio_file>\n", app_name);
    printf("Example1:\n");
    printf("\t%s pc25 48000 /usr/data/mono_s16le_48000.pcm\n", app_name);

    exit(-1);
}

int main(int argc, char **argv)
{
    int fd;
    int ret;
    int len;
    int rate;
    unsigned char* audio_data;
    struct pwm_audio_dev *dev;

    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }

    audio_data = malloc(AUDIO_DATA_BUF_SIZE);
    if (!audio_data) {
        printf("audio data buf malloc fail\n");
        return -1;
    }

    fd = open(argv[3], O_RDONLY);
    if (fd < 0) {
        printf("open audio file %s fail\n", argv[3]);
        free(audio_data);
        return -1;
    }

    rate = atoi(argv[2]);
    dev = pwm_audio_init(argv[1], PWM_AUDIO_BASE_FREQ, PWM_AUDIO_UNIT_TIME, PWM_AUDIO_BUF_COUNT);
    if (!dev) {
        printf("pwm_audio_init fail\n");
        close(fd);
        free(audio_data);
        return -1;
    }

    while (1)
    {
        len = read(fd, audio_data, AUDIO_DATA_BUF_SIZE);
        if (len <= 0)
            break;

        ret = pwm_audio_mono_s16le_write(dev, rate, (const short *)audio_data, len / 2);
        if (ret) {
            printf("pwm_audio_mono_s16le_write fail\n");
            break;
        }

    }

    close(fd);
    free(audio_data);
    pwm_audio_data_flush(dev);
    pwm_audio_exit(dev);
    return 0;
}