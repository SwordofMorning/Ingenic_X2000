#ifndef _SOC_TCU_H_
#define _SOC_TCU_H_

#include "tcu_regs.h"

#define MODE_NAME_LEN 64
#define TCU_CHANNEL_NUM   7
#define TCU_FULL_NUM 0xffff
#define TCU_HALF_NUM 0x7000

enum tcu_irq_type {
    NULL_IRQ_MODE,
    FULL_IRQ_MODE,
    HALF_IRQ_MODE,
    FULL_HALF_IRQ_MODE,
    STORE_IRQ_MODE,
};

enum tcu_mode{
    GENERAL_MODE,
    GATE_MODE,
    DIRECTION_MODE,
    QUADRATURE_MODE,
    POS_MODE,
    CAPTURE_MODE,
    FILTER_MODE,
    STORE_MODE,
};

enum tcu_prescale {
    TCU_PRESCALE_1,
    TCU_PRESCALE_4,
    TCU_PRESCALE_16,
    TCU_PRESCALE_64,
    TCU_PRESCALE_256,
    TCU_PRESCALE_1024
};

enum tcu_shutdown {
    RCNT_AFTER_FULL,
    STOP_AFTER_FULL
};

enum tcu_count_mode {
    FULL_CLEAR_ZERO,
    FFFF_CLEAR_ZERO,
    FFFF_HOLD_ON
};

enum tcu_dir_sel{
    DIR_SEL_HH,
    DIR_SEL_CLK,
    DIR_SEL_GPIO0,
    DIR_SEL_GPIO1,
    DIR_SEL_GPIO_QUA
};

enum tcu_gate_sel{
    GATE_SEL_HZ,
    GATE_SEL_CLK,
    GATE_SEL_GPIO0,
    GATE_SEL_GPIO1
};

enum tcu_dir_pola {
    DIR_POLA_LOW,
    DIR_POLA_HIGH
};

enum tcu_gate_pola {
    GATE_POLA_LOW,
    GATE_POLA_HIGH
};

enum tcu_pos_sel{
    GPIO0_POS_CLR = 1,
    GPIO1_POS_CLR,
    GPIO0_NEG_CLR
};

enum tcu_cap_sel{
    CAPTURE_CLK,
    CAPTURE_GPIO0,
    CAPTURE_GPIO1
};

enum signal_pos_neg{
    SIG_INIT,
    SIG_POS_EN,
    SIG_NEG_EN,
    SIG_POS_NEG_EN
};

enum tcu_clksrc {
    TCU_CLKSRC_NULL,
    TCU_CLKSRC_EXT   = EXT_EN,
    TCU_CLKSRC_GPIO0 = GPIO0_EN,
    TCU_CLKSRC_GPIO1 = GPIO1_EN,
    TCU_CLKSRC_STORE = STORE_EN
};

struct tcu_chn {
    int id;                  /* Channel number */
    struct jz_tcu_gpio *gpio0;
    struct jz_tcu_gpio *gpio1;
    struct jz_tcu_gpio *trigger0;
    struct jz_tcu_gpio *trigger1;
    struct ingenic_tcu *tcu;

    int config_flag;
    int enable_flag;
    unsigned int clk_div;  /* 0/1/2/3/4/5/something else------>1/4/16/64/256/1024/mask */

    int shutdown;
    int half_num, full_num;
    int fil_a_num, fil_b_num;
    int capture_num;
    int capture_flag;

    unsigned int str_val;

    int trigger_chn;

    wait_queue_head_t capture_waiter;
    char mode_name[MODE_NAME_LEN];

    enum tcu_irq_type irq_type;
    enum tcu_prescale prescale;
    enum tcu_count_mode count_mode;

    enum tcu_mode mode;
    enum tcu_clksrc clksrc_ext,clksrc_gpio0,clksrc_gpio1,clksrc_store;
    enum signal_pos_neg sig_ext,sig_gpio0,sig_gpio1,sig_store;
    enum tcu_gate_sel gate_sel;
    enum tcu_dir_sel dir_sel;
    enum tcu_pos_sel pos_sel;
    enum tcu_cap_sel cap_sel;
    enum tcu_gate_pola gate_pola;
    enum tcu_dir_pola dir_pola;
};

