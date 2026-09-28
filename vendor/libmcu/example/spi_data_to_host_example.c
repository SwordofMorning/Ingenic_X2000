#include <stdio.h>
#include <string.h>

#include <soc/base.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <cpu/host_cpu.h>

#include <driver/gpio.h>

#include <driver/spi.h>

#include <driver/systick.h>
#include <driver/hrtimer.h>

/*
 * GPIO_O_0 连接 GPIO_I_1, spi_clk 连接 GPIO_CATCH_SPI_CLK
 * hrtimer 800us 拉高 GPIO_O_0, 模拟 spi从设备 触发 主机 开始传输
 * 检测到 GPIO_I_1 上升沿进中断, 记录此时时间t0, 拉高 GPIO_O_2 且 开始 spi 传输
 * spi 开始传输模拟fb数据且产生 clk 波形, 触发 GPIO_CATCH_SPI_CLK 中断, 记录此时时间t1
 * 测量波形可看到 GPIO_O_2 与 GPIO_O_0 的上升沿跳变时间差, 为进中断所需时间
 * t1 - t0: spi 开始传输到实际产生 clk 的时间, 大于10us将打印上次clk产生到本次clk产生的时间,
 *      本次 spi 开始传输到实际产生 clk 的时间 及 spi 开始传输到实际产生 clk 的平均时间
 */

#define GPIO_O_0    GPIO_PC(4)
#define GPIO_I_1    GPIO_PC(5)
#define GPIO_O_2    GPIO_PD(4)
/*cs pin*/
#define GPIO_O_3    GPIO_PD(3)

#define GPIO_CATCH_SPI_CLK GPIO_PD(5)

#define DEBUG_TRIGGER_TO_CATCH_US 10

#define TIMEOUT_TIME_US 800
#define SPI_MAX_SIZE 512

static uint64_t expires_count;
static uint64_t timeout_count;

/* simulate the signal that spi_slave wants to write */
static struct hrtimer timer;

/*for debug printf*/
static int64_t sum;
static int64_t trigger_count;
static int64_t trigger_average;
static int64_t trigger_time = 0;

static volatile int spi_dma_states;
static volatile int is_data_comming = 0;

#define HOST_START "mcu_spi_ready"
#define MCU_START "mcu_spi_start"
#define HOST_STOP "mcu_spi_end"

unsigned char dma_tx_buf[SPI_MAX_SIZE];
unsigned char dma_rx_buf[SPI_MAX_SIZE];

static struct spi_config_data config = {
    .id = 0,
    .cs_pin = GPIO_O_3,
    .clk_rate = 10 * 1000 * 1000,
    .cs_valid_level = Spi_valid_low,
    .tx_endian = Spi_endian_msb_first,
    .rx_endian = Spi_endian_msb_first,
    .bits_per_word = 8,
    .spi_pol = 0,
    .spi_pha = 0,
    .loop_mode = 0,
    .poll_mode = 1,
};

static struct spi_message msg = {
    .tx_buf = dma_tx_buf,
    .rx_buf = dma_rx_buf,
    .tlen  = SPI_MAX_SIZE,
    .rlen  = SPI_MAX_SIZE,
    .cs_change = 1,
};

#ifdef APP_libmcu_driver_irq
static void host_irq_cb(void)
{
    is_data_comming = 1;
}
#endif

static int mcu_check_host_state(void *state)
{
    int ret;
    int len = 0;
    char buf[20] = {0};

    while (!is_data_comming);
    is_data_comming = 0;

    while (1) {
        ret = host_cpu_read(buf+len, sizeof(buf)-len);
        if (!ret)
            break;

        len += ret;
    }

    if (!strcmp(state, buf))
        return 0;

    return -1;
}

static int mcu_send_data(void *buf, unsigned int len)
{
    int ret;
    int send_len = len;
    /* exit when all data were write */
    while (send_len) {
        while (mcu_test_host_busy());

        while (send_len) {
            ret = host_cpu_write(buf, send_len);
            send_len -= ret;
            if (!ret)
                break;
        }

        len -= send_len;
        mcu_notify_host(len);
    }

    return 0;
}

