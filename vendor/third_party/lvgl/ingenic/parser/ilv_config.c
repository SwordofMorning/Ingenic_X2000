#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "libutils2/data_array.h"
#include "parser/ilv_config.h"

static data_array_t *configs;

ilv_config_type_t *find_config_type(const char *type)
{
    int i;
    for (i = 0; i < data_array_size(configs); i++) {
        ilv_config_type_t *t = data_array_at(configs, i);
        if (!strcmp(t->name, type))
            return t;
    }
    return NULL;
}

ilv_config_type_t *find_config_type2(const char *type, int n)
{
    int i;
    for (i = 0; i < data_array_size(configs); i++) {
        ilv_config_type_t *t = data_array_at(configs, i);
        if (!strncmp(t->name, type, n) && t->name[n] == '\0')
            return t;
    }
    return NULL;
}

static ilv_config_t *find_config(ilv_config_type_t *tt, const char *name)
{
    int i;
    for (i = 0; i < data_array_size(&tt->a); i++) {
        ilv_config_t *t = data_array_at(&tt->a, i);
        if (!strcmp(t->name, name))
            return t;
    }
    return NULL;
}

int ilv_config_add_type(const char *type,
    ilv_config_alloc_cb_t alloc_cb, ilv_config_parse_cb_t parse_cb,
     ilv_config_add_cb_t add_cb, ilv_config_free_cb_t free_cb)
{
    assert(alloc_cb);
    assert(parse_cb);
    assert(add_cb);

    ilv_config_type_t *tt = find_config_type(type);
    if (tt) {
        fprintf(stderr, "ilv_config: type:%s can't registere twice\n", type);
        return -1;
    }

    ilv_config_type_t tmp = {.name = type, .alloc_cb = alloc_cb,
         .parse_cb = parse_cb, .add_cb = add_cb, .free_cb = free_cb,};
    data_array_add(configs, &tmp);
    tt = data_array_at(configs, data_array_size(configs)-1);
    data_array_init(&tt->a, sizeof(ilv_config_t), 4);

    return 0;
}

int ilv_config_add(const char *type, const char *name, void *value)
{
    assert(type);
    assert(name);

    ilv_config_type_t *tt = find_config_type(type);
    if (!tt) {
        fprintf(stderr, "ilv_config: type:%s not registered\n", type);
        return -1;
    }

    ilv_config_t *ct = find_config(tt, name);
    if (ct) {
        fprintf(stderr, "ilv_config: %s.%s can't add twice\n", type, name);
        return -1;
    }

    ilv_config_t tmp = {.name = name, .value = value};
    data_array_add(&tt->a, &tmp);

    return 0;
}

int ilv_config_del(const char *type, const char *name)
{
    assert(type);
    assert(name);

    ilv_config_type_t *tt = find_config_type(type);
    if (!tt) {
        fprintf(stderr, "ilv_config: type:%s not registered\n", type);
        return -1;
    }

    ilv_config_t *ct = find_config(tt, name);
    if (!ct) {
        fprintf(stderr, "ilv_config: %s.%s not added\n", type, name);
        return -1;
    }

    int index = ((void *)ct - (void *)tt->a.a)/tt->a.item_size;
    data_array_del(&tt->a, index);

    return 0;
}

int ilv_config_get(const char *type, const char *name, void **value)
{
    assert(type);
    assert(name);

    ilv_config_type_t *tt = find_config_type(type);
    if (!tt) {
        fprintf(stderr, "ilv_config: type:%s not registered\n", type);
        return -1;
    }

    ilv_config_t *ct = find_config(tt, name);
    if (!ct) {
        fprintf(stderr, "ilv_config: %s.%s not added\n", type, name);
        return -1;
    }

    *value = ct->value;
    return 0;
}

void ilv_config_init_font(void);

void ilv_config_init(void)
{
    configs = data_array_create(sizeof(ilv_config_type_t), 4);

    ilv_config_init_font();
}
