#ifndef _PARSE_STR_H_
#define _PARSE_STR_H_

#include <stdio.h>

struct enum_pair {
    const char *name;
    long value;
};

int parse_enum(struct enum_pair *p, const char *str, long *value);

const char *parse_prefix(struct enum_pair *p, const char *str, long *value);

char *parse_double_quota_word(const char **src);

char *parse_single_quota_word(const char **src);

char *parse_oneword(const char **src);

char **parse_oneline(const char *line);

void free_parse_words(char **words);

int cnt_leading_space(const char *s, int tab_cnt);

int parse_int(const char *str, long *value);

int parse_int_array(const char *str, long *value, int n);

int parse_alloc_int_array(const char *str, long **ptr);

int parse_str(const char *str, long *value);

int parse_str2(const char *str, char **value);

char **parse_str_array(const char *str, const char *seps, int *n);

int check_hex_digital_count(const char *s);

int parse_color(const char *str, long *value, int *opa);

char **get_valid_line_words(FILE *in, int *depth, char **line_ptr);

char **get_valid_line_words2(const char **str, int *depth, char **line_ptr);

#endif /* _PARSE_STR_H_ */
