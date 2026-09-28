#include <stdio.h>
#include <assert.h>
#include <cpu/io.h>
#include <soc/base.h>
#include <soc/gpio.h>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/irq.h>
#include <driver/clk.h>
#include <driver/hrtimer.h>
#include <string.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <malloc.h>
#include <delay.h>
#include <cpu/uncache_mem.h>
#include <driver/dma.h>

#include "uart_regs.h"
#include "uart_baud_data.h"

static const unsigned long iobase[] = {
    UART0_IOBASE,
    UART1_IOBASE,
    UART2_IOBASE,
    UART3_IOBASE,
    UART4_IOBASE,
    UART5_IOBASE,
    UART6_IOBASE,
    UART7_IOBASE,
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

    unsigned int baud_rate;
    struct uart_message *tx_last_msg;
    struct hrtimer tx_timer;
    struct list_head send_list;
    struct list_head recv_list;

#ifdef APP_libmcu_x2600_dma
    unsigned int tx_use_dma;
    struct dma *txdma;
    struct list_head txdma_list;

    unsigned int rx_use_dma;
    int rxdma_size;
    struct dma *rxdma;
    unsigned long rxdma_buf;
    unsigned long dst_old_addr;
#endif
};

static struct uart_data uartdata[8];

struct gpio_func {
    short gpio;
    unsigned short func;
};

struct uart_gpio {
    short is_enable;
    struct gpio_func rx;
    struct gpio_func tx;
    struct gpio_func cts;
    struct gpio_func rts;
};

static const char *uartstr[] = {
    "uart0",
    "uart1",
    "uart2",
    "uart3",
    "uart4",
    "uart5",
    "uart6",
    "uart7",
};

static const int uartirq[] = {
    IRQ_UART0,
    IRQ_UART1,
    IRQ_UART2,
    IRQ_UART3,
    IRQ_UART4,
    IRQ_UART5,
    IRQ_UART6,
    IRQ_UART7,
};

#ifdef APP_libmcu_x2600_dma
static int uart_tdma[8] = {
    [0] = DMA_RQ_UART0_TX,
    [1] = DMA_RQ_UART1_TX,
    [2] = DMA_RQ_UART2_TX,
    [3] = DMA_RQ_UART3_TX,
    [4] = DMA_RQ_UART4_TX,
    [5] = DMA_RQ_UART5_TX,
    [6] = DMA_RQ_UART6_TX,
    [7] = DMA_RQ_UART7_TX,
};

static int uart_rdma[8] = {
    [0] = DMA_RQ_UART0_RX,
    [1] = DMA_RQ_UART1_RX,
    [2] = DMA_RQ_UART2_RX,
    [3] = DMA_RQ_UART3_RX,
    [4] = DMA_RQ_UART4_RX,
    [5] = DMA_RQ_UART5_RX,
    [6] = DMA_RQ_UART6_RX,
    [7] = DMA_RQ_UART7_RX,
};
#endif

enum uart_int_type {
    INT_moden_status,
    INT_tx_request,
    INT_rx_ready,
    INT_rx_line_status,
    INT_reserve_0,
    INT_rx_empty_timeout,
    INT_rx_timeout,
    INT_reserve_1,
};

