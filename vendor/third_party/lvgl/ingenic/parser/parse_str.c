#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "parse_str.h"

char *strchrnul(const char *s, int c);

char *parse_double_quota_word(const char **src)
{
    const char *s = *src;
    char *a = malloc(strlen(s)+1);
    char *d = a;

    // 开头的 " 双引号
    *d++ = *s++;

    char c = 0;
    for (; *s; s++) {
        c = *s;

        if (c == '\\') {
            c = *++s;
            switch (c) {
            case '\\': case '"': case '\'': case ' ': case '\t':
                *d++ = c;
                continue;
            case '\n': case '\r':
                fprintf(stderr, "warn: 暂不支持行连接操作: (%s)\n", *src);
                *d++ = '\\';
                break;
            default:
                fprintf(stderr, "warn: 暂不支持此转义字符: \\%c: (%s)\n", c, *src);
                *d++ = '\\';
                *d++ = c;
                continue;
            }
        }
        if (c == '"') {
            *d++ = c;
            s++;
            break;
        }
        if (c== '\r' || c == '\n')
            break;

        *d++ = c;
    }

    *d = '\0';

    if (c != '"') {
        fprintf(stderr, "warn: 双引号没有成对出现: (%s)\n", *src);
        free(a);
        return NULL;
    }

    *src = s;

    return realloc(a, strlen(a)+1);
}

char *parse_single_quota_word(const char **src)
{
    const char *s = *src;
    char *a = malloc(strlen(s)+1);
    char *d = a;

    // 开头的 " 双引号
    *d++ = *s++;

    char c = 0;
    for (; *s; s++) {
        c = *s;
        if (c == '\'') {
            *d++ = c;
            s++;
            break;
        }
        if (c== '\r' || c == '\n')
            break;

        *d++ = c;
    }

    *d = '\0';

    if (c != '\'') {
        fprintf(stderr, "warn: 单引号没有成对出现: (%s)\n", *src);
        free(a);
        return NULL;
    }

    *src = s;

    return realloc(a, strlen(a)+1);
}

char *parse_oneword_seps(const char **src, const char *seps)
{
    const char *s = *src;
    if (*s == '\0')
        return NULL;

    char *a = malloc(strlen(s)+1);
    char *d = a;
    char c = 0;

    for (; *s; s++) {
        c = *s;
        if (strchr(seps, c))
            break;
        if (c == '\'' || c == '"') {
            char *p = NULL;
            if (c == '\'')
                p = parse_single_quota_word(&s);
            if (c == '"')
                p = parse_double_quota_word(&s);
            if (!p) {
                free(a);
                return NULL;
            }
            int len = strlen(p);
            memcpy(d, p, len);
            d += len;
            s--;
            free(p);
            continue;
        }

        if (c == '\\') {
            c = *++s;
            switch (c) {
            case '\\': case '"': case '\'': case ' ': case '\t':
                *d++ = c;
                continue;
            case '\n': case '\r':
                fprintf(stderr, "warn: 暂不支持行连接操作: (%s)\n", *src);
                *d++ = '\\';
                break;
            default:
                fprintf(stderr, "warn: 暂不支持此转义字符: \\%c: (%s)\n", c, *src);
                *d++ = '\\';
                *d++ = c;
                continue;
            }
        }
        *d++ = c;
    }

    *d = '\0';
    *src = s;

    if (a[0] == '\0') {
        free(a);
        return NULL;
    }
    return realloc(a, strlen(a)+1);
}

char *parse_oneword(const char **src)
{
    return parse_oneword_seps(src, " \n");
}

char **parse_oneline(const char *line)
{
    char **words = NULL;

    int i;
    const char *s = line;
    for (i = 0; *s; i++) {
        while (*s && isspace(*s))
            s++;

        char *word = parse_oneword(&s);
        if (!word)
            break;

        words = realloc(words, sizeof(words[0])*(i+1));
        words[i] = word;
    }

    words = realloc(words, sizeof(words[0])*(i+1));
    words[i] = NULL;

    return words;
}

void free_parse_words(char **words)
{
    int i;
    for (i = 0; words[i]; i++) {
        free(words[i]);
    }
    free(words);
}

int cnt_leading_space(const char *s, int tab_cnt)
{
    int cnt = 0;

    while (*s) {
        if (*s == ' ')
            cnt++;
        else if (*s == '\t')
            cnt += tab_cnt;
        else
            break;
        s++;
    }

    return cnt;
}

int parse_int(const char *str, long *value)
{
    char *e;
    long v = str[0] == '-' ? strtol(str, &e, 0) : strtoul(str, &e, 0);
    if (e == str)
        return -1;
    if (*e != '\0')
        return -1;
    *value = v;
    return 0;
}

int parse_int_array(const char *str, long *value, int n)
{
    int cnt = 0;

    while (str[0]) {
        char *e;
        long v = str[0] == '-' ? strtol(str, &e, 0) : strtoul(str, &e, 0);
        if (e == str)
            return cnt;
        *value++ = v;
        str = e;
        if (str[0])
            str++;
        cnt++;
        if (cnt == n)
            break;
    }

    return cnt;
}

