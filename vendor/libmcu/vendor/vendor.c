#include <stdio.h>
#include <string.h>

#include <soc/base.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <cpu/host_cpu.h>

#include <driver/gpio.h>

static volatile int is_data_comming = 0;

#ifdef APP_libmcu_driver_irq
static void host_irq_cb(void)
{
    is_data_comming = 1;
}
#endif
void vendor_init(void)
{
    printf("%s is called\n", __func__);

#ifdef APP_libmcu_driver_irq
    host_cpu_set_irq_callback(host_irq_cb);
#endif
    // int gpio = GPIO_PA(0);

    // gpio_direction_output(gpio, 1);

    // int count = 0;

    while (1) {
        while (!is_data_comming);
        is_data_comming = 0;

        int len = 0;
        char buf[100];
        int ret;
        int send_len;

        while (1) {
            ret = host_cpu_read(buf+len, sizeof(buf)-len);
            if (!ret)
                break;

            len += ret;
        }

        printf("len: %d : ", len);
        int i;
        for (i = 0; i < len; i++) {
            printf("%c", buf[i]);
        }

        // gpio_set_value(gpio, count++ % 2);

        printf("\n");

        send_len = len;

        while (len) {
            while(mcu_test_host_busy());

            while (len) {
                int ret = host_cpu_write(buf, len);
                len -= ret;
                if (!ret)
                    break;
            }

            mcu_notify_host(send_len);
        }
    }
}