#define UART_GPIO_DEF(id) \
    [id] = { \
        .is_enable = 1, \
        .rx = {APP_libmcu_x2600_uart##id##_rx}, \
        .tx = {APP_libmcu_x2600_uart##id##_tx}, \
        .cts = {APP_libmcu_x2600_uart##id##_cts}, \
        .rts = {APP_libmcu_x2600_uart##id##_rts}, \
    }

struct uart_gpio uartgpio[8] = {
#ifdef APP_libmcu_x2600_uart0
    UART_GPIO_DEF(0),
#endif
#ifdef APP_libmcu_x2600_uart1
    UART_GPIO_DEF(1),
#endif
#ifdef APP_libmcu_x2600_uart2
    UART_GPIO_DEF(2),
#endif
#ifdef APP_libmcu_x2600_uart3
    UART_GPIO_DEF(3),
#endif
#ifdef APP_libmcu_x2600_uart4
    UART_GPIO_DEF(4),
#endif
#ifdef APP_libmcu_x2600_uart5
    UART_GPIO_DEF(5),
#endif
#ifdef APP_libmcu_x2600_uart6
    UART_GPIO_DEF(6),
#endif
#ifdef APP_libmcu_x2600_uart7
    UART_GPIO_DEF(7),
#endif
};

static void uart_gpio_set_func(int id)
{
    struct uart_gpio *uio = &uartgpio[id];

    if (!uio->is_enable)
        return;

    if (uio->rx.gpio != -1)
        gpio_set_func(uio->rx.gpio, uio->rx.func);

    if (uio->tx.gpio != -1)
        gpio_set_func(uio->tx.gpio, uio->tx.func);

    if (uio->cts.gpio != -1)
        gpio_set_func(uio->cts.gpio, uio->cts.func);

    if (uio->rts.gpio != -1)
        gpio_set_func(uio->rts.gpio, uio->rts.func);
}

static void uart_clk_enable(int uart_id, int on)
{
    int uart_bit;

    assert(uart_id <= UART_NUMS);

    uart_bit = CLK_GATE_UART0 + uart_id;

    if (on)
        clk_gate_enable(uart_bit);
    else
        clk_gate_disable(uart_bit);
}

static struct baudtoregs_t *uart_get_bauddata(unsigned int baud)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(baudtoregs); i++) {
        if (baudtoregs[i].baud == baud)
            return &baudtoregs[i];
    }

    return NULL;
}

static void uart_init_config(struct uart_config *config)
{
    unsigned char id = config->uart_id;

    struct baudtoregs_t *bauddata = uart_get_bauddata(config->baud_rate);
    if (bauddata == NULL)
        printf("uart get bauddata fail for rate : %d\n", config->baud_rate);

    /* 关闭uart */
    uart_set_bit(id, UFCR, UFCR_UME, 0);

    /* 设置波特率 div */
    uart_set_bit(id, ULCR, ULCR_DLAB, 1);
    uart_write_reg(id, UDLHR, bauddata->div >> 8);
    uart_write_reg(id, UDLLR, bauddata->div & 0xff);
    uart_set_bit(id, ULCR, ULCR_DLAB, 0);

    /* 设置波特率 M, AC */
    uart_write_reg(id, UMR, bauddata->umr);
    uart_write_reg(id, UACR, bauddata->uacr);

    /* 设置 parity, data bits, stop bits */
    unsigned long ulcr = 0;
    ulcr = set_bit_field(ulcr, ULCR_PARE, config->parity != UART_PARITY_NONE);
    ulcr = set_bit_field(ulcr, ULCR_PARM, config->parity == UART_PARITY_EVEN);
    ulcr = set_bit_field(ulcr, ULCR_SBLS, config->stop_bits == 2);
    ulcr = set_bit_field(ulcr, ULCR_WLS, config->data_bits - 5);

    uart_write_reg(id, ULCR, ulcr);

    /* 设置 loop mode, Modem Control/follow control */
    unsigned long umcr = 0;
    umcr = set_bit_field(umcr, UMCR_FCM,  config->follow_contrl == UART_FC_CTS_RTS);
    umcr = set_bit_field(umcr, UMCR_MDCE, config->follow_contrl == UART_FC_CTS_RTS);
    umcr = set_bit_field(umcr, UMCR_LOOP, config->loop_mode);

    uart_write_reg(id, UMCR, umcr);
}

static void uart_enable_fifo_mode(struct uart_config *config)
{
    int id = config->uart_id;
    /* 设置中断
     * 程序调用接收时动态开启 接收 timeout, rdy, 接收错误 中断
     */
    unsigned long uier = 0;
    uier = set_bit_field(uier, UIER_RTOIE, 0);
    uier = set_bit_field(uier, UIER_RDRIE, 0);
    uier = set_bit_field(uier, UIER_RLSIE, 0);

    /* 发送时动态开启 uart 发送中断 */
    uier = set_bit_field(uier, UIER_TDRIE, 0);

    uart_write_reg(id, UIER, uier);

    unsigned long urtdcr = 0;
    urtdcr = set_bit_field(urtdcr, URTDCR_RTD_EN, config->tx_use_dma || config->rx_use_dma);
    urtdcr = set_bit_field(urtdcr, URTDCR_RTD_R, config->rx_use_dma);
    urtdcr = set_bit_field(urtdcr, URTDCR_RTD_T, config->tx_use_dma);
    uart_write_reg(id, URTDCR, urtdcr);

    /* 使能uart, 使能fifo 模式, 清接收/发送fifo, 设置接收fifo 32字节触发 */
    unsigned long ufcr = 0;

    ufcr = set_bit_field(ufcr, UFCR_UME, 1);
    ufcr = set_bit_field(ufcr, UFCR_FME, 1);
    ufcr = set_bit_field(ufcr, UFCR_RFRT, 1);
    if (config->tx_use_dma || config->rx_use_dma) {
        ufcr = set_bit_field(ufcr, UFCR_DME, 1);// 开启 DMA 模式
        ufcr = set_bit_field(ufcr, UFCR_RDTR, 0);// fifo 1 字节触发
    } else
        ufcr = set_bit_field(ufcr, UFCR_RDTR, 2);

    uart_write_reg(id, UFCR, ufcr);
    /* 如果fifo中有数据,清掉 */
    if (uart_read_reg(id, UTCR)) {
        uart_set_bit(id, UFCR, UFCR_TFRT, 1);
        udelay(10);
    }
}

static void uart_disable(int id)
{
    uart_set_bit(id, UFCR, UFCR_UME, 0);
}

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

static void uart_enable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 1);
}