int parse_alloc_int_array(const char *str, long **ptr)
{
    int cnt = 0;
    long *p = NULL;

    while (str[0]) {
        char *e;
        long v = str[0] == '-' ? strtol(str, &e, 0) : strtoul(str, &e, 0);
        if (e == str) 
            break;
        p = realloc(p, (cnt+1)*sizeof(*p));
        p[cnt] = v;
        str = e;
        if (str[0])
            str++;
        cnt++;
    }

    *ptr = p;

    return cnt;
}

char **parse_str_array(const char *str, const char *seps, int *n)
{
    char **words = NULL;

    int i;
    const char *s = str;
    for (i = 0; *s; i++) {
        char *word = parse_oneword_seps(&s, seps);
        if (!word)
            break;

        if (*s)
            s++;

        words = realloc(words, sizeof(words[0])*(i+1));
        words[i] = word;
    }

    if (n)
        *n = i;

    words = realloc(words, sizeof(words[0])*(i+1));
    words[i] = NULL;

    return words;
}

int parse_str(const char *str, long *value)
{
    int len = strlen(str);
    if (len < 2)
        return -1;
    if (str[0] != '"' && str[0] != '\'')
        return -1;
    if (str[0] != str[len-1])
        return -1;
    *value = (long) strndup(str+1, len-2);
    return 0;
}

int parse_str2(const char *str, char **value)
{
    int len = strlen(str);
    if (len < 2)
        return -1;
    if (str[0] != '"' && str[0] != '\'')
        return -1;
    if (str[0] != str[len-1])
        return -1;
    char *v = strndup(str+1, len-2);
    if (*value)
        free(*value);
    *value = v;
    return 0;
}

int check_hex_digital_count(const char *s)
{
    int i = 0;
    while (*s) {
        if ('0' <= *s && *s <= '9')
            i++;
        else {
            int c = tolower(*s);
            if ('a' <= c && c <= 'f')
                i++;
            else
                break;
        }
        s++;
    }
    return i;
}

int parse_color(const char *str, long *value, int *opa)
{
    const char *s = str;
    if (str[0] == '0' && tolower(str[1] == 'x'))
        s = str+2;

    int count = check_hex_digital_count(s);
    if (count != 8 && count != 6)
        return -1;

    unsigned long v = strtoul(s, NULL, 16);
    if (count == 8) {
        *opa = v >> 24;
        v = v & 0x00ffffff;
    } else {
        *value = v;
        *opa = -1;
    }

    return 0;
}

int parse_enum(struct enum_pair *p, const char *str, long *value)
{
    int i;
    for (i = 0; p[i].name; i++) {
        if (!strcmp(p[i].name, str)) {
            *value = p[i].value;
            return 0;
        }
    }

    return -1;
}

const char *parse_prefix(struct enum_pair *p, const char *str, long *value)
{
    int i;
    for (i = 0; p[i].name; i++) {
        int n = strlen(p[i].name);
        if (!strncmp(p[i].name, str, n)) {
            *value = p[i].value;
            return str+n;
        }
    }

    return NULL;
}

static int tab_cnt = 4;

char **get_valid_line_words(FILE *in, int *depth, char **line_ptr)
{
    char *line = NULL;
    size_t line_size = 0;

    while (1) {
        int ret = getline(&line, &line_size, in);
        if (ret <= 0) {
            free(line);
            if (feof(in))
                return NULL;
            fprintf(stderr, "failed to read file: %s\n", strerror(errno));
            return NULL;
        }

        // 如果是注释,那么跳过
        int cnt = cnt_leading_space(line, tab_cnt);
        if (line[cnt] == '#')
            continue;

        char **words = parse_oneline(line);

        // 如果不是空行,那么返回
        if (words[0]) {
            if (line_ptr)
                *line_ptr = line;
            else
                free(line);
            if (depth)
                *depth = cnt;
            return words;
        }

        free_parse_words(words);
    }

    return NULL;
}

int get_one_line(const char **str, char **line, size_t *line_size)
{
    const char *s = *str;
    if (*s == '\0')
        return 0;

    char *e = strchrnul(s, '\n');
    int n = e-s + (*e != 0);
    if (!*line || *line_size < (n+1)) {
        *line = realloc(*line, n+1);
        *line_size = n+1;
    }

    memcpy(*line, s, n);
    (*line)[n] = '\0';

    *str = *e ? e+1 : e;

    return n;
}

char **get_valid_line_words2(const char **str, int *depth, char **line_ptr)
{
    char *line = NULL;
    size_t line_size = 0;

    while (1) {
        int ret = get_one_line(str, &line, &line_size);
        if (ret <= 0) {
            free(line);
            return NULL;
        }

        // 如果是注释,那么跳过
        int cnt = cnt_leading_space(line, tab_cnt);
        if (line[cnt] == '#')
            continue;

        char **words = parse_oneline(line);

        // 如果不是空行,那么返回
        if (words[0]) {
            if (line_ptr)
                *line_ptr = line;
            else
                free(line);
            if (depth)
                *depth = cnt;
            return words;
        }

        free_parse_words(words);
    }

    return NULL;
}
