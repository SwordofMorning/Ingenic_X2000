#ifndef _libutils2_data_array_H_
#define _libutils2_data_array_H_

typedef struct data_array {
    unsigned char *a;
    int len;
    int size;
    int item_size;
} data_array_t;

void data_array_init(data_array_t *a, int item_size, int init_size);
data_array_t *data_array_create(int item_size, int init_size);
void data_array_add(data_array_t *a, void *e);
void data_array_set(data_array_t *a, int index, void *e);
void *data_array_at(data_array_t *a, int index);
void data_array_del(data_array_t *a, int index);
void data_array_reset_size(data_array_t *a, int size);
void data_array_deinit(data_array_t *a);
void data_array_delete(data_array_t *a);

#define data_array_add(a, e)         data_array_add_(a, (void *)(unsigned long)(e))
#define data_array_set(a, index, e)  data_array_set_(a, (index), (void *)(unsigned long)(e))
#define data_array_size(a)           ((a)->len)

void data_array_add_(data_array_t *a, void *e);
void data_array_set_(data_array_t *a, int index, void *e);

#endif /* _libutils2_data_array_H_ */
