#ifndef __NEMC_REG_H
#define __NEMC_REG_H

#define SMC0R1 0x0014
#define SMC0R2 0x0018

#define SMC1R1 0x0054
#define SMC1R2 0x0058

#define SARC1 0x0034
#define SARC2 0x0038

#define TCH     24, 27
#define TAW     23, 20
#define TBP     16, 19
#define TRAS    12, 15
#define TWAS    8, 11
#define BW      6, 7
#define BL      4, 5
#define SMT     0, 1

#define WP_EN   31, 31
#define TIMDOUBLE   30, 30
#define STRV    24, 29
#define TRDRV   20, 23
#define TWRRV   16, 19
#define TAVDH   8, 11
#define TAVDP   4, 7
#define TAVDS   0, 3

#define NEMC_ADDR(reg)  ((volatile unsigned long *)(NEMC_IOBASE + reg))

void nemc_init_gpio(struct nemc_config config);

#endif