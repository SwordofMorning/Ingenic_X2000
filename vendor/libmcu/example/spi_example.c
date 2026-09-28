#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/spi.h>

struct spi_config_data config = {
    .id = 0,
    .cs_pin = GPIO_PB(0), /* 指定 GPIO 作为 CS 引脚 */
    .clk_rate = 1 * 100 * 1000, /* 配置时钟频率 */
    .cs_valid_level = Spi_valid_high, /* 配置spi有效电平为高 */
    .tx_endian = Spi_endian_msb_first,
    .rx_endian = Spi_endian_msb_first,
    .bits_per_word = 8, /* 数据位宽 8,即传输过程中的最小数据单位为8bit */
    .spi_pha = 0,
    .spi_pol = 0,
    .loop_mode = 0,
    .poll_mode = 1,
};

void spi_test(void)
{
    u8 tx_buf1[] = {0x11, 0x22, 0x33, 0X44, 0x55, 0x66, 0x77, 0x88};
    u8 rx_buf1[64];

    u8 tx_buf2[] = {0x11, 0x22, 0x33, 0X44, 0x55, 0x66, 0x77, 0x88};
    u8 rx_buf2[64];

    int len1 = sizeof(tx_buf1)/sizeof(tx_buf1[0]);
    int len2 = sizeof(tx_buf2)/sizeof(tx_buf2[0]);
    struct spi_message msg[] = {
        {
            .tx_buf = tx_buf1,  /* tx_buf可以为空,表示只接收不发送 */
            .rx_buf = rx_buf1,  /* rx_buf可以为空,表示只发送不接收 */
            .tlen  = len1,      /* tlen 可以为 0,表示只接收不发送 */
            .rlen  = len1,      /* rlen 可以为 0,表示只发送不接收 */
            .cs_change = 1,     /* 第一个transfer结束改变 cs 电平, 0 表示传输结束不改变 */
        },
        {
            .tx_buf = tx_buf2,
            .rx_buf = rx_buf2,
            .tlen  = len2,
            .rlen  = len2,
            .cs_change = 0,     /* 最后一次传输结束都将改变 cs 电平 */
        },
    };

    spi_config_init(&config);

    spi_transfer(&config, msg, sizeof(msg)/sizeof(msg[0]));

    if (memcmp(tx_buf1, rx_buf1, len1) || memcmp(tx_buf2, rx_buf2, len2))
        printf("spi test data err!--------------------\n");
    else
        printf("spi test data right!\n");
}


#include <stdio.h>
#include <string.h>

#include <cpu/uncache_mem.h>
#include <driver/gpio.h>
#include <driver/spi.h>

struct spi_config_data dma_config = {
    .id = 0,
    .cs_pin = GPIO_PB(0),
    .clk_rate = 1 * 100 * 1000,
    .cs_valid_level = Spi_valid_high,
    .tx_endian = Spi_endian_msb_first,
    .rx_endian = Spi_endian_msb_first,
    .bits_per_word = 8,
    .spi_pha = 0,
    .spi_pol = 0,
    .loop_mode = 0,
    .poll_mode = 0,
};

void spi_dma_test(void)
{
    char *buf1 = "spi dma send round 1";
    unsigned char *dma_tx_buf1 = uncache_mem_alloc(32, 32);
    unsigned char *dma_rx_buf1 = uncache_mem_alloc(32, 32);
    memcpy(dma_tx_buf1, buf1, strlen(buf1)+1);

    char *buf2 = "spi dma send round 2";
    unsigned char *dma_tx_buf2 = uncache_mem_alloc(32, 32);
    unsigned char *dma_rx_buf2 = uncache_mem_alloc(32, 32);
    memcpy(dma_tx_buf2, buf2, strlen(buf2)+1);

    char *buf3 = "spi dma send round 3";
    unsigned char *dma_tx_buf3 = uncache_mem_alloc(32, 32);
    unsigned char *dma_rx_buf3 = uncache_mem_alloc(32, 32);
    memcpy(dma_tx_buf3, buf3, strlen(buf3)+1);

    /* len 为 发送和接收数据的个数，在君正平台下,dma传输需要保证发送和接收的个数相等.*/
    int len1 = strlen((const char *)dma_tx_buf1);
    int len2 = strlen((const char *)dma_tx_buf2);
    int len3 = strlen((const char *)dma_tx_buf3);

    /* 以下是一个msg 多个 transfer 传输的例子
     * NOTE:多个 transfer 的最后一次传输,无论cs_change是否为1,都将改变cs电平.*/
    struct spi_message msg[] = {
        {
            // transfer 1
            .tx_buf = dma_tx_buf1,
            .rx_buf = dma_rx_buf1,
            .tlen  = len1,
            .rlen  = len1,
            .cs_change = 1, /* 第一个transfer结束改变 cs 电平 */
            .use_dma = 1,   /* 使用 dma 方式传输 */
        },
        {
            // transfer 2
            .tx_buf = dma_tx_buf2,
            .rx_buf = dma_rx_buf2,
            .tlen  = len2,
            .rlen  = len2,
            .cs_change = 0, /* 第二个transfer结束不改变 cs 电平 */
            .use_dma = 1,
        },
        {
            // transfer 3
            .tx_buf = dma_tx_buf3,
            .rx_buf = dma_rx_buf3,
            .tlen  = len3,
            .rlen  = len3,
            .cs_change = 0, /* 最后一次传输结束都将改变 cs 电平 */
            .use_dma = 1,
        },
    };

    spi_config_init(&dma_config);

    /* SPI 传输 */
    spi_transfer(&dma_config, msg, sizeof(msg)/sizeof(msg[0]));

    if (memcmp(dma_rx_buf1, dma_rx_buf1, len1) ||
        memcmp(dma_rx_buf2, dma_rx_buf2, len2) ||
        memcmp(dma_rx_buf3, dma_rx_buf3, len3))
        printf("spi dma test data err!--------------------\n");
    else
        printf("spi dma test data right!\n");
}


