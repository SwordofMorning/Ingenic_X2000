#ifndef _ICODEC_REGS_H_
#define _ICODEC_REGS_H_

#define CGR         0x00
#define CDCFGR      0x04
#define CACR        0x08
#define CACR1       0x0C
#define CDCR        0x10
#define CDCR1       0x14
#define CDGSR       0x18
#define CDLCBMSR    0x1C
#define CLAVR       0x20
#define CGAINR      0x28
#define CDBCR       0x80
#define CCR         0x84
#define CAACR       0x88
#define CMICCR      0x8C
#define CMICGAINR   0x90
#define CAEC        0x94
#define CALCGR      0x98
#define CANACR      0xA0
#define CANACR1     0xA4
#define CHR         0xA8
#define CHPOUTLGR   0xAC
#define CMR         0x100
#define CTR         0x104
#define CAGCCR      0x108
#define CPGR        0x10C
#define CSRR        0x110
#define CALMAXR     0x114
#define CAHMAXR     0x118
#define CALMINR     0x11c
#define CAHMINR     0x120
#define CAFR        0x124

/* CGR */
#define DIGCORE_RSTN 1, 1
#define SYS_RSTN 0, 0

/* CDCFGR */
#define DAC_MUTE_EN 7, 7
#define DAC_MUTE_SR 4, 6
#define DITHER_LEVEL 3, 3
#define DA_ENABLE 2, 2
#define DITHER_ENABLE 1, 1
#define DITHER_SIGN 0, 0

/* CACR */
#define I2S_TX_LRP 7, 7
#define I2S_TX_WL 5, 6
#define I2S_TX_FMT 3, 4
#define I2S_TX_DATSEL 0, 1

/* CACR1 */
#define I2S_RX_PIN_MST 7, 7
#define I2S_RX_FUN_MST 6, 6
#define I2S_TX_PIN_MST 5, 5
#define I2S_TX_FUN_MST 4, 4
#define I2S_TX_LEN 2, 3
#define I2S_TX_RSTN 1, 1
#define I2S_TX_BCLKINV 0, 0

/* CDCR */
#define I2S_RX_LRP 7, 7
#define I2S_RX_WL  5, 6
#define I2S_RX_FMT  3, 4
#define I2S_LR_SWAP 2, 2

/* CDCR1 */
#define DAC_DEM 4, 5
#define I2S_RX_RSTN 1, 1

/* CDGSR */
#define DAC_VOL 0, 7

/* CDLCBMSR */
#define DACL_BIST_SEL  4, 5
#define DACL_MUTE_CTL 3, 3

/* CLAVR */
#define ADCL_VOL 0, 7

/* CGAINR */
#define ALCL_EN 4, 4
#define HPF_MODE 2, 3
#define ADCL_POL 0, 0

/* CDBCR */
#define SEL_IBIAS_DAC 0, 3

/* CCR */
#define SEL_VREF 0, 7

/* CAACR */
#define EN_VREF 5, 5
#define EN_IBIAS_ADC 4, 4
#define EN_MICBIAS 3, 3
#define GAIN_MICBIAS 0, 2

/* CMICCR */
#define MUTE_MICL 7, 7
#define INITIAL_MICL 6, 6
#define EN_BUF_ADCL 5, 5
#define EN_ZERODET_ADCL 4, 4

/* CMICGAINR */
#define GAIN_MICL 6, 7
#define SEL_IBIAS_ADC 0, 3

/* CAEC */
#define EN_ALCL 5, 5
#define EN_MICL 4, 4

/* CALCGR */
#define GAIN_ALCL 0, 4

/* CANACR */
#define INITIAL_ALCL 7, 7
#define EN_CLK_ADCL 6, 6
#define EN_ADCL 5, 5
#define INITIAL_ADCL 4, 4

/* CANACR1 */
#define EN_IBIAS_DAC 7, 7
#define EN_BUF_DACL 6, 6
#define POP_CTRL_DACL  4, 5
#define EN_VREF_DACL 3, 3
#define EN_CLK_DACL 2, 2
#define EN_DACL 1, 1
#define INITIAL_DACL 0, 0

/* CHR */
#define MUTE_HPOUTL 6, 6
#define INITIAL_HPOUTL 5, 5
#define EN_HPOUTL 4, 4
#define SEL_HPOUTL 0, 3

/* CHPOUTLGR */
#define GAIN_HPOUTL 0, 4

/* CMR */
#define GAIN_METHOD 6, 6
#define CTL_METHOD  4, 5
#define GAIN_HLD_TIM 0, 3

/* CTR */
#define DECAY_TIM  4, 7
#define ATTACK_TIM 0, 3

/* CAGCCR */
#define AGC_MOD 7, 7
#define AGC_ZERO_EN 6, 6
#define AMP_MOD 5, 5
#define FAST_DECR_EN 4, 4
#define NOISE_GATE_EN 3, 3
#define NOISE_GATE_TL 0, 2

/* CPGR */
#define PGA_ZERO_EN 5, 5
#define GAIN_PGA 0, 4

/* CSRR */
#define SLOW_CLK_EN 3, 3
#define SAMPLE_RATE 0, 2

/* CALMAXR */
#define AGC_MAXL_L8B 0, 7

/* CAHMAXR */
#define AGC_MAXL_H8B 0, 7

/* CALMINR */
#define AGC_MINL_L8B 0, 7

/* CAHMINR */
#define AGC_MINL_H8B 0, 7

/* CAFR */
#define AGC_FUN_SEL 6, 6
#define MAX_GAIN_PGA  3, 5
#define MIN_GAIN_PGA 0, 2

#define ICODEC_IOBASE 0x10021000

#define ICODEC_REG_BASE  KSEG1ADDR(ICODEC_IOBASE)

#define ICODEC_ADDR(reg) ((volatile unsigned long *)(ICODEC_REG_BASE + (reg)))

#endif /* _ICODEC_REGS_H_ */