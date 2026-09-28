#include <cpu/io.h>
#include <soc/base.h>
#include <driver/uart.h>

#include <bit_field2.h>
#include <bits_opt.h>

#include "uart_regs.h"

static const unsigned long iobase[] = {
    UART0_IOBASE,
    UART1_IOBASE,
    UART2_IOBASE,
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

void uart_send(struct uart_config *uart, const char *buf, unsigned int len)
{
    int i;
    int id = uart->uart_id;

    for (i = 0; i < len; i++) {
        while (UART_FIFO_LEN <= uart_read_reg(id, UTCR));

        uart_write_reg(id, UTHR, (unsigned char)buf[i]);
    }

    while (!uart_get_bit(id, ULSR, ULSR_TEMP) || !uart_get_bit(id, ULSR, ULSR_TDRQ));
}

void uart_init(void)
{

}

void uart_start(struct uart_config *uart)
{

}

void uart_stop(struct uart_config *uart)
{

}