#include <stdio.h>
#include <string.h>

#include <driver/irq.h>
#include <delay.h>
#include <cpu/uncache_mem.h>

#include <driver/gpio.h>
#include <driver/spi.h>

struct spi_config_data dma_async_config = {
    .id = 0,
    .cs_pin = GPIO_PB(0),
    .clk_rate = 1 * 100 * 1000,
    .cs_valid_level = Spi_valid_high,
    .tx_endian = Spi_endian_msb_first,
    .rx_endian = Spi_endian_msb_first,
    .bits_per_word = 8,
    .spi_pol = 0,
    .spi_pha = 0,
    .loop_mode = 0,
    .poll_mode = 0,
};

#define SPI_MAX_DMA_SIZE 4096

#define BUTTION_GPIO    GPIO_PD(15)

unsigned char *dma_tx_buf1;
unsigned char *dma_rx_buf1;

/* 以下是中断内使用dma传输的例子 */
struct spi_message msg = {
    .tx_buf = NULL,
    .rx_buf = NULL,
    .tlen  = SPI_MAX_DMA_SIZE,
    .rlen  = SPI_MAX_DMA_SIZE,
    .cs_change = 1, /* transfer结束改变 cs 电平 */
    .use_dma = 1,   /* 使用 dma 方式传输 */
};
int spi_dma_states;

void spi_dma_async_cb(struct spi_config_data *config)
{
    /* NOTE: 本次传输完成才可以开始下一次传输 */
    spi_dma_states = 1;
}

static void button_gpio_irq_handler(int irq, void *data)
{
    spi_dma_async_transfer(&dma_async_config, &msg);
    printf("irq: %d %d\n", irq, gpio_get_value(irq_to_gpio(irq)));
}

void spi_dma_in_irq_test(void)
{
    int tx_value = 0;

    dma_tx_buf1 = uncache_mem_alloc(SPI_MAX_DMA_SIZE, 32);
    dma_rx_buf1 = uncache_mem_alloc(SPI_MAX_DMA_SIZE, 32);

    msg.tx_buf = dma_tx_buf1;
    msg.rx_buf = dma_rx_buf1;

    spi_config_init(&dma_async_config);
    spi_dma_start(&dma_async_config, &msg, spi_dma_async_cb);

    /* 按下boot按键即可开始传输一次 */
    gpio_set_func(BUTTION_GPIO, (GPIO_INPUT | GPIO_PULL_HIZ));

    request_irq(gpio_to_irq(BUTTION_GPIO), IRQ_TYPE_EDGE_FALLING, button_gpio_irq_handler, "button", NULL);

    while (1) {
        /* 每次传输都必须进行如下 cache 操作(cache操作的条件见 NOTE1): */
        int i = 0;
        for (i = 0; i < SPI_MAX_DMA_SIZE; i++)
            dma_tx_buf1[i] = i+tx_value;

        memset(dma_rx_buf1, 0, SPI_MAX_DMA_SIZE);

        while (!spi_dma_states) udelay(100);
        spi_dma_states = 0;
        tx_value++;

        if (memcmp(dma_tx_buf1, dma_rx_buf1, SPI_MAX_DMA_SIZE))
            printf("err: rx_buf1 = %x, %x\n", dma_rx_buf1[0], dma_rx_buf1[SPI_MAX_DMA_SIZE - 1]);
        else
            printf("data right!\n");
    }

    spi_dma_stop(&dma_async_config);
}
