#ifndef _SOC_CPM_H_
#define _SOC_CPM_H_

#include <soc/base.h>
#include <cpu/io.h>


#define CPM_CPCCR      0x00
#define CPM_CPCSR      0xD4
#define CPM_DDCDR      0x2C
#define CPM_MACPHYCDR  0x54
#define CPM_MACTXCDR   0x58
#define CPM_MACTXCDR1  0xDC
#define CPM_MACPTPCDC  0x4C
#define CPM_I2S0CDR    0x60
#define CPM_I2S0CDR1   0x70
#define CPM_I2S1CDR    0x7C
#define CPM_I2S1CDR1   0x80
#define CPM_I2S2CDR    0x84
#define CPM_I2S2CDR1   0x88
#define CPM_I2S3CDR    0x8C
#define CPM_I2S3CDR1   0xA0
#define CPM_AUDIOCR    0xAC
#define CPM_LPCDR      0x64
#define CPM_MSC0CDR    0x68
#define CPM_MSC1CDR    0xA4
#define CPM_MSC2CDR    0xA8
#define CPM_SFCCDR     0x74
#define CPM_SSICDR     0x5C
#define CPM_CIMCDR     0x78
#define CPM_PWMCDR     0x6C
#define CPM_ISPCDR     0x30
#define CPM_RSACDR     0x50
#define CPM_MACPHYC0   0xE4
#define CPM_MACPHYC1   0xE8
#define CPM_INTR       0xB0
#define CPM_INTRE      0xB4
#define CPM_SFTINT     0xBC
#define CPM_DRCG       0xD0
#define CPM_CPSPR      0x34
#define CPM_CPSPPR     0x38
#define CPM_USBPCR     0x3C
#define CPM_USBRDT     0x40
#define CPM_USBVBFIL   0x44
#define CPM_USBPCR1    0x48
#define CPM_CPPCR      0x0C
#define CPM_CPAPCR     0x10
#define CPM_CPMPCR     0x14
#define CPM_CPEPCR     0x58

#define CPM_CLKGR  0x20
#define CPM_CLKGR1 0x28

static inline void cpm_write_reg(int reg, unsigned int value)
{
    writel(value, CPM_IOBASE + reg);
}

static inline unsigned int cpm_read_reg(int reg)
{
    return readl(CPM_IOBASE + reg);
}

#define XPCR_PLLFD     20, 28
#define XPCR_PLLRD     14, 19
#define XPCR_PLLOD     11, 13
#define XPCR_PLLRG     5, 7
#define XPCR_PLL_ON    3
#define XPCR_LOCK      2
#define XPCR_PLLEN     0

#define cpm_inl(off)            inl(CPM_IOBASE + (off))
#define cpm_outl(val,off)       outl(val,CPM_IOBASE + (off))
#define cpm_clear_bit(val,off)  do{cpm_outl((cpm_inl(off) & ~(1<<(val))),off);}while(0)
#define cpm_set_bit(val,off)    do{cpm_outl((cpm_inl(off) |  (1<<val)),off);}while(0)
#define cpm_test_bit(val,off)   (cpm_inl(off) & (0x1<<val))

#endif /* _SOC_CPM_H_ */
