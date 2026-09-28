#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_time.h"

static struct timeval mtv;
static struct tm mtm;
static lv_timer_t *timer;

static void m_timer_cb(struct _lv_timer_t *timer)
{
    gettimeofday(&mtv, NULL);
    struct tm *tm = localtime(&mtv.tv_sec);
    mtm = *tm;
}

void ilv_get_time(struct tm *tm, int *usecs)
{
    if (!timer) {
        timer = lv_timer_create(m_timer_cb, 5, NULL);
        m_timer_cb(timer);
    }

    if (tm)
        *tm = mtm;
    if (usecs)
        *usecs = mtv.tv_usec;
}
