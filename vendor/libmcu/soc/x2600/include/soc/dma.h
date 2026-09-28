#ifndef _SOC_DMA_H_
#define _SOC_DMA_H_

#ifndef ALIGN
#define ALIGN(x, n) (((x) + (n) - 1) - ((x) + (n) - 1) % (n))
#endif

enum DMA_request_type {
    /* ============ mcu_dma ============ */
    DMA_RQ_SADC_SEQ1_RX = 0b000000, /* ADC SEQ1 receive-fifo-full transfer request. */
    DMA_RQ_SADC_SEQ2_RX = 0b000001, /* ADC SEQ2 receive-fifo-full transfer request. */
    DMA_RQ_TPC_SHIFT_TX = 0b000010, /* TPC shift transmit-fifo-empty transfer request. */
    DMA_RQ_TPC_DEST_TX = 0b000011, /* TPC dest transmit-fifo-empty transfer request. */
    DMA_RQ_TPC_MOTO0_TX = 0b000100, /* TPC Step Moto0 transmit-fifo-empty transfer request. */
    DMA_RQ_TPC_MOTO1_TX = 0b000101, /* TPC Step Moto1 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM0_TX = 0b101100, /* PWM0 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM1_TX = 0b101101, /* PWM1 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM2_TX = 0b101110, /* PWM2 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM3_TX = 0b101111, /* PWM3 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM4_TX = 0b110000, /* PWM4 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM5_TX = 0b110001, /* PWM5 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM6_TX = 0b110010, /* PWM6 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM7_TX = 0b110011, /* PWM7 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM8_TX = 0b110100, /* PWM8 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM9_TX = 0b110101, /* PWM9 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM10_TX = 0b110110, /* PWM10 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM11_TX = 0b110111, /* PWM11 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM12_TX = 0b111000, /* PWM12 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM13_TX = 0b111001, /* PWM13 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM14_TX = 0b111010, /* PWM14 transmit-fifo-empty transfer request. */
    DMA_RQ_PWM15_TX = 0b111011, /* PWM15 transmit-fifo-empty transfer request. */

    /* ============== pdma ============== */
    DMA_RQ_PCM_TX = 0b000110, /* PCM transmit-fifo-empty transfer request. */
    DMA_RQ_PCM_RX = 0b000111, /* PCM receive-fifo-full transfer request. */
    DMA_RQ_MEM = 0b001000, /* Auto-request. (external address -> external address) */
    DMA_RQ_UART5_TX = 0b001010, /* UART5 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART5_RX = 0b001011, /* UART5 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART4_TX = 0b001100, /* UART4 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART4_RX = 0b001101, /* UART4 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART3_TX = 0b001110, /* UART3 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART3_RX = 0b001111, /* UART3 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART2_TX = 0b010000, /* UART2 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART2_RX = 0b010001, /* UART2 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART1_TX = 0b010010, /* UART1 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART1_RX = 0b010011, /* UART1 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART0_TX = 0b010100, /* UART0 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART0_RX = 0b010101, /* UART0 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_SSI0_TX = 0b010110, /* SSI0 transmit-fifo-empty transfer request. */
    DMA_RQ_SSI0_RX = 0b010111, /* SSI0 receive-fifo-full transfer request. */
    DMA_RQ_SSI1_TX = 0b011000, /* SSI1 transmit-fifo-empty transfer request. */
    DMA_RQ_SSI1_RX = 0b011001, /* SSI1 receive-fifo-full transfer request. */
    DMA_RQ_SLV0_TX = 0b011010, /* SSI SLV transmit-fifo-empty transfer request. */
    DMA_RQ_SLV0_RX = 0b011011, /* SSI SLV receive-fifo-full transfer request. */
    DMA_RQ_UART6_TX = 0b011100, /* UART6 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART6_RX = 0b011101, /* UART6 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_UART7_TX = 0b011110, /* UART7 transmit-fifo-empty transfer request. (external address -> UTHR) */
    DMA_RQ_UART7_RX = 0b011111, /* UART7 receive-fifo-full transfer request. (URBR -> external address) */
    DMA_RQ_CAN0_TX = 0b100000, /* CAN0 tx. */
    DMA_RQ_CAN0_RX = 0b100001, /* CAN0 rx fifo request. 4 or 1 word according to rx_single input port. */
    DMA_RQ_CAN1_TX = 0b100010, /* CAN1 tx. */
    DMA_RQ_CAN1_RX = 0b100011, /* CAN1 rx fifo request. 4 or 1 word according to rx_single input port. */
    DMA_RQ_I2C0_TX = 0b100100, /* I2C0 transmit-fifo-empty transfer request. */
    DMA_RQ_I2C0_RX = 0b100101, /* I2C0 receive-fifo-full transfer request. */
    DMA_RQ_I2C1_TX = 0b100110, /* I2C1 transmit-fifo-empty transfer request. */
    DMA_RQ_I2C1_RX = 0b100111, /* I2C 1 receive-fifo-full transfer request. */
    DMA_RQ_I2C2_TX = 0b101000, /* I2C 2 transmit-fifo-empty transfer request. */
    DMA_RQ_I2C2_RX = 0b101001, /* I2C 2 receive-fifo-full transfer request. */
    DMA_RQ_I2C3_TX = 0b101010, /* I2C 3 transmit-fifo-empty transfer request. */
    DMA_RQ_I2C3_RX = 0b101011, /* I2C 3 receive-fifo-full transfer request. */
    DMA_RQ_AIC_LOOP_RX = 0b111101, /* AIC TL request. */
    DMA_RQ_AIC_TX = 0b111110, /* AIC T request. */
    DMA_RQ_AIC_RX = 0b111111, /* AIC R request. */
};

enum DMA_bus_width {
    DMA_bus_8bit = 1,
    DMA_bus_16bit = 2,
    DMA_bus_32bit = 4,
    DMA_bus_64bit = 8,
};

#endif
