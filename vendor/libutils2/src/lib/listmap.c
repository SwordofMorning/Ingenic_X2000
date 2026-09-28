#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "libutils2/listmap.h"

void listmap_add(struct listmap_t *map, const char *name, void *data)
{
    array_add(&map->list, name);
    array_add(&map->list, data);
}

static int listmap_find(listmap_t *map, const char *name)
{
    int i;
    for (i = 0; i < array_size(&map->list); i += 2) {
        const char *name_ = array_at(&map->list, i);
        if (!map->compare(name_, name)) {
            return i;
        }
    }
    return -1;
}

void listmap_del(listmap_t *map, const char *name)
{
    int i = listmap_find(map, name);
    if (i >= 0) {
        array_del(&map->list, i);
        array_del(&map->list, i);
    }
}

void listmap_set(listmap_t *map, const char *name, void *data)
{
    int i = listmap_find(map, name);
    if (i >= 0) {
        array_set(&map->list, i+1, data);
    } else {
        array_add(&map->list, name);
        array_add(&map->list, data);
    }
}

void listmap_set_va(listmap_t *map, va_list args)
{
    while (1) {
        const char *name = va_arg(args, const char *);
        if (name == NULL)
            break;
        void *data = va_arg(args, void *);
        listmap_set(map, name, data);
    }    
}

void listmap_sets(listmap_t *map, ...)
{
    va_list args;

    va_start(args, map);
    listmap_set_va(map, args);
    va_end(args);
}

void listmap_init_va(listmap_t *map, listmap_compare_method_t compare, map_data_t *src, va_list args)
{
    array_init(&map->list, 8);
    map->src = src;
    map->compare = compare;

    if (src) {
        while (src->name) {
            array_add(&map->list, src->name);
            array_add(&map->list, src->data);
            src++;
        }
    }

    listmap_set_va(map, args);
}

void listmap_init(listmap_t *map, listmap_compare_method_t compare, map_data_t *list, ...)
{
    va_list args;

    va_start(args, list);
    listmap_init_va(map, compare, list, args);
    va_end(args);
}

void listmap_init_str_va(listmap_t *map, map_data_t *src, va_list args)
{
    listmap_init_va(map, (void *) strcmp, src, args);
}

void listmap_init_str(listmap_t *map, map_data_t *list, ...)
{
    va_list args;

    va_start(args, list);
    listmap_init_str_va(map, list, args);
    va_end(args);
}

static int ptr_compare(const char *name0, const char *name1)
{
    return !(name0 == name1);
}

void listmap_init_ptr_va(listmap_t *map, map_data_t *src, va_list args)
{
    listmap_init_va(map, ptr_compare, src, args);
}

void listmap_init_ptr(listmap_t *map, map_data_t *list, ...)
{
    va_list args;

    va_start(args, list);
    listmap_init_ptr_va(map, list, args);
    va_end(args);
}

void listmap_deinit(listmap_t *map)
{
    array_deinit(&map->list);
}

listmap_t *listmap_create_va(listmap_compare_method_t compare, map_data_t *list, va_list args)
{
    listmap_t *map = malloc(sizeof(*map));

    listmap_init_va(map, compare, list, args);

    return map;
}

listmap_t *listmap_create(listmap_compare_method_t compare, map_data_t *list, ...)
{
    va_list args;
    listmap_t *map;

    va_start(args, list);
    map = listmap_create_va(compare, list, args);
    va_end(args);

    return map;
}

listmap_t *listmap_create_str_va(map_data_t *list, va_list args)
{
    return listmap_create_va((void *)strcmp, list, args);
}

listmap_t *listmap_create_str(map_data_t *list, ...)
{
    va_list args;
    listmap_t *map;

    va_start(args, list);
    map = listmap_create_str_va(list, args);
    va_end(args);

    return map;
}

listmap_t *listmap_create_ptr_va(map_data_t *list, va_list args)
{
    return listmap_create_va(ptr_compare, list, args);
}

listmap_t *listmap_create_ptr(map_data_t *list, ...)
{
    va_list args;
    listmap_t *map;

    va_start(args, list);
    map = listmap_create_ptr_va(list, args);
    va_end(args);

    return map;
}

void listmap_delete(listmap_t *map)
{
    listmap_deinit(map);
    free(map);
}

void *listmap_get(listmap_t *map, const char *name, void *null_data)
{
    int i = listmap_find(map, name);
    return i < 0 ? null_data : array_at(&map->list, i+1);
}

void *listmap_get_overlay(listmap_t *src, listmap_t *overlay, const char *name, void *null_data)
{
    int i = listmap_find(overlay, name);
    if (i >= 0)
        return array_at(&overlay->list, i+1);
    return listmap_get(src, name, null_data);
}

void *listmap_get_index(listmap_t *map, int index, void **name)
{
    if (name)
        *name = array_at(&map->list, index*2);
    return array_at(&map->list, index*2+1);
}
