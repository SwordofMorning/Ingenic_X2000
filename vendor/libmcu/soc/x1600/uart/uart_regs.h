
#define URBR  0x000
#define UTHR  0x000
#define UDLLR 0x000
#define UDLHR 0x004
#define UIER  0x004
#define UIIR  0x008
#define UFCR  0x008
#define ULCR  0x00c
#define UMCR  0x010
#define ULSR  0x014
#define UMSR  0x018
#define USPR  0x01c
#define ISR   0x020
#define UMR   0x024
#define UACR  0x028
#define URCR  0x040
#define UTCR  0x044
#define URFCR 0x48
#define URTDCR 0x050

/* UART 中断使能寄存器*/
#define UIER_RTOIE 4, 4 /* 接收超时中断使能，1有效*/
#define UIER_MSIE  3, 3 /* 调制解调器状态中断使能，1*/
#define UIER_RLSIE 2, 2
#define UIER_TDRIE 1, 1
#define UIER_RDRIE 0, 0

/* UART 中断识别寄存器*/
#define UIIR_FFMSEL 6, 7
#define UIIR_INID   1, 3
#define UIIR_INPEND 0, 0

/* UART FIFO控制寄存器*/
#define UFCR_RDTR 6, 7
#define UFCR_UME  4, 4
#define UFCR_DME  3, 3
#define UFCR_TFRT 2, 2
#define UFCR_RFRT 1, 1
#define UFCR_FME  0, 0

/* UART 线路控制寄存器*/
#define ULCR_DLAB 7, 7
#define ULCR_SBK  6, 6
#define ULCR_STPAR 5, 5
#define ULCR_PARM 4, 4
#define ULCR_PARE 3, 3
#define ULCR_SBLS 2, 2
#define ULCR_WLS 0, 1

/* UART 调制解调器控制寄存器*/
#define UMCR_MDCE 7, 7
#define UMCR_FCM 6, 6
#define UMCR_LOOP 4, 4
#define UMCR_RTS 1, 1

/* UART 线路状态寄存器*/
#define ULSR_FIFOE 7, 7
#define ULSR_TEMP 6, 6
#define ULSR_TDRQ 5, 5
#define ULSR_BI 4, 4
#define ULSR_FMER 3, 3
#define ULSR_PARER 2, 2
#define ULSR_OVER 1, 1
#define ULSR_DRY 0, 0

/* UART 调制解调器状态寄存器*/
#define UMSR_CTS 4, 4
#define UMSR_CCTS 0, 0

/* */
#define UART_FIFO_LEN 64
#define UART_NUMS 4
