/*
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *
 * isp-core driver sys interface
 */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>

#include "isp-core/inc/tiziano_core.h"
#include "isp-core/inc/tiziano_isp.h"


/*
 * interface used by isp-core: spin_lock
 */
tisp_spinlock_t tisp_spin_lock_alloc(void)
{
    spinlock_t *lock = kzalloc(sizeof(spinlock_t), GFP_KERNEL);
    if (!lock)
        printk(KERN_ERR "tisp spin lock alloc failed\n");

    return (tisp_spinlock_t )lock;
}
EXPORT_SYMBOL(tisp_spin_lock_alloc);


void tisp_spin_lock_init(tisp_spinlock_t lock)
{
    spin_lock_init((spinlock_t *)lock);
}
EXPORT_SYMBOL(tisp_spin_lock_init);


unsigned long tisp_spin_lock_irqsave(tisp_spinlock_t lock)
{
    unsigned long flags;

    spin_lock_irqsave((spinlock_t *)lock, flags);

    return flags;
}
EXPORT_SYMBOL(tisp_spin_lock_irqsave);


void tisp_spin_unlock_irqrestore(tisp_spinlock_t lock, unsigned long flags)
{
    spin_unlock_irqrestore((spinlock_t *)lock, flags);
}
EXPORT_SYMBOL(tisp_spin_unlock_irqrestore);


void tisp_spin_lock_free(tisp_spinlock_t lock)
{
    if (lock)
        kfree(lock);
}
EXPORT_SYMBOL(tisp_spin_lock_free);



/*
 * interface used by isp-core: completion
 */
tisp_completion_t tisp_completion_alloc(void)
{
    struct completion *compl = kzalloc(sizeof(struct completion), GFP_KERNEL);
    if (!compl)
        printk(KERN_ERR "tisp_completion_t alloc failed\n");

    return (tisp_completion_t )compl;
}
EXPORT_SYMBOL(tisp_completion_alloc);


void tisp_init_completion(tisp_completion_t x)
{
    init_completion((struct completion *)x);
}
EXPORT_SYMBOL(tisp_init_completion);


void tisp_complete(tisp_completion_t x)
{
    complete((struct completion *)x);
}
EXPORT_SYMBOL(tisp_complete);


unsigned long tisp_wait_for_completion_timeout(tisp_completion_t x, unsigned long timeout)
{
    return wait_for_completion_timeout((struct completion *)x, timeout);
}
EXPORT_SYMBOL(tisp_wait_for_completion_timeout);


void tisp_completion_free(tisp_completion_t compl)
{
    if (compl)
        kfree(compl);
}
EXPORT_SYMBOL(tisp_completion_free);



/*
 * interface used by isp-core: dma
 */
void tisp_dma_cache_sync(void *dev, void *vaddr, size_t size, unsigned int direction)
{
    dma_cache_inv((unsigned long)vaddr, size);
}
EXPORT_SYMBOL(tisp_dma_cache_sync);



/*
 * interface used by isp-core: memory
 */
void *tisp_kmalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags);
}
EXPORT_SYMBOL(tisp_kmalloc);


void tisp_kfree(const void *x)
{
    kfree(x);
}
EXPORT_SYMBOL(tisp_kfree);
