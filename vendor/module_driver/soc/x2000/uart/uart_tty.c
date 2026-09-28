#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <linux/console.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include "uart.c"

static struct workqueue_struct *m_queue;

struct m_uart_port {
    struct uart_port port;
    struct uart_config config;
    struct delayed_work txwork;
    struct delayed_work rxwork;
    int is_stop;
    int is_start;
    int is_txdma_en;
    int is_txdma_use;
    int txdma_cnt;
    int txdma_size;
    int rxdma_size;
};

struct m_uart_port my_uart_port[UART_NUMS];

module_param_named(uart0_txdma_en, my_uart_port[0].is_txdma_en, int, 0644);
module_param_named(uart0_txdma_size, my_uart_port[0].txdma_size, int, 0644);
module_param_named(uart0_rxdma_size, my_uart_port[0].rxdma_size, int, 0644);

module_param_named(uart1_txdma_en, my_uart_port[1].is_txdma_en, int, 0644);
module_param_named(uart1_txdma_size, my_uart_port[1].txdma_size, int, 0644);
module_param_named(uart1_rxdma_size, my_uart_port[1].rxdma_size, int, 0644);

module_param_named(uart2_txdma_en, my_uart_port[2].is_txdma_en, int, 0644);
module_param_named(uart2_txdma_size, my_uart_port[2].txdma_size, int, 0644);
module_param_named(uart2_rxdma_size, my_uart_port[2].rxdma_size, int, 0644);

module_param_named(uart3_txdma_en, my_uart_port[3].is_txdma_en, int, 0644);
module_param_named(uart3_txdma_size, my_uart_port[3].txdma_size, int, 0644);
module_param_named(uart3_rxdma_size, my_uart_port[3].rxdma_size, int, 0644);

module_param_named(uart4_txdma_en, my_uart_port[4].is_txdma_en, int, 0644);
module_param_named(uart4_txdma_size, my_uart_port[4].txdma_size, int, 0644);
module_param_named(uart4_rxdma_size, my_uart_port[4].rxdma_size, int, 0644);

module_param_named(uart5_txdma_en, my_uart_port[5].is_txdma_en, int, 0644);
module_param_named(uart5_txdma_size, my_uart_port[5].txdma_size, int, 0644);
module_param_named(uart5_rxdma_size, my_uart_port[5].rxdma_size, int, 0644);

module_param_named(uart6_txdma_en, my_uart_port[6].is_txdma_en, int, 0644);
module_param_named(uart6_txdma_size, my_uart_port[6].txdma_size, int, 0644);
module_param_named(uart6_rxdma_size, my_uart_port[6].rxdma_size, int, 0644);

module_param_named(uart7_txdma_en, my_uart_port[7].is_txdma_en, int, 0644);
module_param_named(uart7_txdma_size, my_uart_port[7].txdma_size, int, 0644);
module_param_named(uart7_rxdma_size, my_uart_port[7].rxdma_size, int, 0644);

module_param_named(uart8_txdma_en, my_uart_port[8].is_txdma_en, int, 0644);
module_param_named(uart8_txdma_size, my_uart_port[8].txdma_size, int, 0644);
module_param_named(uart8_rxdma_size, my_uart_port[8].rxdma_size, int, 0644);

module_param_named(uart9_txdma_en, my_uart_port[9].is_txdma_en, int, 0644);
module_param_named(uart9_txdma_size, my_uart_port[9].txdma_size, int, 0644);
module_param_named(uart9_rxdma_size, my_uart_port[9].rxdma_size, int, 0644);

static struct uart_driver m_uart_drv = {
    .owner		= THIS_MODULE,
    .dev_name	= "ttySC",
    .nr		= UART_NUMS,
};

