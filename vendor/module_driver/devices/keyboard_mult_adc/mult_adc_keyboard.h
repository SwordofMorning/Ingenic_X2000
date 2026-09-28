#ifndef _ADC_KEYBOARD_H_
#define _ADC_KEYBOARD_H_

#define ADC_CHANNELS       4
#define ADC_KEY_NUM            8

struct adc_keys_button {
    unsigned int code;      //按键值
    int value;              //测量电压值(mv)
};

struct adc_keyboard {
    unsigned int channel;           //adc通道
    unsigned int init_value;        //不按按键时的电压值
    unsigned int deviation;         //adc的波动范围，单位:mv
    unsigned int key_detectime;     //按键检测间隔，单位:ms
    int size;                       //按键数量
    struct adc_keys_button *keys;
};

struct adc_keyboards {
    int *adc_used;
    struct adc_keyboard *keyboards;
};

#endif