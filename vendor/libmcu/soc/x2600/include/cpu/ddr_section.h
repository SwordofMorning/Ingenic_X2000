#ifndef _DDR_SECTION_H_
#define _DDR_SECTION_H_

#ifdef APP_libmcu_ddr_text_section

#define __ddr_text __attribute__((section(".ddr_text")))
#define __ddr_data __attribute__((section(".ddr_data")))
#define __ddr_bss  __attribute__((section(".ddr_bss")))

#else

#define __ddr_text
#define __ddr_data
#define __ddr_bss

#endif

#endif /* _DDR_SECTION_H_ */
