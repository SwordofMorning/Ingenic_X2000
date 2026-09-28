#include <stdio.h>
#include <string.h>
#include <ring_mem.h>

char __attribute__((aligned(APP_libmcu_x2580_uncache_mem_size)))uncache_mem[APP_libmcu_x2580_uncache_mem_size];

static void *alloc_ptr = uncache_mem + 32*2; // sizeof(struct ring_mem) * 2;
struct ring_mem *data_from_host = (void *)uncache_mem;
struct ring_mem *data_to_host = (void *)uncache_mem + 32;

void *uncache_mem_alloc(int size, int align)
{
    int free_size = (unsigned int)uncache_mem + APP_libmcu_x2580_uncache_mem_size - (unsigned int)alloc_ptr;

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