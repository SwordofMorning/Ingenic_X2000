#include "common_macro.S"

    .global _start
    .global _vectors_start
    .global exception_handler
    .global reset_handler
    .global main_entry

    .section .start_section,"ax",@progbits
    .option norvc
_start:
reset_handler:
    j main_entry
    .word (('L'<<24)|('E'<<16)|('P'<<8)|('T'<<0))
    .word _start
    .word reset_handler
    .word __image_copy_end
    .word 0
    .word uncache_mem
    .word uncache_mem+32
    .word uncache_mem
    .word APP_libmcu_x2580_uncache_mem_size

main_entry:

    la a0, __bss_start
    la a1, __bss_end
    jal ra, bss_zero

    la sp, user_stack_mem+APP_libmcu_x2580_stack_size

    /* jump to main program entry point (argc = argv = 0) */
    mv a0, zero
    mv a1, zero
    jal ra, c_main
1:
    wfi
    j 1b

    .section .text_exception,"ax",@progbits
    #.option rvc    # may not support

exception_fault:
    add sp, sp, -4
    sw s0, (0*4)(sp)
    mv s0, ra
    csrr ra, mepc
exception_fault_:
    save_context_registers
    mv a0, sp
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

    .section .text_vector,"ax",@progbits
    .option norvc
_vectors_start:
    .rept 32
    j exception_handler
    .endr
    la t0, reset_handler
    jr t0