static void uart_disable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 0);
}

static void uart_enable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    uier = set_bit_field(uier, UIER_RTOIE, 1);
    uier = set_bit_field(uier, UIER_RLSIE, 1);
    uier = set_bit_field(uier, UIER_RDRIE, 1);
    uart_write_reg(id, UIER, uier);
}

static void uart_disable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    uier = set_bit_field(uier, UIER_RTOIE, 0);
    uier = set_bit_field(uier, UIER_RLSIE, 0);
    uier = set_bit_field(uier, UIER_RDRIE, 0);
    uart_write_reg(id, UIER, uier);
}

static void write_uart_data_irq(struct uart_data *uart, int enable_irq)
{
    struct list_head *list = &uart->send_list;
    int id = uart - uartdata;
    struct uart_message *msg = list_first_entry(list, struct uart_message, link);
    char *s = (char *)msg->buf + msg->txrx_len;

    int len = msg->len - msg->txrx_len;
    len = uart_write_fifo(id, s, len);

    msg->txrx_len += len;

    if (msg->txrx_len == msg->len) {
        list_del(&msg->link);
        uart_disable_tx_irq(id);

        unsigned int len = uart_read_reg(id, UTCR) + 1;
        unsigned int time = (len*10*1000*1000)/uart->baud_rate;

        uart->tx_last_msg = msg;
        hrtimer_start(&uart->tx_timer, systick_usec_to_count(time));
    } else {
        if (enable_irq)
            uart_enable_tx_irq(id);
    }
}

static void uart_tx_poll_timer_func(struct hrtimer *timer)
{
    struct uart_data *uart = container_of(timer, struct uart_data, tx_timer);
    int id = uart - uartdata;
    struct uart_message *msg = uart->tx_last_msg;

    int ulsr = uart_read_reg(id, ULSR);
    if (!get_bit_field(ulsr, ULSR_TEMP))
        printf("uart: error: ulsr: %x utcr:%d\n", ulsr, uart_read_reg(id, UTCR));

    msg->is_done = 1;
    if (msg->cb)
        msg->cb((void *)msg);

    uart->tx_last_msg = NULL;

    if (!list_empty(&uart->send_list))
        write_uart_data_irq(uart, 1);
}

