#ifndef _ILV_CONFIG_H_
#define _ILV_CONFIG_H_

#include "libutils2/data_array.h"

typedef struct ilv_config {
    const char *name;
    void *value;
} ilv_config_t;

typedef void *(*ilv_config_alloc_cb_t)(void);
typedef int (*ilv_config_parse_cb_t)(const char *key, const char *value, void *data);
typedef int (*ilv_config_add_cb_t)(void *value);
typedef void (*ilv_config_free_cb_t)(void *value);

typedef struct ilv_config_type {
    const char *name;
    ilv_config_alloc_cb_t alloc_cb;
    ilv_config_parse_cb_t parse_cb;
    ilv_config_add_cb_t add_cb;
    ilv_config_free_cb_t free_cb;
    data_array_t a;
} ilv_config_type_t;

ilv_config_type_t *find_config_type(const char *type);

ilv_config_type_t *find_config_type2(const char *type, int n);

int ilv_config_add_type(const char *type,
    ilv_config_alloc_cb_t alloc_cb, ilv_config_parse_cb_t parse_cb,
     ilv_config_add_cb_t add_cb, ilv_config_free_cb_t free_cb);

int ilv_config_add(const char *type, const char *name, void *value);

int ilv_config_del(const char *type, const char *name);

int ilv_config_get(const char *type, const char *name, void **value);

void ilv_config_init(void);

#endif /* _ILV_CONFIG_H_ */
