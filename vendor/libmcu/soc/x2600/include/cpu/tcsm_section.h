#ifndef _TCSM_SECTION_H_
#define _TCSM_SECTION_H_

#ifdef APP_libmcu_tcsm_text_section

#define __tcsm_text __attribute__((section(".tcsm_text")))
#define __tcsm_data __attribute__((section(".tcsm_data")))
#define __tcsm_bss  __attribute__((section(".tcsm_bss")))

#else

#define __tcsm_text
#define __tcsm_data
#define __tcsm_bss

#endif

#endif /* _TCSM_SECTION_H_ */
