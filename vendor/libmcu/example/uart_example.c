#include <stdio.h>
#include <string.h>
#include <driver/uart.h>

static struct uart_config uart = {
    .uart_id = 3,
    .data_bits = 8,
    .stop_bits = 1,
    .loop_mode = 0,
    .parity = UART_PARITY_NONE,
    .follow_contrl = UART_FC_NONE,
    .baud_rate = 3000000,
    .init_flag = 0,
};

static struct uart_message tx_msg = {
    .uart_id = 3,
};

static struct uart_message rx_msg = {
    .uart_id = 3,
};

static volatile int my_rx_is_done = 0;
static void my_rx_cb(void *data)
{
    printf("Callback is comming, it mean the receive is done.\n");
    my_rx_is_done = 1;
}

void uart_test(void)
{
    int ret;
    int len;

    /* 初始化 uart */
    uart_start(&uart);

    char *buf_1 = "I am uart_send.\r\n";
    len = strlen(buf_1);

    /* uart发送(同步接口) */
    uart_send(&uart, buf_1, len);


    char *buf_2 = "I am uart_send_msg.\r\n";
    len = strlen(buf_2);
    tx_msg.len = len;
    tx_msg.buf = buf_2;
    // tx_msg.cb = my_tx_cb; // 可选

    /* uart发送(异步接口) */
    uart_send_msg(&tx_msg);

    char ch;
    rx_msg.len = 1;
    rx_msg.buf = &ch;
    rx_msg.cb = my_rx_cb; // 可选

    /* uart接收(异步接口) */
    uart_receive_msg(&rx_msg);

    /* 如果有必要,对于异步接口需要判断传输是否完成 */
    /* 方式一:使用 uart_msg_is_done 接口判断 */
    while (!uart_msg_is_done(&tx_msg));

    /* 方式二:利用回调函数设置标志 */
    while (!my_rx_is_done);

    printf("uart_receive_msg get char.ch = %c\n", ch);

    /* uart接收(同步接口) */
    while (1) {
        char ch;
        ret = uart_receive(&uart, &ch, 1);
        if (ret) {
            printf("uart_receive get char.ch = %c\n", ch);
        }
    }

}