#ifndef _PKT_PARSE_H_
#define _PKT_PARSE_H_

int pkt_parse_str(const char *buf, const char *key, char **value);

int pkt_parse_int64(const char *buf, const char *key, int64_t *value, int base);

int pkt_parse_int(const char *buf, const char *key, int *value, int base);

int pkt_parse_short(const char *buf, const char *key, short *value, int base);

int pkt_parse_char(const char *buf, const char *key, char *value, int base);

enum pkt_parse_type {
    pkt_type_string,
    pkt_type_int,
    pkt_type_short,
    pkt_type_char,
    pkt_type_int64,
};

struct pkt_parse_info {
    const char *key;
    int off;
    char type;
    char base;
    const char *parsed_str;
};

#define pkt_off(_s, _mb) ((void *)&(_s)->_mb - (void *)(_s))

#define pkt_string(_key, _s, _mb) \
    { .key = _key, .type = pkt_type_string, .off = pkt_off(_s, _mb), }

#define pkt_int64(_key, _s, _mb, _base) \
    { .key = _key, .type = pkt_type_int64, .off = pkt_off(_s, _mb), .base = _base }

#define pkt_int(_key, _s, _mb, _base) \
    { .key = _key, .type = pkt_type_int, .off = pkt_off(_s, _mb), .base = _base }

#define pkt_short(_key, _s, _mb, _base) \
    { .key = _key, .type = pkt_type_short, .off = pkt_off(_s, _mb), .base = _base }

#define pkt_char(_key, _s, _mb, _base) \
    { .key = _key, .type = pkt_type_char, .off = pkt_off(_s, _mb), .base = _base }

#define pkt_null() {0}

void pkt_parse_struct(void *buf, void *base, struct pkt_parse_info *info);

void pkt_parse_struct_free(struct pkt_parse_info *info);

#endif /* _PKT_PARSE_H_ */
