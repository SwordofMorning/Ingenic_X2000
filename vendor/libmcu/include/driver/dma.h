#ifndef _DMA_H_
#define _DMA_H_

#include <soc/dma.h>

struct dma;

unsigned long dma_read_src_addr(struct dma *dma);

unsigned long dma_read_dst_addr(struct dma *dma);

struct dma *dma_request(
        enum DMA_request_type rq, void (*cb)(void *data), void *data,
        enum DMA_bus_width bus_width, unsigned int unit_size);

void dma_release(struct dma *dma);

void dma_start(struct dma *dma,
        void *src, void *dst, unsigned int len);

void dma_start_linked(struct dma *dma,
        void *src, void *dst, unsigned int len, unsigned int count);

void dma_start_cyclic(struct dma *dma,
        void *src, void *dst, unsigned int len, unsigned int count);

void dma_stop(struct dma *dma);

void dma_init(void);

void dma_queue_desc_init(struct dma *dma,
        void *dst, unsigned int count, int enable_desc_irq);

void dma_queue_start(struct dma *dma);

int dma_queue_get_available_size(struct dma *dma);

void dma_queue_init(struct dma *dma);

int dma_queue_add(struct dma *dma, void *buf, unsigned int len);

void dma_queue_deinit(struct dma *dma);

#endif
