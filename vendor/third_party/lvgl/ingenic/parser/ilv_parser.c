#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "lvgl_ingenic_support.h"
#include "parse_str.h"
#include "parser/ilv_parser.h"
#include "libutils2/data_array.h"
#include "parser/ilv_config.h"

struct free_pair {
    void *data;
    void (*free_cb)(void *);
};

typedef struct config_pair {
    ilv_config_type_t *tt;
    void *value;
} config_pair_t;

static void parse_attrs(ilv_config_type_t *tt, char **words, void *value)
{
    int i;
    for (i = 0; words[i]; i++) {
        char *s = strchr(words[i], '=');
        if (s == NULL) {
            fprintf(stderr, "warn: 属性值后面应该接 '=': %s\n", words[i]);
            continue;
        }

        s[0] = '\0';
        tt->parse_cb(words[i], s+1, value);
        s[0] = '=';
    }
}

char **ilv_parser_get_line_words(ilv_parser_t *list, int *depth, char **line_ptr)
{
    if (list->in)
        return get_valid_line_words(list->in, depth, line_ptr);
    else
        return get_valid_line_words2(&list->str, depth, line_ptr);
}

static int get_next_config_words(ilv_parser_t *list, ilv_config_type_t *tt, void *value, char **line_ptr)
{
    char **words = NULL;
    char *line;
    int depth;

    while (1) {
        words = ilv_parser_get_line_words(list, &depth, &line);
        list->save_words = words;
        if (!words)
            return 0;

        char *w0 = words[0];
        int wlen = strlen(w0);
        if (w0[wlen-1] == ':' && w0[0] != ':') {
            if (line_ptr)
                *line_ptr = line;
            else
                free(line);
            return depth;
        }

        if (w0[0] == ':')
            fprintf(stderr, "error: 控件类型不能为空 : (%s)\n", line);
        if (!list->cur_group) {
            fprintf(stderr, "error: 控件类型后面应该接 ':' : (%s)\n", line);
            fprintf(stderr, "error: 属性没有依附控件 或 控件类型后面应该接 ':' : (%s)\n", line);
        } else {
            parse_attrs(tt, words, value);
        }

        free(line);
        free_parse_words(words);
    }

    return 0;
}

static int parse_config(ilv_parser_t *list)
{
    int len  = strlen("config.");
    const char *type = list->save_words[0]+len;
    len = strlen(type);
    if (len == 0 || type[len-1] != ':') {
        fprintf(stderr, "error: config 类型后应接 ':' : %s\n", list->save_words[0]);
        return -1;
    }

    ilv_config_type_t *tt = find_config_type2(type, strlen(type)-1);
    if (!tt) {
        fprintf(stderr, "error: 未知的config类型: %s\n", list->save_words[0]);
        return -1;
    }

    void *value = tt->alloc_cb();
    parse_attrs(tt, &list->save_words[1], value);
    free_parse_words(list->save_words);
    int ret = get_next_config_words(list, tt, value, NULL);
    config_pair_t config = {.tt = tt, .value = value};
    data_array_add(list->configs, &config);

    return ret;
}


void ilv_parser_add_delete_data(ilv_parser_t *list, void *data, void (*free_cb)(void *))
{
    assert(free_cb);
    struct free_pair t = {.data = data, .free_cb = free_cb};
    data_array_add(list->free_datas, &t);
}

int ilv_parser_add_style(ilv_parser_t *list, int type, long value)
{
    // int count = data_array_size(list->cur_group);
    // if (type == LV_style_view) {
    //     lv_view_data_t *t = (void *)value;
    //     printf("add-view[%d]: %s\n", count, t->type);
    // } else if (type == LV_style_layout_start)
    //     printf("add-style[%d]: layout start\n", count);
    // else if (type == LV_style_layout_end)
    //     printf("add-style[%d]: layout end\n", count);
    // else if (type == LV_style_view_name)
    //     printf("add-style[%d]: id=%s\n", count, (char *)value);
    // else
    //     printf("add-style[%d]: %d %ld\n", count, type, value);

    ilv_style_t s = {.style = type, .value = value};
    data_array_add(list->cur_group, &s);

    return 0;
}

int ilv_parser_add_style_str(ilv_parser_t *list, int type, const char *key, const char *value)
{
    char *str = NULL;
    if (parse_str2(value, &str)) {
        fprintf(stderr, "error: 无法获取字符串属性值: %s=%s\n", key, value);
        return -1;
    }
    ilv_parser_add_delete_data(list, str, free);
    ilv_parser_add_style(list, type, (long)str);
    return 0;
}

int ilv_parser_add_style_int(ilv_parser_t *list, int type, const char *key, const char *value)
{
    long v;
    if (parse_int(value, &v)) {
        fprintf(stderr, "error: 无法获取整数属性值: %s=%s\n", key, value);
        return -1;
    }
    return ilv_parser_add_style(list, type, v);
}

