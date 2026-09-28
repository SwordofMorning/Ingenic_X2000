#ifndef _libutils2_listmap_H_
#define _libutils2_listmap_H_

#include "libutils2/array.h"

typedef struct map_data_t {
    const char *name;
    void *data;
} map_data_t;

typedef int (*listmap_compare_method_t)(const char *name0, const char *name1);

typedef struct listmap_t {
    array_t list;
    map_data_t *src;
    listmap_compare_method_t compare;
} listmap_t;

void listmap_add(struct listmap_t *map, const char *name, void *data);

void listmap_set(listmap_t *map, const char *name, void *data);

void listmap_set_va(listmap_t *map, va_list args);

void listmap_sets(listmap_t *map, ...);

void listmap_del(listmap_t *map, const char *name);

void listmap_init_str_va(listmap_t *map, map_data_t *src, va_list args);
void listmap_init_str(listmap_t *map, map_data_t *list, ...);

void listmap_init_ptr_va(listmap_t *map, map_data_t *src, va_list args);
void listmap_init_ptr(listmap_t *map, map_data_t *list, ...);

void listmap_init_va(listmap_t *map, listmap_compare_method_t compare, map_data_t *src, va_list args);
void listmap_init(listmap_t *map, listmap_compare_method_t compare, map_data_t *list, ...);

void listmap_deinit(listmap_t *map);

listmap_t *listmap_create_str_va(map_data_t *list, va_list args);
listmap_t *listmap_create_str(map_data_t *list, ...);

listmap_t *listmap_create_ptr_va(map_data_t *list, va_list args);
listmap_t *listmap_create_ptr(map_data_t *list, ...);

listmap_t *listmap_create_va(listmap_compare_method_t comapre, map_data_t *list, va_list args);
listmap_t *listmap_create(listmap_compare_method_t comapre, map_data_t *list, ...);

void listmap_delete(listmap_t *map);

void *listmap_get(listmap_t *map, const char *name, void *null_data);

void *listmap_get_overlay(listmap_t *src, listmap_t *overlay, const char *name, void *null_data);

#define listmap_get_int(map, name, null_data) \
    (unsigned long) listmap_get((map), (name), (void *)(unsigned long)(null_data))

void *listmap_get_index(listmap_t *map, int index, void **name);

#define listmap_size(map) (array_size((map)->list)/2)

#endif /* _libutils2_listmap_H_ */