struct ingenic_tcu {
    unsigned int index;               /* Controller number */
    int is_enable;
    int is_finish;
    struct clk *clk;
    const char *clk_name;
    int irq;
    const char *irq_name;
    struct tcu_chn *channel;
    struct miscdevice tcu_mdev;
    spinlock_t lock;
};

/* enable/start operation */
static inline void tcu_enable_chn_counter(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TESR, 1 << tcu_chn->id);
}

static inline void tcu_disable_chn_counter(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TECR, 1 << tcu_chn->id);
}

static inline void tcu_start_chn_counter(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSCR, 1 << tcu_chn->id);
}

static inline void tcu_stop_chn_counter(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSSR, 1 << tcu_chn->id);
}

/* comparison match flag clr operation */
static inline void tcu_clear_full_flag(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TFCR, 1 << tcu_chn->id);
}

static inline void tcu_clear_half_flag(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TFCR, 1 << (tcu_chn->id + 16));
}

/* comparison match interrupt mask set/clr operation */
static inline void tcu_full_mask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TMSR, 1 << tcu_chn->id);
}

static inline void tcu_half_mask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TMSR, 1 << (tcu_chn->id + 16));
}

static inline void tcu_full_unmask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TMCR, 1 << tcu_chn->id);
}

static inline void tcu_half_unmask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TMCR, 1 << (tcu_chn->id + 16));
}

/*Timer Data FULL/HALF Register*/
static inline void tcu_set_chn_full(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, CH_TDFR(tcu_chn->id), tcu_chn->full_num);
}

static inline void tcu_set_chn_half(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, CH_TDHR(tcu_chn->id), tcu_chn->half_num);
}

/* Counter clear to zero*/
static inline unsigned int tcu_get_tcnt(int index, struct tcu_chn *tcu_chn)
{
    return tcu_read_reg(index, CH_TCNT(tcu_chn->id));
}

static inline void tcu_clear_tcnt(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, CH_TCNT(tcu_chn->id), 0);
}

/* store function */
static inline void tcu_store_flag_set(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSFSR, 1 << tcu_chn->id);
}

static inline void tcu_clear_store_flag(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSFCR, 1 << tcu_chn->id);
}

static inline unsigned int tcu_store_mask_get(int index, struct ingenic_tcu *tcu)
{
    return tcu_read_reg(index, TCU_TSMR);
}

static inline void tcu_store_mask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSMSR, 1 << tcu_chn->id);
}

static inline void tcu_store_unmask(int index, struct tcu_chn *tcu_chn)
{
    tcu_write_reg(index, TCU_TSMCR, 1 << tcu_chn->id);
}

static inline unsigned int tcu_store_get_val(int index, struct tcu_chn *tcu_chn)
{
    return tcu_read_reg(index, TCU_STORE_VAL(tcu_chn->id));
}

static inline void tcu_store_set_filter(int index, struct tcu_chn *tcu_chn,
        unsigned int val)
{
    tcu_write_reg(index, TCU_STORE_FIL_VAL(tcu_chn->id), val & 0x3ff);
}

/* store enable/disable */
static inline void tcu_store_cnt_enable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | STORE_EN);
}

static inline void tcu_store_cnt_disable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr & ~STORE_EN);
}

static inline void tcu_store_neg_enable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | STORE_NEG_EN);
}

static inline void tcu_store_neg_disable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr & ~STORE_NEG_EN);
}

static inline void tcu_store_pos_enable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | STORE_POS_EN);
}

static inline void tcu_store_pos_disable(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id));
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr & ~STORE_POS_EN);
}

/*TCU counter set prescale frequency */
static inline void tcu_set_prescale(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~TCR_PRESCALE_MASK;
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | TCR_PRESCALE(tcu_chn->prescale));
}

/* TCU counter set shutdown */
static inline void tcu_set_shutdown(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tscr;
    tscr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~TCR_SHUTDOWN_MASK;
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tscr | TCR_SHUTDOWN(tcu_chn->shutdown));
}

/* TCU counter set count mode */
static inline void tcu_set_count_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~TCR_COUNT_MODE_MASK;
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | TCR_COUNT_MODE(tcu_chn->count_mode));
}

