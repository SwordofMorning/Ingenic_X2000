#include <stdio.h>
#include <assert.h>

#define portBYTE_ALIGNMENT_MASK (0x07)
#define portBYTE_ALIGNMENT      (8)

static unsigned char heap_space[APP_libmcu_lib_malloc_heap_size];

struct block_link {
    struct block_link *next_free_blk;
    unsigned long blk_size;
};

static const unsigned long heap_struct_size	= (sizeof(struct block_link) + (( portBYTE_ALIGNMENT - 1 ))) & ~(portBYTE_ALIGNMENT_MASK);

#define MIN_BLOCK_SIZE (heap_struct_size << 1)

static struct block_link xStart, *pxEnd = NULL;

static unsigned long free_bytes_remaining = 0U;
static unsigned long minimum_ever_free_bytes_remaining = 0U;

static unsigned long block_allocated_bit = 0;

static void heap_init(void)
{
    struct block_link *first_free_blk;
    unsigned char *aligned_heap;
    unsigned long addr;
    unsigned long total_heap_size = APP_libmcu_lib_malloc_heap_size;

    addr = (unsigned long)heap_space;

    if (addr & portBYTE_ALIGNMENT_MASK) {
        addr += (portBYTE_ALIGNMENT - 1);
        addr &= ~(portBYTE_ALIGNMENT_MASK);
        total_heap_size -= (addr - (unsigned long)heap_space);
    }

    aligned_heap = (unsigned char *)addr;

    xStart.next_free_blk = (void *)aligned_heap;
    xStart.blk_size = 0;

    addr = (unsigned long)aligned_heap + total_heap_size;
    addr -= heap_struct_size;
    addr &= ~(portBYTE_ALIGNMENT_MASK);
    pxEnd = (void *)addr;
    pxEnd->blk_size = 0;
    pxEnd->next_free_blk = NULL;

    first_free_blk = (void *)aligned_heap;
    first_free_blk->blk_size = addr - (unsigned long)first_free_blk;
    first_free_blk->next_free_blk = pxEnd;

    minimum_ever_free_bytes_remaining = first_free_blk->blk_size;
    free_bytes_remaining = first_free_blk->blk_size;

    block_allocated_bit = (1 << (sizeof(unsigned long) * 8 - 1));
}

static void add_to_free_list(struct block_link *link)
{
    struct block_link *iterator;
    unsigned char *p;

    for (iterator = &xStart; iterator->next_free_blk < link; iterator = iterator->next_free_blk);

    p = (unsigned char *)iterator;
    if ((p + iterator->blk_size) == (unsigned char *)link) {
        iterator->blk_size += link->blk_size;
        link = iterator;
    }

    p = (unsigned char *)link;
    if ((p + link->blk_size) == (unsigned char *)iterator->next_free_blk) {
        if (iterator->next_free_blk != pxEnd) {
            link->blk_size += iterator->next_free_blk->blk_size;
            link->next_free_blk = iterator->next_free_blk->next_free_blk;
        } else {
            link->next_free_blk = pxEnd;
        }
    } else {
        link->next_free_blk = iterator->next_free_blk;
    }

    if (iterator != link)
        iterator->next_free_blk = link;
}

void *malloc(unsigned long size)
{
    struct block_link *block, *pre_block, *new_block_link;
    void *mem = NULL;

    if (!size) {
        printf("malloc: failed to malloc, the malloc size must greater than 0.\n");
        return mem;
    }

    if (!pxEnd)
        heap_init();

    if (!(size & block_allocated_bit)) {
        if (size) {
            size += heap_struct_size;
            if ((size & portBYTE_ALIGNMENT_MASK) != 0) {
                size += (portBYTE_ALIGNMENT - (size & portBYTE_ALIGNMENT_MASK));
                assert((size & portBYTE_ALIGNMENT_MASK) == 0);
            }
        }

        if ((size > 0) && (size <= free_bytes_remaining)) {
            pre_block = &xStart;
            block = xStart.next_free_blk;
            while ((block->blk_size < size) && (block->next_free_blk != NULL)) {
                pre_block = block;
                block = block->next_free_blk;
            }

            if (block != pxEnd) {
                mem = (void *)((unsigned char *)pre_block->next_free_blk + heap_struct_size);

                pre_block->next_free_blk = block->next_free_blk;

                if ((block->blk_size - size) > MIN_BLOCK_SIZE) {
                    new_block_link = (void *)((unsigned char *)block + size);
                    new_block_link->blk_size = block->blk_size - size;
                    block->blk_size = size;

                    add_to_free_list(new_block_link);
                }

                free_bytes_remaining -= block->blk_size;

                if (free_bytes_remaining < minimum_ever_free_bytes_remaining)
                    minimum_ever_free_bytes_remaining = free_bytes_remaining;

                block->blk_size |= block_allocated_bit;
                block->next_free_blk = NULL;
            }
        } else {
            printf("malloc: Failed to malloc, the requested space is greater than %ld.\n", free_bytes_remaining);
        }
    } else {
        printf("malloc: Failed to malloc, the requested space is greater than %ld.\n", block_allocated_bit);
    }

    return mem;
}

void free(void *mem)
{
    unsigned char *p = (unsigned char *)mem;
    struct block_link *link;

    if (p) {
        p -= heap_struct_size;
        link = (void *)p;
        assert((link->blk_size & block_allocated_bit) != 0);
        assert(link->next_free_blk == NULL);

        if ((link->blk_size & block_allocated_bit) != 0) {
            if (link->next_free_blk == NULL) {
                link->blk_size &= ~block_allocated_bit;

                free_bytes_remaining += link->blk_size;
                add_to_free_list(link);
            }
        }
    }
}

unsigned long get_free_heap_size(void)
{
    return free_bytes_remaining;
}