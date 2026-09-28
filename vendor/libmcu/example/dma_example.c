#include <stdio.h>
#include <string.h>

#include <cpu/uncache_mem.h>
#include <driver/dma.h>

#include <delay.h>
#include <driver/systick.h>

unsigned char *test_src;
unsigned char *test_dst;

void dma_test_cb(void *data)
{
    printf("dma_transfer end!!!\n");
}

void dma_test(void)
{
    int base = 0, total_size = 1024, times = 10;
    test_src = uncache_mem_alloc(total_size, 32);
    test_dst = uncache_mem_alloc(total_size, 32);

    while (times--) {
        printf("===========================%d\n", times);
        memset(test_src, base, total_size);
        memset(test_dst, 99, total_size);
        int i;
        for (i = 0; i < 1024; i++)
            test_src[i] = i + base;

        base++;

        struct dma *dma;
        dma = dma_request(DMA_RQ_MEM, dma_test_cb, NULL, DMA_bus_32bit, 8);

        /* 循环的 dma 传输 */
        // dma_start_cyclic(dma, test_src, test_dst, total_size, 2);

        /* 多段dma 链式传输,用处不大,除非传输大小超过16M */
        dma_start_linked(dma, test_src, test_dst, total_size, 2);

        /* 普通的一段dma 传输 */
        // dma_start(dma, test_src, test_dst, total_size);

        udelay(100);

        dma_stop(dma);
        dma_release(dma);

        if (memcmp(test_src, test_dst, total_size))
            printf("dma test data err!--------------------\n");
        else
            printf("dma test data right!\n");
        printf("===========================%d\n", times);
        mdelay(1000);
    }
}
