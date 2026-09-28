#include <stdio.h>
#include <string.h>
#include <cpu/uncache_mem.h>
#include <driver/uart.h>

static struct uart_config uart = {
    .uart_id = 0,
    .data_bits = 8,
    .stop_bits = 1,
    .loop_mode = 0,
    .parity = UART_PARITY_NONE,
    .follow_contrl = UART_FC_NONE,
    .baud_rate = 3000000,
    .init_flag = 0,
    .tx_use_dma = 1,
    .rx_use_dma = 1,
    .rxdma_size = 256,
};

static struct uart_message tx_msg = {
    .uart_id = 0,
};

static volatile int my_tx_is_done = 0;
static void my_tx_cb(void *data)
{
    printf("Callback is comming, it mean the transfer is done.\n");
    my_tx_is_done = 1;
}

void uart_dma_test(void)
{
    /* 初始化 uart */
    uart_start(&uart);

    while (1) {
        char *buf = "I am uart_send_dma_msg.\r\n";
        char *buf_1 = uncache_mem_alloc(32, 32);
        memcpy(buf_1, buf, strlen(buf));
        tx_msg.len = strlen(buf_1);
        tx_msg.buf = buf_1;
        tx_msg.cb = my_tx_cb;

        /* uart_dma发送(异步接口) */
        uart_send_dma_msg(&tx_msg);
        /* 如果有必要,对于异步接口需要判断传输是否完成 */
        /* 方式一:使用 uart_msg_is_done 接口判断 */
        // while (!uart_msg_is_done(&tx_msg));

        /* 方式二:利用回调函数设置标志 */
        while (!my_tx_is_done);
        my_tx_is_done = 0;

        char str[64+1];
        while (1) {
            int id = uart.uart_id;

            /* uart_dma接收, 获取当前是否可读 */
            while (!uart_receive_dma_get_size(id));

            /* uart_dma接收, 读取数据 */
            int len = uart_receive_dma(id, str, 64);
            str[len] = 0;
            printf("uart_receive_dma get str = %s\n", str);
        }
    }
}
