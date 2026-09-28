#include <time.h>
#include <stdint.h>
#include <libutils2/boot_time.h>

uint64_t boot_time_usecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

double boot_time_secs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (double) ts.tv_sec + (double) (ts.tv_nsec) / 1000000000;
}
