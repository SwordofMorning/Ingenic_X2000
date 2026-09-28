#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/asm.h>

    .set mips32r2
    .set noreorder
    .extern _libc_main

    .global    _start
    .section .start_section,"ax",@progbits
    .ent _start
    .type _start, @function
_start:
    b _code_start
    nop
    .word APP_libmcu_version
    .word _start
    .word __image_copy_end
    .word data_from_host
    .word data_to_host
_code_start:
    /* Clear ERL,EXL,IE */
    mtc0   zero, CP0_STATUS

    //----------------------------------------------------
    // clear BSS section
    //----------------------------------------------------
    la    k0, __bss_start
    la    k1, __bss_end
    beq   k0, k1, 2f
    nop
 1: sw    zero, 0(k0)
    addiu k0, 4
    bne   k0, k1, 1b
    nop

 2:
    //----------------------------------------------------
    // setup stack jump to C code
    //----------------------------------------------------
    la    sp, _user_stack_end
    la    k0, c_main
    jalr    k0
    nop

    .end    _start
    .size    _start, .-_start

    .org 0x100
    b    exception_entry
    nop
