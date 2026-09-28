#include <stdio.h>
#include <string.h>
#include <soc/base.h>
#include <bit_field2.h>
#include <cpu/tcsm_section.h>
#include <cpu/uncache_mem.h>
#include <assert.h>

#include <driver/clk.h>
#include <driver/irq.h>
#include <cpu/irqflags.h>
#include <driver/dma.h>
#include <stdlib.h>

#include "dma_regs.h"


struct dma_desc {
    unsigned long dcm; /* dma channel command */
    unsigned long dsa; /* Source Address */
    unsigned long dta; /* Target Address */
    unsigned long dtc; /* Descriptor Offset address, Transfer Counter */
    unsigned long sd;  /* Target Stride Address, Source Stride Address */
    unsigned long drt; /* DMA Request Type */
    unsigned long reserved[2];
};

#define DMA_CHANNELS     32

static __tcsm_data const unsigned long dmabase[] = {
    [0] = PDMA_IOBASE, // pdma
    [1] = PDMA_MCU_IOBASE, // mcu dma
};

#define DMA_ADDR(id, reg)   ((volatile unsigned long *)(dmabase[id] + reg))

static inline void dma_write(int id, unsigned int reg, int val)
{
    *DMA_ADDR(id, reg) = val;
}

static inline unsigned int dma_read(int id, unsigned int reg)
{
    return *DMA_ADDR(id, reg);
}

static inline void dma_set_bits(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field_v(DMA_ADDR(id, reg), start, end, val);
}

static inline unsigned int dma_get_bits(int id, unsigned int reg, int start, int end)
{
    return get_bit_field_v(DMA_ADDR(id, reg), start, end);
}

static inline void *m_dma_alloc_coherent(int size)
{
    void *mem = malloc(size);
    assert(mem);

    return mem;
}

static inline void m_dma_free_coherent(void *mem)
{
    free(mem);
}

static void hal_dma_global_enable(int id)
{
    unsigned long dmac = dma_read(id, DMAC);
    set_bit_field_v(&dmac, DMAC_DMAE, 1);
    set_bit_field_v(&dmac, DMAC_HLT, 0);
    set_bit_field_v(&dmac, DMAC_AR, 0);
    dma_write(id, DMAC, dmac);
}

static void hal_dma_global_disable(int id)
{
    dma_set_bits(id, DMAC, DMAC_DMAE, 0);
}

struct dma {
    int id;
    enum DMA_request_type rq;
    enum DMA_bus_width bus_width;
    unsigned int unit_size;
    void (*cb)(void *data);
    void *data;
    int is_started;
    unsigned int ch;
    struct dma_desc *desc;
    unsigned long desc_count;
    unsigned long src_start;
    unsigned long dst_start;
    unsigned long src_end;
    unsigned long dst_end;

    int q_start;
    int q_end;
    int q_enable_irq;
};

static struct dma *datas[2][DMA_CHANNELS];
static unsigned int is_enabled[2];

static unsigned int is_init[2];

/*
 * 获取DMA通道索引
 *
 * SoC 的32个DMA通道硬件资源大小核共用，
 * 对该资源的申请方式准从以下方式
 * 大核DMA通道申请从最低位bit0开始
 * 小核DMA通道申请从最高位bit31开始
 *
 *
 * Note:
 * 软件无法保证大小核资源申请的互斥,当大核与小核申请通道数重叠时会遇到未知错误,
 * 务必保证大小核DMA通道不冲突
 */
static inline int get_dma_channel(int id)
{
    int i;
    unsigned int flag;

    local_irq_save(flag);

    for (i = DMA_CHANNELS - 1; i >= 0; i--) {
        if (!(is_enabled[id] & (1 << i))) {
            /*
            * 确保dma是使能的
            */
            if (!is_enabled[id] && !is_init[id]) {
                is_init[id] = 1;
                if (id == 0)
                    clk_gate_enable(CLK_GATE_DMAC);
                else
                    clk_gate_enable(CLK_GATE_DMAC1);
                hal_dma_global_enable(id);
            }
            is_enabled[id] |= (1 << i);
            break;
        }
    }

    local_irq_restore(flag);

    return i;
}

