#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <common.h>
#include <assert.h>
#include <driver/clk.h>
#include <driver/dma.h>
#include <soc/base.h>
#include <driver/irq.h>

#include "tpc_hal.h"
#include <driver/systick.h>


struct tpc_dma {
    struct dma *dma;

    volatile int is_busy;
    volatile int is_loop;

    void *buffer;
    int buffer_size;

    volatile int data_size;
    void *dev_fifo;
    volatile unsigned int dma_pos;
    volatile unsigned int write_pos;
    volatile unsigned int stop_pos;
    int unit_size;

    int tpc_unit_size;
};

void dma_stop_after_current_transfer(struct dma *dma, void *mem);

static struct tpc_dma tpc_dma[4];

static unsigned int sub_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + size - delta) % size;
}

static unsigned int add_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + delta) % size;
}

static void stop_dma(struct tpc_dma *t_dma)
{
    dma_stop(t_dma->dma);

    t_dma->is_busy = 0;
    t_dma->stop_pos = 0;
    t_dma->write_pos = 0;
    t_dma->dma_pos = 0;
    t_dma->data_size = 0;
}


static void tpc_dma_cb(void *data)
{
    struct tpc_dma *t_dma = data;

    stop_dma(t_dma);
}


static unsigned int tpc_get_writable_size(struct tpc_dma *t_dma)
{
    unsigned int buffer_size = t_dma->buffer_size;
    unsigned int pos = dma_read_src_addr(t_dma->dma) - (unsigned int)t_dma->buffer;
    unsigned int size = sub_pos(buffer_size, pos, t_dma->dma_pos);

    t_dma->dma_pos = pos;

    if (size <= t_dma->buffer_size)
        t_dma->data_size -= size;
    else
        t_dma->data_size = 0;


    if (t_dma->data_size == 0) {
        unsigned int align_pos = ALIGN(pos, t_dma->unit_size);
        t_dma->write_pos = add_pos(buffer_size, align_pos, t_dma->unit_size);
    }

    unsigned int max_size = sub_pos(buffer_size, t_dma->dma_pos, t_dma->write_pos);

    return max_size;
}


static void tpc_do_write_buffer(struct tpc_dma *t_dma, void *mem, unsigned int bytes)
{
    unsigned int buffer_size = t_dma->buffer_size;
    unsigned int pos = t_dma->write_pos;
    void *dst = t_dma->buffer;

    if (pos + bytes <= buffer_size) {
        memcpy(dst+pos, mem, bytes);
    } else {
        unsigned int size1 = buffer_size - pos;
        memcpy(dst+pos, mem, size1);
        memcpy(dst, mem+size1, bytes-size1);
    }

    t_dma->write_pos = add_pos(buffer_size, pos, bytes);
    t_dma->data_size += bytes;
}


void tpc_hal_dma_start(struct tpc_dma *t_dma)
{
    if (!t_dma->is_loop)
        dma_start(t_dma->dma, t_dma->buffer, t_dma->dev_fifo, t_dma->buffer_size);
    else
        dma_start_cyclic(t_dma->dma, t_dma->buffer, t_dma->dev_fifo, t_dma->buffer_size, t_dma->buffer_size / t_dma->tpc_unit_size);

    t_dma->is_busy = 1;
}


int tpc_hal_dma_write_data(struct tpc_dma *t_dma, void *mem, int mem_size)
{
    int bytes = mem_size > t_dma->buffer_size ? t_dma->buffer_size : mem_size;
    unsigned int n;

    if (!t_dma->is_busy) {
        n = t_dma->buffer_size - t_dma->write_pos;
        if (n > bytes)
            n = bytes;

        if (n <= 0)
            return 0;

        t_dma->stop_pos = add_pos(t_dma->buffer_size, t_dma->write_pos, n - t_dma->tpc_unit_size);

        tpc_do_write_buffer(t_dma, mem, n);

        return n;
    }

    n = tpc_get_writable_size(t_dma);

    if (n > bytes)
        n = bytes;

    n = (n / t_dma->tpc_unit_size) * t_dma->tpc_unit_size;

    if (!n)
        return 0;

    t_dma->stop_pos = add_pos(t_dma->buffer_size, t_dma->write_pos, n - t_dma->tpc_unit_size);

    tpc_do_write_buffer(t_dma, mem, n);

    return n;
}


void tpc_hal_dma_init(struct tpc_dma *t_dma, int is_loop, int size, int tpc_unit_size)
{
    if (t_dma->is_busy)
        return;

    t_dma->is_loop = is_loop;
    t_dma->tpc_unit_size = tpc_unit_size;

    t_dma->buffer = malloc(size);
    t_dma->buffer_size = size;
}

void tpc_hal_dma_deinit(struct tpc_dma *t_dma)
{
    if (t_dma->buffer)
        free(t_dma->buffer);

    t_dma->buffer = NULL;
    t_dma->buffer_size = 0;
}

struct tpc_dma *tpc_hal_dma_request(enum DMA_request_type type, void *dst)
{
    int id = type - DMA_RQ_TPC_SHIFT_TX;
    if (id > 3)
        panic("tpc: %d is not tpc dma\n", type);

    struct tpc_dma *t_dma = &tpc_dma[id];
    t_dma->unit_size = 4;

    t_dma->dma = dma_request(type, tpc_dma_cb, (void *)t_dma, DMA_bus_32bit, t_dma->unit_size);

    t_dma->dev_fifo = dst;

    return t_dma;
}


void tpc_hal_dma_stop(struct tpc_dma *t_dma, int is_quick)
{
    if (is_quick) {
        stop_dma(t_dma);
        return;
    }

    if (t_dma->is_loop)
        dma_stop_after_current_transfer(t_dma->dma, (void *)(t_dma->stop_pos + t_dma->buffer));

}

