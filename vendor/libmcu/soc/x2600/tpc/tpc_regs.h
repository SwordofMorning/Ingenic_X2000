#ifndef __TPC_REGS_H__
#define __TPC_REGS_H__

#include <soc/base.h>
#include <bit_field2.h>

#define TPC_IOBASE 0x13620000
#define TPC_ADDR(reg)   ((volatile unsigned long *)(TPC_IOBASE + reg))

#define PFCENS              0x0000
#define PFCENC              0x0004
// #define PFCEN               0x0008

#define PDENS               0x000c
#define PDENC               0x0010
#define PDEN                0x0014

#define PMENS(id)           (0x0018+id*0x0c)
#define PMENC(id)           (0x001c+id*0x0c)
#define PMEN(id)            (0x0020+id*0x0c)


#define PDSCD               0x0030
#define PAFC                0x0040
#define PDSMC               0x0044
#define PDLMC               0x0048
#define PDDMC               0x004c

#define PMMC(id)            (0x0050+id*0x04)

#define PFSM                0x0058
#define PIOC                0x005c
#define PDSBC               0x0060
#define PDSWC               0x0064
#define PDSDF               0x0068
#define PDSDFC              0x006c
#define PDLBC               0x0070
#define PDLWC               0x0074
#define PDLAC               0x0078
#define PDDBC               0x007c
#define PDDWC               0x0080
#define PDDMAC              0x0084
#define PDDTF               0x0088
#define PDDTFC              0x008C


#define PMBC(id)               (0x0090+id*0x20)
#define PMSSC(id)              (0x0094+id*0x20)
#define PMESC(id)              (0x0098+id*0x20)
#define PMMSC(id)              (0x009c+id*0x20)
#define PMTF(id)               (0x00a0+id*0x20)
#define PMTFC(id)              (0x00a4+id*0x20)
#define PMRIM0(id)             (0x00a8+id*0x20)
#define PMRIM1(id)             (0x00ac+id*0x20)


#define PMCMS               0x00d0
#define PMCILC              0x00d4
#define PMCSLC              0x00d8
#define PMCELC              0x00dc


#define PMCRLC0(io)            (0x00e0+io*0x08)
#define PMCRLC1(io)            (0x00e4+io*0x08)


#define PINTS               0x0130
#define PINTM0              0x0134
#define PINTM1              0x0138
#define PFFS                0x013c

#define PWDTF               0x0120
#define PWDTFE              0x0124
#define PWDTV               0x0128
#define PWDTC               0x012c



#define PAFENS          0,0
#define PAFENC          0,0
#define PFCEN           0,0

#define PDAEN           0,0
#define PDAAEN          1,1
#define PDMEN           2,2
#define PDSEN           3,3
#define PDLEN           4,4
#define PDDEN           5,5

#define PMAEN          0,0
#define PMAAEN         1,1
#define PMMEN          2,2
#define PMCEN(io)      16+io,16+io


#define PRESCALE        0,7

#define SAT          0,0
#define QSTP         1,1

#define PMFS            0,3
#define PSFTS           4,7
#define PLATS           8,11
#define PDSTS           12,15
#define PDSS            16,19
#define PSM0S           20,23
#define PSM1S           24,27

#define PSFTCLK           0,0
#define PSFTD(io)         1+io,1+io
#define PLAT              9,9
#define PDST(io)          10+io,10+io
#define PSMC(io)          18+io,18+io


#define PDSIL             0,0
#define PDSDE             1,1
#define PDSPHA            2,2
#define PDSPOL            3,3
#define PDSTUP            4,5
#define PDHD              6,7
#define PDSCN             8,11
#define PDSDI             15,15
#define PDSBL             16,24

#define PDSWCNT           0,23

#define PDSDATA           0,31

#define PDSDFF            0,0
#define PDSD_THRSHD       16,23

#define PDLIL             0,0

#define PDLWCNT           0,23

#define PDLACNT           0,23

#define PDLIL             0,0
#define PDDCN             8,11

#define PDDWCNT           0,23

#define PDDMCNT           0,23

#define PDDDATA           0,31

#define PDDTFF            0,0
#define PDDT_THRSHD       16,19

#define PMRSN            0,6
#define PMASI            8,14

#define PMSCNT           0,31

#define PMECNT           0,31

#define PMMCNT           0,31

#define PMTDATA          0,31

#define PMTFF            0,0
#define PMT_THRSHD       16,21 /*19还是21呀*/

#define PM_INTM0         0,31
#define PM_INTM1         0,31

#define PMCMS_SELECT      0,0

#define PMC0IL            0,0
#define PMC1IL            1,1
#define PMC2IL            2,2
#define PMC3IL            3,3
#define PMC4IL            4,4
#define PMC5IL            5,5
#define PMC6IL            6,6
#define PMC7IL            7,7


#define PMC0SL            0,0
#define PMC1SL            1,1
#define PMC2SL            2,2
#define PMC3SL            3,3
#define PMC4SL            4,4
#define PMC5SL            5,5
#define PMC6SL            6,6
#define PMC7SL            7,7

#define PMC0EL            0,0
#define PMC1EL            1,1
#define PMC2EL            2,2
#define PMC3EL            3,3
#define PMC4EL            4,4
#define PMC5EL            5,5
#define PMC6EL            6,6
#define PMC7EL            7,7

#define PDFINT              0,0
#define PSFINT              1,1
#define PLFINT              2,2
#define PDDFINT             3,3 /*加热*/
#define PSFUINT             4,4
#define PDFUINT             5,5
#define PDOINT              6,6
#define SM0FINT             8,8
#define SM0SSINT            9,9
#define SM0RSINT            10,10
#define SM0UINT             11,11
#define SM0OINT             12,12
#define SM1FINT             16,16
#define SM1SSINT            17,17
#define SM1RSINT            18,18
#define SM1UINT             19,19
#define SM1OINT             20,20
#define PFFINT              24,24
#define PFAINT              25,25
#define WDTHINT             30,30
#define WDTFINT             31,31

#define PSFE                0,0
#define PSFF                1,1
#define PDFE                2,2
#define PDFF                3,3
#define PSMFE(id)           4+id*2,4+id*2
#define PSMFF(id)           5+id*2,5+id*2


static inline void tpc_write_reg(unsigned int reg, unsigned int value)
{
    *TPC_ADDR(reg) = value;
}

static inline unsigned int tpc_read_reg(unsigned int reg)
{
    return *TPC_ADDR(reg);
}

static inline void tpc_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field_v(TPC_ADDR(reg), start, end, val);
}

static inline unsigned int tpc_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field_v(TPC_ADDR(reg), start, end);
}

static inline void tpc_clear_interrupt(int start, int end, unsigned int val)
{
    unsigned long flags = 0;
    set_bit_field_v(&flags, start, end, val);
    tpc_write_reg(PINTS, flags);
}

#endif