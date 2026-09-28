#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <utils/string.h>
#include <utils/gpio.h>
#include <soc/gpio.h>

typedef int (*gpio_cb)(int port, enum gpio_function func, unsigned long pin);
struct gpio_set {
    gpio_cb single;
    gpio_cb multi;
};
static struct gpio_set *set_func;

static inline const char *is_gpio_port(const char *str)
{
    if (!strimatch(str, "gpio_p"))
        str += 6;
    else if (!strimatch(str, "p"))
        str += 1;
    else
        return NULL;

    if (tolower(str[0]) < 'a' || tolower(str[0]) > 'g')
        return NULL;

    return str;
}

int str_match_gpio(const char **str_p)
{
    int pin, port, is_q = 0;
    const char *str = *str_p;

    if (!strncmp(str, "-1", 2)) {
        *str_p = str + 2;
        return -1;
    }

    str = is_gpio_port(str);
    if (str == NULL)
        return -EINVAL;

    port = tolower(str[0]) - 'a';
    if (port < 0 || port > 20)
        return -EINVAL;

    str++;
    if (str[0] == '(') {
        is_q = 1;
        str++;
    }

    if (!isdigit(str[0]))
        return -EINVAL;

    pin = str[0] - '0';

    if (!isdigit(str[1])) {
        str += 1;
    } else {
        pin = pin * 10 + (str[1] - '0');

        if (pin >= 32)
            return -EINVAL;

        str += 2;
    }

    if (is_q) {
        if (str[0] != ')')
            return -EINVAL;
        str++;
    }

    if (str_p)
        *str_p = str;

    return port * 32 + pin;
}
EXPORT_SYMBOL(str_match_gpio);

int str_to_gpio(const char *str)
{
    int gpio = str_match_gpio(&str);

    return (gpio < -1 || *str != '\0') ? -EINVAL : gpio;
}
EXPORT_SYMBOL(str_to_gpio);

char *gpio_to_str(int gpio, char *buf)
{
    if (!buf)
        buf = kmalloc(5, GFP_KERNEL);

    if (gpio == -1) {
        sprintf(buf, "-1");
        return buf;
    }

    if (gpio < -1 || gpio >= (('g' - 'a' + 1) * 32)) {
        sprintf(buf, "error");
        return buf;
    }

    sprintf(buf, "P%c%02d", 'A' + (gpio / 32), gpio % 32);

    return buf;
}
EXPORT_SYMBOL(gpio_to_str);

static int param_gpio_set(const char *arg, const struct kernel_param *kp)
{
    int ret;
    int *gpio = (int *) kp->arg;

    ret = str_to_gpio(arg);
    if (ret < -1)
        return ret;

    *gpio = ret;

    return 0;
}

static int param_gpio_get(char *buffer, const struct kernel_param *kp)
{
    int *gpio = (int *) kp->arg;

    gpio_to_str(*gpio, buffer);

    return strlen(buffer);
}

struct kernel_param_ops param_gpio_ops = {
    .set = param_gpio_set,
    .get = param_gpio_get,
};
EXPORT_SYMBOL(param_gpio_ops);

int gpio_port_set_func(int port, unsigned long pins, enum gpio_function func)
{
    int ret;

    if (set_func && set_func->multi) {
        ret = set_func->multi(port, func, pins);
        if (ret)
            return ret;
    } else {
        int pin = 0;

        while (pins >> pin) {
            if(pins & BIT(pin)) {
                ret = jzgpio_set_func(port, func, BIT(pin));
                if(ret)
                    return ret;
            }

            pin++;
        }
    }

    return 0;
}
EXPORT_SYMBOL(gpio_port_set_func);

int gpio_set_func(int gpio, enum gpio_function func)
{
    int port = gpio / 32;
    if (set_func && set_func->single) {
        int pin = gpio % 32;

        return set_func->single(port, func, pin);
    } else {
        int pin = BIT(gpio % 32);

        return jzgpio_set_func(port, func, pin);
    }

    return 0;
}
EXPORT_SYMBOL(gpio_set_func);

static int gpio_plat_probe(struct platform_device *pdev)
{
    set_func = pdev->dev.platform_data;
    return 0;
}

static int gpio_plat_remove(struct platform_device *pdev)
{
    set_func = NULL;
    return 0;
}

static struct platform_driver gpio_drv = {
    .driver = {
        .name = "ingenic-utils-gpiodev",
        .owner = THIS_MODULE,
    },
    .probe = gpio_plat_probe,
    .remove = gpio_plat_remove,
};

int utils_gpio_init(void)
{
    return platform_driver_register(&gpio_drv);
}

void utils_gpio_deinit(void)
{
    platform_driver_unregister(&gpio_drv);
}