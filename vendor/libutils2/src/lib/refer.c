#include <stdio.h>
#include <stdatomic.h>
#include "libutils2/refer.h"

void refer_init(refer_t *refer, refer_delete_cb_t cb, int count)
{
    atomic_store(&refer->count, count);
    refer->cb = cb;
#ifdef REFER_DEBUG
    refer->func = NULL;
    refer->name = NULL;
#endif
}

int refer_add(refer_t *refer, int count)
{
    int v = atomic_fetch_add(&refer->count, count);
#ifdef REFER_DEBUG
    fprintf(stderr, "refer: add refer:%p %s@%s (%d)+(%d) -> %d\n", refer, refer->name, refer->func, v, count, v+count);
#endif
    if (v < 0) {
#ifndef REFER_DEBUG
        fprintf(stderr, "refer: don't add bad refer:%p cb:%p\n", refer, refer->cb);
#else
        fprintf(stderr, "refer: don't add bad refer:%p %s@%s cb:%p\n", refer, refer->name, refer->func, refer->cb);
#endif
        atomic_store(&refer->count, -1);
        return v;
    }

    return v+count;
}

int refer_sub(refer_t *refer, int count)
{
    int v = atomic_fetch_sub(&refer->count, count);
#ifdef REFER_DEBUG
    fprintf(stderr, "refer: sub refer:%p %s@%s (%d)-(%d) -> %d\n", refer, refer->name, refer->func, v, count, v-count);
#endif
    if (v <= 0) {
#ifndef REFER_DEBUG
        fprintf(stderr, "refer: don't sub bad refer:%p cb:%p\n", refer, refer->cb);
#else
        fprintf(stderr, "refer: don't sub bad refer:%p %s@%s cb:%p\n", refer, refer->name, refer->func, refer->cb);
#endif
        atomic_store(&refer->count, -1);
        return v;
    }
    if (v-count <= 0) {
#ifdef REFER_DEBUG
        fprintf(stderr, "refer: clean refer:%p %s@%s cb:%p\n", refer, refer->name, refer->func, refer->cb);
#endif
        if (refer->cb)
            refer->cb(refer);
    }
    return v-count;
}
