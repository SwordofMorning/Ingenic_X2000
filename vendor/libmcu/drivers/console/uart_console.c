#include <string.h>
#include <driver/console.h>
#include <driver/uart.h>

static struct uart_config uart = {
    .uart_id = APP_libmcu_driver_console_uart_index,
    .data_bits = 8,
    .stop_bits = 1,
    .loop_mode = 0,
    .parity = UART_PARITY_NONE,
    .follow_contrl = UART_FC_NONE,
    .baud_rate = APP_libmcu_driver_console_buad_rate,
#ifdef APP_libmcu_driver_console_no_need_to_init
    .init_flag = 1,
#endif
};

void console_put_char(char ch)
{
    if (uart.uart_id < 0)
        return;

    static int is_r;

    /*
    * 编译器会将结尾是 "\n" "\r\n" 的纯字符串替换成 "\r"
    */
    if (ch == '\r') {
        uart_send(&uart, "\r\n", 2);
        is_r = 1;
    } else {
        if (ch == '\n' && !is_r)
            uart_send(&uart, "\r\n", 2);
        else
            uart_send(&uart, &ch, 1);
        is_r = 0;
    }
}

void console_puts(const char *str)
{
    while (*str)
        console_put_char(*str++);
}

char console_get_char(void)
{
    char ch;

    int ret = uart_receive(&uart, &ch, 1);

    if (ret)
        return ch;
    else
        return 0;
}

void console_init(void)
{
    uart_start(&uart);
}