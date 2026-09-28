#ifndef _SOC_ADC_REG_H_
#define _SOC_ADC_REG_H_

#include <bit_field.h>
#include <asm/addrspace.h>

#define SADC_IOBASE                     0x13650000
#define SADC_ADDR(reg)                  ((volatile unsigned long *)CKSEG1ADDR(SADC_IOBASE + reg))

#define ADC_SR                          0x00
#define ADC_IE                          0x04
#define ADC_IE_ForRiscV                 0x08
#define ADC_IR                          0x0C
#define ADC_IR_ForRiscV                 0x10
#define ADC_CR                          0x14
#define ADC_CFR                         0x18
#define ADC_CLKR0                       0x1C
#define ADC_CLKR1                       0x20
#define ADC_CLKR2                       0x24
#define ADC_EXT_GPIO_CR                 0x28
#define ADC_AWD_CR(n)                   (0x2C + (n) * 4) // (n = [0, 15])
#define ADC_AWD_SR                      0x6C
#define ADC_AWD_IM                      0x70
#define ADC_AWD_IM_ForRiscV             0x74
#define ADC_AWD_IR                      0x78
#define ADC_AWD_IR_ForRiscV             0x7C
#define ADC_DBG_CR                      0x80
#define ADC_DBG_FSM                     0x84
#define ADC_SEQ0_CR                     0x100
#define ADC_SEQ0_CNR0                   0x104
#define ADC_SEQ0_DCR                    0x108
#define ADC_SEQ0_DR0                    0x10C
#define ADC_SEQ0_DR1                    0x110
#define ADC_SEQ1_CR                     0x200
#define ADC_SEQ1_CNR0                   0x204
#define ADC_SEQ1_CNR1                   0x208
#define ADC_SEQ1_DCR                    0x20C
#define ADC_SEQ1_DR                     0x210
#define ADC_SEQ1_DLY(n)                 (0x214 + (n) * 4) // (n=[0, 7])
#define ADC_SEQ1_CONT_CR(n)             (0x234 + (n) * 4) // (n=[0, 7])
#define ADC_SEQ1_RCNT                   0x254
#define ADC_SEQ1_DMA_RCNT               0x258
#define ADC_SEQ2_CR                     0x300
#define ADC_SEQ2_CNR0                   0x304
#define ADC_SEQ2_DCR                    0x30C
#define ADC_SEQ2_DR                     0x310
#define ADC_SEQ2_DLY(n)                 (0x314 + (n) * 4) // (n=[0, 1])
#define ADC_SEQ2_RCNT                   0x354
#define ADC_SEQ2_DMA_RCNT               0x358


/* ADC_SR */
#define SR_AWD                          31
#define SR_PHY_EOC_ERR                  30
#define SR_SEQ2_DATA_OVR                23
#define SR_SEQ2_DMA_FIN                 22
#define SR_SEQ2_DR                      21
#define SR_SEQ2_EVT_OVR                 20
#define SR_SEQ2_FSM                     16, 19
#define SR_SEQ1_DATA_OVR                15
#define SR_SEQ1_DMA_FIN                 14
#define SR_SEQ1_DR                      13
#define SR_SEQ1_EVT_OVR                 12
#define SR_SEQ1_FSM                     8, 11
#define SR_SEQ0_DR                      5
#define SR_SEQ0_EVT_OVR                 4
#define SR_SEQ0_FSM                     0, 3

/* ADC_IE */
#define IE_PHY_EOC_ERR                  30
#define IE_SEQ2_DATA_OVR                21
#define IE_SEQ2_DMA_FIN                 20
#define IE_SEQ2_DR                      19
#define IE_SEQ2_EVT_OVR                 18
#define IE_SEQ2_EOQ                     17
#define IE_SEQ2_SOQ                     16
#define IE_SEQ1_CONT_EOG                15
#define IE_SEQ1_CONT_SOG                14
#define IE_SEQ1_DATA_OVR                13
#define IE_SEQ1_DMA_FIN                 12
#define IE_SEQ1_DR                      11
#define IE_SEQ1_EVT_OVR                 10
#define IE_SEQ1_EOQ                     9
#define IE_SEQ1_SOQ                     8
#define IE_SEQ0_DR                      3
#define IE_SEQ0_EVT_OVR                 2
#define IE_SEQ0_EOQ                     1
#define IE_SEQ0_SOQ                     0

