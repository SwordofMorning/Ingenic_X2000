#include <stdio.h>
#include <string.h>
#include <ring_mem.h>

#ifndef APP_libmcu_x2600_run_in_tcsm
int uncache_mem_size = APP_libmcu_x2600_uncache_mem_size;
char __attribute__((aligned(APP_libmcu_x2600_uncache_mem_size)))uncache_mem[APP_libmcu_x2600_uncache_mem_size];

struct ring_mem *data_from_host = (void *)uncache_mem;
struct ring_mem *data_to_host = (void *)uncache_mem + 32;
static void *alloc_ptr = uncache_mem + 32*2; // sizeof(struct ring_mem) * 2;
#else
int uncache_mem_size = 8192; // use cpu sram as uncache mem(from 0x12400000 to 0x12401FFF)
extern unsigned char __uncache_start;
char *uncache_mem = (char *)&__uncache_start;

struct ring_mem *data_from_host = (void *)&__uncache_start;
struct ring_mem *data_to_host = (void *)&__uncache_start + 32;
static void *alloc_ptr = &__uncache_start + 32*2; // sizeof(struct ring_mem) * 2;
#endif

void *uncache_mem_alloc(int size, int align)
{
    int free_size = (unsigned int)uncache_mem + uncache_mem_size - (unsigned int)alloc_ptr;

    int apos = 0;
    if (align && (unsigned long)alloc_ptr % align)
        apos = align - (unsigned long)alloc_ptr % align;

    if (free_size < (size + apos)) {
        printf("uncache_mem: alloc_err free size = %d, alloc_size = %d\n", free_size, size);
        return NULL;
    }

    void *ret_ptr = alloc_ptr;

    alloc_ptr += size + apos;

    return ret_ptr + apos;
}