#ifndef _ADC_REGS_H_
#define _ADC_REGS_H_

#include <cpu/io.h>
#include <soc/base.h>

#define ADC_SR               io_addr(SADC_IOBASE + 0x0000)
#define ADC_IE               io_addr(SADC_IOBASE + 0x0004)
#define ADC_IE_ForRiscV      io_addr(SADC_IOBASE + 0x0008)
#define ADC_IR               io_addr(SADC_IOBASE + 0x000C)
#define ADC_IR_ForRiscV      io_addr(SADC_IOBASE + 0x0010)
#define ADC_CR               io_addr(SADC_IOBASE + 0x0014)
#define ADC_CFR              io_addr(SADC_IOBASE + 0x0018)
#define ADC_CLKR0            io_addr(SADC_IOBASE + 0x001C)
#define ADC_CLKR1            io_addr(SADC_IOBASE + 0x0020)
#define ADC_CLKR2            io_addr(SADC_IOBASE + 0x0024)
#define ADC_EXT_GPIO_CR      io_addr(SADC_IOBASE + 0x0028)
#define ADC_AWD_CR(n)        io_addr(SADC_IOBASE + 0x002c+(n)*4) // (n=0,1,...15)
#define ADC_AWD_SR           io_addr(SADC_IOBASE + 0x006C)
#define ADC_AWD_IM           io_addr(SADC_IOBASE + 0x0070)
#define ADC_AWD_IM_ForRiscV  io_addr(SADC_IOBASE + 0x0074)
#define ADC_AWD_IR           io_addr(SADC_IOBASE + 0x0078)
#define ADC_AWD_IR_ForRiscV  io_addr(SADC_IOBASE + 0x007C)
#define ADC_DBG_CR           io_addr(SADC_IOBASE + 0x0080)
#define ADC_DBG_FSM          io_addr(SADC_IOBASE + 0x0084)
#define ADC_SEQ0_CR          io_addr(SADC_IOBASE + 0x0100)
#define ADC_SEQ0_CNR0        io_addr(SADC_IOBASE + 0x0104)
#define ADC_SEQ0_DCR         io_addr(SADC_IOBASE + 0x0108)
#define ADC_SEQ0_DR0         io_addr(SADC_IOBASE + 0x010c)
#define ADC_SEQ0_DR1         io_addr(SADC_IOBASE + 0x0110)
#define ADC_SEQ1_CR          io_addr(SADC_IOBASE + 0x0200)
#define ADC_SEQ1_CNR0        io_addr(SADC_IOBASE + 0x0204)
#define ADC_SEQ1_CNR1        io_addr(SADC_IOBASE + 0x0208)
#define ADC_SEQ1_DCR         io_addr(SADC_IOBASE + 0x020C)
#define ADC_SEQ1_DR          io_addr(SADC_IOBASE + 0x0210)
#define ADC_SEQ1_DLY(n)      io_addr(SADC_IOBASE + 0x0214+(n)*4) // (n=0,1,...7)
#define ADC_SEQ1_CONT_CR(n)  io_addr(SADC_IOBASE + 0x0234+(n)*4) // (n=0.1,...7)
#define ADC_SEQ1_RCNT        io_addr(SADC_IOBASE + 0x254)
#define ADC_SEQ1_DMA_RCNT    io_addr(SADC_IOBASE + 0x258)
#define ADC_SEQ2_CR          io_addr(SADC_IOBASE + 0x0300)
#define ADC_SEQ2_CNR0        io_addr(SADC_IOBASE + 0x0304)
#define ADC_SEQ2_DCR         io_addr(SADC_IOBASE + 0x030C)
#define ADC_SEQ2_DR          io_addr(SADC_IOBASE + 0x0310)
#define ADC_SEQ2_DLY(n)      io_addr(SADC_IOBASE + 0x0314+(n)*4) // (n=0,1)
#define ADC_SEQ2_RCNT        io_addr(SADC_IOBASE + 0x354)
#define ADC_SEQ2_DMA_RCNT    io_addr(SADC_IOBASE + 0x358)


