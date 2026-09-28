#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libutils2/array.h"

void array_init(array_t *a, int init_size)
{
    if (init_size < 4)
        init_size = 4;
    a->len = 0;
    a->a = malloc(init_size*sizeof(a->a[0]));
    a->size = init_size;
    assert(a->a);
}

array_t *array_create(int init_size)
{
    array_t *a = malloc(sizeof(*a));
    array_init(a, init_size);
    return a;
}

static void check_alloc_mem(array_t *a, int size)
{
    assert(a->len <= a->size);

    if (size > a->size) {
        size = size + size / 4;
        a->a = realloc(a->a, size*sizeof(a->a[0]));
        a->size = size;
        assert(a->a);
    }
}

void array_add_(array_t *a, void *e)
{
    check_alloc_mem(a, a->len+1);

    a->a[a->len] = e;
    a->len++;
}

void *array_at(array_t *a, int index)
{
    assert(index < a->len);
    return a->a[index];
}

void array_set_(array_t *a, int index, void *e)
{
    int size = index > a->len ? index : a->len;
    check_alloc_mem(a, size+1);

    a->a[index] = e;

    if (index >= a->len)
        a->len = index+1;
}

void array_del(array_t *a, int index)
{
    assert(index < a->len);

    int len = a->len-(index+1);
    if (len)
        memmove(&a->a[index], &a->a[index+1], len*sizeof(a->a[0]));
    a->a[a->len-1] = NULL;

    a->len--;
}

void array_reset_size(array_t *a, int size)
{
    check_alloc_mem(a, size);

    a->len = size;
}

void array_deinit(array_t *a)
{
    memset(a->a, 0, a->size*sizeof(a->a[0]));
    free(a->a);
    memset(a, 0, sizeof(*a));
}

void array_delete(array_t *a)
{
    array_deinit(a);
    free(a);
}

// void test_array(void)
// {
//     array_t a;

//     array_init(&a, 2);

//     array_add(&a, "[] 第0行");
//     array_add(&a, "[] 第1行");
//     array_add(&a, "[] 第2行");
//     array_add(&a, "[] 第3行");
//     array_add(&a, "[] 第4行");
//     array_add(&a, "[] 第5行");
//     array_del(&a, 2);
//     array_del(&a, 0);
//     array_add(&a, "[] 第0行");
//     array_add(&a, "[] 第2行");

//     int i;
//     for (i = 0; i < array_size(&a); i++)
//         printf("%d . %s\n", i, (char *)array_at(&a, i));
// }

// int main(int argc, char *argv[])
// {
//     test_array();
//     return 0;
// }
