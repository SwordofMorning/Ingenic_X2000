#ifndef _SOC_CLK_H_
#define _SOC_CLK_H_

#define CLK_RTC_RATE 32768
#define CLK_EXT_RATE (24*1000*1000)

enum clk_pll_type {
    CLK_PLL_APLL,
    CLK_PLL_MPLL,
    CLK_PLL_EPLL,

    CLK_PLL_NUMS,
};

enum clk_cpccr_type {
    CLK_CPCCR_CCLK,
    CLK_CPCCR_L2CLK,
    CLK_CPCCR_H0CLK,
    CLK_CPCCR_H2CLK,
    CLK_CPCCR_PCLK,
    CLK_CPCCR_SCLK_A,

    CLK_CPCCR_NUMS,
};

enum clk_div_type {
    CLK_DIV_DDR,
    CLK_DIV_MAC,
    CLK_DIV_LPC,
    CLK_DIV_MSC0,
    CLK_DIV_MSC1,
    CLK_DIV_SFC,
    CLK_DIV_SSI,
    CLK_DIV_PWM,
    CLK_DIV_TPC,
    CLK_DIV_CIM,
    CLK_DIV_G2D,
    CLK_DIV_CAN0,
    CLK_DIV_CAN1,
    CLK_DIV_SADC,

    CLK_DIV_NUMS,
};

enum clk_i2s_type {
    CLK_I2S,
    CLK_PCM,

    CLK_I2S_NUMS,
};

enum clk_gate_type {
    CLK_GATE_0,

    CLK_GATE_NEMC = CLK_GATE_0 + 31,
    CLK_GATE_INTC = CLK_GATE_0 + 30,
    CLK_GATE_RTC = CLK_GATE_0 + 29,
    CLK_GATE_OST = CLK_GATE_0 + 28,
    CLK_GATE_DTRNG = CLK_GATE_0 + 27,
    CLK_GATE_I2ST = CLK_GATE_0 + 26,
    CLK_GATE_DMIC = CLK_GATE_0 + 25,
    CLK_GATE_AUDIO = CLK_GATE_0 + 24,
    CLK_GATE_MIPI_DSI = CLK_GATE_0 + 23,
    CLK_GATE_SSI_SLV = CLK_GATE_0 + 22,
    CLK_GATE_SSI1 = CLK_GATE_0 + 21,
    CLK_GATE_SSI0 = CLK_GATE_0 + 20,
    CLK_GATE_UART7 = CLK_GATE_0 + 19,
    CLK_GATE_UART6 = CLK_GATE_0 + 18,
    CLK_GATE_UART5 = CLK_GATE_0 + 17,
    CLK_GATE_UART4 = CLK_GATE_0 + 16,
    CLK_GATE_UART3 = CLK_GATE_0 + 15,
    CLK_GATE_UART2 = CLK_GATE_0 + 14,
    CLK_GATE_UART1 = CLK_GATE_0 + 13,
    CLK_GATE_UART0 = CLK_GATE_0 + 12,
    CLK_GATE_I2C3 = CLK_GATE_0 + 11,
    CLK_GATE_I2C2 = CLK_GATE_0 + 10,
    CLK_GATE_I2C1 = CLK_GATE_0 + 9,
    CLK_GATE_I2C0 = CLK_GATE_0 + 8,
    CLK_GATE_RSV8 = CLK_GATE_0 + 7,
    CLK_GATE_MAC = CLK_GATE_0 + 6,
    CLK_GATE_USB = CLK_GATE_0 + 5,
    CLK_GATE_OTG = CLK_GATE_0 + 4,
    CLK_GATE_MSC1 = CLK_GATE_0 + 3,
    CLK_GATE_MSC0 = CLK_GATE_0 + 2,
    CLK_GATE_SFC = CLK_GATE_0 + 1,
    CLK_GATE_EFUSE = CLK_GATE_0 + 0,

