#ifndef __TIZIANO_CORE_H__
#define __TIZIANO_CORE_H__
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <tiziano-core-ctrl.h>

#include "tiziano-params.h"
#include "tiziano-msca.h"

#define TISP_VERSION_SIZE 8
#define TISP_VERSION_ID "1.00"
#define TISP_PRIV_PARAM_FLAG_SIZE       32
#define SOC_TYPE_SIZE 8

#define ISPSUBABS(A,B) ((A>B)?(A-B):(B-A))
#define ISPADDABS(X,X1,X2) ((X1>X2)?(X1-X):(X1+X))
#define ISPINT(X,X1,X2,Y1,Y2)    (ISPADDABS((ISPSUBABS(Y1,Y2) * ISPSUBABS(X,X1) / ISPSUBABS(X1,X2)),Y1,Y2))
/* #define RATIO_INTE(ori, ratio, max) ((ratio > 128)?((ratio-128)*(max>ori?max-ori:0)/128+ori):(ori*ratio/128)) */
#define RATIO_INTE(ori, ratio, min, max) \
        ((ratio > 128) ? \
         (ratio-128)*((max>ori&&min<ori)?max-ori:0)/128 + ori : \
         ratio*((max>ori&&min<ori)?ori-min:0)/128 + min)

#define    MODULES        (0)
#define    TOP_MODULE     (1)
#define    AE_MODULE      (2)
#define    AWB_MODULE     (3)
#define    BLC_MODULE     (4)
#define    WDR_MODULE     (5)
#define    TMO_MODULE     (6)
#define    DPC_MODULE     (7)
#define    LSC_MODULE     (8)
#define    GIB_MODULE     (9)
#define    ADR_MODULE     (10)
#define    DMSC_MODULE    (11)
#define    CCM_MODULE     (12)
#define    GAMMA_MODULE   (13)
#define    DEFOG_MODULE   (14)
#define    LCE_MODULE     (15)
#define    MDNS_MODULE    (16)
#define    YDNS_MODULE    (17)
#define    CDNS_MODULE    (18)
#define    BCSH_MODULE    (19)
#define    CLM_MODULE     (20)
#define    YSP_MODULE     (21)
#define    SDNS_MODULE    (22)
#define    HLDC_MODULE    (23)
#define    AF_MODULE      (24)
#define    MSCA_MODULE    (25)
#define    RESERVE_MODULE (26)

//wdr 当前src中是否包含wdr参数
//wdr_switch 如果日夜切换，配置为0，如果wdr，现行切换，配置为1
//wdr_en 当前wdr是否使能
// TISP_MODULE_PARAMS_COPY(wdr_en         , dnwp , wdr_switch , tparamsP[vinum] , ae_ev_list_l , src , tisp_ae_par , status , AE_MODULE , sizeof(struct tisp_ae) , sizeof(struct tisp_ae_process   ));
#define TISP_MODULE_PARAMS_COPY_1(wdr_en , wdr  , wdr_switch , parent          , begain, src , module      , status , num       , size, psize) \
        if((status >> num) & 0x1){                                      \
                if(wdr_switch == 0){                                    \
                        memcpy((void *)&(parent->module), (void *)src, 2 * psize - size); \
                }                                                       \
                if(wdr_en == 1){                                        \
                        memcpy((void *)&(parent->module.begain), (void *)src + psize, size - psize); \
                } else if (wdr_en == 0){                                \
                        memcpy((void *)&(parent->module.begain), (void *)(src + 2 * psize - size), size - psize); \
                }                                                       \
                src += wdr ? size : psize;}

// TISP_MODULE_PARAMS_COPY(       wdr_en , dnwp , wdr_switch, tparamsP[vinum] , top_lin_bypass, top_wdr_bypass, src , tisp_top_par     , status , TOP_MODULE     , sizeof(struct tisp_top)     , sizeof(struct tisp_top_process  ));
#define TISP_MODULE_PARAMS_COPY(wdr_en , wdr, wdr_switch, parent, begain, wdr_begain, src, module, status , num, size, psize) \
        if((status >> num) & 0x1){                                      \
                if(wdr_switch == 0){                                    \
                        memcpy((void *)&(parent->module), (void *)&(src->module), psize); \
                }                                                       \
                if(wdr_en == 1) {                                       \
                        memcpy((void *)&(parent->module.begain), (void *)&(src->module.wdr_begain), (int)&(src->module.wdr_begain) - (int)&(src->module.begain)); \
                } else if (wdr_en == 0){                                \
                        if (1 == wdr) {                                 \
                                memcpy((void *)&(parent->module.begain), (void *)&(src->module.begain), (int)&(src->module.wdr_begain) - (int)&(src->module.begain)); \
                        }                                               \
                }                                                       \
        }

#ifdef CONFIG_PM

enum {
        TISP_PM_AE,
        TISP_PM_AWB,
        TISP_PM_BLC,
        TISP_PM_GIB,
        TISP_PM_LSC,
        TISP_PM_TMO,
        TISP_PM_DPC,
        TISP_PM_ADR,
        TISP_PM_DMSC,
        TISP_PM_GAMMA,
        TISP_PM_DEFOG,
        TISP_PM_LCE,
        TISP_PM_MDNS,
        TISP_PM_AF,
        TISP_PM_YDNS,
        TISP_PM_CDNS,
        TISP_PM_SDNS,
        TISP_PM_YSP,
        TISP_PM_BCSH,
        TISP_PM_CLM,
        TISP_PM_CCM,
        TISP_PM_GSM,    /**< no deinit */
        TISP_PM_HLDC,
        TISP_PM_TSTP,
        TISP_PM_RAW,    /**< no deinit */
        TISP_PM_CSC,    /**< no deinit */
        TISP_PM_MSCA,

        TISP_PM_WDR,
        TISP_PM_BUTT,
};

typedef struct {
        int32_t (*g_size)(int32_t vinum, uint32_t *size);
        int32_t (*suspend)(int32_t vinum, uint32_t **addr);
        int32_t (*resume)(int32_t vinum, uint32_t **addr);
} tisp_pm_cb;
extern tisp_pm_cb tpm_cb[TISP_PM_BUTT];

#define tisp_pm_call(m, f, vinum, args...)         \
        (!(m) ? -EFAULT : (!((m)->f) ? -EFAULT : (m)->f(vinum, ##args)))

#endif /* CONFIG_PM */

extern tisp_params_data_t *tparams_day[2];
extern tisp_params_data_t *tparams_night[2];
extern tisp_params_process_data_t *tparamsP[2];
extern int deir_en[2];
extern tisp_init_dnw_t dnw;
extern unsigned int top_bypass_global[2];
extern tisp_show_bin *tsbin[2];

typedef struct tisp_private_parameters_header{
        char soc[SOC_TYPE_SIZE];
        char flag[TISP_PRIV_PARAM_FLAG_SIZE];
        unsigned int size;
        unsigned int day_size;
        unsigned int night_size;
        unsigned int crc;
} TXispPrivParamHeader;

typedef struct tiziano_parameters_manger {
        char version[TISP_VERSION_SIZE];
        TXispPrivParamHeader header;
        void *data;                                                             //the base address of all data.
        unsigned int data_size;
        void *base_buf;                                                 //the address of private0 data.
} TISPParamManage;

int system_reg_write(unsigned int reg, unsigned int value);
unsigned int  system_reg_read(unsigned int reg);
void tisp_params_copy(int vinum, tisp_params_data_t *data, int wdr_en, int wdr_switch, int dnwp);
void tisp_bypass_update(int vinum);
void tisp_ipc_triger(void);
#endif /* __TIZIANO_CORE_H__ */