/*Timer control set  pos and neg*/
static inline void tcu_set_pos(int index, struct tcu_chn *tcu_chn,unsigned int pos)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) | pos;
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr);
}

static inline void tcu_set_neg(int index, struct tcu_chn *tcu_chn,unsigned int neg)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) | neg;
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr);
}

static inline void tcu_set_pos_neg(int index, struct tcu_chn *tcu_chn, unsigned int pos, unsigned int neg)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) | (pos | neg);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr);
}

static inline void tcu_clr_pos_neg(int index, struct tcu_chn *tcu_chn, unsigned int pos, unsigned int neg)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~ (pos | neg);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr);
}

static inline void tcu_config_sig_pos_neg(int index, struct tcu_chn *tcu_chn, enum tcu_clksrc clksrc)
{
    switch (clksrc) {
    case TCU_CLKSRC_EXT:
        tcu_clr_pos_neg(index, tcu_chn, CLK_POS, CLK_NEG);
        switch(tcu_chn->sig_ext) {
        case SIG_POS_EN:
            tcu_set_pos(index, tcu_chn, CLK_POS);
            break;
        case SIG_NEG_EN:
            tcu_set_neg(index, tcu_chn, CLK_NEG);
            break;
        case SIG_POS_NEG_EN:
            tcu_set_pos_neg(index, tcu_chn, CLK_POS, CLK_NEG);
            break;
        default:
            break;
        }
        break;
    case TCU_CLKSRC_GPIO0:
        tcu_clr_pos_neg(index, tcu_chn, GPIO0_POS, GPIO0_NEG);
        switch (tcu_chn->sig_gpio0) {
        case SIG_POS_EN:
            tcu_set_pos(index, tcu_chn, GPIO0_POS);
            break;
        case SIG_NEG_EN:
            tcu_set_neg(index, tcu_chn, GPIO0_NEG);
            break;
        case SIG_POS_NEG_EN:
            tcu_set_pos_neg(index, tcu_chn, GPIO0_POS, GPIO0_NEG);
            break;
        default:
            break;
        }
        break;
    case TCU_CLKSRC_GPIO1:
        tcu_clr_pos_neg(index, tcu_chn, GPIO1_POS, GPIO1_NEG);
        switch (tcu_chn->sig_gpio1) {
        case SIG_POS_EN:
            tcu_set_pos(index, tcu_chn, GPIO1_POS);
            break;
        case SIG_NEG_EN:
            tcu_set_neg(index, tcu_chn, GPIO1_NEG);
            break;
        case SIG_POS_NEG_EN:
            tcu_set_pos_neg(index, tcu_chn, GPIO1_POS, GPIO1_NEG);
            break;
        default:
            break;
        }
        break;
    case TCU_CLKSRC_STORE:
        tcu_clr_pos_neg(index, tcu_chn, STORE_POS_EN, STORE_NEG_EN);
        switch (tcu_chn->sig_store) {
        case SIG_POS_EN:
            tcu_set_pos(index, tcu_chn, STORE_POS_EN);
            break;
        case SIG_NEG_EN:
            tcu_set_neg(index, tcu_chn, STORE_NEG_EN);
            break;
        case SIG_POS_NEG_EN:
            tcu_set_pos_neg(index, tcu_chn, STORE_POS_EN, STORE_NEG_EN);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* TCU select timer clock input and counting mode pos or neg */
static inline void tcu_set_clksrc(int index, struct tcu_chn *tcu_chn)
{
    enum tcu_clksrc ext = tcu_chn->clksrc_ext;
    enum tcu_clksrc gpio0 = tcu_chn->clksrc_gpio0;
    enum tcu_clksrc gpio1 = tcu_chn->clksrc_gpio1;
    enum tcu_clksrc store = tcu_chn->clksrc_store;

    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~(EXT_EN | GPIO0_EN | GPIO1_EN | STORE_EN);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | ext | gpio0 | gpio1 | store);

    tcu_config_sig_pos_neg(index, tcu_chn, ext);
    tcu_config_sig_pos_neg(index, tcu_chn, gpio0);
    tcu_config_sig_pos_neg(index, tcu_chn, gpio1);
    tcu_config_sig_pos_neg(index, tcu_chn, store);
}

#endif /* _SOC_TCU_H_ */