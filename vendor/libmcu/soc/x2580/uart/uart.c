#include <stdio.h>
#include <assert.h>
#include <soc/base.h>

#include <driver/uart.h>

#include <string.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <malloc.h>
#include <delay.h>

#include "uart_regs.h"
#include "uart_baud_data.h"

static const unsigned long iobase[] = {
    UART0_IOBASE,
    UART1_IOBASE,
    UART2_IOBASE,
    UART3_IOBASE,
    UART4_IOBASE,
    UART5_IOBASE,
};

#define UART_ADDR(id, reg)    ((volatile unsigned long *)(iobase[id] + reg))

static inline void uart_write_reg(int id, unsigned int reg, unsigned int value)
{
    *UART_ADDR(id, reg) = value;
}

static inline unsigned int uart_read_reg(int id, unsigned int reg)
{
    return *UART_ADDR(id, reg);
}

static inline unsigned int uart_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(*UART_ADDR(id, reg), start, end);
}

static inline void uart_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    *UART_ADDR(id, reg) = set_bit_field(*UART_ADDR(id, reg), start, end, val);
}

struct uart_data {
    unsigned char is_inited;
    unsigned char is_irq_inited;
    unsigned char is_gpio_inited;

    struct list_head send_list;
    struct list_head recv_list;
};

static struct uart_data uartdata[10];

static const char *uartstr[] = {
    "uart0",
    "uart1",
    "uart2",
    "uart3",
    "uart4",
    "uart5",
    "uart6",
    "uart7",
    "uart8",
    "uart9",
};

static int uart_write_fifo(int id, char *buf, unsigned int len)
{
    int i;
    int n = UART_FIFO_LEN - uart_read_reg(id, UTCR);
    if (n < len)
        len = n;

    for (i = 0; i < len; i++)
        uart_write_reg(id, UTHR, (unsigned char)buf[i]);

    return len;
}

static int uart_read_fifo(int id, char *buf, unsigned int len)
{
    int i;

    int n = uart_read_reg(id, URCR);

    if (n < len)
        len = n;

    for (i = 0; i < len; i++)
        ((unsigned char *)buf)[i] = uart_read_reg(id, URBR);

    return len;
}

static void uart_disable(int id)
{
    uart_set_bit(id, UFCR, UFCR_UME, 0);
}

static void uart_disable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 0);
}

static void uart_disable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    uier = set_bit_field(uier, UIER_RTOIE, 0);
    uier = set_bit_field(uier, UIER_RDRIE, 0);
    uart_write_reg(id, UIER, uier);
}

static void uart_enable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    uier = set_bit_field(uier, UIER_RTOIE, 1);
    uier = set_bit_field(uier, UIER_RLSIE, 1);
    uier = set_bit_field(uier, UIER_RDRIE, 1);
    uart_write_reg(id, UIER, uier);
}

static void uart_enable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 1);
}

static int uart_send_poll(int id, char *buf, unsigned int len)
{
    int ret;

    while (len) {
        ret = uart_write_fifo(id, buf, len);
        while (uart_read_reg(id, UTCR));
        buf += ret;
        len -= ret;
    }

    return len;
}

static int uart_receive_poll(int id, char *buf, unsigned int len)
{
    int ret;
    int rx_len = len;

    while (rx_len) {
        ret = uart_read_fifo(id, buf, rx_len);
        buf += ret;
        rx_len -= ret;

        if (!ret)
            return 0;
    }

    return len;
}

void uart_send(struct uart_config *config, const char *buf, unsigned int len)
{
    uart_send_poll(config->uart_id, (char *)buf, len);
}

int uart_receive(struct uart_config *config, char *buf, unsigned int len)
{
    return uart_receive_poll(config->uart_id, buf, len);
}

void uart_start(struct uart_config *config)
{
    unsigned char id = config->uart_id;

    struct uart_data *uart = &uartdata[id];

    assert(id < UART_NUMS);
    assert(!uart->is_inited);

    if (!config->init_flag) {

    }

    INIT_LIST_HEAD(&uart->send_list);
    INIT_LIST_HEAD(&uart->recv_list);

    uart->is_gpio_inited = 1;
    uart->is_inited = 1;
}

void uart_init(void){}