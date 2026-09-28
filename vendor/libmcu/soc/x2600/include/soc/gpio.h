#ifndef _SOC_GPIO_H_
#define _SOC_GPIO_H_

enum gpio_function {
    GPIO_FUNC_0  = 0b1000000, // GPIO as function 0 / device 0
    GPIO_FUNC_1  = 0b1000001, // GPIO as function 1 / device 1
    GPIO_FUNC_2  = 0b1000010, // GPIO as function 2 / device 2
    GPIO_FUNC_3  = 0b1000011, // GPIO as function 3 / device 3
    GPIO_OUTPUT0 = 0b1001000, // GPIO output low  level
    GPIO_OUTPUT1 = 0b1001001, // GPIO output high level
    GPIO_INPUT   = 0b1001010, // GPIO as input
    GPIO_INT_LO  = 0b1011000, // Low  Level trigger interrupt
    GPIO_INT_HI  = 0b1011001, // High Level trigger interrupt
    GPIO_INT_FE  = 0b1011010, // Fall Edge trigger interrupt
    GPIO_INT_RE  = 0b1011011, // Rise Edge trigger interrupt
    GPIO_INT_DE  = 0b1111010, // dual Edge trigger interrupt

    GPIO_INT_MASK_LO = 0b1011100, // Port is low level triggered interrupt input. Interrupt is masked.
    GPIO_INT_MASK_HI = 0b1011101, // Port is high level triggered interrupt input. Interrupt is masked.
    GPIO_INT_MASK_FE = 0b1011110, // Port is fall edge triggered interrupt input. Interrupt is masked.
    GPIO_INT_MASK_RE = 0b1011111, // Port is rise edge triggered interrupt input. Interrupt is masked.
    GPIO_INT_MASK_DE = 0b1111110, // Port is dual Edge triggered interrupt input. Interrupt is masked.

    GPIO_PULL_HIZ  = 0b1000000000,    //no pull
    GPIO_PULL_UP   = 0b1010000000,    //pull high
    GPIO_PULL_DOWN = 0b1100000000,    //pull low
};

#define GPIO_PA(n) (0 * 32 + (n))
#define GPIO_PB(n) (1 * 32 + (n))
#define GPIO_PC(n) (2 * 32 + (n))
#define GPIO_PD(n) (3 * 32 + (n))
#define GPIO_PE(n) (4 * 32 + (n))
#define GPIO_PF(n) (5 * 32 + (n))

enum gpio_port {
    GPIO_PORT_A, GPIO_PORT_B,
    GPIO_PORT_C, GPIO_PORT_D, GPIO_PORT_E,
    /* this must be last */
    GPIO_NR_PORTS,
};

#endif /* _SOC_GPIO_H_ */
