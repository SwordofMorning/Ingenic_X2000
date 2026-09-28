#ifndef _SOC_CAN_REGS_H_
#define _SOC_CAN_REGS_H_

#define CANMODE     0x00
#define CANCMD      0x04
#define CANSTAT     0x08
#define CANINT      0x0C
#define CANBTR      0x10
#define CANFLT      0x18
#define CANERR      0x1C
#define CANAFID0    0x20
#define CANAFMK0    0x24
#define CANAFID1    0x28
#define CANAFMK1    0x2C
#define CANAFID2    0x30
#define CANAFMK2    0x34
#define CANAFID3    0x38
#define CANAFMK3    0x3C
#define CANTFIR     0x40
#define CANTXID     0x44
#define CANTXDATA0  0x48
#define CANTXDATA1  0x4C
#define CANRFIR     0x90
#define CANRXID     0x94
#define CANRXDATA0  0x98
#define CANRXDATA1  0x9C
#define CANSL       0xFC

/* CANMODE */
#define CANMODE_RXCNT4 14, 14
#define CANMODE_TXAR 13, 13
#define CANMODE_SLEEP 9, 9
#define CANMODE_DMAEN 8, 8
#define CANMODE_ERAR 3, 3
#define CANMODE_STE 2, 2
#define CANMODE_LOW 1, 1
#define CANMODE_RSTM 0, 0

/* CANCMD */
#define CANCMD_CTB 6, 6
#define CANCMD_OFR 5, 5
#define CANCMD_SRR 4, 4
#define CANCMD_CDO 3, 3
#define CANCMD_RRB 2, 2
#define CANCMD_AT 1, 1
#define CANCMD_TR 0, 0

/* CANSTAT */
#define CANSTAT_TBF 8, 8
#define CANSTAT_BOS 7, 7
#define CANSTAT_ES 6, 6
#define CANSTAT_TS 5, 5
#define CANSTAT_RS 4, 4
#define CANSTAT_TCS 3, 3
#define CANSTAT_TBS 2, 2
#define CANSTAT_DOS 1, 1
#define CANSTAT_RBS 0, 0

/* CANINT */
#define CANINT_ALL 16, 27
#define CANINT_OFIE 27, 27
#define CANINT_TBIE 26, 26
#define CANINT_DMAIE 25, 25
#define CANINT_WKIE 24, 24
#define CANINT_BEIE 23, 23
#define CANINT_ALIE 22, 22
#define CANINT_EPIE 21, 21
#define CANINT_BOIE 20, 20
#define CANINT_DOIE 19, 19
#define CANINT_EIE 18, 18
#define CANINT_TIE 17, 17
#define CANINT_RIE 16, 16
#define CANINT_OFI 11, 11
#define CANINT_TBI 10, 10
#define CANINT_DMAI 9, 9
#define CANINT_WKI 8, 8
#define CANINT_BEI 7, 7
#define CANINT_ALI 6, 6
#define CANINT_EPI 5, 5
#define CANINT_BOI 4, 4
#define CANINT_DOI 3, 3
#define CANINT_EI 2, 2
#define CANINT_TI 1, 1
#define CANINT_RI 0, 0

/* CANBTR */
#define CANBTR_SAW 15, 15
#define CANBTR_TSEG2 12, 14
#define CANBTR_TSEG1 8, 11
#define CANBTR_SJW 6, 7
#define CANBTR_CANCS 0, 5

/* CANFLT */
#define CANFLT_ALC 16, 20
#define CANFLT_FMS 8, 11
#define CANFLT_FTER 0, 3

/* CANERR */
#define CANERR_CANTEC 24, 31
#define CANERR_CANREC 16, 23
#define CANERR_CANEWLR 8, 15
#define CANERR_ERRC 6, 7
#define CANERR_DIR 5, 5
#define CANERR_SEG 0, 4

/* CANAFID0/CANAFID1/CANAFID2/CANAFID3 */
#define CANAFIDx_CANAFID 0, 28

/* CANAFMK0/CANAFMK1/CANAFMK2/CANAFMK3 */
#define CANAFMKx_CANAFMK 0, 28

/* CANTFIR */
#define CANTFIR_FF 7, 7
#define CANTFIR_RTR 6, 6
#define CANTFIR_DLC 0, 3

/* CANTXID */
#define CANTXID_CANTXID 0, 28

/* CANTXDATA0/CANTXDATA1 */
#define CANTXDATAx_CANTXDATA 0, 31

/* CANRFIR */
#define CANRFIR_RXFMS 16, 19
#define CANRFIR_FF 7, 7
#define CANRFIR_RTR 6, 6
#define CANRFIR_DLC 0, 3

/* CANRXID */
#define CANRXID_CANRXID 0, 31

/* CANRXDATA0/CANRXDATA1 */
#define CANRXDATAx_CANRXDATA 0, 31

/* CANSL */
#define CANSL_IPRESET 31, 31
#define CANSL_CANTXOFF 25, 25
#define CANSL_CANSLEN 24, 24
#define CANSL_CANSLKEY 16, 19

#endif /* _CAN_REGS_H_ */