static void rx_work_func(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct m_uart_port *p = container_of(dwork, struct m_uart_port, rxwork);
    struct uart_port *port = &p->port;
    int has_ch = 0;
    unsigned long flags;

    while (1) {
        int size = soc_uart_read_fifosize(&p->config, NULL);
        if (!size)
            break;
        char c;
        soc_uart_receive(&p->config, &c, 1);
        spin_lock_irqsave(&port->lock, flags);

        uart_insert_char(port, 0, (1 << 1), c,
                    TTY_NORMAL);

        spin_unlock_irqrestore(&port->lock, flags);

        has_ch = 1;
    }

    if (has_ch)
        tty_flip_buffer_push(&port->state->port);

    if (!p->is_stop)
        queue_delayed_work(m_queue, &p->rxwork, msecs_to_jiffies(10));
}

static void dma_tx_callback(void *data)
{
    struct m_uart_port *p = data;
    struct uart_port *port = &p->port;
    struct circ_buf *xmit = &port->state->xmit;

    soc_uart_dma_tx_finish(&p->config);
    p->is_txdma_use = 0;
    p->txdma_cnt = 0;

    if (uart_tx_stopped(port))
        return;

    if (!uart_circ_empty(xmit)) {
        queue_delayed_work(m_queue, &p->txwork, 0);
        return;
    }
}

static int tx_do_copy_data(struct circ_buf *xmit, unsigned char *buf, int size, int max)
{
    if (size > max)
        size = max;

    memcpy(buf, &xmit->buf[xmit->tail], size);
    xmit->tail = (xmit->tail + size) & (UART_XMIT_SIZE - 1);

    return size;
}

