#include <stdio.h>
#include <cpu/host_cpu.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <ring_mem.h>

#include <asm/riscv_csr.h>
#include <delay.h>
#include <cpu/uncache_mem.h>
#include <cpu/lep_ccu.h>

extern struct ring_mem *data_from_host;
extern struct ring_mem *data_to_host;

int host_cpu_read(void *buf, unsigned int size)
{
    int len = 0;

    while (size) {
        int ret = ring_mem_read(data_from_host, buf, size);
        if (!ret)
            break;
        len += ret;
        size -= ret;
        buf += ret;
    }

    return len;
}

int host_cpu_write(void *buf, unsigned int size)
{
    int len = 0;

    while (size) {
        int ret = ring_mem_write(data_to_host, buf, size);
        if (!ret)
            break;
        len += ret;
        size -= ret;
        buf += ret;
    }

    return len;
}

int host_cpu_has_readable_data(void)
{
    return ring_mem_readable_size(data_from_host);
}

#ifdef APP_libmcu_driver_irq
static void (*m_irq_cb)(void);

int mcu_test_host_busy(void)
{
    return 0;
}

void host_cpu_set_irq_callback(void (*cb)(void))
{
    m_irq_cb = cb;
}

void mcu_notify_host(int len)
{
    writel(len, (unsigned long)CCU_TO_HOST);
}


static void irq_soft_func(int irq, void *data)
{
    if (m_irq_cb)
        m_irq_cb();

    writel(0, (unsigned long)CCU_FROM_HOST);
}

void host_cpu_irq_init(void)
{
    void *mem = uncache_mem_alloc(APP_libmcu_x2600_data_from_host_size, 0);
    if (mem)
        ring_mem_init(data_from_host, mem, APP_libmcu_x2600_data_from_host_size);

    void *out_buffer = uncache_mem_alloc(APP_libmcu_x2600_data_to_host_size, 0);
    if (out_buffer)
        ring_mem_init(data_to_host, out_buffer, APP_libmcu_x2600_data_to_host_size);

    request_irq(IRQ_V_HOST_NOTIFY, 0, irq_soft_func, "soft", NULL);
}
#endif