static inline void put_dma_channel(int id, unsigned int ch)
{
    if (is_enabled[id])
        dma_write(id, DCS(ch), 0);

    if (is_enabled[id] & (1 << ch)) {
        is_enabled[id] &= ~(1 << ch);
        /*
        * 不动态关闭dma了,比较耗时
        */
        if (!is_enabled[id] && 0) {
            is_init[id] = 0;
            hal_dma_global_disable(id);
            if (id == 0)
                clk_gate_disable(CLK_GATE_DMAC);
            else
                clk_gate_disable(CLK_GATE_DMAC1);
        }
    }
}

static inline int rq_is_mcu_dma(enum DMA_request_type rq)
{
    if ((rq >= DMA_RQ_PCM_TX && rq <= DMA_RQ_I2C3_RX) || (rq >= DMA_RQ_AIC_LOOP_RX && rq <= DMA_RQ_AIC_RX))
        return 0; // pdma
    else
        return 1; // mcu_dma
}

static inline int src_is_mem(enum DMA_request_type rq)
{
    if (rq_is_mcu_dma(rq))
        return (rq != DMA_RQ_SADC_SEQ1_RX) && (rq != DMA_RQ_SADC_SEQ2_RX); // mcu_dma
    else
        return (rq == DMA_RQ_MEM) || !(rq & 0x1); // pdma
}

static inline int dst_is_mem(enum DMA_request_type rq)
{
    if (rq_is_mcu_dma(rq))
        return (rq == DMA_RQ_SADC_SEQ1_RX) || (rq == DMA_RQ_SADC_SEQ2_RX); // mcu_dma
    else
        return (rq == DMA_RQ_MEM) || (rq & 0x1); // pdma
}

static void init_dma_desc(struct dma_desc *desc,
        void *src, void *dst, unsigned int len,
        enum DMA_request_type rq,
        enum DMA_bus_width bus_width, unsigned int unit_size)
{
    unsigned int width = 0;
    if (bus_width == DMA_bus_32bit)
        width = 0;
    else if (bus_width == DMA_bus_16bit)
        width = 2;
    else if (bus_width == DMA_bus_8bit)
        width = 1;
    else
        panic("dma: invalid bus_width: %d\n", bus_width);

    unsigned int tsz = 0;
    if (unit_size == 1)
        tsz = 1;
    else if (unit_size == 2)
        tsz = 2;
    else if (unit_size == 4)
        tsz = 0;
    else if (unit_size == 8) {
        tsz = 0;
        unit_size = 4;
    } else if (unit_size == 16)
        tsz = 3;
    else if (unit_size == 32)
        tsz = 4;
    else if (unit_size == 64)
        tsz = 5;
    else if (unit_size >= 128) {
        tsz = 6;
        unit_size = 128;
    } else
        panic("dma: invalid unit_size: %d\n", unit_size);

    assert((len / unit_size) < (16 * 1024 * 1024));

    unsigned long dcm = 0;
    /* 如果src/dst是mem, 那么地址自增
     * 如果src/dst是device, 那么地址不自增
     */
    set_bit_field_v(&dcm, DCM_SAI, src_is_mem(rq));
    set_bit_field_v(&dcm, DCM_DAI, dst_is_mem(rq));
    set_bit_field_v(&dcm, DCM_SP, width);
    set_bit_field_v(&dcm, DCM_DP, width);
    set_bit_field_v(&dcm, DCM_SAIW, 0);
    set_bit_field_v(&dcm, DCM_DAIW, 0);
    set_bit_field_v(&dcm, DCM_STDE, 0); /* 没有 stride */
    set_bit_field_v(&dcm, DCM_TSZ, tsz); /* 设置unit size */
    set_bit_field_v(&dcm, DCM_TIE, 0); /* 暂不开中断 */
    set_bit_field_v(&dcm, DCM_LINK, 0); /* 暂不设置link */

    desc->dsa = (unsigned long)src;
    desc->dta = (unsigned long)dst;
    desc->dtc = 0;
    set_bit_field_v(&desc->dtc, DTC_DOA, ((unsigned long)&desc[1] >> 4) & 0xff);
    set_bit_field_v(&desc->dtc, DTC_DTC, len / unit_size);
    desc->sd = 0; /* 没有 stride */
    desc->drt = rq;
    desc->dcm = dcm;
}

static inline unsigned long read_src_addr(struct dma *dma)
{
    unsigned long addr;

    while (1) {
        addr = dma_read(dma->id, DSA(dma->ch));
        if (addr >= dma->src_start && addr <= dma->src_end)
            return addr;
    }
}

