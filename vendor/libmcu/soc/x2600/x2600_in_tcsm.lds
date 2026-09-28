
INCLUDE soc/x2600/x2600_in_tcsm.lds.in

MEMORY
{
    .sram : ORIGIN = sram_start, LENGTH = sram_size
    .tcsm_text : ORIGIN = 0x12b00000, LENGTH = 0x4000
    .tcsm_data : ORIGIN = 0x12b04000, LENGTH = 0x4000
    .uncached : ORIGIN = 0x12400000, LENGTH = 0x2000
}

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    .uncache_text :
    {
        __uncache_start = .;
    } >.uncached

    .sram_text :
    {
        __sram_start = .;
        *(.start_section)
        *(.text_bss_zero)
        *(.ddr_text*)
    } >.sram

    .sram_data :
    {
        *(.ddr_data*)
    } >.sram

    .sram_bss :
    {
        __sram_bss_start = .;
        *(.ddr_bss*)
        __sram_bss_end = .;
    } >.sram

    __tcsm_text_sram_start = .;

    .text :
    {
        __tcsm_start = .;
        __tcsm_text_start = .;
        *(.tcsm_text_vector*)
        *(.tcsm_text*)
        *(.text*)
        __tcsm_text_end = .;
    } >.tcsm_text AT>.sram

    .text_tcsm_empty :
    {
        __tcsm_text_empty_start = .;
    } > .sram
    __tcsm_text_sram_end = .;

    __tcsm_data_sram_start = .;
    .rodata :
    {
        . = ALIGN(4);
        __tcsm_data_start = .;
        *(SORT_BY_ALIGNMENT(.rodata*))
    } >.tcsm_data AT>.sram

    .srodata :
    {
        . = ALIGN(4);
        *(SORT_BY_ALIGNMENT(.srodata*))
    } >.tcsm_data AT>.sram

    .sdata :
    {
        . = ALIGN(4);
        *(SORT_BY_ALIGNMENT(.sdata*))
    } >.tcsm_data AT>.sram

    .data :
    {
        . = ALIGN(4);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(SORT_BY_ALIGNMENT(.data*))
    } >.tcsm_data AT>.sram

    .preinit_array :
    {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
    } >.tcsm_data AT>.sram

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array ))
        PROVIDE (__init_array_end = .);
    } >.tcsm_data AT>.sram

    .ctors :
    {
        . = ALIGN(4);
        PROVIDE (__ctors_start = .);
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        PROVIDE (__ctors_end = .);
    } >.tcsm_data AT>.sram

    .fini_array :
    {
        __fini_array_start = .;
        *(.fini_array)
        __fini_array_end = .;
    } >.tcsm_data AT>.sram

    __ksymtab_strings :
    {
        KEEP(*(SORT(__ksymtab_strings*)))
    } >.tcsm_data AT>.sram

    ___ksymtab :
    {
        . = ALIGN(8);
        __ksymtab_start = .;
        KEEP(*(SORT(___ksymtab*)))
        __ksymtab_end = .;
    } >.tcsm_data AT>.sram

    .func_symbol_index :
    {
        . = ALIGN(16);
        __func_symbol_index_start = .;
        KEEP(*(SORT(.func_symbol_index*)))
        __func_symbol_index_end = .;
    } >.tcsm_data AT>.sram

    .func_symbol_str :
    {
        __func_symbol_str_start = .;
        KEEP(*(SORT(.func_symbol_str*)))
        __func_symbol_str_end = .;
    } >.tcsm_data AT>.sram

    /* module param */
    __param :
    {
        . = ALIGN(4);
        __param_start = .;
        KEEP(*(__param))
        __param_stop = .;
        __tcsm_data_end = .;
    } >.tcsm_data AT>.sram

    .data_tcsm_empty :
    {
        __tcsm_data_empty_start = .;
    } >.sram
    __tcsm_data_sram_end = .;

    . = ALIGN(4);
    __image_copy_end = .;

    .bss : {
        . = ALIGN(4);
        __tcsm_bss_start = .;
        *(.sbss.*)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
    } >.tcsm_data

    .bss_end : {
        . = ALIGN(4);
        __tcsm_bss_end = .;
    } >.tcsm_data

    /* must be last */
    .user_heap : {
        . =  ALIGN(8) ;
        _user_heap_start = .;
        KEEP(*(.user_heap))
        . = __tcsm_text_start + 0x8000;
        _user_heap_end = .;
        __tcsm_end = .;
    } >.tcsm_data

    .sram_end :
    {
        . = __sram_start + sram_size;
    } >.sram

    /DISCARD/ : {
        *(.dynbss)
        *(.dynstr)
        *(.dynamic)
        *(.interp)
        *(.hash)
        *(.gnu.*)
        *(.plt)
        *(.got.plt)
        *(.rel.plt)
        *(.rel.dyn)
    }
}