volatile unsigned char ch_;
static void uart_irq_handler(int irq, void *data)
{
    struct uart_data *uart = data;
    int id = uart - uartdata;
    int ret, len;
    int iir = uart_read_reg(id, UIIR);
    int int_type = get_bit_field(iir, UIIR_INID);

    if (get_bit_field(iir, UIIR_INPEND))
        return;

    struct uart_message *msg;

    switch (int_type)
    {
    case INT_tx_request: {
        write_uart_data_irq(&uartdata[id], 0);
        break;
    }

    case INT_rx_line_status:

        while (uart_get_bit(id, ULSR, ULSR_FIFOE)) {
            // 读接收 fifo，直到没有 fifo error
            ch_ = uart_read_reg(id, URBR);
        }

        while (uart_get_bit(id, ULSR, ULSR_DRY)) {
            // 这种状态是 fifo 出错了，不能体现fifo len，
            // 但 fifo 里面还有数据
            // 读接收 fifo，直到读空
            ch_ = uart_read_reg(id, URBR);
        }

        break;

    case INT_rx_ready:
    case INT_rx_timeout: {

        struct list_head *list = &(uartdata[id].recv_list);

        msg = list_first_entry(list, struct uart_message, link);

        /* rx fifo 可读数据长度
        */
        int n = uart_read_reg(id, URCR);

        char *s = (char *)msg->buf + msg->txrx_len;

        len = msg->len - msg->txrx_len;

        len = len > n ? n : len;

        if (!len)
            break;

        ret = uart_read_fifo(id, s, len);

        msg->txrx_len += ret;

        if (msg->txrx_len == msg->len) {
            list_del(&msg->link);
            if (list_empty(list))
                uart_disable_rx_irq(id);

            msg->is_done = 1;
            if (msg->cb)
                msg->cb((void *)msg);

            break;
        }
    }

        break;

    case INT_moden_status:
        panic("why moden staus: %d\n", id);

    default:
        break;
    }
}