static inline unsigned long read_dst_addr(struct dma *dma)
{
    unsigned long addr;

    while (1) {
        addr = dma_read(dma->id, DTA(dma->ch));
        if (addr >= dma->dst_start && addr <= dma->dst_end)
            return addr;
    }
}

unsigned long dma_read_src_addr(struct dma *dma)
{
    unsigned long addr;
    unsigned int flag;

    local_irq_save(flag);

    if (dma->is_started == -1)
        addr = 0;
    else if (dma->is_started == 1)
        addr = read_src_addr(dma);
    else
        addr = dma->src_end;

    local_irq_restore(flag);

    return addr;
}

unsigned long dma_read_dst_addr(struct dma *dma)
{
    unsigned long addr;
    unsigned int flag;

    local_irq_save(flag);

    if (dma->is_started == -1)
        addr = 0;
    else if (dma->is_started == 1)
        addr = read_dst_addr(dma);
    else
        addr = dma->dst_end;

    local_irq_restore(flag);

    return addr;
}

struct dma *dma_request(
        enum DMA_request_type rq, void (*cb)(void *data), void *data,
        enum DMA_bus_width bus_width, unsigned int unit_size)
{
    struct dma *dma = m_dma_alloc_coherent(sizeof(struct dma));
    assert(dma);

    dma->bus_width = bus_width;
    dma->rq = rq;
    dma->cb = cb;
    dma->data = data;
    dma->unit_size = unit_size;
    dma->is_started = -1;
    dma->desc = NULL;
    dma->id = rq_is_mcu_dma(dma->rq);

    return dma;
}

void dma_release(struct dma *dma)
{
    unsigned int flag;

    local_irq_save(flag);

    assert(dma->is_started != 1);

    local_irq_restore(flag);

    datas[dma->id][dma->ch] = NULL;
    m_dma_free_coherent(dma);
}

void dma_start(struct dma *dma,
        void *src, void *dst, unsigned int len)
{
    struct dma_desc *desc;
    enum DMA_request_type rq = dma->rq;
    enum DMA_bus_width bus_width = dma->bus_width;
    unsigned int unit_size = dma->unit_size;

    assert(len);
    assert(unit_size);
    assert(len >= unit_size);
    assert(unit_size >= bus_width);
    assert(!(len % unit_size));
    assert(!(unit_size % bus_width));
    assert(dma->is_started != 1);

    /* 开启全局dma, 并且获得一个通道
     */
    int ch = get_dma_channel(dma->id);
    assert(ch >= 0);

    if (!dma->desc) {
        desc = uncache_mem_alloc(sizeof(*desc), 32);
        dma->desc = desc;
        dma->desc_count = 1;
    } else {
        desc = dma->desc;
    }

    dma->ch = ch;
    dma->is_started = 1;
    dma->src_start = (unsigned long)src;
    dma->dst_start = (unsigned long)dst;
    dma->src_end = dma->src_start + (src_is_mem(rq) ? len : 0);
    dma->dst_end = dma->dst_start + (dst_is_mem(rq) ? len : 0);
    datas[dma->id][ch] = dma;

    /* 初始化 dma desc
     */
    init_dma_desc(desc, src, dst, len, rq, bus_width, unit_size);
    set_bit_field_v(&desc->dcm, DCM_TIE, 1); /* 开启中断 */

    /* 设置desc 地址
     */
    dma_write(dma->id, DDA(ch), (unsigned long)desc);

    /* 载入desc到寄存器
     */
    dma_write(dma->id, DDS, 1 << ch);

    /* 使用 8 word descriptor,
     * 清 AR, HLT, 开启传输
     */
    unsigned long dcs = 0;
    set_bit_field_v(&dcs, DCS_NDES, 0);
    set_bit_field_v(&dcs, DCS_DES8, 1);
    set_bit_field_v(&dcs, DCS_CTE, 1);
    dma_write(dma->id, DCS(ch), dcs);
}