static int tx_copy_data(struct m_uart_port *p)
{
    struct circ_buf *xmit = &p->port.state->xmit;
    struct uart_data *uart = &uartdata[p->config.uart_id];
    if (uart->txbuf_size == p->txdma_cnt || uart_circ_empty(xmit))
        return p->txdma_cnt;

    int n;
    unsigned char *buf = uart->txdma_buf + p->txdma_cnt;
    int left = uart->txbuf_size - p->txdma_cnt;

    if (xmit->tail > xmit->head) {
        n = CIRC_CNT(UART_XMIT_SIZE, xmit->tail, UART_XMIT_SIZE);
        n = tx_do_copy_data(xmit, buf, n, left);
        p->txdma_cnt += n;
        buf += n;
        left -= n;
    }

    if (left) {
        n = uart_circ_chars_pending(xmit);
        n = tx_do_copy_data(xmit, buf, n, left);
        p->txdma_cnt += n;
        buf += n;
        left -= n;
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(&p->port);

    return p->txdma_cnt;
}

static void tx_work_func(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct m_uart_port *p = container_of(dwork, struct m_uart_port, txwork);
    struct uart_data *uart = &uartdata[p->config.uart_id];
    unsigned long flags;

    if (p->is_txdma_use)
        return;

    spin_lock_irqsave(&p->port.lock, flags);
    unsigned int size = tx_copy_data(p);
    spin_unlock_irqrestore(&p->port.lock, flags);

    if (!size)
        return;

    p->is_txdma_use = 1;
    p->config.txdma_cb = dma_tx_callback;
    p->config.txdma_cb_param = p;

    soc_uart_send_dma_async(&p->config, uart->txdma_buf, size);
}

static void m_uart_start_tx(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;
    struct circ_buf *xmit = &port->state->xmit;
    unsigned int to_send;

    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
        return;

    if (p->config.tx_dma_mode) {
        if (p->is_txdma_use)
            return;

        unsigned int size = tx_copy_data(p);
        if (size < TXDMA_MIN_SIZE)
            queue_delayed_work(m_queue, &p->txwork, msecs_to_jiffies(10));
        else
            queue_delayed_work(m_queue, &p->txwork, 0);

        return;
    }

    to_send = uart_circ_chars_pending(xmit);

    while (!uart_circ_empty(xmit)) {
        int i;
        for (i = 0; i < to_send; ++i) {
            int ret = uart_write_fifo(p->config.uart_id, &xmit->buf[xmit->tail], 1);
            if (ret != 1)
                break;
            // uart_send_poll_nowait(&p->config, &xmit->buf[xmit->tail], 1);
            xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        }
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);
}

static void m_uart_stop_tx(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;
    int count = 1000*1000*1000;

    while (!uart_get_tx_fifo_empty(&p->config) && count--);
}

static int m_uart_startup(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;
    struct uart_config *config = &p->config;

    config->uart_id = p - my_uart_port;
    config->data_bits = 8;
    config->stop_bits = 1;
    config->loop_mode = 0;
    config->tx_poll_mode = 0;
    config->rx_poll_mode = 0;
    config->tx_dma_mode = p->is_txdma_en;
    config->tx_dma_bufsize = p->txdma_size;
    config->rx_dma_mode = 1;
    config->rx_dma_bufsize = p->rxdma_size;
    config->parity = UART_PARITY_NONE;
    config->follow_contrl = UART_FC_NONE;
    config->baud_rate = 115200;

    p->is_stop = 0;
    p->is_start = 0;
    p->is_txdma_use = 0;
    p->txdma_cnt = 0;
    INIT_DELAYED_WORK(&p->rxwork, rx_work_func);
    INIT_DELAYED_WORK(&p->txwork, tx_work_func);

    return 0;
}

static void m_uart_shutdown(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;
    p->is_stop = 1;
    cancel_delayed_work(&p->rxwork);

    if (!(p->config.tx_dma_mode && p->txdma_cnt))
        cancel_delayed_work(&p->txwork);

    while (p->txdma_cnt) usleep_range(500, 500);

	p->is_start = 0;
    soc_uart_stop(&p->config);
}

static void m_uart_set_termios(struct uart_port *port,
                    struct ktermios *termios,
                    struct ktermios *old)
{
    struct m_uart_port *p = (void *)port;
    struct uart_config *config = &p->config;

    if (p->is_start) {
        cancel_delayed_work_sync(&p->rxwork);
        soc_uart_stop(config);
    }

    p->is_start = 1;

	switch (termios->c_cflag & CSIZE) {
    case CS5:
        config->data_bits = 5; break;
    case CS6:
        config->data_bits = 6; break;
    case CS7:
        config->data_bits = 7; break;
    case CS8:
    default:
        config->data_bits = 8; break;
	}

    config->stop_bits = 1;
    config->parity = UART_PARITY_NONE;
    config->follow_contrl = UART_FC_NONE;

    if (termios->c_cflag & CSTOPB)
        config->stop_bits = 2;

    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            config->parity = UART_PARITY_ODD;
        else
            config->parity = UART_PARITY_EVEN;
    }

    if (UART_ENABLE_MS(port, termios->c_cflag))
        config->follow_contrl = UART_FC_CTS_RTS;

    config->baud_rate = uart_get_baud_rate(port, termios, old, 0, 4000000);

    uart_update_timeout(port, termios->c_cflag, config->baud_rate);

    // printk(KERN_ERR "uart_id: %d\n", config->uart_id);
    // printk(KERN_ERR "data_bits: %d\n", config->data_bits);
    // printk(KERN_ERR "stop_bits: %d\n", config->stop_bits);
    // printk(KERN_ERR "loop_mode: %d\n", config->loop_mode);
    // printk(KERN_ERR "rx_poll_mode: %d\n", config->rx_poll_mode);
    // printk(KERN_ERR "rx_dma_mode: %d\n", config->rx_dma_mode);
    // printk(KERN_ERR "tx_poll_mode: %d\n", config->tx_poll_mode);
    // printk(KERN_ERR "parity: %d\n", config->parity);
    // printk(KERN_ERR "follow_contrl: %d\n", config->follow_contrl);
    // printk(KERN_ERR "baud_rate: %d\n", config->baud_rate);

    soc_uart_start(&p->config);

    int enable_rts = port->mctrl & TIOCM_RTS;
    uart_set_rts(&p->config, enable_rts);

    queue_delayed_work(m_queue, &p->rxwork, msecs_to_jiffies(10));
}

