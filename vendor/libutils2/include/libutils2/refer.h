#ifndef _LIBUTILS2_REFER_H_
#define _LIBUTILS2_REFER_H_

#include <stdatomic.h>

// 如果 add 或者 sub 报错的情况, 
// 取消#define REFER_DEBUG的注释
// 手动赋值 refer.func refer.name 变量
// 重新编译libutils2和应用

// #define REFER_DEBUG

struct refer;
typedef void (*refer_delete_cb_t)(void *refer);

typedef struct refer {
    atomic_int count;
    refer_delete_cb_t cb;
#ifdef REFER_DEBUG
    const char *func;
    const char *name;
#endif
} refer_t;

/**
 * 初始化引用计数
 * 初始值是0的情况主动调用cb
 * 当引用计数减少后等于0时,会调用refer->cb回调函数
 * @param cb 引用计数归零时的回调
 * @param count 引用计数的初始值,
*/
void refer_init(refer_t *refer, refer_delete_cb_t cb, int count);

/**
 * 引用计数增加
 * @param count 引用计数的增加的值
 * @return 返回增加后引用计数的值
 */
int refer_add(refer_t *refer, int count);

/**
 * 引用计数减少
 * @param count 引用计数的减少的值
 * @return 返回减少后引用计数的值
 */
int refer_sub(refer_t *refer, int count);

#endif /* _LIBUTILS2_REFER_H_ */