int ilv_parser_add_style_point(ilv_parser_t *list, int type, const char *key, const char *value)
{
    long a[2];
    if (parse_int_array(value, a, 2) != 2) {
        fprintf(stderr, "error: 无法获取坐标属性值: %s=%s\n", key, value);
        return -1;
    }
    lv_point_t *p = malloc(sizeof(*p));
    *p = (lv_point_t) {a[0], a[1]};
    ilv_parser_add_delete_data(list, p, free);
    ilv_parser_add_style(list, type, (long)p);
    return 0;
}

#define LV_PALETTE_BLACK 100
#define LV_PALETTE_WHITE 101

static struct enum_pair color_values[] = {
    {"red", LV_PALETTE_RED},
    {"pink", LV_PALETTE_PINK},
    {"purple", LV_PALETTE_PURPLE},
    {"deep_purple", LV_PALETTE_DEEP_PURPLE},
    {"indigo", LV_PALETTE_INDIGO},
    {"blue", LV_PALETTE_BLUE},
    {"light_blue", LV_PALETTE_LIGHT_BLUE},
    {"cyan", LV_PALETTE_CYAN},
    {"teal", LV_PALETTE_TEAL},
    {"green", LV_PALETTE_GREEN},
    {"light_green", LV_PALETTE_LIGHT_GREEN},
    {"lime", LV_PALETTE_LIME},
    {"yellow", LV_PALETTE_YELLOW},
    {"amber", LV_PALETTE_AMBER},
    {"orange", LV_PALETTE_ORANGE},
    {"deep_orange", LV_PALETTE_DEEP_ORANGE},
    {"brown", LV_PALETTE_BROWN},
    {"blue_grey", LV_PALETTE_BLUE_GREY},
    {"grey", LV_PALETTE_GREY},
    {"black", LV_PALETTE_BLACK},
    {"white", LV_PALETTE_WHITE},
    {NULL, 0},
};

static int parse_enum_color(const char *value, long *color)
{
    lv_color_t c;
    long palette;
    const char *s = parse_prefix(color_values, value, &palette);
    if (!s)
        return -1;

    if (palette == LV_PALETTE_WHITE)
        c = lv_color_white();
    else if (palette == LV_PALETTE_BLACK)
        c = lv_color_black();
    else {
        int lighten = 1;
        int index = 1;

        if (*s) {
            s++;
            if (!strncmp(s, "lighten", 7)) {
                lighten = 1;
                index = strtoul(s+7, NULL, 10);
            } else if (!strncmp(s, "darken", 6)) {
                lighten = 0;
                index = strtoul(s+6, NULL, 10);
            }
            if (index == 0)
                index = 1;
        }

        c = lighten ? lv_palette_lighten(palette, index)
                    : lv_palette_darken(palette, index);
    }

    *color = lv_color_to32(c);
    return 0;
}

int ilv_parser_add_style_color_opa(ilv_parser_t *list, int type, const char *key, const char *value)
{
    long color;
    int opa = -1;

    if (parse_color(value, &color, &opa)) {
        if (parse_enum_color(value, &color)) {
            fprintf(stderr, "error: 颜色格式不对,必须是8或6个16进制字符或者颜色枚举单词: %s=%s\n", key, value);
            return -1;
        }
    }

    ilv_parser_add_style(list, type, color);
    // LV_style_xxx_opa = LV_style_xxx_color + 1
    // 所以当 有 opa 的时候,直接设置 LV_style_xxx_opa
    if (opa != -1 && type != LV_style_bg_grad_color)
        ilv_parser_add_style(list, type+1, opa);

    return 0;
}

int ilv_parser_add_style_color(ilv_parser_t *list, int type, const char *key, const char *value)
{
    long color;
    int opa = -1;
    if (parse_color(value, &color, &opa)) {
        fprintf(stderr, "error: 颜色格式不对,必须是8或6个16进制字符: %s=%s\n", key, value);
        return -1;
    }
    ilv_parser_add_style(list, type, color);
    return 0;
}

int parse_view_group(ilv_parser_t *list);

static ilv_parser_t *create_parser(void)
{
    ilv_parser_t *list = malloc(sizeof(*list));
    memset(list, 0, sizeof(*list));

    list->groups = data_array_create(sizeof(void *), 4);
    list->configs = data_array_create(sizeof(config_pair_t), 4);
    list->free_datas = data_array_create(sizeof(struct free_pair), 4);
    list->root_index = -1;

    return list;
}

void ilv_parser_delete(ilv_parser_t *list)
{
    int i;

    for (i = 0; i < data_array_size(list->groups); i++) {
        data_array_t **array = data_array_at(list->groups, i);
        data_array_delete(*array);
    }

    for (i = 0; i < data_array_size(list->free_datas); i++) {
        struct free_pair *t = data_array_at(list->free_datas, i);
        t->free_cb(t->data);
    }

    if (list->config_applied) {
        for (i = 0; i < data_array_size(list->configs); i++) {
            config_pair_t *t = data_array_at(list->configs, i);
            if (t->tt->free_cb)
                t->tt->free_cb(t->value);
        }
    }

    free(list);
}

