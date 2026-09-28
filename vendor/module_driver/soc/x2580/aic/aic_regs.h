#ifndef _AIC_REGS_H_
#define _AIC_REGS_H_

#define TX_FIFO_SIZE 64
#define RX_FIFO_SIZE 32
#define RX_FIFO_TRIGGER (RX_FIFO_SIZE / 2)
#define TX_FIFO_TRIGGER (TX_FIFO_SIZE / 2)

#define AICFR 0x00
#define AICCR 0x04
#define I2SCR 0x10
#define AICSR 0x14
#define I2SSR 0x1C
#define I2SDIV 0x30
#define AICDR 0x34
#define AICLR 0x38
#define AICTFLR 0x3C

/* AICFR */
#define rg_codec_srst 31, 31
#define ad2da_loop 28, 28
#define RFTH 24, 27
#define TFTH 16, 20
#define RST_O_STATUS 14, 14
#define RST_I_STATUS 13, 13
#define MSB 12, 12
#define DMODE 8, 8
#define LSMP 6, 6
#define ICDC 5, 5
#define AUSEL 4, 4
#define RST 3, 3
#define BCKD 2, 2
#define SYNCD 1, 1
#define ENB 0, 0

/* AICCR */
#define ETFLOR 31, 31
#define ETFLS 30, 30
#define en2old 29, 29
#define PACK16 28, 28
#define excodec 27, 27
#define CHANNEL 24, 26
#define ETFL 23, 23
#define TLDMS 22, 22
#define OSS 19, 21
#define ISS 16, 18
#define RDMS 15, 15
#define TDMS 14, 14
#define MONOCTR 12, 13
#define M2S 11, 11
#define ENDSW 10, 10
#define ASVTSU 9, 9
#define TFLUSH 8, 8
#define RFLUSH 7, 7
#define EROR 6, 6
#define ETUR 5, 5
#define ERFS 4, 4
#define ETFS 3, 3
#define ENLBF 2, 2
#define ERPL 1, 1
#define EREC 0, 0

/* I2SCR */
#define firstchn_is_left 31, 31
#define RFIRST 17, 17
#define SWLH 16, 16
#define AMSL 0, 0

/* AICSR */
#define RFL 24, 29
#define TFLL 15, 20
#define TFL 8, 14
#define ROR 6, 6
#define TUR 5, 5
#define RFS 4, 4
#define TFS 3, 3
#define TFLOR 2, 2
#define TFLS 1, 1

/* I2SSR */
#define CHBSY 5, 5
#define TBSY 4, 4
#define RBSY 3, 3
#define BSY 2, 2

/* I2SDIV */
#define IDV 17, 24
#define DV 1, 8

#endif /* _AIC_REGS_H_ */
