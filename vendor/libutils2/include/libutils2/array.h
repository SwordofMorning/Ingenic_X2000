#ifndef _LIBUTILS2_ARRAY_H_
#define _LIBUTILS2_ARRAY_H_

typedef struct array {
    void **a;
    int len;
    int size;
} array_t;

void array_init(array_t *a, int init_size);
array_t *array_create(int init_size);
void *array_at(array_t *a, int index);
void array_del(array_t *a, int index);
void array_reset_size(array_t *a, int size);
void array_deinit(array_t *a);
void array_delete(array_t *a);

#define array_add(a, e)        array_add_(a, (void *)(unsigned long)(e))
#define array_set(a, index, e) array_set_(a, (index), (void *)(unsigned long)(e))
#define array_size(a)          ((a)->len)

void array_add_(array_t *a, void *e);
void array_set_(array_t *a, int index, void *e);

#endif /* _LIBUTILS2_ARRAY_H_ */

