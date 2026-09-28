#include <stdlib.h>
#include <string.h>
#include <driver/systick.h>
#include <assert.h>
#include <driver/irq.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <errno.h>

#include <soc/adc.h>

#define DEV_NAME            "adc"

#define CLKDIV              (120 - 1)
#define CLKDIV_US           (2 - 1)
#define CLKDIV_MS           (100 - 1)

#define STABLE_TIME 1
#define REPEAT_TIME 1

#define ADC_SAMPLE_TIMEOUT      100
#define ADC_MAX_CHANNEL         5

struct adc_dev {
    struct clk* clk;

    uint32_t        init_flag;
    uint32_t        exit_flag;

    volatile int data_wait;
    int irq;
};

static struct adc_dev adc_device;

static void adc_irq_handler(int irq, void *data)
{
    adc_hal_clean_all_interrupt_flag();

    adc_device.data_wait = 1;
}

int adc_read_data(unsigned int channel)
{
    int val = 0;
    long long time1;
    long long time2;
    int timeout;

    if(channel > ADC_MAX_CHANNEL) {
        return -EINVAL;
    }

    if (adc_device.exit_flag) {
        val = -ENODEV;
        goto adc_sample_exit;
    }

    adc_hal_enable_channel(channel);

    time1 = systick_get_time_usec();
    while (!adc_device.data_wait) {
        time2 = systick_get_time_usec();
        timeout = time2 - time1;
        if (timeout > ADC_SAMPLE_TIMEOUT *1000) {
            val = -ETIMEDOUT;
            break;
        }
    }

    adc_device.data_wait = 0;

    if (adc_device.exit_flag) {
        val = -ENODEV;
        goto adc_sample_exit;
    }

    if (val)
        goto adc_sample_exit;

    val = adc_hal_read_channel_data(channel);

adc_sample_exit:
    adc_hal_disable_channel(channel);

    return val;
}

void adc_init(void)
{
    assert(!adc_device.init_flag);
    adc_device.init_flag = 1;

    clk_enable(CLK_GATE_SADC, 1);

    adc_hal_disable_controller();
    adc_hal_mask_all_interrupt();
    adc_hal_clean_all_interrupt_flag();

    adc_hal_set_clkdiv(CLKDIV, CLKDIV_US, CLKDIV_MS);
    adc_hal_set_wait_sampling_stable_time(STABLE_TIME);

    adc_hal_enable_controller();

    request_irq(IRQ_SADC, 0, adc_irq_handler, DEV_NAME, NULL);

    adc_hal_enable_all_interrupt();

}

void adc_deinit(void)
{
    assert(!adc_device.exit_flag);
    adc_device.exit_flag = 1;

    disable_irq(IRQ_SADC);
    release_irq(IRQ_SADC);

    adc_hal_disable_controller();

    clk_enable(CLK_GATE_SADC, 0);

}
