#ifndef __TIZIANO_SYS_H__
#define __TIZIANO_SYS_H__

#include <linux/types.h>

typedef void * tisp_spinlock_t;
typedef void * tisp_completion_t;

extern int system_reg_write(void *hdl, unsigned int reg, unsigned int value);
extern unsigned int  system_reg_read(void *hdl, unsigned int reg);
extern int system_lock(void *hdl);
extern int system_unlock(void *hdl);
extern int system_irq_func_set(void *hdl, int irq, void *func, void *data);

extern tisp_spinlock_t tisp_spin_lock_alloc(void);
extern void tisp_spin_lock_init(tisp_spinlock_t lock);
extern unsigned long tisp_spin_lock_irqsave(tisp_spinlock_t lock);
extern void tisp_spin_unlock_irqrestore(tisp_spinlock_t lock, unsigned long flags);
extern void tisp_spin_lock_free(tisp_spinlock_t lock);

extern tisp_completion_t tisp_completion_alloc(void);
extern void tisp_init_completion(tisp_completion_t x);
extern void tisp_complete(tisp_completion_t x);
extern unsigned long tisp_wait_for_completion_timeout(tisp_completion_t x, unsigned long timeout);
extern void tisp_completion_free(tisp_completion_t compl);

extern void tisp_dma_cache_sync(void *dev, void *vaddr, size_t size, unsigned int direction);

extern void *tisp_kmalloc(size_t size, gfp_t flags);
extern void tisp_kfree(const void *x);

#endif
