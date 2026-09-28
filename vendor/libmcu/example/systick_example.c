#include <stdio.h>
#include <string.h>
#include <delay.h>
#include <driver/systick.h>

void systick_example(void)
{
    /* please ensure support long long type(XCFG_FORMAT_LONGLONG == 1) */
    uint64_t count0 = systick_get_count();
    uint64_t usec0 = systick_get_time_usec();
    uint64_t nsec0 = systick_get_time_nsec();

    udelay(1*1000*1000);

    uint64_t count1 = systick_get_count();
    uint64_t usec1 = systick_get_time_usec();
    uint64_t nsec1 = systick_get_time_nsec();

    uint64_t count = count1 - count0;
    uint64_t usec = usec1 - usec0;
    uint64_t nsec = nsec1 - nsec0;

    printf("count: %lld, usec_to_count: %lld, nsec_to_count: %lld\n", count,
            systick_usec_to_count(usec), systick_nsec_to_count(nsec));

    printf("count_to_usec: %lld, usec: %lld\n", systick_count_to_usec(count), usec);

    printf("count_to_nsec: %lld, nsec: %lld\n", systick_count_to_nsec(count), nsec);
}