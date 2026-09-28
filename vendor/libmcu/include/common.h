#ifndef _COMMON_H_
#define _COMMON_H_

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (int)( sizeof(x) / sizeof(x)[0] )
#endif

#ifdef DEBUG
#define debug(x...) \
    do { \
        printf(x); \
    } while (0)
#else
#define debug(x...) \
    do { \
        if (0) \
            printf(x); \
    } while (0)
#endif

#define assert_range(x, start, end) assert(((x) >= (start)) && ((x) <= (end)))

#define assert_bool(x) assert((x) >= 0 && (x) <= 1)

#ifndef ALIGN
#define ALIGN(x, n) (((x) + (n) - 1) - ((x) + (n) - 1) % (n))
#endif

#ifdef APP_libmcu_ddr_text_section
#include <cpu/ddr_section.h>
#else
#define __ddr_text
#define __ddr_data
#define __ddr_bss
#endif

#ifdef APP_libmcu_tcsm_text_section
#include <cpu/tcsm_section.h>
#else
#define __tcsm_text
#define __tcsm_data
#define __tcsm_bss
#endif

#endif /* _COMMON_H_ */

