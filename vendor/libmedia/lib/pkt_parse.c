#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <libmedia/utils/pkt_parse.h>

static char *parse_key(const char *buf, const char *key, char **end_p)
{
    buf = strstr(buf, key);
    if (buf == NULL)
        return NULL;
 
    buf += strlen(key);
    if (*buf != '=')
        return NULL;

    buf += 1;
    char *end = strchr(buf, '\n');
    if (end == NULL) {
        fprintf(stderr, "pkt: %s=%s not end with \\n\n", key, buf);
        return NULL;
    }

    if (end_p)
        *end_p = end;

    return (void *)buf;
}

int pkt_parse_str(const char *buf, const char *key, char **value)
{
    assert(value);

    char *end;
    buf = parse_key(buf, key, &end);
    if (buf == NULL)
        return -1;

    int len = end - buf;
    char *p = malloc(len + 1);
    assert(p);
    memcpy(p, buf, len);
    p[len] = '\0';

    *value = p;

    return 0;
}

int pkt_parse_int(const char *buf, const char *key, int *value, int base)
{
    assert(value);

    buf = parse_key(buf, key, NULL);
    if (buf == NULL)
        return -1;

    char *end;
    int v = strtol(buf, &end, base);
    if (end == NULL || *end != '\n') {
        fprintf(stderr, "pkt: %s=%s not a valid int\\n\n", key, buf);
        return -1;
    }

    *value = v;

    return 0;
}

int pkt_parse_short(const char *buf, const char *key, short *value, int base)
{
    int v;
    if (pkt_parse_int(buf, key, &v, base))
        return -1;
    *value = v;
    return 0;
}

int pkt_parse_char(const char *buf, const char *key, char *value, int base)
{
    int v;
    if (pkt_parse_int(buf, key, &v, base))
        return -1;
    *value = v;
    return 0;
}

int pkt_parse_int64(const char *buf, const char *key, int64_t *value, int base)
{
    assert(value);

    buf = parse_key(buf, key, NULL);
    if (buf == NULL)
        return -1;

    char *end;
    int64_t v = strtoll(buf, &end, base);
    if (end == NULL || *end != '\n') {
        fprintf(stderr, "pkt: %s=%s not a valid int\\n\n", key, buf);
        return -1;
    }

    *value = v;

    return 0;
}

void pkt_parse_struct(void *buf, void *base, struct pkt_parse_info *info)
{
    int ret;

    while (info->key) {
        info->parsed_str = NULL;
        printf("%p %d %d\n", base, (int)info->type, info->off);
        switch (info->type) {
        case pkt_type_string:
            ret = pkt_parse_str(buf, info->key, base+info->off);
            if (ret == 0)
                info->parsed_str = *(char **)(base+info->off);
            break;
        case pkt_type_int64:
            pkt_parse_int64(buf, info->key, base+info->off, info->base);
            break;
        case pkt_type_int:
            pkt_parse_int(buf, info->key, base+info->off, info->base);
            break;
        case pkt_type_short:
            pkt_parse_short(buf, info->key, base+info->off, info->base);
            break;
        case pkt_type_char:
            pkt_parse_char(buf, info->key, base+info->off, info->base);
            break;
        default:
            fprintf(stderr, "pkt: unknow type: %d\n", info->type);
            break;
        }
        info++;
    }
}

void pkt_parse_struct_free(struct pkt_parse_info *info)
{
    while (info->key) {
        if (info->parsed_str)
            free((void *)info->parsed_str);
        info++;
    }
}
