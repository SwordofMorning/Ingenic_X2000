#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <soc/gpio.h>
#include <utils/gpio.h>
#include <linux/kthread.h>

#define KEY_PRESS 0
#define DEBOUNCE_TIME_MS 10

/* 配置矩阵按键使用的GPIO口 */
static int matrix_keyboard_gpio[3] = {GPIO_PC(05), GPIO_PC(04), GPIO_PC(03)};

/* 配置矩阵键盘键值,默认是一个 m * m 的二维数组 */
static int matrix_keyboard_keycode[3][3] = {
    {KEY_1, KEY_2, KEY_3},
    {KEY_4, KEY_5, KEY_6},
    {KEY_7, KEY_8, KEY_9}
};

struct matrix_keyboard {
    int *gpio;          /*存储矩阵按键使用的GPIO口*/
    int gpio_cnt;       /*存储使用的io的个数*/
    int *key_code;      /*存储键值*/

    struct task_struct *thread;
    struct input_dev *input_dev;

    int key_data;   /*存储被按下的键值*/
};

static void report_key_press(struct matrix_keyboard *matrix)
{
    input_event(matrix->input_dev, EV_KEY, matrix->key_data, 1);
    input_sync(matrix->input_dev);
}

static void report_key_release(struct matrix_keyboard *matrix)
{
    input_event(matrix->input_dev, EV_KEY, matrix->key_data, 0);
    input_sync(matrix->input_dev);
}

/* init all gpio to input */
static int matrix_keyboard_gpio_request(struct matrix_keyboard *dev)
{
    int i, ret;
    char buf[10];
    char gpio_name[128];

    for (i = 0; i < dev->gpio_cnt; i++) {
        sprintf(gpio_name, "matrix_keyboad_gpio%d", i);
        ret = gpio_request(dev->gpio[i], gpio_name);
        if (ret) {
            printk(KERN_ERR "matrix_keyboard: failed to request %s gpio: %s", gpio_name, gpio_to_str(dev->gpio[i], buf));
            return -1;
        }

        gpio_direction_input(dev->gpio[i]);
    }

    return 0;
}

static void matrix_keyboard_gpio_release(struct matrix_keyboard *dev)
{
    int i = 0;

    for (i = 0; i < dev->gpio_cnt; i++)
        gpio_free(dev->gpio[i]);
}

/* gpio1: output; gpio2: input */
static int is_highest_priority_key(const int gpio1, const int gpio2)
{
    int value = -1;

    gpio_direction_output(gpio1, 1);
    msleep(DEBOUNCE_TIME_MS);

    value = gpio_get_value(gpio2);
    if (value == KEY_PRESS) {
        return 1;
    }

    gpio_direction_output(gpio1, 0);
    msleep(DEBOUNCE_TIME_MS);
    return 0;
}

static int scan_key_press(struct matrix_keyboard *dev, int *in_gpio, int *out_gpio)
{
    int i, j;
    int value = -1;

    /* start scan */
    for (i = 0; i < dev->gpio_cnt; i++) {
        gpio_direction_output(dev->gpio[i], 0);
        msleep(DEBOUNCE_TIME_MS);

        for (j = 0; j < dev->gpio_cnt; j++) {
            if (i == j)
                continue;
            value = gpio_get_value(dev->gpio[j]);
            if (value == KEY_PRESS) {
                if (is_highest_priority_key(dev->gpio[i], dev->gpio[j])) {
                    dev->key_data = dev->key_code[j * dev->gpio_cnt + j];
                } else {
                    dev->key_data = dev->key_code[i * dev->gpio_cnt + j];
                }
                *in_gpio = dev->gpio[j];
                *out_gpio = dev->gpio[i];
                return 1;
            }
        }

        gpio_direction_input(dev->gpio[i]);
        msleep(DEBOUNCE_TIME_MS);
    }

    return 0;
}

static int matrix_keyboard_thread(void *data)
{
    int ret;
    struct matrix_keyboard *dev = (struct matrix_keyboard *)data;
    int in_gpio = -1, out_gpio = -1;

    while (!kthread_should_stop()) {
        /* key press */
        ret = scan_key_press(dev, &in_gpio, &out_gpio);

        if (ret) {
            report_key_press(dev);

            while(!kthread_should_stop()) {
                /* key release */
                if (gpio_get_value(in_gpio) != KEY_PRESS)
                    break;

                msleep(DEBOUNCE_TIME_MS);
            }
            report_key_release(dev);
        }

        gpio_direction_input(out_gpio);
        msleep(DEBOUNCE_TIME_MS);
    }

    return 0;
}

