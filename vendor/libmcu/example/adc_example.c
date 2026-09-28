#include <stdio.h>
#include <string.h>
#include <driver/adc.h>

/*
 * unit: mV
 * x2000  vref voltage [ 1800 ]
 * x1830  vref voltage [ 900 - 1800 ]
 * x1520  vref voltage [ 3300 ]
 */
#define VREF_VOLTAGE        1800

/*
 * 以x2000_darwin_v13板为例 执行cmd_mcu reset命令时
 * 分别按住          UP_KEY DOWN_KEY LEFT_KEY RIGHT_KEY MENU_KEY OK_KEY 按键
 * ADC1的电压值分别为    0     300      600       900      1200    1500  mv左右
 */
void adc_example(void)
{
    int i;
    unsigned int val;

    for (i = 0; i < 5; i++) {
        val = adc_read_data(i);
        printf("ADC%d sample voltage: %dmV\n", i, val * VREF_VOLTAGE / 1024);
    }
}