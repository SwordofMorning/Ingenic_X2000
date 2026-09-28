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
#define AICFR2 0x3C
#define CDCCR 0x80

/* AICFR */
#define RFTH 24, 27
#define TFTH 16, 20
#define TRST 14, 14
#define RRST 13, 13
#define MSB 12, 12
#define LSMP 6, 6
#define ICDC 5, 5
#define AUSEL 4, 4
#define TMASTER 2, 2
#define ENB 0, 0

/* AICCR */
#define ETFLOR 31, 31
#define ETFLS 30, 30
#define PACK16 28, 28
#define CHANNEL 24, 26
#define ETFL 23, 23
#define TLDMS 22, 22
#define OSS 19, 21
#define ISS 16, 18
#define RDMS 15, 15
#define TDMS 14, 14
#define MONOCTR 12, 13
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
#define FstChnlLeft 31, 31
#define RFIRST 17, 17
#define SWLH 16, 16
#define AMSL 0, 0

/* AICSR */
#define RFL 24, 29
#define TFLL 15, 20
#define TFL 8, 13
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
#define MDIV 27, 31
#define TDIV 0, 8

/* AICDR */
#define DATA 0, 23

/* AICLR */
#define DATALP 0, 23

/* AICFR2 */
#define TFLTH 16, 19
#define RPL_EXEN 1, 1
#define REC_EXEN 0, 0

/* CDCCR */
#define AD2DA_LOOP 4, 4
#define CODEC_SRST 0, 0

#endif /* _AIC_REGS_H_ */
