#ifndef _MALLOC_H_
#define _MALLOC_H_


void *malloc(unsigned long size);

void free(void *mem);

unsigned long get_free_heap_size(void);

#endif