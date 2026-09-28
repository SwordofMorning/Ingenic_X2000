#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libutils2/data_array.h"

#define POS(array, index) ((array)->a+(array)->item_size*(index))

void data_array_init(data_array_t *a, int item_size, int init_size)
{
    assert(item_size > 0);

    if (init_size < 4)
        init_size = 4;
    a->len = 0;
    a->a = malloc(init_size*item_size*sizeof(a->a[0]));
    a->size = init_size;
    a->item_size = item_size;
    assert(a->a);
}

data_array_t *data_array_create(int item_size, int init_size)
{
    data_array_t *a = malloc(sizeof(*a));
    data_array_init(a, item_size, init_size);
    return a;
}

static void check_alloc_mem(data_array_t *a, int size)
{
    assert(a->len <= a->size);

    if (size > a->size) {
        size = size + size / 4;
        a->a = realloc(a->a, size*a->item_size*sizeof(a->a[0]));
        a->size = size;
        assert(a->a);
    }
}

void data_array_add_(data_array_t *a, void *e)
{
    check_alloc_mem(a, a->len+1);

    memcpy(POS(a, a->len), e, a->item_size);
    a->len++;
}

void *data_array_at(data_array_t *a, int index)
{
    assert(index < a->len);
    return POS(a, index);
}

void data_array_get(data_array_t *a, int index, void *e)
{
    assert(index < a->len);

    memcpy(e, POS(a, index), a->item_size);
}

void data_array_set_(data_array_t *a, int index, void *e)
{
    int size = index > a->len ? index : a->len;
    check_alloc_mem(a, size+1);

    memcpy(POS(a, index), e, a->item_size);

    if (index >= a->len)
        a->len = index+1;
}

void data_array_del(data_array_t *a, int index)
{
    assert(index < a->len);

    int len = a->len-(index+1);
    if (len)
        memmove(POS(a, index), POS(a, index+1), len*a->item_size);

    memset(POS(a, a->len-1), 0, a->item_size);

    a->len--;
}

void data_array_reset_size(data_array_t *a, int size)
{
    check_alloc_mem(a, size);

    a->len = size;
}

void data_array_deinit(data_array_t *a)
{
    memset(a->a, 0, a->size*a->item_size*sizeof(a->a[0]));
    free(a->a);
    memset(a, 0, sizeof(*a));
}

void data_array_delete(data_array_t *a)
{
    data_array_deinit(a);
    free(a);
}

// void test_array(void)
// {
//     data_array_t a;

//     data_array_init(&a, 5, 2);

//     data_array_add(&a, "[] 0");
//     data_array_add(&a, "[] 1");
//     data_array_add(&a, "[] 2");
//     data_array_add(&a, "[] 3");
//     data_array_add(&a, "[] 4");
//     data_array_add(&a, "[] 5");
//     data_array_del(&a, 2);
//     data_array_del(&a, 0);
//     data_array_add(&a, "[] 0");
//     data_array_add(&a, "[] 2");

//     int i;
//     for (i = 0; i < data_array_size(&a); i++)
//         printf("%d . %s\n", i, (char *)data_array_at(&a, i));
// }

// int main(int argc, char *argv[])
// {
//     test_array();

//     return 0;
// }