    CLK_GATE_1 = CLK_GATE_0 + 32,

    CLK_GATE_AHB0 = CLK_GATE_1 + 31,
    CLK_GATE_APB0 = CLK_GATE_1 + 30,
    CLK_GATE_AHB2 = CLK_GATE_1 + 29,
    CLK_GATE_APB = CLK_GATE_1 + 28,
    CLK_GATE_ARB = CLK_GATE_1 + 27,
    CLK_GATE_RSV58 = CLK_GATE_1 + 26,
    CLK_GATE_RSV57 = CLK_GATE_1 + 25,
    CLK_GATE_RSV56 = CLK_GATE_1 + 24,
    CLK_GATE_DDR = CLK_GATE_1 + 23,
    CLK_GATE_PCM1 = CLK_GATE_1 + 22,
    CLK_GATE_PCM0 = CLK_GATE_1 + 21,
    CLK_GATE_BMON = CLK_GATE_1 + 20,
    CLK_GATE_JPEGE = CLK_GATE_1 + 19,
    CLK_GATE_JPEGD = CLK_GATE_1 + 18,
    CLK_GATE_FELIX = CLK_GATE_1 + 17,
    CLK_GATE_ROTATE = CLK_GATE_1 + 16,
    CLK_GATE_G2D = CLK_GATE_1 + 15,
    CLK_GATE_LCD = CLK_GATE_1 + 14,
    CLK_GATE_CIM = CLK_GATE_1 + 13,
    CLK_GATE_RSV44 = CLK_GATE_1 + 12,
    CLK_GATE_TCSM = CLK_GATE_1 + 11,
    CLK_GATE_SADC = CLK_GATE_1 + 10,
    CLK_GATE_TCU1 = CLK_GATE_1 + 9,
    CLK_GATE_TCU0 = CLK_GATE_1 + 8,
    CLK_GATE_TPC = CLK_GATE_1 + 7,
    CLK_GATE_PWM = CLK_GATE_1 + 6,
    CLK_GATE_CAN1 = CLK_GATE_1 + 5,
    CLK_GATE_CAN0 = CLK_GATE_1 + 4,
    CLK_GATE_HASH = CLK_GATE_1 + 3,
    CLK_GATE_AES = CLK_GATE_1 + 2,
    CLK_GATE_DMAC1 = CLK_GATE_1 + 1,
    CLK_GATE_DMAC = CLK_GATE_1 + 0,

    CLK_GATE_NUMS = CLK_GATE_AHB0 + 1,
};

enum clk_type {
    clk_type_nums,
};

unsigned int clk_pll_set_rate(enum clk_pll_type id, unsigned int rate);
unsigned int clk_pll_get_rate(enum clk_pll_type id);
int clk_pll_is_enable(enum clk_pll_type id);

unsigned int clk_cpccr_get_rate(enum clk_cpccr_type id);

enum clk_div_parent_type {
    CLK_DIV_PARENT_SCLK_A,
    CLK_DIV_PARENT_MPLL,
    CLK_DIV_PARENT_EPLL,
    CLK_DIV_PARENT_EXT_CLK,
    CLK_DIV_PARENT_NULL,
};

unsigned int clk_div_get_rate(enum clk_div_type id);
void clk_div_set_rate(enum clk_div_type id, unsigned int rate);
void clk_div_enable(enum clk_div_type id);
void clk_div_disable(enum clk_div_type id);
int clk_div_is_enable(enum clk_div_type id);
void clk_div_set_parent(enum clk_div_type id, enum clk_div_parent_type parent);

unsigned int clk_i2s_get_rate(enum clk_i2s_type id);
int clk_i2s_is_enable(enum clk_i2s_type id);

void clk_gate_enable(enum clk_gate_type id);
void clk_gate_disable(enum clk_gate_type id);
int clk_gate_is_enable(enum clk_gate_type id);

#endif /* _SOC_CLK_H_ */
