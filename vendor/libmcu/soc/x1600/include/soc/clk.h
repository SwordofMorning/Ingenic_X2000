#ifndef _SOC_CLK_H_
#define _SOC_CLK_H_

struct clk {
    const char *name;
    unsigned long rate;
    unsigned char id;
    unsigned char type;
    unsigned char type_value;
    unsigned char parent;
    unsigned char count;
    unsigned char is_init_enabled;
};

enum {
    CLK_ID_EXT     = 0,
    CLK_ID_EXT0,
    CLK_ID_EXT1,

    CLK_ID_PLL,
    CLK_ID_APLL,
    CLK_ID_MPLL,
    CLK_ID_EPLL,

    CLK_ID_CPPCR,
    CLK_ID_SCLK_A,
    CLK_ID_CCLK,
    CLK_ID_L2CLK,
    CLK_ID_H0CLK,
    CLK_ID_H2CLK,
    CLK_ID_PCLK,

    CLK_ID_CGU,
    CLK_ID_CGU_DDR,
    CLK_ID_CGU_MACPHY,

    CLK_ID_I2S_CGU,
    CLK_ID_CGU_I2S0,
    CLK_ID_CGU_I2S1,
    CLK_ID_CGU_LCD,
    CLK_ID_CGU_MSC0,
    CLK_ID_CGU_MSC1,
    CLK_ID_CGU_SFC,
    CLK_ID_CGU_SSI,
    CLK_ID_CGU_CIM,
    CLK_ID_CGU_PWM,
    CLK_ID_CGU_CAN0,
    CLK_ID_CGU_CAN1,
    CLK_ID_CGU_CDBUS,
};

enum clk_type {
    CLK_GATE_0,

    CLK_GATE_DDR   = CLK_GATE_0 + 31,
    CLK_GATE_AHB0  = CLK_GATE_0 + 29,
    CLK_GATE_APB0  = CLK_GATE_0 + 28,
    CLK_GATE_RTC   = CLK_GATE_0 + 27,
    CLK_GATE_AES   = CLK_GATE_0 + 24,
    CLK_GATE_LCD   = CLK_GATE_0 + 23,
    CLK_GATE_CIM   = CLK_GATE_0 + 22,
    CLK_GATE_PDMA  = CLK_GATE_0 + 21,
    CLK_GATE_OST   = CLK_GATE_0 + 20,
    CLK_GATE_SSI0  = CLK_GATE_0 + 19,
    CLK_GATE_TCU   = CLK_GATE_0 + 18,
    CLK_GATE_DTRNG = CLK_GATE_0 + 17,
    CLK_GATE_UART2 = CLK_GATE_0 + 16,
    CLK_GATE_UART1 = CLK_GATE_0 + 15,
    CLK_GATE_UART0 = CLK_GATE_0 + 14,
    CLK_GATE_SADC  = CLK_GATE_0 + 13,
    CLK_GATE_AUDIO = CLK_GATE_0 + 11,
    CLK_GATE_SSI_SLV = CLK_GATE_0 + 10,
    CLK_GATE_I2C1  = CLK_GATE_0 + 8,
    CLK_GATE_I2C0  = CLK_GATE_0 + 7,
    CLK_GATE_MSC1  = CLK_GATE_0 + 5,
    CLK_GATE_MSC0  = CLK_GATE_0 + 4,
    CLK_GATE_OTG   = CLK_GATE_0 + 3,
    CLK_GATE_SFC   = CLK_GATE_0 + 2,
    CLK_GATE_EFUSE = CLK_GATE_0 + 1,
    CLK_GATE_NEMC  = CLK_GATE_0 + 0,

    CLK_GATE_1 = CLK_GATE_0 + 32,

    CLK_GATE_AR       = CLK_GATE_1 + 30,
    CLK_GATE_MIPI_CSI = CLK_GATE_1 + 28,
    CLK_GATE_INTC     = CLK_GATE_1 + 26,
    CLK_GATE_GMAC0    = CLK_GATE_1 + 23,
    CLK_GATE_UART3    = CLK_GATE_1 + 16,
    CLK_GATE_I2S0_t   = CLK_GATE_1 + 9,
    CLK_GATE_I2S0_r   = CLK_GATE_1 + 8,
    CLK_GATE_HASH     = CLK_GATE_1 + 6,
    CLK_GATE_PWM      = CLK_GATE_1 + 5,
    CLK_GATE_CDBUS     = CLK_GATE_1 + 2,
    CLK_GATE_CAN1     = CLK_GATE_1 + 1,
    CLK_GATE_CAN0     = CLK_GATE_1 + 0,

    CLK_NUMS = CLK_GATE_1 + 32,
};

enum {
    CDIV = 0,
    L2CDIV,
    H0DIV,
    H2DIV,
    PDIV,
    SCLKA,
};

unsigned long ext_pll_get_rate(int clk_id);
unsigned long sclk_a_get_rate(void);
unsigned long cpccr_get_rate(int clk_id);
int cgu_set_parent(int clk_id, int parent_id);
int cgu_set_rate(int clk_id, unsigned int rate);
int cgu_enable(int clk_id, int on);
int clk_enable(enum clk_type id, int on);
#endif /* _SOC_CLK_H_ */
