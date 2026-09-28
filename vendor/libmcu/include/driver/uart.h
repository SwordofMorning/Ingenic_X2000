#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

#include <list.h>

enum uart_parity {
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
};

enum uart_follow_contrl {
    UART_FC_NONE,
    UART_FC_CTS_RTS,
};

struct uart_config {
    unsigned char uart_id;
    unsigned char data_bits;
    unsigned char stop_bits;
    unsigned char loop_mode;
    enum uart_parity parity;
    enum uart_follow_contrl follow_contrl;
    unsigned int baud_rate;
    unsigned int init_flag;

    unsigned int tx_use_dma;
    unsigned int rx_use_dma;
    unsigned int rxdma_size; /* 若使用dma模式, 请确保 接收数据长度 不超过该值 */
};

struct uart_message {
    unsigned int uart_id;
    void *buf;
    unsigned int len;
    unsigned int txrx_len;
    unsigned int is_done;
    struct list_head link;
    void (*cb)(void *data);
};

void uart_send(struct uart_config *uart, const char *buf, unsigned int len);

void uart_send_msg(struct uart_message *msg);

int uart_receive(struct uart_config *uart, char *buf, unsigned int len);

int uart_receive_msg(struct uart_message *msg);

int uart_msg_is_done(struct uart_message *msg);

void uart_send_dma_msg(struct uart_message *msg);

int uart_receive_dma(int id, char *buf, int len);

int uart_receive_dma_get_size(int id);

void uart_start(struct uart_config *uart);

void uart_stop(struct uart_config *uart);

void uart_init(void);

#endif /* _DRIVER_UART_H_ */
