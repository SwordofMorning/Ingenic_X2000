#ifndef _SOC_CPM_H_
#define _SOC_CPM_H_

#include <soc/base.h>
#include <cpu/io.h>

#define CPM_CLKGR  0x20
#define CPM_CLKGR1 0x28
#define CPM_SFTINT 0xbc

static inline void cpm_write_reg(int reg, unsigned int value)
{
    writel(value, CPM_IOBASE + reg);
}

static inline unsigned int cpm_read_reg(int reg)
{
    return readl(CPM_IOBASE + reg);
}

#define cpm_inl(off)            inl(CPM_IOBASE + (off))
#define cpm_outl(val,off)       outl(val,CPM_IOBASE + (off))
#define cpm_clear_bit(val,off)  do{cpm_outl((cpm_inl(off) & ~(1<<(val))),off);}while(0)
#define cpm_set_bit(val,off)    do{cpm_outl((cpm_inl(off) |  (1<<val)),off);}while(0)
#define cpm_test_bit(val,off)   (cpm_inl(off) & (0x1<<val))

#endif /* _SOC_CPM_H_ */
