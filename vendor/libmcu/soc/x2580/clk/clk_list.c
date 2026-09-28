#include <driver/clk.h>
#include <stdio.h>

void clk_list(void)
{
    printf("-> CLK_APLL: %d\n", clk_pll_get_rate(CLK_PLL_APLL));
    printf("-> CLK_MPLL: %d\n", clk_pll_get_rate(CLK_PLL_MPLL));
    printf("-> CLK_VPLL: %d\n", clk_pll_get_rate(CLK_PLL_VPLL));

    printf("-> CLK_CCLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_CCLK));
    printf("-> CLK_L2CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_L2CLK));
    printf("-> CLK_H0CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_H0CLK));
    printf("-> CLK_H2CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_H2CLK));
    printf("-> CLK_PCLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_PCLK));
    printf("-> CLK_SCLK_A: %d\n", clk_cpccr_get_rate(CLK_CPCCR_SCLK_A));

    printf("-> CLK_DIV_DDR: %d\n", clk_div_get_rate(CLK_DIV_DDR));
    printf("-> CLK_DIV_RSA: %d\n", clk_div_get_rate(CLK_DIV_RSA));
    printf("-> CLK_DIV_SSI_SLV: %d\n", clk_div_get_rate(CLK_DIV_SSI_SLV));
    printf("-> CLK_DIV_MACPHY: %d\n", clk_div_get_rate(CLK_DIV_MACPHY));
    printf("-> CLK_DIV_SFC0: %d\n", clk_div_get_rate(CLK_DIV_SFC0));
    printf("-> CLK_DIV_LPC: %d\n", clk_div_get_rate(CLK_DIV_LPC));
    printf("-> CLK_DIV_MSC0: %d\n", clk_div_get_rate(CLK_DIV_MSC0));
    printf("-> CLK_DIV_MSC1: %d\n", clk_div_get_rate(CLK_DIV_MSC1));
    printf("-> CLK_DIV_SSI: %d\n", clk_div_get_rate(CLK_DIV_SSI));
    printf("-> CLK_DIV_SFC1: %d\n", clk_div_get_rate(CLK_DIV_SFC1));
    printf("-> CLK_DIV_ISPM: %d\n", clk_div_get_rate(CLK_DIV_ISPM));
    printf("-> CLK_DIV_CIM0: %d\n", clk_div_get_rate(CLK_DIV_CIM0));
    printf("-> CLK_DIV_LDC: %d\n", clk_div_get_rate(CLK_DIV_LDC));
    printf("-> CLK_DIV_ISPS: %d\n", clk_div_get_rate(CLK_DIV_ISPS));
    printf("-> CLK_DIV_ISPA: %d\n", clk_div_get_rate(CLK_DIV_ISPA));
    printf("-> CLK_DIV_BT0: %d\n", clk_div_get_rate(CLK_DIV_BT0));
    printf("-> CLK_DIV_ALGENC: %d\n", clk_div_get_rate(CLK_DIV_ALGENC));
    printf("-> CLK_DIV_PWM: %d\n", clk_div_get_rate(CLK_DIV_PWM));

    printf("-> CLK_I2S_TX: %d\n", clk_i2s_get_rate(CLK_I2S_TX));
    printf("-> CLK_I2S_RX: %d\n", clk_i2s_get_rate(CLK_I2S_RX));

    printf("-> CLK_GATE_DDR: %d\n", clk_gate_is_enable(CLK_GATE_DDR));
    printf("-> CLK_GATE_TCU: %d\n", clk_gate_is_enable(CLK_GATE_TCU));
    printf("-> CLK_GATE_RTC: %d\n", clk_gate_is_enable(CLK_GATE_RTC));
    printf("-> CLK_GATE_DES: %d\n", clk_gate_is_enable(CLK_GATE_DES));
    printf("-> CLK_GATE_RSA: %d\n", clk_gate_is_enable(CLK_GATE_RSA));
    printf("-> CLK_GATE_VO: %d\n", clk_gate_is_enable(CLK_GATE_VO));
    printf("-> CLK_GATE_MIPI_CSI: %d\n", clk_gate_is_enable(CLK_GATE_MIPI_CSI));
    printf("-> CLK_GATE_LCD: %d\n", clk_gate_is_enable(CLK_GATE_LCD));
    printf("-> CLK_GATE_ISP: %d\n", clk_gate_is_enable(CLK_GATE_ISP));
    printf("-> CLK_GATE_PDMA: %d\n", clk_gate_is_enable(CLK_GATE_PDMA));
    printf("-> CLK_GATE_SFC0: %d\n", clk_gate_is_enable(CLK_GATE_SFC0));
    printf("-> CLK_GATE_SSI1: %d\n", clk_gate_is_enable(CLK_GATE_SSI1));
    printf("-> CLK_GATE_UART5: %d\n", clk_gate_is_enable(CLK_GATE_UART5));
    printf("-> CLK_GATE_UART4: %d\n", clk_gate_is_enable(CLK_GATE_UART4));
    printf("-> CLK_GATE_UART3: %d\n", clk_gate_is_enable(CLK_GATE_UART3));
    printf("-> CLK_GATE_UART2: %d\n", clk_gate_is_enable(CLK_GATE_UART2));
    printf("-> CLK_GATE_UART1: %d\n", clk_gate_is_enable(CLK_GATE_UART1));
    printf("-> CLK_GATE_UART0: %d\n", clk_gate_is_enable(CLK_GATE_UART0));
    printf("-> CLK_GATE_SADC: %d\n", clk_gate_is_enable(CLK_GATE_SADC));
    printf("-> CLK_GATE_DMIC: %d\n", clk_gate_is_enable(CLK_GATE_DMIC));
    printf("-> CLK_GATE_AIC: %d\n", clk_gate_is_enable(CLK_GATE_AIC));
    printf("-> CLK_GATE_SSI_SLV: %d\n", clk_gate_is_enable(CLK_GATE_SSI_SLV));
    printf("-> CLK_GATE_I2C2: %d\n", clk_gate_is_enable(CLK_GATE_I2C2));
    printf("-> CLK_GATE_I2C1: %d\n", clk_gate_is_enable(CLK_GATE_I2C1));
    printf("-> CLK_GATE_I2C0: %d\n", clk_gate_is_enable(CLK_GATE_I2C0));
    printf("-> CLK_GATE_SSI0: %d\n", clk_gate_is_enable(CLK_GATE_SSI0));
    printf("-> CLK_GATE_MSC1: %d\n", clk_gate_is_enable(CLK_GATE_MSC1));
    printf("-> CLK_GATE_MSC0: %d\n", clk_gate_is_enable(CLK_GATE_MSC0));
    printf("-> CLK_GATE_OTG: %d\n", clk_gate_is_enable(CLK_GATE_OTG));
    printf("-> CLK_GATE_SC_HASH: %d\n", clk_gate_is_enable(CLK_GATE_SC_HASH));
    printf("-> CLK_GATE_EFUSE: %d\n", clk_gate_is_enable(CLK_GATE_EFUSE));
    printf("-> CLK_GATE_NEMC: %d\n", clk_gate_is_enable(CLK_GATE_NEMC));
    printf("-> CLK_GATE_IVDC: %d\n", clk_gate_is_enable(CLK_GATE_IVDC));
    printf("-> CLK_GATE_CPU: %d\n", clk_gate_is_enable(CLK_GATE_CPU));
    printf("-> CLK_GATE_APB0: %d\n", clk_gate_is_enable(CLK_GATE_APB0));
    printf("-> CLK_GATE_JPEG: %d\n", clk_gate_is_enable(CLK_GATE_JPEG));
    printf("-> CLK_GATE_LDC: %d\n", clk_gate_is_enable(CLK_GATE_LDC));
    printf("-> CLK_GATE_SFC1: %d\n", clk_gate_is_enable(CLK_GATE_SFC1));
    printf("-> CLK_GATE_SYS_OST: %d\n", clk_gate_is_enable(CLK_GATE_SYS_OST));
    printf("-> CLK_GATE_AHB0: %d\n", clk_gate_is_enable(CLK_GATE_AHB0));
    printf("-> CLK_GATE_BUS_MONITOR: %d\n", clk_gate_is_enable(CLK_GATE_BUS_MONITOR));
    printf("-> CLK_GATE_I2D: %d\n", clk_gate_is_enable(CLK_GATE_I2D));
    printf("-> CLK_GATE_PWM: %d\n", clk_gate_is_enable(CLK_GATE_PWM));
    printf("-> CLK_GATE_DRAWBOX: %d\n", clk_gate_is_enable(CLK_GATE_DRAWBOX));
    printf("-> CLK_GATE_AES: %d\n", clk_gate_is_enable(CLK_GATE_AES));
    printf("-> CLK_GATE_GMAC: %d\n", clk_gate_is_enable(CLK_GATE_GMAC));
    printf("-> CLK_GATE_LZMA: %d\n", clk_gate_is_enable(CLK_GATE_LZMA));
    printf("-> CLK_GATE_IPU: %d\n", clk_gate_is_enable(CLK_GATE_IPU));
    printf("-> CLK_GATE_DTRNG: %d\n", clk_gate_is_enable(CLK_GATE_DTRNG));
    printf("-> CLK_GATE_ALGENC: %d\n", clk_gate_is_enable(CLK_GATE_ALGENC));
}
