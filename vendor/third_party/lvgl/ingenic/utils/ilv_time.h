#ifndef _ILV_TIMER_H_
#define _ILV_TIMER_H_

#include <time.h>
#include <sys/time.h>

void ilv_get_time(struct tm *tm, int *usecs);

#endif /* _ILV_TIMER_H_ */