/* ADC_IR */
#define IR_AWD                          31
#define IR_PHY_EOC_ERR                  30
#define IR_SEQ2_DATA_OVR                21
#define IR_SEQ2_DMA_FIN                 20
#define IR_SEQ2_DR                      19
#define IR_SEQ2_EVT_OVR                 18
#define IR_SEQ2_EOQ                     17
#define IR_SEQ2_SOQ                     16
#define IR_SEQ1_CONT_EOG                15
#define IR_SEQ1_CONT_SOG                14
#define IR_SEQ1_DATA_OVR                13
#define IR_SEQ1_DMA_FIN                 12
#define IR_SEQ1_DR                      11
#define IR_SEQ1_EVT_OVR                 10
#define IR_SEQ1_EOQ                     9
#define IR_SEQ1_SOQ                     8
#define IR_SEQ0_DR                      3
#define IR_SEQ0_EVT_OVR                 2
#define IR_SEQ0_EOQ                     1
#define IR_SEQ0_SOQ                     0

/* ADC_CR */
#define CR_SEQ2_START                   10
#define CR_SEQ1_START                   9
#define CR_SEQ0_START                   8
#define CR_SEQ2_RESET                   2
#define CR_SEQ1_RESET                   1
#define CR_SEQ0_RESET                   0

/* ADC_CFR */
#define CFR_PHY_SEL_EOC                 27
#define CFR_PHY_SEL_EN                  26
#define CFR_PHY_RESET                   25
#define CFR_PHY_PD                      24
#define CFR_BREAK_MD                    16
#define CFR_SEQ2_PRI                    4, 5
#define CFR_SEQ1_PRI                    2, 3
#define CFR_SEQ0_PRI                    0, 1

/* ADC_CLKR0 */
#define ADCCLK_DIV                      24, 31
#define SEQ1_CONTCLK_DIV                0, 23

/* ADC_CLKR1 */
#define SEQ1_DLYCLK_DIV                 0, 23

/* ADC_CLKR2 */
#define SEQ2_DLYCLK_DIV                 0, 23

/* ADC_EXT_GPIO_CR */
#define FIL                             0, 9

/* ADC_AWD_CRn */
#define HTR_EN                          31
#define HTR                             16, 27
#define LTR_EN                          15
#define LTR                             0, 11

/* ADC_AWD_SR & ADC_AWD_IM & ADC_AWD_IR */
#define CH0_HTR_FLG                     16
#define CH0_LTR_FLG                     0

/* ADC_DBG_CR */
#define ADC_ANA_REG                     16, 31

/* ADC_DBG_FSM */
#define ADC_ARB_FSM                     0, 7

/* ADC_SEQ0_CR */
#define SEQ0_CR_LEN                     12, 13
#define SEQ0_CH_NUM_EN                  0

/* ADC_SEQ0_CNR0 */
#define SEQ0_CNR_CH_NUM3                12, 15
#define SEQ0_CNR_CH_NUM2                8, 11
#define SEQ0_CNR_CH_NUM1                4, 7
#define SEQ0_CNR_CH_NUM0                0, 3

/* ADC_SEQ0_DCR */
#define SEQ0_DRT_CUS3                   19
#define SEQ0_DRT_CUS2                   18
#define SEQ0_DRT_CUS1                   17
#define SEQ0_DRT_CUS0                   16
#define SEQ0_DRT                        13
#define SEQ0_CNT                        0, 2

/* ADC_SEQ0_DR0 */
#define SEQ0_DR_CH_NUM1                 28, 31
#define SEQ0_DR0_DATA1                  16, 27
#define SEQ0_DR_CH_NUM0                 12, 15
#define SEQ0_DR0_DATA0                  0, 11

/* ADC_SEQ0_DR1 */
#define SEQ0_DR_CH_NUM3                 28, 31
#define SEQ0_DR1_DATA3                  16, 27
#define SEQ0_DR_CH_NUM2                 12, 15
#define SEQ0_DR1_DATA2                  0, 11

/* ADC_SEQ1_CR */
#define SEQ1_STORAGE0                   16
#define SEQ1_CR_LEN                     12, 15
#define SEQ1_EXT_TCU_CH_SEL             8, 11
#define SEQ1_EXTSEL                     6, 7
#define SEQ1_CONT_GLEN                  2, 4
#define SEQ1_CONT_EN                    1
#define SEQ1_CH_NUM_EN                  0

