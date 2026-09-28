#include "common_macro.S"

    .global _start
    .global _vectors_start
    .global exception_handler
    .global _start
    .global reset_handler

    .section .start_section,"ax",@progbits
    .option norvc
_start:
# reset_handler 必须是 128字节对齐
reset_handler:
    j main_entry
    .word (('L'<<24)|('E'<<16)|('P'<<8)|('T'<<0))
    .word _start
    .word reset_handler
    .word __image_copy_end
    .word 0
    .word uncache_mem    #data_from_host 大核到小核必须是uncache的地址(uncache mem 默认头256字节用来通信)
    .word uncache_mem+32
    .word uncache_mem
    .word APP_libmcu_x2600_uncache_mem_size     #默认4096个字节


main_entry:

    /* 清零 BSS 段 */
    la a0, __bss_start
    la a1, __bss_end
    jal ra, bss_zero

    la sp, user_stack_mem+APP_libmcu_x2600_stack_size

#ifdef APP_libmcu_tcsm_text_section
    la a0, __tcsm_bss_start
    la a1, __tcsm_bss_end
    jal ra, bss_zero

    jal ra, tcsm_text_init
#endif
    /* jump to main program entry point (argc = argv = 0) */
    mv a0, zero
    mv a1, zero
    jal ra, c_main
1:
    wfi
    j 1b

#ifdef APP_libmcu_tcsm_text_section
    .section .tcsm_text_exception,"ax",@progbits
#endif
    .option rvc

#必须将mepc读到ra的操作，单独出来。否则gdb回溯不了，应该和gdb程序有关
exception_fault:
    add sp, sp, -4
    sw s0, (0*4)(sp)
    mv s0, ra
    csrr ra, mepc
exception_fault_:
    save_context_registers
#ifdef APP_libmcu_driver_irq
    jal ra, handler_fault_c
#endif
    restore_context_registers
    mv ra, s0
    lw s0, (0*4)(sp)
    add sp, sp, 4
    mret

exception_handler:
    csrr t0, mcause
    srli t0, t0, 30
    beq t0,zero,exception_fault

exception_int:
    save_context_registers
#ifdef APP_libmcu_driver_irq
    jal ra, handler_int_c
#endif
    restore_context_registers
    mret

#ifdef APP_libmcu_tcsm_text_section
    .section .tcsm_text_vector,"ax",@progbits
#else
    .balignl 256
#endif
    .option norvc
# _vectors_start 必须是 256字节对齐
_vectors_start:
    .rept 32
    j exception_handler
    .endr
    la t0, reset_handler
    jr t0
