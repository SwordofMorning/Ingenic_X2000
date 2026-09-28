#ifndef _ICODEC_REGS_H_
#define _ICODEC_REGS_H_

#define RSTR        0x00
#define DACCR1      0x04
#define DACCR2      0x08
#define DACCR3      0x0c
#define DACCR4      0x10
#define DACCR5      0x14
#define DACDGR      0x18
#define ADCCR1      0x24
#define ADCCR2      0x28
#define ADCCR3      0x2c
#define ADCCR4      0x30
#define ADCDGR      0x34
#define BIASCR1     0x80
#define BIASCR2     0x84
#define BIASCR3     0x88
#define DACLCR      0xa0
#define HPCR        0xa4
#define HPDSR       0xa8
#define HPGR        0xac
#define ADCLCR      0xc0
#define MICCR       0xc4
#define ALCGR       0xc8
#define MICGR       0xcc
#define AGCCR1      0x100
#define AGCCR2      0x104
#define AGCCR3      0x108
#define PGAGR       0x10c
#define AGCSRR      0x110
#define AGCMAXLR    0x114
#define AGCMAXHR    0x118
#define AGCMINLR    0x11c
#define AGCMINHR    0x120
#define AGCGR       0x124
#define ALCOGR      0x138

/* RSTR */
#define ADCRST 5, 5
#define DACRST 4, 4
#define BISTEN 1, 1
#define SYSRST 0, 0

/* DACCR1 */
#define DAC_I2SWL 4, 5
#define DAC_I2SFMT 2, 3

/* DACCR2 */
#define DAC_I2SLRP 7, 7
#define DAC_I2SLRSW 6, 6
#define DAC_FUNCMSTEN 5, 5
#define DAC_PINMSTEN 4, 4
#define DAC_I2SRST 0, 0

/* DACCR3 */
#define MUTEN 7, 7
#define MUTSR 4, 6
#define DAEN 3, 3
#define DITHEREN 2, 2
#define DITHERLVL 1, 1
#define DITHERSIGN 0, 0

/* DACCR4 */
#define DEEMPSEL 0, 1

/* DACCR5 */
#define DAC_BISTSELL 2, 3
#define DAC_MUTEL 0, 0

/* DACDGR */
#define DAC_DIGGAIN 0, 7

/* ADCCR1 */
#define ADC_I2SDATSEL 6, 7
#define ADC_I2SWL 4, 5
#define ADC_I2SFMT 2, 3

/* ADCCR2 */
#define ADC_I2SLRP 7, 7
#define ADC_FUNCMSTEN 5, 5
#define ADC_PINMSTEN 4, 4
#define ADC_SCKINV 1, 1
#define ADC_I2SRST 0, 0

/* ADCCR3 */
#define HPFMODE 2, 3
#define POLL 0, 0

/* ADCCR4 */
#define ADC_BISTSELL 2, 3
#define ADC_PGAGAINENL 0, 0

/* ADCDGR */
#define ADC_DIGGAIN 0, 7

/* BIASCR1 */
#define MICBIASGAIN 5, 7
#define MICBIASEN 3, 3
#define ADCIBIASEN 2, 2
#define DACIBIASEN 1, 1
#define VREFEN 0, 0

/* BIASCR2 */
#define SELVREF 0, 7

/* BIASCR3 */
#define IBIASADC 4, 7
#define IBIASDAC 0, 3

/* DACLCR */
#define POPCTRL 5, 6
#define DACLINITIAL 4, 4
#define DACLVREFEN 3, 3
#define DACLBUFEN 2, 2
#define DACLCLKEN 1, 1
#define DACLEN 0, 0

/* HPCR */
#define HPLMUTE 5, 5
#define HPLINITIAL 4, 4
#define HPLEN 0, 0

/* HPDSR */
#define HPLDRV 0, 3

/* HPGR */
#define HPLGAIN 0, 4

/* ADCLCR */
#define ADCLSRC 5, 6
#define ADCLINITIAL 4, 4
#define ZERODETEN 3, 3
#define ADCLBUFEN 2, 2
#define ADCLCLKEN 1, 1
#define ADCLEN 0, 0

/* MICCR */
#define SE_EN 5, 5
#define MICLMUTE 4, 4
#define MICLINITIAL 3, 3
#define ALCLINITIAL 2, 2
#define MICLEN 1, 1
#define ALCLEN 0, 0

/* ALCGR */
#define ALCLGAIN 0, 4

/* MICGR */
#define MICLGAIN 0, 1

/* AGCCR1 */
#define GAINATTACK 6, 6
#define CTRLMETH 4, 5
#define HOLDTIME 0, 3

/* AGCCR2 */
#define DECAYTIME 4, 7
#define ATTACKTIME 0, 3

/* AGCCR3 */
#define AGMODE 7, 7
#define ZECREN 6, 6
#define REMODE 5, 5
#define FASTDEC 4, 4
#define NSGATE 3, 3
#define NSTHRE 0, 2

/* PGAGR */
#define LPGAZECR 5, 5
#define LPGAGAIN 0, 4

/* AGCSRR */
#define SLOWCLK 3, 3
#define APPRSR 0, 2

/* AGCMAXLR */
#define MAXLOW 0, 7

/* AGCMAXHR */
#define MAXHIGH 0, 7

/* AGCMINLR */
#define MINLOW 0, 7

/* AGCMINHR */
#define MINHIGH 0, 7

/* AGCGR */
#define AGCEN 6, 6
#define PGAINMAX 3, 5
#define PGAINMIN 0, 2

/* ALCOGR */
#define ALCOGAIN 0, 3

#define ICODEC_IOBASE 0x10021000

#define ICODEC_REG_BASE  KSEG1ADDR(ICODEC_IOBASE)

#define ICODEC_ADDR(reg) ((volatile unsigned long *)(ICODEC_REG_BASE + (reg)))

#endif /* _ICODEC_REGS_H_ */