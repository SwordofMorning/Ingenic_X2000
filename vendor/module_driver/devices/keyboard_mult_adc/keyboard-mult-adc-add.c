#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/module.h>
#include "mult_adc_keyboard.h"

static int adc_used[ADC_CHANNELS];

static struct adc_keys_button key_info[ADC_CHANNELS][ADC_KEY_NUM];

static struct adc_keyboard keyboards[ADC_CHANNELS] = {
    {.keys = key_info[0]},
    {.keys = key_info[1]},
    {.keys = key_info[2]},
    {.keys = key_info[3]},
};

static struct adc_keyboards keyboard_info = {
    .adc_used = adc_used,
    .keyboards = keyboards,
};

static void keyboard_dev_release(struct device *dev){}

static struct platform_device keyboard_dev = {
    .name          = "mult_adc_key",
    .dev           = {
        .platform_data = &keyboard_info,
        .release = &keyboard_dev_release,
    },
};

#define MODULE_PARAMS(index) \
    module_param_named(adc##index, adc_used[index], int, 0644); \
    module_param_named(adc##index##_key1_code, key_info[index][0].code, int, 0644);  \
    module_param_named(adc##index##_key1_value, key_info[index][0].value, int, 0644);  \
    module_param_named(adc##index##_key2_code, key_info[index][1].code, int, 0644);  \
    module_param_named(adc##index##_key2_value, key_info[index][1].value, int, 0644);  \
    module_param_named(adc##index##_key3_code, key_info[index][2].code, int, 0644);  \
    module_param_named(adc##index##_key3_value, key_info[index][2].value, int, 0644);  \
    module_param_named(adc##index##_key4_code, key_info[index][3].code, int, 0644);  \
    module_param_named(adc##index##_key4_value, key_info[index][3].value, int, 0644);  \
    module_param_named(adc##index##_key5_code, key_info[index][4].code, int, 0644);  \
    module_param_named(adc##index##_key5_value, key_info[index][4].value, int, 0644);  \
    module_param_named(adc##index##_key6_code, key_info[index][5].code, int, 0644);  \
    module_param_named(adc##index##_key6_value, key_info[index][5].value, int, 0644);  \
    module_param_named(adc##index##_key7_code, key_info[index][6].code, int, 0644);  \
    module_param_named(adc##index##_key7_value, key_info[index][6].value, int, 0644);  \
    module_param_named(adc##index##_key8_code, key_info[index][7].code, int, 0644);  \
    module_param_named(adc##index##_key8_value, key_info[index][7].value, int, 0644);  \
    module_param_named(adc##index##_channel, keyboards[index].channel, int, 0644);  \
    module_param_named(adc##index##_init_value, keyboards[index].init_value, int, 0644);  \
    module_param_named(adc##index##_deviation, keyboards[index].deviation, int, 0644);  \
    module_param_named(adc##index##_key_detectime, keyboards[index].key_detectime, int, 0644);

MODULE_PARAMS(0);
MODULE_PARAMS(1);
MODULE_PARAMS(2);
MODULE_PARAMS(3);

int adc_keyboard_dev_init(void)
{
    int ret, i, j;

    for (i = 0; i < ADC_CHANNELS; i ++) {
        if (!adc_used[i])
            continue;
        for (j = 0; j < ADC_KEY_NUM; j ++) {
            if (key_info[i][j].code != -1)
                keyboards[i].size ++;
        }
    }
    ret = platform_device_register(&keyboard_dev);
    if (ret)
        pr_err("jz mult adc keyboard register fail");

    return 0;
}

int adc_keyboard_dev_deinit(void)
{
    platform_device_unregister(&keyboard_dev);

    return 0;
}
