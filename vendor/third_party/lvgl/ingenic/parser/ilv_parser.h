#ifndef _ILV_PARSER_H_
#define _ILV_PARSER_H_

#include <stdio.h>
#include "style/ilv_style.h"
#include "parser/ilv_config.h"
#include "parse_str.h"
#include "libutils2/data_array.h"

/*
    这里的函数围绕 ilv_parser 服务
    1 ilv_parser 可以将字符串解析成 1个或者多个view的描述数组
      view的描述数组成员是 ilv_style_t, 以 {NULL, NULL} 结尾

    2 附带的, ilv_parser 还可以解析出多个 config, 比如说字体的设置

    3 为什么需要 ilv_parser 解析出 view 来?
      a, 实现 view 的布局和控制/业务代码分离
      b, 用lvgl 原生代码或者ilv_style_t 都需要太多的代码,不方便快速构建ui
      c, 方便快速构建ui,以及理清layout 结构
      d, 可以拿 demo_ilock_txt/res/ui_desktop.txt 做对比 demo_ilock/ui_desktop.c 

    4 请参考 demo_ilock_txt/main.c 和 demo_ilock_txt/res/ui_desktop.txt 的例子
*/

typedef struct ilv_parser {
    data_array_t *groups;
    data_array_t *configs;
    data_array_t *free_datas;

    FILE *in;
    const char *str;
    data_array_t *cur_group;

    int config_applied;
    int root_index;

    view_type_t *vt;
    char **save_words;
    int save_depth;
    char *line;
} ilv_parser_t;

/**
 * 解析文件,应用configs,直接返回默认的view,相当于如下代码
 * 
 * // 解析文件
 * ilv_parser_t *parser = ilv_parse_file(file);
 * // 获得默认的view的styles
 * ilv_style_t *style = ilv_parser_get_default_view_style(parser);
 * // 使能文件中所有的config 配置
 * ilv_parser_apply_configs(parser);
 * // 生成默认的view
 * lv_obj_t *obj = ilv_create_view(style);
 * // 将 ilv_parser 添加到view,view delete的时候会自动删除 ilv_parser
 * ilv_parser_add_to_view(obj, parser);
 * 
 * return obj;
 */
lv_obj_t *ilv_parse_view_file(lv_obj_t *parent, const char *file);
lv_obj_t *ilv_parse_view_str(lv_obj_t *parent, const char *str);

/**
 * 解析文件,得到ilv_parser
 */
ilv_parser_t *ilv_parse_file(const char *file);
ilv_parser_t *ilv_parse_str(const char *str);

/**
 * 删除ilv_parser
 */
void ilv_parser_delete(ilv_parser_t *list);

/**
 * 应用 ilv_parser中的config配置
 * 比如 config.font: id='font_normal' path='res/wqy-microhei.ttc' size=35
 * 会触发 字体 config 的加载,这样指定了字体是 "font_normal" 的view被创建时就不会报错
 */
void ilv_parser_apply_configs(ilv_parser_t *list);

/**
 * 获得 解析到的 view styles 的个数
 */
int ilv_parser_get_view_cnt(ilv_parser_t *list);

/**
 * 获得 默认的 view styles 的 index
 */
int ilv_parser_get_default_view_index(ilv_parser_t *list);

/**
 * 通过 index 获得对应的 view styles
 */
ilv_style_t *ilv_parser_get_view_style(ilv_parser_t *list, int index);

/**
 * 获得默认的 view styles
 * 由 is_root=1 指定默认 view styles, 如果没有那么使用第一个view styles
 */
ilv_style_t *ilv_parser_get_default_view_style(ilv_parser_t *list);

/**
 * 通过 名字 获得对应的的 view styles
 * 由 id=name 指定view的名字
 */
ilv_style_t *ilv_parser_get_view_style_by_name(ilv_parser_t *list, const char *name);

/**
 * 通过 view的类型 获得对应的的 view styles
 */
ilv_style_t *ilv_parser_get_view_style_by_type(ilv_parser_t *list, const char *type);

/**
 * 将 ilv_parser 添加到 view 中,
 * 这样 view 删除的时候,会自动调用 ilv_parser_delete 删除ilv_parser
 */
void ilv_parser_add_to_view(lv_obj_t *obj, ilv_parser_t *list);

/**
 * 获得添加到 view 中的 ilv_parser, 没有添加过则返回NULL
 */
ilv_parser_t *ilv_parser_get_from_view(lv_obj_t *obj);

// 以下是编写 view/config 解析代码时需要用到的api
// 暂时不做说明,需要看对应的实现以及使用的例子
void ilv_parser_add_delete_data(ilv_parser_t *list, void *data, void (*free_cb)(void *));
int ilv_parser_add_style(ilv_parser_t *list, int type, long value);
int ilv_parser_add_style_str(ilv_parser_t *list, int type, const char *key, const char *value);
int ilv_parser_add_style_int(ilv_parser_t *list, int type, const char *key, const char *value);
int ilv_parser_add_style_point(ilv_parser_t *list, int type, const char *key, const char *value);
int ilv_parser_add_style_color_opa(ilv_parser_t *list, int type, const char *key, const char *value);
int ilv_parser_add_style_color(ilv_parser_t *list, int type, const char *key, const char *value);

void ilv_parser_shortclick_event(lv_event_t *e);

#endif /* _ILV_PARSER_H_ */
