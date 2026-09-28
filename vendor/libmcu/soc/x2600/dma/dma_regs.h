#ifndef _DMA_REGS_H_
#define _DMA_REGS_H_

/* ============ base: 0x13420000 & 0x13660000 ============ */
#define DSA(n) (0x00 + (n) * 0x20) /* channel n Source Address */
#define DTA(n) (0x04 + (n) * 0x20) /* channel n Target Address */
#define DTC(n) (0x08 + (n) * 0x20) /* channel n Transfer Count */
#define DRT(n) (0x0C + (n) * 0x20) /* channel n Request Source */
#define DCS(n) (0x10 + (n) * 0x20) /* channel n Control/Status */
#define DCM(n) (0x14 + (n) * 0x20) /* channel n Command */
#define DDA(n) (0x18 + (n) * 0x20) /* channel n Descriptor Address */
#define DSD(n) (0x1C + (n) * 0x20) /* channel n Stride Difference */

#define DMAC   0x1000 /* DMA Control */
#define DIRQP  0x1004 /* DMA Interrupt Pending */
#define DDB    0x1008 /* DMA Doorbell */
#define DDS    0x100C /* DMA Doorbell Set */
#define DIP    0x1010 /* Descriptor Interrupt Pending */
#define DIC    0x1014 /* Descriptor Interrupt Clear */
#define DMACP  0x101C /* DMA Channel Programmable, only valid in PDMA, keep value 0 in DMA_MCU */

#define DSIRQP  0x1020 /* Channel soft IRQ to MCU, only valid in PDMA, set in DMA_MCU have no effect */
#define DSIRQM  0x1024 /* Channel soft IRQ mask, only valid in PDMA, set in DMA_MCU have no effect */
#define DCIRQP  0x1028 /* Channel IRQ to MCU, only valid in PDMA, set in DMA_MCU have no effect */
#define DCIRQM  0x102C /* Channel IRQ to MCU mask, only valid in PDMA, set in DMA_MCU have no effect */
#define SACIDX0 0x1060 /* DMA Channel Source Address Compare INDEX 0, wrap start address, only for can */
#define SACIDX1 0x1064 /* DMA Channel Source Address Compare INDEX 1, wrap start address, only for can */
#define DACIDX0 0x1068 /* DMA Channel Destination Address Compare INDEX 0, wrap start address, only for can */
#define DACIDX1 0x106c /* DMA Channel Destination Address Compare INDEX 1, wrap start address, only for can */

/* ================== base: 0x13420000 ================== */
#define DMCS    0x1030 /* MCU Control and Status */
#define DMNMB   0x1034 /* MCU Normal Mailbox */
#define DMSMB   0x1038 /* MCU Security Mailbox */
#define DMINT   0x103C /* MCU Interrupt */

/* DCSn */
#define DCS_NDES 31, 31
#define DCS_DES8 30, 30
#define DCS_CDOA 8, 15
#define DCS_STOP 5, 5
#define DCS_AR 4, 4
#define DCS_TT 3, 3
#define DCS_HLT 2, 2
#define DCS_CTE 0, 0

/* DCMn */
#define DCM_SAI 23, 23
#define DCM_DAI 22, 22
#define DCM_SAIW 21, 21
#define DCM_DAIW 20, 20
#define DCM_RDIL 16, 19
#define DCM_SP 14, 15
#define DCM_DP 12, 13
#define DCM_TSZ 8, 11
#define DCM_SACIDX 7, 7
#define DCM_DACIDX 6, 6
#define DCM_STDE 2, 2
#define DCM_TIE 1, 1
#define DCM_LINK 0, 0

/* DDAn */
#define DDA_DBA 12, 31
#define DDA_DOA 4, 11

/* DSDn */
#define DSD_TSD 16, 31
#define DSD_SSD 0, 15

/* DMAC */
#define DMAC_FSSI 30, 30
#define DMAC_FUART 28, 28
#define DMAC_FAIC 27, 27
#define DMAC_HLT 3, 3
#define DMAC_AR 2, 2
#define DMAC_CH01 1, 1
#define DMAC_DMAE 0, 0

#define DTC_DOA 24, 31
#define DTC_DTC 0, 23

#endif /* _DMA_REGS_H_ */
