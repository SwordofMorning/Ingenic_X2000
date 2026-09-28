#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern long _user_heap_start;
extern long _user_heap_end;

char *heap_ptr = (char *)&_user_heap_start;

void *_sbrk (int incr)
{
    char *base;
    char *end = (char *)((unsigned int)&_user_heap_end);

    base = heap_ptr;

    if (heap_ptr + incr >= end) {
        printf("heap memory out of range\n");
        printf("start: %p,%p now: %p, asked:%d\n",
            (char *)&_user_heap_start, end, heap_ptr, incr);
        return ((char *)-1);
    } else {
        heap_ptr += incr;
    }

    return base;
}
