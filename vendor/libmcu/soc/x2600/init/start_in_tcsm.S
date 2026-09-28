#include "common_macro.S"

    .global _start
    .global _vectors_start
    .global exception_handler
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
    .word __uncache_start
    .word __uncache_start+32
    .word 0
    .word 0

tcsm_copy:
    bge a0, a2, 2f
    1:
        lw s0, 0(a1)
        sw s0, 0(a0)
        addi a0, a0, 4
        addi a1, a1, 4
        blt a0, a2, 1b
    2:
        jr ra
    .size   tcsm_copy, .-tcsm_copy

main_entry:
    la a0, __tcsm_start
    la a1, __tcsm_end
    jal ra, bss_zero

    la a0, __tcsm_text_start
    la a1, __tcsm_text_sram_start
    la a2, __tcsm_text_end
    jal ra, tcsm_copy

    la a0, __tcsm_data_start
    la a1, __tcsm_data_sram_start
    la a2, __tcsm_data_end
    jal ra, tcsm_copy

    /* 清零 BSS 段 */
    la a0, __sram_bss_start
    la a1, __sram_bss_end
    jal ra, bss_zero

    la sp, user_stack_mem+APP_libmcu_x2600_stack_size

    /* jump to main program entry point (argc = argv = 0) */
    la t0, c_main
    jalr t0
3:
    wfi
    j 3b


    .section .tcsm_text_exception,"ax",@progbits
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

    .section .tcsm_text_vector,"ax",@progbits
    .option norvc
# _vectors_start 必须是 256字节对齐
_vectors_start:
    .rept 32
    j exception_handler
    .endr
    la t0, reset_handler
    jr t0