#define SR_AWD           31
#define SR_PHY_EOC_ERR   30
#define SR_SEQ2_DATA_OVR 23
#define SR_SEQ2_DMA_FIN  22
#define SR_SEQ2_DR       21
#define SR_SEQ2_EVT_OVR  20
#define SR_SEQ2_FSM      16,19
#define SR_SEQ1_DATA_OVR 15
#define SR_SEQ1_DMA_FIN  14
#define SR_SEQ1_DR       13
#define SR_SEQ1_EVT_OVR  12
#define SR_SEQ1_FSM      8,11
#define SR_SEQ0_DR       5
#define SR_SEQ0_EVT_OVR  4
#define SR_SEQ0_FSM      0,3

#define AWD           31
#define PHY_EOC_ERR   30
#define SEQ2_DATA_OVR 21
#define SEQ2_DMA_FIN  20
#define SEQ2_DR       19
#define SEQ2_EVT_OVR  18
#define SEQ2_EOQ      17
#define SEQ2_SOQ      16
#define SEQ1_CONT_EOG 15
#define SEQ1_CONT_SOG 14
#define SEQ1_DATA_OVR 13
#define SEQ1_DMA_FIN  12
#define SEQ1_DR       11
#define SEQ1_EVT_OVR  10
#define SEQ1_EOQ      9
#define SEQ1_SOQ      8
#define SEQ0_DR       3
#define SEQ0_EVT_OVR  2
#define SEQ0_EOQ      1
#define SEQ0_SOQ      0

#define SEQ2_START 10,10
#define SEQ1_START 9,9
#define SEQ0_START 8,8
#define SEQ2_RESET 2,2
#define SEQ1_RESET 1,1
#define SEQ0_RESET 0,0

#define PHY_SEL_EOC 27,27
#define PHY_SEL_EN  26,26
#define PHY_RESET   25,25
#define PHY_PD      24,24
#define BREAK_MD    16,16
#define SEQ2_PRI    4,5
#define SEQ1_PRI    2,3
#define SEQ0_PRI    0,1

#define ADCCLK_DIV       24,31
#define SEQ1_CONTCLK_DIV 0,23

#define SEQ1_DLYCLK_DIV 0,23
#define SEQ2_DLYCLK_DIV 0,23

#define FIL 0,9

#define HTR_EN 31
#define HTR    16,27
#define LTR_EN 15
#define LTR    0,11

#define CH0_HTR 16
#define CH0_LTR 0

#define ADC_ANA_REG 16,31

#define ADC_ARB_FSM 0,7

#define SEQ0_LEN      12,13
#define SEQ0_CH_NUM_EN 0, 0

#define SEQ0_CH_NUM0 0, 3
#define SEQ0_CH_NUM1 4,7
#define SEQ0_CH_NUM2 8,11
#define SEQ0_CH_NUM3 12,15

#define SEQ0_DRT_CUS0 16
#define SEQ0_DRT_CUS1 17
#define SEQ0_DRT_CUS2 18
#define SEQ0_DRT_CUS3 19

#define SEQ0_DRT 13,13
#define SEQ0_CNT 0,2

#define SEQ0_DR0_CH_NUM1 28,31
#define SEQ0_DR0_DATA1 16, 27
#define SEQ0_DR0_CH_NUM0 12,15
#define SEQ0_DR0_DATA0 0, 11

#define SEQ0_DR1_CH_NUM3 28,31
#define SEQ0_DR1_DATA3 16,27
#define SEQ0_DR1_CH_NUM2 12,15
#define SEQ0_DR1_DATA2 0,11

#define SEQ1_STORAGE0       16
#define SEQ1_LEN            12,15
#define SEQ1_EXT_TCU_CH_SEL 8,11
#define SEQ1_EXTSEL         6,7
#define SEQ1_CONT_GLEN      2,4
#define SEQ1_CONT_EN        1,1
#define SEQ1_CH_NUM_EN      0,0

#define SEQ1_EXT_TCU_CH_SEL_tcu_half(n)    SEQ1_EXT_TCU_CH_SEL, ((n)*2+0)
#define SEQ1_EXT_TCU_CH_SEL_tcu_full(n)    SEQ1_EXT_TCU_CH_SEL, ((n)*2+1)
#define SEQ1_EXT_TCU_CH_SEL_gpio_rising    SEQ1_EXT_TCU_CH_SEL, 1
#define SEQ1_EXT_TCU_CH_SEL_gpio_falling   SEQ1_EXT_TCU_CH_SEL, 2
#define SEQ1_EXT_TCU_CH_SEL_gpio_both      SEQ1_EXT_TCU_CH_SEL, 3

