#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <libhardware2/pwm.h>

#define PWM_MAGIC_NUMBER    'P'
#define PWM_REQUEST             _IOW(PWM_MAGIC_NUMBER, 11, char)
#define PWM_RELEASE             _IOW(PWM_MAGIC_NUMBER, 22, unsigned int)
#define PWM_CONFIG              _IOW(PWM_MAGIC_NUMBER, 33, struct pwm_config_data)
#define PWM_SET_LEVEL           _IOWR(PWM_MAGIC_NUMBER, 44, unsigned int)
#define PWM_DMA_INIT            _IOW(PWM_MAGIC_NUMBER, 55, struct pwm_dma_config)
#define PWM_DMA_UPDATE          _IOWR(PWM_MAGIC_NUMBER, 66, struct pwm_dma_data)
#define PWM_DMA_DISABLE_LOOP    _IOW(PWM_MAGIC_NUMBER, 77, char)
#define PWM_NOT_REALLY_ENABLE   _IOW(PWM_MAGIC_NUMBER, 88, unsigned int)
#define PWM_NOT_REALLY_DISABLE  _IOW(PWM_MAGIC_NUMBER, 89, unsigned int)
#define PWM_ENABLE_CHANNELS     _IOW(PWM_MAGIC_NUMBER, 98, unsigned int)
#define PWM_DISABLE_CHANNELS    _IOW(PWM_MAGIC_NUMBER, 99, unsigned int)

struct pwm_handle {
    int fd;
    int id;
};

struct pwm_dma_data {
    struct pwm_data *data;
    unsigned int data_count;
    unsigned int dma_loop;
    int id;
};

struct pwm_dma_config {
    int id;
    enum pwm_idle_level idle_level;
    enum pwm_dma_start_level start_level;
};