static inline void check_irq_requested(int id)
{
    struct uart_data *uart = &uartdata[id];

    if (!uart->is_irq_inited) {
        request_irq(uartirq[id], 0, uart_irq_handler,
                    uartstr[id], &uartdata[id]);
        uart->is_irq_inited = 1;
    }
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

void uart_send_msg(struct uart_message *msg)
{
    assert(msg);

    int id = msg->uart_id;

    struct uart_data *uart = &uartdata[id];

    assert(uart->is_inited);

    msg->is_done = 0;
    msg->txrx_len = 0;

    check_irq_requested(id);

    struct list_head *list = &(uartdata[id].send_list);
    int is_empty = list_empty(list);

    list_add_tail(&msg->link, list);

    /* 开启tx fifo 为空的中断
     */
    if (is_empty && !uart->tx_last_msg)
        write_uart_data_irq(&uartdata[id], 1);
}

int uart_receive_msg(struct uart_message *msg)
{
    assert(msg);

    int id = msg->uart_id;

    struct uart_data *uart = &uartdata[id];

    assert(uart->is_inited);

    msg->is_done = 0;
    msg->txrx_len = 0;

    check_irq_requested(id);

    struct list_head *list = &(uartdata[id].recv_list);

    list_add_tail(&msg->link, list);

    uart_enable_rx_irq(id);

    return 0;
}

#ifdef APP_libmcu_x2600_dma
int uart_msg_is_done(struct uart_message *msg)
{
    return msg->is_done;
}

void uart_send_dma_msg(struct uart_message *msg)
{
    assert(msg);

    int id = msg->uart_id;

    struct uart_data *uart = &uartdata[id];

    assert(uart->is_inited && uart->tx_use_dma);

    msg->is_done = 0;

    uart_set_bit(id, URTDCR, URTDCR_RTD_T, 1);

    list_add_tail(&msg->link, &uart->txdma_list);
    dma_start(uart->txdma, (void *)msg->buf, (void *)(iobase[id] + UTHR), msg->len);
}

static void uart_txdma_cb(void *data)
{
    struct uart_data *uart = (struct uart_data *)data;
    int id = uart - uartdata;
    struct list_head *list = &uart->txdma_list;
    struct uart_message *msg = list_first_entry(list, struct uart_message, link);

    list_del(&msg->link);
    msg->is_done = 1;
    if (msg->cb)
        msg->cb((void *)msg);

    if (list_empty(list)) {
        uart_set_bit(id, URTDCR, URTDCR_RTD_T, 0);
        return;
    }

    struct uart_message *msg_new = list_first_entry(list, struct uart_message, link);
    dma_start(uart->txdma, (void *)msg_new->buf, (void *)(iobase[id] + UTHR), msg_new->len);
}

int uart_receive_dma(int id, char *buf, int len)
{
    int n, n0;
    struct uart_data *uart = &uartdata[id];
    int dma_buf_size = uart->rxdma_size;
    unsigned long start_addr = uart->rxdma_buf;
    unsigned long end_addr = start_addr + dma_buf_size;
    unsigned int size = len;

    unsigned long dst_now_addr = dma_read_dst_addr(uart->rxdma);
    while (size) {
        if (dst_now_addr == uart->dst_old_addr)
            break;

        if (dst_now_addr > uart->dst_old_addr) {
            n = dst_now_addr - uart->dst_old_addr;
            if (n > size)
                n = size;

            memcpy(buf, (void *)uart->dst_old_addr, n);
        } else {
            n = dma_buf_size - (uart->dst_old_addr - dst_now_addr);
            n0 = end_addr - uart->dst_old_addr;
            if (n0 > size) {
                n = size;
                memcpy(buf, (void *)uart->dst_old_addr, n);
            } else {
                if (n > size)
                    n = size;

                memcpy(buf, (void *)uart->dst_old_addr, n0);
                memcpy(buf + n0, (void *)start_addr, n - n0);
            }
        }
        uart->dst_old_addr = start_addr + (uart->dst_old_addr - start_addr + n) % dma_buf_size;
        buf += n;
        size -= n;
    }

    return len - size;
}

int uart_receive_dma_get_size(int id)
{
    struct uart_data *uart = &uartdata[id];
    unsigned long dst_now_addr = dma_read_dst_addr(uart->rxdma);

    return (uart->rxdma_size + dst_now_addr - uart->dst_old_addr) % uart->rxdma_size;
}
#endif

void uart_stop(struct uart_config *config)
{
    unsigned char id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(id < UART_NUMS);
    assert(uart->is_inited);

#ifdef APP_libmcu_x2600_dma
    if (uart->tx_use_dma) {
        dma_stop(uart->txdma);
        dma_release(uart->txdma);
        uart->tx_use_dma = 0;
    }

    if (uart->rx_use_dma) {
        dma_stop(uart->rxdma);
        dma_release(uart->rxdma);
        uart->rx_use_dma = 0;
    }
#endif

    uart_disable(id);
    uart_clk_enable(id, 0);

    uart->is_inited = 0;
}

void uart_start(struct uart_config *config)
{
    unsigned char id = config->uart_id;

    struct uart_data *uart = &uartdata[id];

    assert(id < UART_NUMS);
    assert(!uart->is_inited);

    if (!config->init_flag) {
        uart_clk_enable(id, 1);

        uart_init_config(config);

        uart_enable_fifo_mode(config);

        if (!uart->is_gpio_inited)
            uart_gpio_set_func(id);
    }

    INIT_LIST_HEAD(&uart->send_list);
    INIT_LIST_HEAD(&uart->recv_list);

    uart->baud_rate = config->baud_rate;
    hrtimer_init(&uart->tx_timer, uart_tx_poll_timer_func);
    uart->is_gpio_inited = 1;
    uart->is_inited = 1;

#ifdef APP_libmcu_x2600_dma
    if (config->tx_use_dma) {
        uart->tx_use_dma = config->tx_use_dma;
        INIT_LIST_HEAD(&uart->txdma_list);
        uart->txdma = dma_request(uart_tdma[id], uart_txdma_cb, uart, DMA_bus_8bit, 1);
    }

    if (config->rx_use_dma) {
        uart->rx_use_dma = config->rx_use_dma;
        uart->rxdma_size = config->rxdma_size;
        assert(uart->rxdma_size >= 256);
        uart->rxdma_buf = (unsigned long)uncache_mem_alloc(uart->rxdma_size, 32);
        assert(uart->rxdma_buf);
        uart->rxdma = dma_request(uart_rdma[id], NULL, NULL, DMA_bus_8bit, 1);
        dma_start_cyclic(uart->rxdma, (void *)(iobase[id] + URBR), (void *)uart->rxdma_buf, uart->rxdma_size, 1);
        uart->dst_old_addr = dma_read_dst_addr(uart->rxdma);
    }
#endif
}

void uart_init(void){}
