
#include "parse_str.h"
#include "lvgl/lvgl.h"
#include "utils/ilv_utils.h"
#include "parser/ilv_config.h"
#include <unistd.h>
#include <stdlib.h>

struct config_font_data {
    char *name;
    char *path;
    long size;
    long is_default;

    lv_style_t *font;
};

static void *config_font_alloc(void)
{
    struct config_font_data *data = malloc(sizeof(*data));
    memset(data, 0, sizeof(*data));
    return data;
}

static int config_font_parse(const char *key, const char *value, void *data_)
{
    struct config_font_data *data = data_;

    if (!strcmp(key, "id")) {
        if (parse_str2(value, &data->name)) {
            fprintf(stderr, "error: 字符串书写不正确 %s=%s\n", key, value);
            return -1;
        }
    } else if (!strcmp(key, "path")) {
        if (parse_str2(value, &data->path)) {
            fprintf(stderr, "error: 字符串书写不正确 %s=%s\n", key, value);
            return -1;
        }
        if (data->path[0] == '\0') {
            fprintf(stderr, "error: 路径不能为空 %s=%s\n", key, value);
            return -1;
        }
        return 0;
    } else if (!strcmp(key, "size")) {
        if (parse_int(value, &data->size)) {
            fprintf(stderr, "error: 整数书写不正确 %s=%s\n", key, value);
            return -1;
        }
    } else if (!strcmp(key, "default")) {
        if (parse_int(value, &data->is_default)) {
            fprintf(stderr, "error: 整数书写不正确 %s=%s\n", key, value);
            return -1;
        }
    } else {
        fprintf(stderr, "error: 未知的font属性 %s=%s\n", key, value);
        return -1;
    }

    return 0;
}

static int config_font_add(void *data_)
{
    struct config_font_data *data = data_;

    if (!data->name) {
        fprintf(stderr, "error: font: id 属性未指定\n");
        return -1;
    }

    if (!data->path) {
        fprintf(stderr, "error: font: path 属性未指定\n");
        return -1;
    }

    char *path = realpath(data->path, NULL);
    if (!path || access(path, F_OK)) {
        fprintf(stderr, "error: font: 文件不存在 %s\n", data->path);
        if (path)
            free(path);
        return -1;
    }

    if (!data->size)
        data->size = 35;

    data->font = ilv_load_font(path, data->size);
    free(path);
    if (!data->font)
        return -1;

    int ret = ilv_config_add("font", data->name, data->font);
    if (ret) {
        ilv_del_font(data->font);
        data->font = NULL;
        return -1;
    }

    if (data->is_default) {
        lv_obj_add_style(lv_scr_act(), data->font, 0);
        lv_obj_add_style(lv_layer_top(), data->font, 0);
    }

    return 0;
}

static void config_font_free(void *data_)
{
    struct config_font_data *data = data_;
    if (data->is_default) {
        lv_obj_remove_style(lv_scr_act(), data->font, 0);
        lv_obj_remove_style(lv_layer_top(), data->font, 0);
    }
    if (data->font) {
        ilv_config_del("font", data->name);
    }
    if (data->name) free(data->name);
    if (data->path) free(data->path);
    free(data);
}

static lv_style_t lvgl_default_font;

void ilv_config_init_font(void)
{
    ilv_config_add_type("font",
        config_font_alloc, config_font_parse, config_font_add, config_font_free);

    lv_style_init(&lvgl_default_font);
    lv_style_set_text_font(&lvgl_default_font, LV_FONT_DEFAULT);
    ilv_config_add("font", "lvgl_default", &lvgl_default_font);
}
