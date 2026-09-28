#ifndef _SOC_PWM_H_
#define _SOC_PWM_H_

struct pwm_handle_gpio {
    char id;
    unsigned short func;
    short gpio;
};

struct pwm_handle {
    unsigned char is_config;
    unsigned char is_request;
    unsigned char is_enable;
    unsigned char func_is_clear;
    unsigned char not_really_disable;
    unsigned char not_really_enable;

    unsigned long rate;
    unsigned long real_rate;
    unsigned long half_num;
    unsigned long full_num;
    unsigned int levels;

    unsigned int idle_level;
    unsigned int init_level;
    unsigned int high_levels;
    unsigned int low_levels;

    struct pwm_handle_gpio *gpio_def;

    unsigned char dma_loop;
    unsigned char dma_complete;
    struct dma *pwm_dma;
    void *dma_data;
    unsigned int dma_data_len;
    unsigned char dma_queue;
};

#endif // _SOC_PWM_H_