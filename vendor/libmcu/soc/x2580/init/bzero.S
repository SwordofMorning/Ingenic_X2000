
    .section .text_bss_zero, "ax", @progbits
    .globl  bss_zero;
    .align  4;

bss_zero:
    bge a0, a1, 2f
    1:
        sw zero, 0(a0)
        addi a0, a0, 4
        blt a0, a1, 1b
    2:
        jr ra
    .size   bss_zero, .-bss_zero