static int matrix_keyboard_open(struct input_dev *input_dev)
{
    struct matrix_keyboard *dev;

    dev = input_get_drvdata(input_dev);
    if (NULL == dev) {
        printk(KERN_ERR "matrix_keyboard: Failed input_get_drvdata\n");
        return -1;
    }

    dev->thread = kthread_create(matrix_keyboard_thread, dev, "matrix_keyboard");
    if (IS_ERR_OR_NULL(dev->thread)) {
        printk(KERN_ERR "matrix_keyboard: Failed to create matrix keyboard thread\n");
        return -1;
    }

    wake_up_process(dev->thread);
    return 0;
}

static void matrix_keyboard_close(struct input_dev *input_dev)
{
    int i;
    struct matrix_keyboard *dev;

    dev = input_get_drvdata(input_dev);
    if (NULL == dev) {
        printk(KERN_ERR "matrix_keyboard: Failed input_get_drvdata\n");
        return;
    }

    kthread_stop(dev->thread);
    dev->thread = NULL;

    for (i = 0; i < dev->gpio_cnt; i++)
        gpio_direction_input(dev->gpio[i]);
}

static int keyboard_probe(struct platform_device *platform_dev)
{
    int i;
    int ret;
    struct input_dev *input_dev;
    struct matrix_keyboard *dev;

    dev = kzalloc(sizeof(struct matrix_keyboard), GFP_KERNEL);
    if (!dev) {
        printk(KERN_ERR "matrix_keyboard: Failed to allocate driver structre\n");
        return -ENOMEM;
    }

    if (!(dev->input_dev = input_allocate_device())) {
        printk(KERN_ERR "matrix_keyboard: jz matrix keyboard drvier allocate memory failed!\n");
        kfree(dev);
        return -ENOMEM;
    }

    input_dev = dev->input_dev;
    input_dev->name = "jz matrix keyboard";
    input_dev->id.bustype = BUS_HOST;
    input_dev->id.vendor  = 0x0005;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0100;

    input_dev->open = matrix_keyboard_open;
    input_dev->close = matrix_keyboard_close;
    input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_SYN);

    dev->gpio = matrix_keyboard_gpio;
    dev->gpio_cnt = sizeof(matrix_keyboard_gpio) / sizeof(matrix_keyboard_gpio[0]);
    dev->key_code = matrix_keyboard_keycode[0];

    for (i = 0; i < dev->gpio_cnt * dev->gpio_cnt; i++) {
        if (dev->key_code[i] != -1)
            set_bit(dev->key_code[i], input_dev->keybit);
    }

    ret = matrix_keyboard_gpio_request(dev);
    if (ret) {
        input_free_device(dev->input_dev);
        kfree(dev);
        return ret;
    }

    input_set_drvdata(dev->input_dev, dev);
    platform_set_drvdata(platform_dev, dev);

    ret = input_register_device(dev->input_dev);
    if (ret) {
        matrix_keyboard_gpio_release(dev);
        input_free_device(dev->input_dev);
        kfree(dev);
        return ret;
    }

    printk("matrix_keyboard: jz matrix keyboard driver has been initialized successfully!\n");

    return 0;
}

static int keyboard_remove(struct platform_device *platform_dev)
{
    struct matrix_keyboard *dev;
    dev = platform_get_drvdata(platform_dev);

    input_unregister_device(dev->input_dev);
    input_free_device(dev->input_dev);

    matrix_keyboard_gpio_release(dev);
    kfree(dev);

    return 0;
}

static void jz_matrix_keyboard_dev_release(struct device *dev){}

static struct platform_driver matrix_keys_device_driver = {
    .probe        = keyboard_probe,
    .remove        = keyboard_remove,
    .driver        = {
        .owner    = THIS_MODULE,
        .name    = "matrix_keys",
    },
};

struct platform_device ingenic_matrix_keyboard_device = {
    .name = "matrix_keys",
    .dev  = {
        .release = jz_matrix_keyboard_dev_release,
    },
};

static int __init matrix_keys_init(void)
{
    int ret = platform_device_register(&ingenic_matrix_keyboard_device);
    if (ret)
        return ret;

    return platform_driver_register(&matrix_keys_device_driver);
}

static void __exit matrix_keys_exit(void)
{
    platform_device_unregister(&ingenic_matrix_keyboard_device);

    platform_driver_unregister(&matrix_keys_device_driver);
}

module_init(matrix_keys_init);
module_exit(matrix_keys_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MATRIX_KEYBOARD driver");