static void dma_start_linked_inner(struct dma *dma,
        void *src, void *dst, unsigned int len, unsigned int count, int cyclic)
{
    enum DMA_request_type rq = dma->rq;
    enum DMA_bus_width bus_width = dma->bus_width;
    unsigned int unit_size = dma->unit_size;
    unsigned int len2 = len / count;

    assert(len);
    assert(unit_size);
    assert(!(len % count));
    assert(len2 >= unit_size);
    assert(unit_size >= bus_width);
    assert(!(len2 % unit_size));
    assert(!(unit_size % bus_width));
    assert(dma->is_started != 1);
    assert(count <= 128);

    /* 开启全局dma, 并且获得一个通道
     */
    int ch = get_dma_channel(dma->id);
    assert(ch >= 0);

    struct dma_desc *desc;

    if (!dma->desc || dma->desc_count < count) {
        unsigned int align = 1;

        while (align < count) {
            align = align * 2;
        }

        align = align * sizeof(*desc);

        /*
         * 链式的dma是用offset来描述下一个描述符的地址的
         * 必须做到dma0描述符数组不能跨页
         **/
        desc = uncache_mem_alloc(align, count * sizeof(*desc));
        dma->desc = desc;
        dma->desc_count = count;
    } else {
        desc = dma->desc;
    }

    dma->ch = ch;
    dma->is_started = 1;
    dma->src_start = (unsigned long)src;
    dma->dst_start = (unsigned long)dst;
    dma->src_end = dma->src_start + (src_is_mem(rq) ? len : 0);
    dma->dst_end = dma->dst_start + (dst_is_mem(rq) ? len : 0);
    datas[dma->id][ch] = dma;

    int i;
    for (i = 0; i< count; i++) {
        /* 初始化 dma desc
         */
        init_dma_desc(&desc[i], src, dst, len2, rq, bus_width, unit_size);
        set_bit_field_v(&desc[i].dcm, DCM_LINK, 1); /* 开启LINK */
        if (src_is_mem(rq))
            src += len2;
        if (dst_is_mem(rq))
            dst += len2;
    }

    i = count - 1;
    if (cyclic) {
        /* 设置循环dma link */
        set_bit_field_v(&desc[i].dtc, DTC_DOA, ((unsigned long)desc >> 4) & 0xff);
        set_bit_field_v(&desc[i].dcm, DCM_TIE, 0); /* 开启中断 */
    } else {
        set_bit_field_v(&desc[i].dcm, DCM_TIE, 1); /* 开启中断 */
        set_bit_field_v(&desc[i].dcm, DCM_LINK, 0); /* 关闭LINK */
    }

    /* 设置desc 地址
     */
    dma_write(dma->id, DDA(ch), (unsigned long)desc);

    /* 载入desc到寄存器
     */
    dma_write(dma->id, DDS, 1 << ch);

    /* 使用 8 word descriptor,
     * 清 AR, HLT, 开启传输
     */
    unsigned long dcs = 0;
    set_bit_field_v(&dcs, DCS_NDES, 0);
    set_bit_field_v(&dcs, DCS_DES8, 1);
    set_bit_field_v(&dcs, DCS_CTE, 1);
    dma_write(dma->id, DCS(ch), dcs);

    dma_read_src_addr(dma);/* 先读一次无效值, 使得 uart dma 模式读的值有效 */
    dma_read_dst_addr(dma);
}

void dma_queue_desc_init(struct dma *dma,
        void *dst, unsigned int count, int enable_desc_irq)
{
    enum DMA_request_type rq = dma->rq;
    enum DMA_bus_width bus_width = dma->bus_width;
    unsigned int unit_size = dma->unit_size;

    assert(unit_size);
    assert(unit_size >= bus_width);
    assert(!(unit_size % bus_width));
    assert(dma->is_started != 1);
    assert(count <= 128);

    /* 开启全局dma, 并且获得一个通道
     */
    int ch = get_dma_channel(dma->id);
    assert(ch >= 0);

    struct dma_desc *desc;

    if (!dma->desc || dma->desc_count < count) {
        unsigned int align = 1;

        while (align < count) {
            align = align * 2;
        }

        align = align * sizeof(*desc);

        /*
         * 链式的dma是用offset来描述下一个描述符的地址的
         * 必须做到dma0描述符数组不能跨页
         **/
        desc = uncache_mem_alloc(align, count * sizeof(*desc));
        dma->desc = desc;
        dma->desc_count = count;
    } else {
        desc = dma->desc;
    }

    dma->ch = ch;
    dma->is_started = 1;
    datas[dma->id][ch] = dma;
    dma->q_enable_irq = !!enable_desc_irq;

    /* 初始化 dma desc
        */
    int i = 0;
    for (i = 0; i < count; i++) {
        init_dma_desc(&desc[i], NULL, dst, 0, rq, bus_width, unit_size);
        set_bit_field_v(&desc[i].dcm, DCM_TIE, !!enable_desc_irq); /* 打开desc的中断 */
        set_bit_field_v(&desc[i].dcm, DCM_LINK, 1); /* 开启LINK */
    }

    i--;
    /* 设置循环dma link */
    set_bit_field_v(&desc[i].dtc, DTC_DOA, ((unsigned long)desc >> 4) & 0xff);
    set_bit_field_v(&desc[i].dcm, DCM_TIE, 1); /* 开启中断 */

    /* 设置desc 地址
     */
    dma_write(dma->id, DDA(dma->ch), (unsigned long)dma->desc);

    /* 载入desc到寄存器
     */
    dma_write(dma->id, DDS, 1 << dma->ch);
}

