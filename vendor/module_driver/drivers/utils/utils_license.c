#include <linux/module.h>

#ifdef MD_UTILS_LOG
extern int utils_log_init(void);
extern void utils_log_deinit(void);
#endif

#ifdef MD_UTILS_GPIO
extern int utils_gpio_init(void);
extern void utils_gpio_deinit(void);
#endif

int utils_init(void)
{
#ifdef MD_UTILS_LOG
    utils_log_init();
#endif

#ifdef MD_UTILS_GPIO
    utils_gpio_init();
#endif

    return 0;
}
module_init(utils_init);

void utils_deinit(void)
{
#ifdef MD_UTILS_LOG
    utils_log_deinit();
#endif

#ifdef MD_UTILS_GPIO
    utils_gpio_deinit();
#endif
}
module_exit(utils_deinit);
MODULE_LICENSE("GPL");