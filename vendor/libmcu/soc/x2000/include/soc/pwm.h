#ifndef _SOC_PWM_H_
#define _SOC_PWM_H_

struct pwm_handle_gpio {
    char id;
    unsigned short func;
    short gpio;
};

struct pwm_handle {
    unsigned int is_config:2;
    unsigned int is_request:1;
    unsigned int is_enable:1;
    unsigned int idle_level:1;
    unsigned int init_level:1;
    unsigned int dma_loop:1;
    volatile int dma_wait:1;
    unsigned int func_is_clear:1;
    unsigned int not_really_disable:1;
    unsigned int not_really_enable:1;

    unsigned long rate;
    unsigned long real_rate;
    unsigned long half_num;
    unsigned long full_num;
    unsigned int levels;

    unsigned int high_levels;
    unsigned int low_levels;
    struct pwm_handle_gpio *gpio_def;


    void *dma_data;

}__packed;


#endif