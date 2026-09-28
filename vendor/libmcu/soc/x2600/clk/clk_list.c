#include <driver/clk.h>
#include <stdio.h>
#include <cpu/ddr_section.h>

__ddr_text void clk_list(void)
{
    printf("-> CLK_APLL: %d\n", clk_pll_get_rate(CLK_PLL_APLL));
    printf("-> CLK_MPLL: %d\n", clk_pll_get_rate(CLK_PLL_MPLL));
    printf("-> CLK_VPLL: %d\n", clk_pll_get_rate(CLK_PLL_EPLL));

    printf("-> CLK_CCLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_CCLK));
    printf("-> CLK_L2CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_L2CLK));
    printf("-> CLK_H0CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_H0CLK));
    printf("-> CLK_H2CLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_H2CLK));
    printf("-> CLK_PCLK: %d\n", clk_cpccr_get_rate(CLK_CPCCR_PCLK));
    printf("-> CLK_SCLK_A: %d\n", clk_cpccr_get_rate(CLK_CPCCR_SCLK_A));


    printf("-> CLK_DIV_DDR: %d\n", clk_div_get_rate(CLK_DIV_DDR));
    printf("-> CLK_DIV_MAC: %d\n", clk_div_get_rate(CLK_DIV_MAC));
    printf("-> CLK_DIV_LPC: %d\n", clk_div_get_rate(CLK_DIV_LPC));
    printf("-> CLK_DIV_MSC0: %d\n", clk_div_get_rate(CLK_DIV_MSC0));
    printf("-> CLK_DIV_MSC1: %d\n", clk_div_get_rate(CLK_DIV_MSC1));
    printf("-> CLK_DIV_SFC: %d\n", clk_div_get_rate(CLK_DIV_SFC));
    printf("-> CLK_DIV_SSI: %d\n", clk_div_get_rate(CLK_DIV_SSI));
    printf("-> CLK_DIV_PWM: %d\n", clk_div_get_rate(CLK_DIV_PWM));
    printf("-> CLK_DIV_TPC: %d\n", clk_div_get_rate(CLK_DIV_TPC));
    printf("-> CLK_DIV_CIM: %d\n", clk_div_get_rate(CLK_DIV_CIM));
    printf("-> CLK_DIV_G2D: %d\n", clk_div_get_rate(CLK_DIV_G2D));
    printf("-> CLK_DIV_CAN0: %d\n", clk_div_get_rate(CLK_DIV_CAN0));
    printf("-> CLK_DIV_CAN1: %d\n", clk_div_get_rate(CLK_DIV_CAN1));
    printf("-> CLK_DIV_SADC: %d\n", clk_div_get_rate(CLK_DIV_SADC));

    printf("-> CLK_I2S: %d\n", clk_i2s_get_rate(CLK_I2S));
    printf("-> CLK_PCM: %d\n", clk_i2s_get_rate(CLK_PCM));

    printf("-> CLK_GATE_NEMC: %d\n", clk_gate_is_enable(CLK_GATE_NEMC));
    printf("-> CLK_GATE_INTC: %d\n", clk_gate_is_enable(CLK_GATE_INTC));
    printf("-> CLK_GATE_RTC: %d\n", clk_gate_is_enable(CLK_GATE_RTC));
    printf("-> CLK_GATE_OST: %d\n", clk_gate_is_enable(CLK_GATE_OST));
    printf("-> CLK_GATE_DTRNG: %d\n", clk_gate_is_enable(CLK_GATE_DTRNG));
    printf("-> CLK_GATE_I2ST: %d\n", clk_gate_is_enable(CLK_GATE_I2ST));
    printf("-> CLK_GATE_DMIC: %d\n", clk_gate_is_enable(CLK_GATE_DMIC));
    printf("-> CLK_GATE_AUDIO: %d\n", clk_gate_is_enable(CLK_GATE_AUDIO));
    printf("-> CLK_GATE_MIPI_DSI: %d\n", clk_gate_is_enable(CLK_GATE_MIPI_DSI));
    printf("-> CLK_GATE_SSI_SLV: %d\n", clk_gate_is_enable(CLK_GATE_SSI_SLV));
    printf("-> CLK_GATE_SSI1: %d\n", clk_gate_is_enable(CLK_GATE_SSI1));
    printf("-> CLK_GATE_SSI0: %d\n", clk_gate_is_enable(CLK_GATE_SSI0));
    printf("-> CLK_GATE_UART7: %d\n", clk_gate_is_enable(CLK_GATE_UART7));
    printf("-> CLK_GATE_UART6: %d\n", clk_gate_is_enable(CLK_GATE_UART6));
    printf("-> CLK_GATE_UART5: %d\n", clk_gate_is_enable(CLK_GATE_UART5));
    printf("-> CLK_GATE_UART4: %d\n", clk_gate_is_enable(CLK_GATE_UART4));
    printf("-> CLK_GATE_UART3: %d\n", clk_gate_is_enable(CLK_GATE_UART3));
    printf("-> CLK_GATE_UART2: %d\n", clk_gate_is_enable(CLK_GATE_UART2));
    printf("-> CLK_GATE_UART1: %d\n", clk_gate_is_enable(CLK_GATE_UART1));
    printf("-> CLK_GATE_UART0: %d\n", clk_gate_is_enable(CLK_GATE_UART0));
    printf("-> CLK_GATE_I2C3: %d\n", clk_gate_is_enable(CLK_GATE_I2C3));
    printf("-> CLK_GATE_I2C2: %d\n", clk_gate_is_enable(CLK_GATE_I2C2));
    printf("-> CLK_GATE_I2C1: %d\n", clk_gate_is_enable(CLK_GATE_I2C1));
    printf("-> CLK_GATE_I2C0: %d\n", clk_gate_is_enable(CLK_GATE_I2C0));
    printf("-> CLK_GATE_RSV8: %d\n", clk_gate_is_enable(CLK_GATE_RSV8));
    printf("-> CLK_GATE_MAC: %d\n", clk_gate_is_enable(CLK_GATE_MAC));
    printf("-> CLK_GATE_USB: %d\n", clk_gate_is_enable(CLK_GATE_USB));
    printf("-> CLK_GATE_OTG: %d\n", clk_gate_is_enable(CLK_GATE_OTG));
    printf("-> CLK_GATE_MSC1: %d\n", clk_gate_is_enable(CLK_GATE_MSC1));
    printf("-> CLK_GATE_MSC0: %d\n", clk_gate_is_enable(CLK_GATE_MSC0));
    printf("-> CLK_GATE_SFC: %d\n", clk_gate_is_enable(CLK_GATE_SFC));
    printf("-> CLK_GATE_EFUSE: %d\n", clk_gate_is_enable(CLK_GATE_EFUSE));
    printf("-> CLK_GATE_AHB0: %d\n", clk_gate_is_enable(CLK_GATE_AHB0));
    printf("-> CLK_GATE_APB0: %d\n", clk_gate_is_enable(CLK_GATE_APB0));
    printf("-> CLK_GATE_AHB2: %d\n", clk_gate_is_enable(CLK_GATE_AHB2));
    printf("-> CLK_GATE_APB: %d\n", clk_gate_is_enable(CLK_GATE_APB));
    printf("-> CLK_GATE_ARB: %d\n", clk_gate_is_enable(CLK_GATE_ARB));
    printf("-> CLK_GATE_RSV58: %d\n", clk_gate_is_enable(CLK_GATE_RSV58));
    printf("-> CLK_GATE_RSV57: %d\n", clk_gate_is_enable(CLK_GATE_RSV57));
    printf("-> CLK_GATE_RSV56: %d\n", clk_gate_is_enable(CLK_GATE_RSV56));
    printf("-> CLK_GATE_DDR: %d\n", clk_gate_is_enable(CLK_GATE_DDR));
    printf("-> CLK_GATE_PCM1: %d\n", clk_gate_is_enable(CLK_GATE_PCM1));
    printf("-> CLK_GATE_PCM0: %d\n", clk_gate_is_enable(CLK_GATE_PCM0));
    printf("-> CLK_GATE_BMON: %d\n", clk_gate_is_enable(CLK_GATE_BMON));
    printf("-> CLK_GATE_JPEGE: %d\n", clk_gate_is_enable(CLK_GATE_JPEGE));
    printf("-> CLK_GATE_JPEGD: %d\n", clk_gate_is_enable(CLK_GATE_JPEGD));
    printf("-> CLK_GATE_FELIX: %d\n", clk_gate_is_enable(CLK_GATE_FELIX));
    printf("-> CLK_GATE_ROTATE: %d\n", clk_gate_is_enable(CLK_GATE_ROTATE));
    printf("-> CLK_GATE_G2D: %d\n", clk_gate_is_enable(CLK_GATE_G2D));
    printf("-> CLK_GATE_LCD: %d\n", clk_gate_is_enable(CLK_GATE_LCD));
    printf("-> CLK_GATE_CIM: %d\n", clk_gate_is_enable(CLK_GATE_CIM));
    printf("-> CLK_GATE_RSV44: %d\n", clk_gate_is_enable(CLK_GATE_RSV44));
    printf("-> CLK_GATE_TCSM: %d\n", clk_gate_is_enable(CLK_GATE_TCSM));
    printf("-> CLK_GATE_SADC: %d\n", clk_gate_is_enable(CLK_GATE_SADC));
    printf("-> CLK_GATE_TCU1: %d\n", clk_gate_is_enable(CLK_GATE_TCU1));
    printf("-> CLK_GATE_TCU0: %d\n", clk_gate_is_enable(CLK_GATE_TCU0));
    printf("-> CLK_GATE_TPC: %d\n", clk_gate_is_enable(CLK_GATE_TPC));
    printf("-> CLK_GATE_PWM: %d\n", clk_gate_is_enable(CLK_GATE_PWM));
    printf("-> CLK_GATE_CAN1: %d\n", clk_gate_is_enable(CLK_GATE_CAN1));
    printf("-> CLK_GATE_CAN0: %d\n", clk_gate_is_enable(CLK_GATE_CAN0));
    printf("-> CLK_GATE_HASH: %d\n", clk_gate_is_enable(CLK_GATE_HASH));
    printf("-> CLK_GATE_AES: %d\n", clk_gate_is_enable(CLK_GATE_AES));
    printf("-> CLK_GATE_DMAC1: %d\n", clk_gate_is_enable(CLK_GATE_DMAC1));
    printf("-> CLK_GATE_DMAC: %d\n", clk_gate_is_enable(CLK_GATE_DMAC));
}