int pwm_enable_channels(unsigned int channels)
{
    int fd, ret;
    fd = open("/dev/jz_pwm", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "pwm:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, PWM_ENABLE_CHANNELS, &channels);
    if (ret < 0) {
        fprintf(stderr, "pwm:set multi channel mode failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int pwm_disable_channels(unsigned int channels)
{
    int fd, ret;
    fd = open("/dev/jz_pwm", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "pwm:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, PWM_DISABLE_CHANNELS, &channels);
    if (ret < 0) {
        fprintf(stderr, "pwm:set multi channel mode failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int pwm_not_really_enable(long handle)
{
    int ret;
    unsigned int id;
    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);
    id = pwm->id;

    ret = ioctl(pwm->fd, PWM_NOT_REALLY_ENABLE, &id);
    if (ret < 0) {
        fprintf(stderr, "pwm:not really enable set failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int pwm_not_really_disable(long handle)
{
    int ret,id;
    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);
    id = pwm->id;

    ret = ioctl(pwm->fd, PWM_NOT_REALLY_DISABLE, &id);
    if (ret < 0) {
        fprintf(stderr, "pwm:not really enable set failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

long pwm_request(const char *gpio_name)
{
    assert(gpio_name);

    if (strlen(gpio_name) > 11) {
        fprintf(stderr, "pwm:pwm_request failed: invalid gpio:%s !\n", gpio_name);
        return -1;
    }

    struct pwm_handle *pwm = (struct pwm_handle *)malloc(sizeof(struct pwm_handle));
    if (!pwm) {
        fprintf(stderr, "pwm:fail to malloc pwm: %s\n", strerror(errno));
        return -1;
    }

    pwm->fd = open("/dev/jz_pwm", O_RDWR);
    if (pwm->fd < 0) {
        fprintf(stderr, "pwm:open dev failed: %s\n", strerror(errno));
        goto free_pwm;
    }

    pwm->id = ioctl(pwm->fd, PWM_REQUEST, gpio_name);
    if (pwm->id < 0) {
        fprintf(stderr, "pwm:request failed: %s\n", strerror(errno));
        goto close_pwm;
    }

    return (long)pwm;

close_pwm:
    close(pwm->fd);
free_pwm:
    free(pwm);
    return -1;
}

void pwm_release(long handle)
{
    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);

    int ret = ioctl(pwm->fd, PWM_RELEASE, pwm->id);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d release failed %s!\n", pwm->id, strerror(errno));

    close(pwm->fd);

    free(pwm);
}

int pwm_config(long handle, struct pwm_config_data *cfg)
{
    int ret = 0;

    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);
    assert(cfg);

    cfg->id = pwm->id;

    ret = ioctl(pwm->fd, PWM_CONFIG, cfg);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d config failed %s!\n", pwm->id, strerror(errno));

    return ret;
}

int pwm_set_level(long handle, unsigned int level)
{
    int ret = 0;

    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);

    unsigned int tmp[2] = {pwm->id, level};

    ret = ioctl(pwm->fd, PWM_SET_LEVEL, tmp);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d set level failed %s!\n", pwm->id, strerror(errno));

    return ret;
}

int pwm_dma_init(long handle, enum pwm_idle_level idle_level, enum pwm_dma_start_level start_level)
{
    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    struct pwm_dma_config dma_config;
    assert(pwm);

    dma_config.id = pwm->id;
    dma_config.idle_level = idle_level;
    dma_config.start_level = start_level;

    int ret = ioctl(pwm->fd, PWM_DMA_INIT, &dma_config);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d dma init failed %s!\n", pwm->id, strerror(errno));

    return ret;
}

int pwm_dma_update(long handle, struct pwm_data *data, unsigned int data_count)
{
    int ret = 0;
    struct pwm_dma_data dma_data;

    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);

    if (data == NULL || data_count == 0) {
        fprintf(stderr, "pwm:ch %d no have dma data!\n", pwm->id);
        return -1;
    }

    dma_data.id = pwm->id;
    dma_data.data = data;
    dma_data.data_count = data_count;
    dma_data.dma_loop = 0;

    ret = ioctl(pwm->fd, PWM_DMA_UPDATE, &dma_data);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d dma update failed %s!\n", pwm->id, strerror(errno));

    return ret;
}

int pwm_dma_send(long handle, unsigned short high, unsigned short low, unsigned int data_count)
{
    int i;
    int ret = 0;
    struct pwm_data *data;

    if (high == 0 || low == 0 || data_count == 0) {
        fprintf(stderr, "pwm: dma send data_count high and low cannot be zero!\n");
        return -1;
    }

    data = malloc(data_count * sizeof(struct pwm_data));
    if (data == NULL) {
        fprintf(stderr, "pwm:malloc dma data error!\n");
        return -1;
    }

    for (i = 0; i < data_count; i++) {
        data[i].high = high;
        data[i].low = low;
    }

    ret = pwm_dma_update(handle, data, data_count);

    free(data);

    return ret;
}

int pwm_dma_enable_loop(long handle, struct pwm_data *data, unsigned int data_count)
{
    int ret = 0;
    struct pwm_dma_data dma_data;

    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);

    if (data == NULL || data_count == 0) {
        fprintf(stderr, "pwm:ch %d no have dma data!\n", pwm->id);
        return -1;
    }

    if (data_count % 4) {
        fprintf(stderr, "pwm:ch %d dma loop data length must 4 word align!\n", pwm->id);
        return -1;
    }

    dma_data.id = pwm->id;
    dma_data.data = data;
    dma_data.data_count = data_count;
    dma_data.dma_loop = 1;

    ret = ioctl(pwm->fd, PWM_DMA_UPDATE, &dma_data);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d dma enable loop failed %s!\n", pwm->id, strerror(errno));

    return ret;
}

int pwm_dma_disable_loop(long handle)
{
    struct pwm_handle *pwm = (struct pwm_handle *)handle;
    assert(pwm);

    int ret = ioctl(pwm->fd, PWM_DMA_DISABLE_LOOP, pwm->id);
    if (ret < 0)
        fprintf(stderr, "pwm:ch %d dma disable loop failed %s!\n", pwm->id, strerror(errno));

    return ret;
}