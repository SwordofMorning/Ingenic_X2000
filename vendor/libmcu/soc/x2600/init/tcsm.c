#include <string.h>

extern unsigned char __tcsm_text_sram_start;
extern unsigned char __tcsm_text_sram_end;
extern unsigned char __tcsm_text_start;
extern unsigned char __tcsm_text_end;

extern unsigned char __tcsm_data_sram_start;
extern unsigned char __tcsm_data_sram_end;
extern unsigned char __tcsm_data_start;
extern unsigned char __tcsm_data_end;

extern unsigned char __tcsm_bss_start;
extern unsigned char __tcsm_bss_end;

void tcsm_text_init(void)
{
    int len = &__tcsm_text_end - &__tcsm_text_start;
    memcpy(&__tcsm_text_start, &__tcsm_text_sram_start, len);

    len = &__tcsm_data_end - &__tcsm_data_start;
    memcpy(&__tcsm_data_start, &__tcsm_data_sram_start, len);
}
