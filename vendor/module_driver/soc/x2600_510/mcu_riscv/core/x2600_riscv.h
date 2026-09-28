#ifndef __RISCV_CORE_H__
#define __RISCV_CORE_H__

#include <linux/mutex.h>
#include <utils/ring_mem.h>

struct chip_header {
    unsigned long code;
    unsigned long tag;
    unsigned long img_start;
    unsigned long entry;
    unsigned long img_end;
    unsigned long version;

    unsigned long ring_mem_for_host_write;
    unsigned long ring_mem_for_host_read;
    unsigned long uncache_addr;
    unsigned long uncache_size;
};

void host_notify_riscv(void);
void *riscv_get_header(void);
int riscv_resume(void);
void riscv_set_status(int status);
int wait_riscv_wakeup(unsigned long *timeout);
int riscv_core_ioctl(unsigned int cmd, unsigned long arg);
int riscv_core_init(struct mutex *lock, int is_jtag_en);
void riscv_core_deinit(void);

#endif