void dma_queue_start(struct dma *dma)
{
    assert(dma->q_end >= 0);

    /* 使用 8 word descriptor,
     * 清 AR, HLT, 开启传输
     */
    unsigned long dcs = 0;
    set_bit_field_v(&dcs, DCS_NDES, 0);
    set_bit_field_v(&dcs, DCS_DES8, 1);
    set_bit_field_v(&dcs, DCS_CTE, 1);
    dma_write(dma->id, DCS(dma->ch), dcs);

    // dma_read_src_addr(dma);/* 先读一次无效值, 使得 uart dma 模式读的值有效 */
    // dma_read_dst_addr(dma);
    dma_read(dma->id, DSA(dma->ch));/* 同上 */
    dma_read(dma->id, DTA(dma->ch));
}

void dma_start_linked(struct dma *dma,
        void *src, void *dst, unsigned int len, unsigned int count)
{
    dma_start_linked_inner(dma, src, dst, len, count, 0);
}

void dma_start_cyclic(struct dma *dma,
        void *src, void *dst, unsigned int len, unsigned int count)
{
    dma_start_linked_inner(dma, src, dst, len, count, 1);
}

void dma_stop_after_current_transfer(struct dma *dma, void *mem)
{
    struct dma_desc *desc;
    unsigned long unit_len = (dma->src_end - dma->src_start) / dma->desc_count;

    int index = ((unsigned int)(mem) - dma->src_start) / unit_len;

    desc = &dma->desc[index];

    unsigned long flags;

    local_irq_save(flags);

    set_bit_field_v(&desc->dcm, DCM_LINK, 0);
    set_bit_field_v(&desc->dcm, DCM_TIE, 1);

    local_irq_restore(flags);
}


void dma_stop(struct dma *dma)
{
    unsigned int flag;

    local_irq_save(flag);

    if (dma->is_started == 1) {
        dma->src_end = read_src_addr(dma);
        dma->dst_end = read_dst_addr(dma);
        put_dma_channel(dma->id, dma->ch);
        dma->is_started = 0;
    }

    local_irq_restore(flag);
}

static void dma_irq_handler(int irq, void *data)
{
    int i = 0;
    int id = 0;
    unsigned long pending;

    if (irq == IRQ_PDMA_MCU)
        id = 1;

    pending = dma_read(id, DIRQP);

    while (1) {
        while (i < 32 && !(pending & (1 << i++)));
        if (i == 32)
            break;
        i = i - 1;

        set_bit_field_v(&pending, i, i, 0);

        unsigned long dcs = dma_read(id, DCS(i));

        if (get_bit_field_v(&dcs, DCS_AR))
            printf("warning: dma address error, %lx\n", dcs);

        if (get_bit_field_v(&dcs, DCS_HLT))
            printf("warning: dma Halt, %lx\n", dcs);

        struct dma *dma = datas[id][i];
        if (!dma)
            continue;

        dma->is_started = 0;

        put_dma_channel(id, i);

        if (dma->cb)
            dma->cb(dma->data);
    }
}

static void dma_queue_sync_pos(struct dma *dma)
{
    unsigned long desc_addr = dma_read(dma->id, DDA(dma->ch));
    int cur = (desc_addr - (unsigned long)&dma->desc[0]) / sizeof(struct dma_desc);
    int start = dma->q_start;
    int end = dma->q_end;

    if (start <= end) {
        if (cur > end || cur < start)
            end = cur;
    } else {
        if (cur > end && cur < start)
            end = cur;
    }
    start = cur;

    dma->q_start = start;
    dma->q_end = end;
}

static void dma_queue_sync_pos_lock(struct dma *dma)
{
    unsigned int flag;

    local_irq_save(flag);

    dma_queue_sync_pos(dma);

    local_irq_restore(flag);
}