#define SEQ1_EXTSEL_software            SEQ1_EXTSEL, 0
#define SEQ1_EXTSEL_gpio                SEQ1_EXTSEL, 1
#define SEQ1_EXTSEL_tcu0                SEQ1_EXTSEL, 2
#define SEQ1_EXTSEL_tcu1                SEQ1_EXTSEL, 3

/* ADC_SEQ1_CNR0 */
#define SEQ1_CH_NUM07(n)                (4 * n), (4 * n + 3)

/* ADC_SEQ1_CNR1 */
#define SEQ1_CH_NUM815(n)               (4 * (n-8)), (4 * (n-8) + 3)

/* ADC_SEQ1_DCR */
#define SEQ1_DRT_CUS0                   16
#define SEQ1_DRT                        13, 15
#define SEQ1_DMA_MD                     8
#define SEQ1_FIFO_CNT                   0, 5

/* ADC_SEQ1_DR */
#define SEQ1_CH_NUM                     12, 15
#define SEQ1_DATA                       0, 11

/* ADC_SEQ1_DLYn */
#define SEQ1_DLY_CNT1                   16, 31
#define SEQ1_DLY_CNT0                   0, 15

/* ADC_SEQ1_CONT_CRn */
#define SEQ1_CONT_CNT                   16, 31
#define SEQ1_CONT_CR_LEN                0, 3

/* ADC_SEQ1_RCNT */
#define SEQ1_RCNT_MAX                   16, 31
#define SEQ1_RCNT_CNT                   0, 15

/* ADC_SEQ1_DMA_RCNT */
#define SEQ1_DMA_RCNT_CNT               0, 15

/* ADC_SEQ2_CR */
#define SEQ2_STORAGE0                   16
#define SEQ2_CR_LEN                     12, 14
#define SEQ2_EXT_TCU_CH_SEL             8, 11
#define SEQ2_EXTSEL                     6, 7
#define SEQ2_CH_NUM_EN                  0

#define SEQ2_EXTSEL_software            SEQ2_EXTSEL, 0
#define SEQ2_EXTSEL_gpio                SEQ2_EXTSEL, 1
#define SEQ2_EXTSEL_tcu0                SEQ2_EXTSEL, 2
#define SEQ2_EXTSEL_tcu1                SEQ2_EXTSEL, 3

/* ADC_SEQ2_DCR */
#define SEQ2_DRT_CUS0                   16
#define SEQ2_DRT                        13, 15
#define SEQ2_DMA_MD                     8
#define SEQ2_FIFO_CNT                   0, 4

/* ADC_SEQ2_DR */
#define SEQ2_CH_NUM                     12, 15
#define SEQ2_DATA                       0, 11

/* ADC_SEQ2_DLYn */
#define SEQ2_DLY_CNT0_4                 0
#define SEQ2_DLY_CNT1_5                 8
#define SEQ2_DLY_CNT2_6                 16
#define SEQ2_DLY_CNT3_7                 24

/* ADC_SEQ2_RCNT */
#define SEQ2_RCNT_MAX                   16, 31
#define SEQ2_RCNT_CNT                   0, 15

/* ADC_SEQ2_DMA_RCNT */
#define SEQ2_DMA_RCNT_CNT               0, 15


static inline void adc_write_reg(unsigned long reg, unsigned int value)
{
    *SADC_ADDR(reg) = value;
}

static inline unsigned int adc_read_reg(unsigned long reg)
{
    return *SADC_ADDR(reg);
}

static inline void adc_set_bit(unsigned long reg, int bit, unsigned int val)
{
    set_bit_field(SADC_ADDR(reg), bit, bit, val);
}

static inline unsigned int adc_get_bit(unsigned long reg, int bit)
{
    return get_bit_field(SADC_ADDR(reg), bit, bit);
}

static inline void adc_set_bits(unsigned long reg, int start, int end, unsigned int val)
{
    set_bit_field(SADC_ADDR(reg), start, end, val);
}

static inline unsigned int adc_get_bits(unsigned long reg, int start, int end)
{
    return get_bit_field(SADC_ADDR(reg), start, end);
}

#endif // _SOC_ADC_REG_H_