static int mcu_send_spi_data(void)
{
    if (is_data_comming && !mcu_check_host_state(HOST_STOP)) {
        printf("stop get and send data to host\n");
        return 1;
    }

    return mcu_send_data(dma_rx_buf, SPI_MAX_SIZE);
}

static int frame_size = 720 * 1280 * 4;
static int colors[3] = {0xffff0000, 0xff00ff00, 0xff0000ff};
static int color_index = 0;
static int fb_pos = 0;
/* 模拟 尺寸为 720 × 1280,格式为 ARGB 的fb 数据, 每帧颜色依次为 红绿蓝 数据 */
void simulate_fb_data(void)
{
    int i, n;
    unsigned int *p = (unsigned int *)dma_tx_buf;

    if (fb_pos + SPI_MAX_SIZE <= frame_size) {
        n = SPI_MAX_SIZE / 4;
        fb_pos += SPI_MAX_SIZE;
        for (i = 0; i < n; i++)
            *p++ = colors[color_index];
        return;
    }

    n = (frame_size - fb_pos) / 4;
    for (i = 0; i < n; i++)
        *p++ = colors[color_index];

    color_index++;
    color_index = color_index % 3;

    fb_pos = fb_pos + SPI_MAX_SIZE - frame_size;
    n = fb_pos / 4;
    for (i = 0; i < n; i++)
        *p++ = colors[color_index];
}

static void set_trigger_gpio_low(void)
{
    gpio_direction_output(GPIO_O_0, 0);
    gpio_direction_output(GPIO_O_2, 0);
}

static void timer_callback(struct hrtimer *timer)
{
    expires_count += timeout_count;
    hrtimer_restart_at_expires(timer, expires_count);
    gpio_direction_output(GPIO_O_0, 1);
}

static void trigger_handler(int irq, void *data)
{
    trigger_time = systick_get_time_usec();

    gpio_direction_output(GPIO_O_2, 1);

    enable_irq(gpio_to_irq(GPIO_CATCH_SPI_CLK));

    spi_dma_states = 1;
}

static void catch_spi_clk_irq(int irq, void *data)
{
    static int64_t old_time = 0;

    int64_t now = systick_get_time_usec();

    disable_irq(gpio_to_irq(GPIO_CATCH_SPI_CLK));

    if (now - trigger_time > DEBUG_TRIGGER_TO_CATCH_US)
        printf("%lld, %lld, %lld\n", (now - old_time), (now - trigger_time), trigger_average);

    old_time = now;

    sum += now - trigger_time;
    trigger_count++;
    trigger_average = sum / trigger_count;
}

void spi_to_host_test(void)
{
#ifdef APP_libmcu_driver_irq
    host_cpu_set_irq_callback(host_irq_cb);
#endif

    while (mcu_check_host_state(HOST_START));

    set_trigger_gpio_low();
    spi_config_init(&config);

    hrtimer_init(&timer, timer_callback);
    request_irq(gpio_to_irq(GPIO_I_1), IRQ_TYPE_EDGE_RISING, trigger_handler, "gpio_trigger", NULL);
    request_irq_disabled(gpio_to_irq(GPIO_CATCH_SPI_CLK), IRQ_TYPE_EDGE_RISING, catch_spi_clk_irq, "catch_spi_clk", NULL);

    mcu_send_data(MCU_START, strlen(MCU_START)+1);

    timeout_count = systick_usec_to_count(TIMEOUT_TIME_US);
    expires_count = systick_get_count() + timeout_count;
    hrtimer_start_at_expires(&timer, expires_count);

    while (1) {
        simulate_fb_data();
        memset(dma_rx_buf, 0, SPI_MAX_SIZE);

        while (!spi_dma_states) ;
        spi_dma_states = 0;

        spi_transfer(&config, &msg, 1);

        if (mcu_send_spi_data())
            break;

        set_trigger_gpio_low();
    }

    set_trigger_gpio_low();
    disable_irq(gpio_to_irq(GPIO_I_1));
    disable_irq(gpio_to_irq(GPIO_CATCH_SPI_CLK));
    release_irq(gpio_to_irq(GPIO_I_1));
    release_irq(gpio_to_irq(GPIO_CATCH_SPI_CLK));
}
