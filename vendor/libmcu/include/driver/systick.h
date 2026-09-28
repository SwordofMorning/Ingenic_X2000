#ifndef _DRIVER_SYSTICK_H_
#define _DRIVER_SYSTICK_H_

#include <stdint.h>

typedef void (*systick_event_handler_t)(void);

void systick_init(void);

uint64_t systick_count_to_usec(uint64_t count);

uint64_t systick_usec_to_count(uint64_t usec);

uint64_t systick_count_to_nsec(uint64_t count);

uint64_t systick_nsec_to_count(uint64_t nsec);

uint64_t systick_get_time_usec(void);

uint64_t systick_get_time_nsec(void);

uint64_t systick_get_count(void);

void systick_set_next_usec(uint64_t usec);

void systick_set_event_count(uint64_t relative_count);

void systick_set_event_usec(uint64_t relative_usec);

void systick_set_event_callback(systick_event_handler_t callback);

#endif /* _DRIVER_TIMER_H_ */
