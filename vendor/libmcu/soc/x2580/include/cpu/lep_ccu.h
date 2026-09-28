#ifndef _LEP_CCU_H_
#define _LEP_CCU_H_

#include <cpu/io.h>

#define LEP_CCU_BASE               0x12a00000

#define CCU_CCSR           io_addr(LEP_CCU_BASE + 0x0000)
#define CCU_CRER           io_addr(LEP_CCU_BASE + 0x0004)
#define CCU_FROM_HOST      io_addr(LEP_CCU_BASE + 0x0008)
#define CCU_TO_HOST        io_addr(LEP_CCU_BASE + 0x000C)
#define CCU_TIME_L         io_addr(LEP_CCU_BASE + 0x0010)
#define CCU_TIME_H         io_addr(LEP_CCU_BASE + 0x0014)
#define CCU_TIME_CMP_L     io_addr(LEP_CCU_BASE + 0x0018)
#define CCU_TIME_CMP_H     io_addr(LEP_CCU_BASE + 0x001C)
#define CCU_INTC_MASK_L    io_addr(LEP_CCU_BASE + 0x0020)
#define CCU_INTC_MASK_H    io_addr(LEP_CCU_BASE + 0x0024)
#define CCU_INTC_PEND_L    io_addr(LEP_CCU_BASE + 0x0028)
#define CCU_INTC_PEND_H    io_addr(LEP_CCU_BASE + 0x002c)

#define CCU_PMA_ADR_0      io_addr(LEP_CCU_BASE + 0x0040)
#define CCU_PMA_ADR_1      io_addr(LEP_CCU_BASE + 0x0044)
#define CCU_PMA_ADR_2      io_addr(LEP_CCU_BASE + 0x0048)
#define CCU_PMA_ADR_3      io_addr(LEP_CCU_BASE + 0x004c)
#define CCU_PMA_CFG_0      io_addr(LEP_CCU_BASE + 0x0060)
#define CCU_PMA_CFG_1      io_addr(LEP_CCU_BASE + 0x0064)
#define CCU_PMA_CFG_2      io_addr(LEP_CCU_BASE + 0x0068)
#define CCU_PMA_CFG_3      io_addr(LEP_CCU_BASE + 0x006c)

#define CFCR_LEP_Reset     31, 31

#define CCSR_Timer_en      5, 5
#define CCSR_Reset         4, 4
#define CCSR_Sleep         3, 3
#define CCSR_Bus_idle      2, 2
#define CCSR_Bus_mask      1, 1
#define CCSR_IE            0, 0

#endif /* _LEP_CCU_H_ */