int dma_queue_get_available_size(struct dma *dma)
{
    dma_queue_sync_pos_lock(dma);

    if (dma->q_end < 0)
        return dma->desc_count;

    if ((dma->q_end + 1) % dma->desc_count != dma->q_start)
        return (dma->q_start + dma->desc_count - dma->q_end - 1) % dma->desc_count;
    else
        return 0;
}

void dma_queue_init(struct dma *dma)
{
    dma->q_start = 0;
    dma->q_end = -1;
}

int dma_queue_add(struct dma *dma, void *buf, unsigned int len)
{
    if (!dma_queue_get_available_size(dma)) {
        printf("dma queue add fail\n");
        return -1;
    }

    if (dma->q_end < 0) {
        dma->q_end = 0;
    } else {
        dma->q_end = (dma->q_end + 1) % dma->desc_count;
    }

    dma->desc[dma->q_end].dsa = (unsigned long)buf;
    set_bit_field_v(&dma->desc[dma->q_end].dtc, DTC_DTC, len / dma->unit_size);

    return 0;
}

void dma_queue_deinit(struct dma *dma)
{
    unsigned int flag;

    local_irq_save(flag);

    if (dma->is_started == 1) {
        put_dma_channel(dma->id, dma->ch);
        dma->is_started = 0;
    }

    local_irq_restore(flag);
}

static void dma_desc_irq_handler(int irq, void *data)
{
    int i = 0;
    int id = 1;
    unsigned long pending;

    pending = dma_read(id, DIP);

    dma_write(id, DIC, ~pending);

    while (1) {
        while (i < 32 && !(pending & (1 << i++)));
        if (i == 32)
            break;
        i = i - 1;

        set_bit_field_v(&pending, i, i, 0);

        struct dma *dma = datas[id][i];
        if (!dma)
            continue;

        dma_queue_sync_pos(dma);

        if (dma->cb)
            dma->cb(dma->data);
    }
}

void dma_init(void)
{
    request_irq(IRQ_PDMA, 0, dma_irq_handler, "pdma", NULL);
    request_irq(IRQ_PDMA_MCU, 0, dma_irq_handler, "mcu_dma", NULL);
    request_irq(IRQ_PDMA_MCU_D, 0, dma_desc_irq_handler, "mcu_dma_d", NULL);
}

void dma_deinit(void)
{
    disable_irq(IRQ_PDMA);
    release_irq(IRQ_PDMA);
    disable_irq(IRQ_PDMA_MCU);
    release_irq(IRQ_PDMA_MCU);
    disable_irq(IRQ_PDMA_MCU_D);
    release_irq(IRQ_PDMA_MCU_D);
}

void dma_dump_regs(int ch)
{
    int i;
    for (i = 0; i < 2; i++) {
        printf("DSA(n):0x%08x\n", dma_read(i, DSA(ch)));
        printf("DTA(n):0x%08x\n", dma_read(i, DTA(ch)));
        printf("DTC(n):0x%08x\n", dma_read(i, DTC(ch)));
        printf("DRT(n):0x%08x\n", dma_read(i, DRT(ch)));
        printf("DCS(n):0x%08x\n", dma_read(i, DCS(ch)));
        printf("DCM(n):0x%08x\n", dma_read(i, DCM(ch)));
        printf("DDA(n):0x%08x\n", dma_read(i, DDA(ch)));
        printf("DSD(n):0x%08x\n", dma_read(i, DSD(ch)));

        printf("DMAC:0x%08x\n", dma_read(i, DMAC));
        printf("DIRQP:0x%08x\n", dma_read(i, DIRQP));
        printf("DDB:0x%08x\n", dma_read(i, DDB));
        printf("DDS:0x%08x\n", dma_read(i, DDS));
        printf("DIP:0x%08x\n", dma_read(i, DIP));
        printf("DIC:0x%08x\n", dma_read(i, DIC));
    }

    printf("DMCS:0x%08x\n", (unsigned int)*((volatile unsigned long *)(PDMA_IOBASE + DMCS)));
    printf("DMNMB:0x%08x\n", (unsigned int)*((volatile unsigned long *)(PDMA_IOBASE + DMNMB)));
    printf("DMSMB:0x%08x\n", (unsigned int)*((volatile unsigned long *)(PDMA_IOBASE + DMSMB)));
    printf("DMINT:0x%08x\n", (unsigned int)*((volatile unsigned long *)(PDMA_IOBASE + DMINT)));
}