static void parse_all(ilv_parser_t *list)
{
    while (1) {
        char *line;
        list->save_words = ilv_parser_get_line_words(list, &list->save_depth, &line);
        if (!list->save_words)
            return;

        char *w0 = list->save_words[0];
        int wlen = strlen(w0);
        if (w0[0] == ':')
            fprintf(stderr, "error: 控件类型不能为空 : (%s)\n", line);
        else if (w0[wlen-1] != ':')
            fprintf(stderr, "error: 属性没有依附控件 或 控件类型后面应该接 ':' : (%s)\n", line);
        else
            break;
        free_parse_words(list->save_words);
    }

    while (1) {
        if (!list->save_words)
            break;

        if (!strncmp(list->save_words[0], "config.", strlen("config."))) {
            list->save_depth = parse_config(list);
            continue;
        }

        list->cur_group = data_array_create(sizeof(ilv_style_t), 4);

        list->save_depth = parse_view_group(list);
        if (data_array_size(list->cur_group)) {
            ilv_parser_add_style(list, 0, 0);
            data_array_add(list->groups, &list->cur_group);
        } else {
            data_array_delete(list->cur_group);
        }
        list->cur_group = NULL;
        continue;
    }

    if (list->root_index < 0 && data_array_size(list->groups))
        list->root_index = 0;
}

ilv_parser_t *ilv_parse_file(const char *file)
{
    ilv_parser_t *list = create_parser();

    FILE *in = fopen(file, "r");
    if (!in) {
        fprintf(stderr, "file open failed: %s : %s", file, strerror(errno));
        free(list);
        return NULL;
    }

    list->in = in;

    parse_all(list);

    fclose(in);

    return list;
}

ilv_parser_t *ilv_parse_str(const char *str)
{
    ilv_parser_t *list = create_parser();

    list->str = str;

    parse_all(list);

    return list;
}

void ilv_parser_apply_configs(ilv_parser_t *list)
{
    if (list->config_applied)
        return;
    list->config_applied = 1;
    int i;
    for (i = 0; i < data_array_size(list->configs); i++) {
        config_pair_t *config = data_array_at(list->configs, i);
        if (config->tt->add_cb)
            config->tt->add_cb(config->value);
    }
}

ilv_style_t *ilv_parser_get_view_style(ilv_parser_t *list, int index)
{
    if (index >= data_array_size(list->groups))
        return NULL;
    data_array_t **array = data_array_at(list->groups, 0);
    return data_array_at(*array, 0);
}

ilv_style_t *ilv_parser_get_default_view_style(ilv_parser_t *list)
{
    return ilv_parser_get_view_style(list, list->root_index);
}

int ilv_parser_get_view_cnt(ilv_parser_t *list)
{
    return data_array_size(list->groups);
}

int ilv_parser_get_default_view_index(ilv_parser_t *list)
{
    return list->root_index;
}

ilv_style_t *ilv_parser_get_view_style_by_name(ilv_parser_t *list, const char *name)
{
    int i;
    for (i = 0; i < data_array_size(list->groups); i++) {
        data_array_t **array = data_array_at(list->groups, i);
        ilv_style_t *s = data_array_at(*array, 0);
        if (ilv_check_view_name(s, name))
            return s;
    }
    return NULL;
}

ilv_style_t *ilv_parser_get_view_style_by_type(ilv_parser_t *list, const char *type)
{
    int i;
    for (i = 0; i < data_array_size(list->groups); i++) {
        data_array_t **array = data_array_at(list->groups, i);
        ilv_style_t *s = data_array_at(*array, 0);
        if (ilv_check_view_type(s, type))
            return s;
    }
    return NULL;
}

void ilv_parser_add_to_view(lv_obj_t *obj, ilv_parser_t *list)
{
    ilv_add_user_data(obj, "ilv-parser", list, (void *)ilv_parser_delete);
}

ilv_parser_t *ilv_parser_get_from_view(lv_obj_t *obj)
{
    return ilv_get_user_data(obj, "ilv-parser");
}

static lv_obj_t *create_view(lv_obj_t *parent, ilv_parser_t *list)
{
    if (!list)
        return NULL;

    if (!ilv_parser_get_view_cnt(list)) {
        ilv_parser_delete(list);
        return NULL;
    }

    ilv_parser_apply_configs(list);

    lv_obj_t *obj = ilv_create_view(parent, ilv_parser_get_default_view_style(list));

    ilv_parser_add_to_view(obj, list);

    return obj;
}

lv_obj_t *ilv_parse_view_file(lv_obj_t *parent, const char *file)
{
    assert (file);

    return create_view(parent, ilv_parse_file(file));
}

lv_obj_t *ilv_parse_view_str(lv_obj_t *parent, const char *str)
{
    assert (str);

    return create_view(parent, ilv_parse_str(str));
}
