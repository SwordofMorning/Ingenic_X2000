#ifndef _GPIO_REGS_H_
#define _GPIO_REGS_H_

#define GPIO_PORT_OFF    0x1000
#define GPIO_SHADOW_OFF  0x7000

#define PXPIN      0x00   /* PIN Level Register */
#define PXINT      0x10   /* Port Interrupt Registers */
#define PXINTS     0x14   /* Port Interrupt Set Register */
#define PXINTC     0x18   /* Port Interrupt Clear Register */
#define PXMSK      0x20   /* Port Interrupt Mask Reg */
#define PXMSKS     0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC     0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1     0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S    0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C    0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0     0x40   /* Port Pattern 0 Register */
#define PXPAT0S    0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C    0x48   /* Port Pattern 0 Clear Register */
#define PXFLG      0x50   /* Port Flag Register */
#define PXFLGC     0x58   /* Port Flag clear Register */

#define PXPUEN     0x110  /* Port Pull-up Function State Register */
#define PXPUENS    0x114  /* Port Pull-up Function Set Register, they are write-only Registers */
#define PXPUENC    0x118  /* Port Pull-up Function clear Register, they are write-only Registers */
#define PXPDEN     0x120  /* Port Pull-down Function Register */
#define PXPDEHS    0x124  /* Port Pull-down Function set Register, they are write-only Registers */
#define PXPDEHC    0x128  /* Port Pull-down Function clear Register, they are write-only Registers */

#define PZGID2LD   0xF0   /* GPIOZ Group ID to load */

#endif /* _GPIO_REGS_H_ */
