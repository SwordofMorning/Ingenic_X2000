#include <stdio.h>
#include <string.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <driver/irq.h>
#include <cpu/host_cpu.h>
#include <driver/gpio.h>
#include <common.h>
#include <driver/nemc.h>
#include <stdlib.h>
#include <delay.h>
#include <driver/systick.h>
#include <cpu/uncache_mem.h>
#include <driver/dma.h>

enum nemc_operation {
    READ_DATA,
    WRITE_DATA,
};

enum data_mode {
    ADDR_MODE,
    DATA_MODE,
};

struct data_header {
    enum nemc_operation operation;
    enum data_mode mode;
    unsigned long src_addr;
    unsigned long dst_addr;
    unsigned long data_size;
};

#define DATA_BUF_SIZE (1024 * 1)

static volatile int is_dma_finish = 0;
static volatile int is_data_comming = 0;
static struct dma *nemc_dma;

#ifdef APP_libmcu_driver_irq
static void host_irq_cb(void)
{
    is_data_comming = 1;
}
#endif

static int mcu_write_host_nemc_data(void *buf, int size)
{
    int len = size;
    int ret;

    while(mcu_test_host_busy());

    while (len) {
        ret = host_cpu_write(buf, len);
        len -= ret;
        if (!ret)
            break;
    }

    mcu_notify_host(size);

    return len;
}

static void mcu_nemc_is_ready(void)
{
    char buf[] = "mcu_nemc_ready";
    int len = strlen(buf) + 1;
    mcu_write_host_nemc_data(buf, len);
}

static void mcu_nemc_dma_is_finish(void)
{
    char buf[] = "mcu_nemc_finish";
    int len = strlen(buf) + 1;
    mcu_write_host_nemc_data(buf, len);
}

static int mcu_read_host_nemc_data(void *buf, int size)
{
    int len = 0;
    int ret;

    while (!is_data_comming);
    is_data_comming = 0;

    while (1) {
        ret = host_cpu_read(buf+len, size-len);
        if (!ret)
            break;

        len += ret;
    }

    return len;
}

static void nemc_dma_cb(void *data)
{
    is_dma_finish = 1;
}

static void nemc_free_dma(void)
{
    dma_stop(nemc_dma);
    dma_release(nemc_dma);
}

static int nemc_start_dma(void *src, unsigned int len, void *dst, void (*cb)(void *data), void *data, int data_width)
{
    int dma_bus_width = DMA_bus_8bit;

    if (data_width == 16) {
        data_width = DMA_bus_16bit;
    }

    is_dma_finish = 0;
    nemc_dma = dma_request(DMA_RQ_MEM, cb, data, dma_bus_width, data_width);
    dma_start(nemc_dma, src, dst, len);
    return 0;
}

static int valid_nemc_addr(int id, unsigned long nemc_addr)
{
    if ((id == 0) && (nemc_addr > NEMC0_ADDR_END || nemc_addr < NEMC0_ADDR_START))
        return -1;

    if ((id == 1) && (nemc_addr > NEMC1_ADDR_END || nemc_addr < NEMC1_ADDR_START))
        return -1;

    return 0;
}

void nemc_example(void)
{
    printf("%s is called\n", __func__);

#ifdef APP_libmcu_driver_irq
    host_cpu_set_irq_callback(host_irq_cb);
#endif

    struct nemc_timing timing = {
        .tras = 7,
        .taw = 15,
        .twas = 7,
        .tbp = 15,
        .tch = 7,
        .strv = 15,
        .is_double_timing = 0,
    };
    struct nemc_config config = {
        .id = 0,
        .addr_width = 8,
        .data_width = 16,
        .mode = NORMAL_MODE,
        .use_wait_pin = 0,
    };
    nemc_init(config, timing);

    void *uncache_buf = uncache_mem_alloc(DATA_BUF_SIZE, 32);
    void *dma_src, *dma_dst;
    unsigned long nemc_addr;

    while(1) {
        struct data_header header = {0};
        /* 通知大核 准备好接收数据 */
        mcu_nemc_is_ready();
        mcu_read_host_nemc_data(&header, sizeof(header));

        /* 检查 nemc 地址是否合理 */
        if (header.operation == WRITE_DATA)
            nemc_addr = header.dst_addr;
        else
            nemc_addr = header.src_addr;
        if(valid_nemc_addr(config.id, nemc_addr) == -1)
            continue;

        dma_src = (void *)header.src_addr;
        dma_dst = (void *)header.dst_addr;

        /*  如果为传输数据的模式，则先使用 mcu 自己的 buf 存储数据
            如果为传输地址的模式，则直接使用 host 传输过来的地址对应的内存
         */
        if (header.mode == DATA_MODE) {
            if (header.data_size > DATA_BUF_SIZE) {
                printf("The data transferred is larger than uncache buf\n");
            }

            if (header.operation == WRITE_DATA) {
                dma_src = uncache_buf;
                mcu_read_host_nemc_data(uncache_buf, header.data_size);
            } else if (header.operation == READ_DATA)
                dma_dst = uncache_buf;
        }

        nemc_start_dma(dma_src, header.data_size, dma_dst, nemc_dma_cb, NULL, config.data_width);

        while (is_dma_finish != 1)
            udelay(100);

        nemc_free_dma();

        /* 通知大核 数据已经搬运完成 */
        mcu_nemc_dma_is_finish();

        if (header.mode == DATA_MODE && header.operation == READ_DATA) {
            mcu_write_host_nemc_data((void *)dma_dst, header.data_size);
        }

        printf("finis a transfer\n");
    }
}