#define SEQ1_EXTSEL_software SEQ1_EXTSEL, 0
#define SEQ1_EXTSEL_gpio     SEQ1_EXTSEL, 1
#define SEQ1_EXTSEL_tcu0     SEQ1_EXTSEL, 2
#define SEQ1_EXTSEL_tcu1     SEQ1_EXTSEL, 3

#define SEQ1_CH_NUM0 0
#define SEQ1_CH_NUM8 0

#define SEQ1_DRT_CUS0 16
#define SEQ1_DRT      13,15
#define SEQ1_DMA_MD   8,8
#define SEQ1_FIFO_CNT 0,5

#define SEQ1_CH_NUM 12,15
#define SEQ1_DATA 0,11

#define SEQ1_DLY_CNT0 0,15
#define SEQ1_DLY_CNT1 16,31

#define SEQ1_CONT_CNT 16, 31
#define SEQ1_GRP_LEN 0,3

#define SEQ1_MAX 16,31
#define SEQ1_BUS_CNT 0,15

#define SEQ1_DMA_CNT 0,15

#define SEQ2_STORAGE0       16
#define SEQ2_LEN            12,14
#define SEQ2_EXT_TCU_CH_SEL 8,11
#define SEQ2_EXTSEL         6,7
#define SEQ2_CH_NUM_EN      0,0

#define SEQ2_EXT_TCU_CH_SEL_tcu_half(n)    SEQ2_EXT_TCU_CH_SEL, ((n)*2+0)
#define SEQ2_EXT_TCU_CH_SEL_tcu_full(n)    SEQ2_EXT_TCU_CH_SEL, ((n)*2+1)
#define SEQ2_EXT_TCU_CH_SEL_gpio_rising    SEQ2_EXT_TCU_CH_SEL, 1
#define SEQ2_EXT_TCU_CH_SEL_gpio_falling   SEQ2_EXT_TCU_CH_SEL, 2
#define SEQ2_EXT_TCU_CH_SEL_gpio_both      SEQ2_EXT_TCU_CH_SEL, 3

#define SEQ2_EXTSEL_software SEQ2_EXTSEL, 0
#define SEQ2_EXTSEL_gpio     SEQ2_EXTSEL, 1
#define SEQ2_EXTSEL_tcu0     SEQ2_EXTSEL, 2
#define SEQ2_EXTSEL_tcu1     SEQ2_EXTSEL, 3

#define SEQ2_CH_NUM0 0

#define SEQ2_DRT_CUS0 16
#define SEQ2_DRT      13,15
#define SEQ2_DMA_MD   8,8
#define SEQ2_FIFO_CNT 0,4

#define SEQ2_CH_NUM 12,15
#define SEQ2_DATA   0,11

#define SEQ2_DLY_CNT04 0,7
#define SEQ2_DLY_CNT15 8,15
#define SEQ2_DLY_CNT26 16,23
#define SEQ2_DLY_CNT37 24,31

#define SEQ2_MAX     16,31
#define SEQ2_BUS_CNT 0,15

#define SEQ2_DMA_CNT 0,15

#include <bits_opt.h>
#include <bit_field2.h>

static inline void adc_write_reg(volatile unsigned long *reg, unsigned int value)
{
    *reg = value;
}

static inline unsigned int adc_read_reg(volatile unsigned long *reg)
{
    return *reg;
}

static inline void adc_set_bits(volatile unsigned long *reg, int start, int end, unsigned int value)
{
    *reg = set_bit_field(*reg, start, end, value);
}

static inline void adc_set_bit(volatile unsigned long *reg, int bit, unsigned int value)
{
    *reg = set_bit_field(*reg, bit, bit, value);
}

static inline unsigned int adc_get_bits(volatile unsigned long *reg, int start, int end)
{
    return get_bit_field(*reg, start, end);
}

static inline int adc_get_bit(volatile unsigned long *reg, int bit)
{
    return !!test_bit(*reg, bit);
}

#endif /* _ADC_REGS_H_ */