static const char *m_uart_type(struct uart_port *port)
{
    return "ttySC";
}

static void m_uart_config_port(struct uart_port *port, int flags)
{
    if (flags & UART_CONFIG_TYPE)
        port->type = PORT_8250;
}

static void m_uart_null_void(struct uart_port *port)
{
    /* Do nothing */
}

static unsigned int m_uart_get_mctrl(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;

    return uart_get_cts(&p->config) ? TIOCM_CTS : 0;
}

static void m_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct m_uart_port *p = (void *)port;

    int enable_rts = mctrl & TIOCM_RTS;
    uart_set_rts(&p->config, enable_rts);
}

static void m_break_ctl(struct uart_port *port, int state)
{
}

static int m_request_port(struct uart_port *port)
{
    return 0;
}

static unsigned int m_tx_empty(struct uart_port *port)
{
    struct m_uart_port *p = (void *)port;

    return uart_get_tx_fifo_empty(&p->config) ? TIOCSER_TEMT : 0;
}


static const struct uart_ops m_uart_ops = {
    .tx_empty   = m_tx_empty,
    .set_mctrl  = m_uart_set_mctrl,
    .get_mctrl  = m_uart_get_mctrl,
    .stop_tx    = m_uart_stop_tx,
    .start_tx   = m_uart_start_tx,
    .stop_rx    = m_uart_null_void,
    .break_ctl  = m_break_ctl,
    .startup    = m_uart_startup,
    .shutdown   = m_uart_shutdown,
    .set_termios    = m_uart_set_termios,
    .type       = m_uart_type,
    .config_port    = m_uart_config_port,
    .release_port   = m_uart_null_void,
    .request_port   = m_request_port,
};

static int m_uart_probe(struct platform_device *pdev)
{
    struct m_uart_port *p = &my_uart_port[pdev->id];

    p->port.dev = &pdev->dev;
    p->port.type = PORT_8250;
    p->port.ops = &m_uart_ops;
    p->port.line = pdev->id;
    p->port.fifosize = 64;
    p->port.membase = (void *)1;
    spin_lock_init(&p->port.lock);

    platform_set_drvdata(pdev, &p->port);

    return uart_add_one_port(&m_uart_drv, &p->port);
}

static int m_uart_remove(struct platform_device *pdev)
{
    printk(KERN_ERR "m_uart_remove\n");
    return 0;
}

static void m_release(struct device *dev) {}

static struct platform_device m_uart_platform_dev[UART_NUMS];

static struct platform_driver m_uart_platform_drv = {
    .driver = {
        .name = "ingenic_m_uart",
    },
    .probe	= m_uart_probe,
    .remove	= m_uart_remove,
};

static int __init gpio_m_uart_init(void)
{
    int ret;

    m_queue = create_singlethread_workqueue("uart-tty");

    ret = uart_register_driver(&m_uart_drv);
    if (ret) {
        pr_err("Registering UART driver failed\n");
        return ret;
    }

    int i;
    for (i = 0; i < ARRAY_SIZE(m_uart_platform_dev); i++) {
        if (!uartgpio[i].is_enable)
            continue;
        struct platform_device *pdev = &m_uart_platform_dev[i];
        pdev->id = i;
        pdev->name = "ingenic_m_uart";
        pdev->dev.release = m_release;
        ret = platform_device_register(pdev);
        if (ret) {
            pr_err("uart_tty: failed to register platform dev: %d\n", i);
            return ret;
        }
    }

    ret = platform_driver_register(&m_uart_platform_drv);
    if (ret) {
        pr_err("uart_tty: failed to register platform driver\n");
        return ret;
    }

    return ret;
}
module_init(gpio_m_uart_init);

static void __exit gpio_m_uart_exit(void)
{
    uart_unregister_driver(&m_uart_drv);
}
module_exit(gpio_m_uart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("INGENIC SPI TO UART serial driver");