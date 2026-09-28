#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "mult_adc_keyboard.h"

struct adc_keyboard_dev {
    int is_open;
    struct input_dev *input_dev;
    struct workqueue_struct *aux_workqueue;
    struct work_struct aux_work;
    struct adc_keyboards *keyboards;
    void *pdata;
};

static unsigned int debug = 0;
module_param(debug, int, 0644);

extern int adc_enable(void);
extern int adc_disable(void);
extern int adc_read_channel_voltage(unsigned int channel);

static unsigned int get_keyboard_code(struct adc_keyboard *keyboard, int adc_value)
{
    int max, min;
    int i;

    for (i = 0; i < ADC_KEY_NUM; i ++) {
        if (keyboard->keys[i].code == -1)
            continue;
        max = keyboard->keys[i].value + keyboard->deviation;
        min = keyboard->keys[i].value - keyboard->deviation;
        // printk("max = %d\n", max);
        // printk("min = %d\n", min);
        if ((adc_value > min) && (adc_value < max))
            return keyboard->keys[i].code;
    }
    return 0;
}


static void report_key_value(struct input_dev* input_dev, unsigned int code, int is_down)
{
    input_event(input_dev, EV_KEY, code, is_down);  //key up
    input_sync(input_dev);
}

static void auxwork_handler(struct work_struct *p_work)
{
    unsigned int time = 0;
    unsigned int adc_val = 0;
    unsigned int old_key[ADC_CHANNELS] = {0};
    unsigned int key[ADC_CHANNELS] = {0};
    int i;
    struct adc_keyboard_dev *dev;
    struct adc_keyboards *keyboards;
    struct adc_keyboard *keyboard;

    dev = container_of(p_work, struct adc_keyboard_dev, aux_work);
    keyboards = dev->keyboards;

    adc_enable();

    while (1) {
        if (!dev->is_open)
            break;

        for (i = 0; i < ADC_CHANNELS; i ++) {
            if (!keyboards->adc_used[i])
                continue;

            keyboard = &(keyboards->keyboards[i]);
            time = keyboard->key_detectime;
            adc_val = adc_read_channel_voltage(keyboard->channel);
            if (debug)
                printk(KERN_EMERG "adc%d_val = %d\n", keyboard->channel, adc_val);
            if (adc_val < 0) {
                printk(KERN_ERR "jz_adc_aux read value error !!\n");
                goto to_sleep;
            }

            key[i] = get_keyboard_code(keyboard, adc_val);
            if(key[i] == old_key[i])
                continue;

            if (old_key[i])
                report_key_value(dev->input_dev, old_key[i], 0);
            if (key[i])
                report_key_value(dev->input_dev, key[i], 1);

            old_key[i] = key[i];
        }

to_sleep:
        usleep_range(time*1000,time*1000);
    }
}

static int adc_keyboard_open(struct input_dev *input_dev)
{
    struct adc_keyboard_dev *dev;

    dev = input_get_drvdata(input_dev);
    if (NULL == dev) {
        printk(KERN_ERR "Failed input_get_drvdata\n");
        return -1;
    }
    dev->is_open = true;
    dev->aux_workqueue = create_singlethread_workqueue("mult_aux_workqueue");
    if (!dev->aux_workqueue) {
        printk(KERN_ERR "Failed to create test_workqueue\n");
        return -1;
    }

    INIT_WORK(&dev->aux_work, auxwork_handler);
    queue_work(dev->aux_workqueue, &dev->aux_work);

    return 0;
}

static void adc_keyboard_close(struct input_dev *input_dev)
{
    struct adc_keyboard_dev *dev;

    dev = input_get_drvdata(input_dev);
    dev->is_open = false;
    destroy_workqueue(dev->aux_workqueue);

    adc_disable();
}


static int keyboard_probe(struct platform_device *platform_dev)
{
    int ret;
    int index, i, j;
    struct input_dev *input_dev;
    struct adc_keyboard_dev *dev;
    struct adc_keyboards *keyboards;
    struct adc_keyboard *keyboard;
    struct adc_keys_button *key;

    dev = kzalloc(sizeof(struct adc_keyboard_dev), GFP_KERNEL);
    if (!dev) {
        printk(KERN_ERR "Failed to allocate driver structre\n");
        return -ENOMEM;
    }
    if (!(dev->input_dev = input_allocate_device())) {
        printk(KERN_ERR "jz mult adc keyboard drvier allocate memory failed!\n");
        return ENOMEM;
    }

    input_dev = dev->input_dev;
    input_dev->name = "jz mult adc keyboard";
    // input_dev->phys = "input/event0";
    input_dev->id.bustype = BUS_HOST;
    input_dev->id.vendor  = 0x0005;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0100;

    input_dev->open    = adc_keyboard_open;
    input_dev->close   = adc_keyboard_close;
    input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_SYN);

    dev->keyboards = (struct adc_keyboards *)platform_dev->dev.platform_data;
    keyboards = dev->keyboards;
    for (i = 0; i < ADC_CHANNELS; i ++) {
        if (!keyboards->adc_used[i])
            continue;
        index = 0;
        keyboard = &(keyboards->keyboards[i]);
        for (j = 0; j < ADC_KEY_NUM; j ++) {
            key = &(keyboard->keys[index]);
            if (key->code != -1) {
                set_bit(key->code, input_dev->keybit);
                index++;
            }
        }
        if (index != keyboard->size)
            printk(KERN_ERR "jz mult adc keyboard missing key %d != %d\n", index, keyboard->size);
    }

    input_set_drvdata(dev->input_dev, dev);
    platform_set_drvdata(platform_dev, dev);

    ret = input_register_device(dev->input_dev);
    if (ret) {
        input_free_device(dev->input_dev);
        kfree(dev);
        return ret;
    }


    printk("jz mult adc keyboard driver has been initialized successfully!\n");

    return 0;
}


static int keyboard_remove(struct platform_device *platform_dev)
{
    struct adc_keyboard_dev *dev;
    dev = platform_get_drvdata(platform_dev);
    input_unregister_device(dev->input_dev);
    input_free_device(dev->input_dev);
    kfree(dev);

    return 0;
}


struct platform_driver adc_keys_drv = {
    .probe   = keyboard_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "mult_adc_key",
        },
};

extern int adc_keyboard_dev_init(void);

int __init adc_keyboard_drv_init(void)
{
    adc_keyboard_dev_init();

    platform_driver_register(&adc_keys_drv);

    return 0;
}

extern int adc_keyboard_dev_deinit(void);

void __exit adc_keyboard_drv_exit(void)
{
    adc_keyboard_dev_deinit();

    platform_driver_unregister(&adc_keys_drv);
}

module_init(adc_keyboard_drv_init);
module_exit(adc_keyboard_drv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MULT_ADC_KEYBOARD driver");