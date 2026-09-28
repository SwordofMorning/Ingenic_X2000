
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <stdatomic.h>
#include <malloc.h>

#include <stdio.h>
#include <stdatomic.h>

#include "libutils2/os.h"
#include "libutils2/event_queue.h"

/*
 * struct cb_data
 * struct event_data
 * struct event_action
 */

/*
 * 这是事件回调的结构体
 * 一个事件回调对应一个 event_src 和 一个 event_queue
 * link 连接到 event_src 的链表
 * link_queue 连接到 event_queue 的链表
 * 事件回调会分别引用 event_src 和 event_queue 保证它们不会被free
 * 当所有的事件回调都被删除完毕 event_src event_queue 才可以 free
 */
struct cb_data {
    refer_t refer;
    struct list_head link;
    struct list_head link_queue;
    event_queue_t *queue;
    event_src_t *src;
    event_cb_t cb;
    void *user_data;
    int code;
};

/*
 * 这是事件数据的结构体
 * 可以节约一点内存,event_src分发到多个event_queue时,减少事件和事件数据的复制
 * 多个 event_action 引用到一个 event_data
 * 本质是一个 code+data 回调到多个 cb+user_data
 * 所有的 event_action 解引用的时候, event_data 被释放
 * 当然 用户给定的 data 也调用 data_free_cb 被释放
 */
struct event_data {
    refer_t refer;
    refer_delete_cb_t data_free_cb;
    event_t event;
};

/*
 * 这是分发到 event_queue 中的事件动作结构体
 * 多个 event_action 引用到一个 event_data
 * 当前 event_queue 中的事件被回调之后,解引用其对应的 event_data
 * 所有的 event_queue 都回调完之后, 对应事件的 event_data 被释放
 */
struct event_action {
    struct list_head link;
    struct event_data *event;
    event_cb_t cb;
    void *user_data;
};

static LIST_HEAD(src_list);
static pthread_mutex_t src_lock = PTHREAD_MUTEX_INITIALIZER;

static event_src_t *get_src(const char *name)
{
    struct list_head *pos;

    list_for_each(pos, &src_list) {
        event_src_t *s = list_entry(pos, event_src_t, link);
        if (!strcmp(s->name, name))
            return s;
    }

    return NULL;
}

event_src_t *event_src_get(const char *name)
{
    pthread_mutex_lock(&src_lock);
    event_src_t *src = get_src(name);
    if (src)
        refer_add(&src->refer, 1);
    pthread_mutex_unlock(&src_lock);

    return src;
}

void event_src_get_(event_src_t *src)
{
    refer_add(&src->refer, 1);
}

void event_src_put(event_src_t *src)
{
    refer_sub(&src->refer, 1);
}

void event_queue_get(event_queue_t *queue)
{
    refer_add(&queue->refer, 1);
}

void event_queue_put(event_queue_t *queue)
{
    refer_sub(&queue->refer, 1);
}

static void clean_src(void *refer)
{
    event_src_t *src = refer;

    pthread_mutex_lock(&src_lock);
    list_del_init(&src->link);
    pthread_mutex_unlock(&src_lock);

    pthread_spin_destroy(&src->lock);
}

int event_src_init(event_src_t *src, const char *name)
{
    assert(name);

    pthread_mutex_lock(&src_lock);

    event_src_t *s = get_src(name);
    if (s) {
        fprintf(stderr, "event: src:%s already registed\n", name);
        pthread_mutex_unlock(&src_lock);
        return -1;
    }

    refer_init(&src->refer, clean_src, 1);
    pthread_spin_init(&src->lock, 0);
    INIT_LIST_HEAD(&src->list);
    src->name = name;

    list_add_tail(&src->link, &src_list);

    pthread_mutex_unlock(&src_lock);

    return 0;
}

void event_src_deinit(event_src_t *src)
{
    event_src_put(src);
}

static void delete_event_data(void *refer)
{
    struct event_data *e = refer;
    if (e->data_free_cb)
        e->data_free_cb(e->event.data);
    event_src_put(e->event.src);
    free(e);
}

static struct event_data *create_event_data(event_src_t *src, int code, void *data, int size, refer_delete_cb_t data_free_cb)
{
    struct event_data *e = malloc(sizeof(*e));
    refer_init(&e->refer, delete_event_data, 1);
    e->data_free_cb = data_free_cb;
    e->event.src = src;
    e->event.code = code;
    e->event.data = data;
    e->event.size = size;
    event_src_get_(src);
    return e;
}

static void event_data_refer(struct event_data *e)
{
    refer_add(&e->refer, 1);
}

static void event_data_unrefer(struct event_data *e)
{
    refer_sub(&e->refer, 1);
}

void event_src_send_event_ptr(event_src_t *src, int code, void *data, int size, refer_delete_cb_t data_free_cb)
{
    struct event_data *event = create_event_data(src, code, data, size, data_free_cb);

    pthread_spin_lock(&src->lock);
    struct list_head *pos;
    list_for_each(pos, &src->list) {
        struct cb_data *c = list_entry(pos, struct cb_data, link);
        if (c->code == code || c->code == EVENT_ALL) {
            event_queue_t *queue = c->queue;
            struct event_action *e = malloc(sizeof(*e));
            e->event = event;
            e->cb = c->cb;
            e->user_data = c->user_data;
            event_data_refer(event);

            pthread_spin_lock(&queue->event_lock);
            list_add_tail(&e->link, &queue->event_list);
            pthread_spin_unlock(&queue->event_lock);
            sem_post(&queue->sem);
        }
    }
    pthread_spin_unlock(&src->lock);

    event_data_unrefer(event);
}

void event_src_send_event(event_src_t *src, int code, void *data, int size)
{
    if (!size)
        return event_src_send_event_ptr(src, code, data, size, NULL);
    else {
        void *d = malloc(size);
        memcpy(d, data, size);
        event_src_send_event_ptr(src, code, d, size, free);
    }
}

static void clean_queue(void *refer)
{
    event_queue_t *queue = refer;

    while (1) {
        struct event_action *e = NULL;
        pthread_spin_lock(&queue->event_lock);
        if (!list_empty(&queue->list)) {
            e = list_first_entry(&queue->list, struct event_action, link);
            list_del(&e->link);
        }
        pthread_spin_unlock(&queue->event_lock);
        if (!e)
            break;

        event_data_unrefer(e->event);
        free(e);
    }

    pthread_spin_destroy(&queue->lock);
    pthread_spin_destroy(&queue->event_lock);
    sem_destroy(&queue->sem);
}

void event_queue_init(event_queue_t *queue)
{
    pthread_spin_init(&queue->event_lock, 0);
    INIT_LIST_HEAD(&queue->event_list);
    pthread_spin_init(&queue->lock, 0);
    INIT_LIST_HEAD(&queue->list);
    sem_init(&queue->sem, 0, 0);

    refer_init(&queue->refer, clean_queue, 1);
}

void event_queue_deinit(event_queue_t *queue)
{
    event_queue_put(queue);
}

static void delete_cb_data(void *refer)
{
    struct cb_data *c = refer;
    event_src_put(c->src);
    event_queue_put(c->queue);
    free(c);
}

static struct cb_data *create_cb_data(event_queue_t *queue, event_src_t *src, event_cb_t cb, int code, void *user_data)
{
    struct cb_data *c = malloc(sizeof(*c));
    c->src = src;
    c->queue = queue;
    c->cb = cb;
    c->user_data = user_data;
    c->code = code;
    event_src_get_(c->src);
    event_queue_get(c->queue);
    refer_init(&c->refer, delete_cb_data, 2);
    return c;
}

static void cb_data_unrefer(struct cb_data *c)
{
    refer_sub(&c->refer, 1);
}

int event_queue_add_cb(event_queue_t *queue, const char *src_name, event_cb_t cb, int code, void *user_data)
{
    event_src_t *src = event_src_get(src_name);
    if (!src) {
        fprintf(stderr, "event: src:%s not registed\n", src_name);
        return -1;
    }

    struct cb_data *q = create_cb_data(queue, src, cb, code, user_data);

    pthread_spin_lock(&queue->lock);
    list_add_tail(&q->link_queue, &queue->list);
    pthread_spin_unlock(&queue->lock);

    pthread_spin_lock(&src->lock);
    list_add_tail(&q->link, &src->list);
    pthread_spin_unlock(&src->lock);

    event_src_put(src);

    return 0;
}

static void remove_events(event_queue_t *queue, event_cb_t cb, int code, void *user_data)
{
    struct event_action *e = NULL;
    struct list_head *pos, *n;

    pthread_spin_lock(&queue->event_lock);
    list_for_each_safe(pos, n, &queue->event_list) {
        e = list_entry(pos, struct event_action, link);
        if (e->cb == cb && e->user_data == user_data && e->event->event.code == code) {
            list_del(&e->link);
            event_data_unrefer(e->event);
            free(e);
        }
    }
    pthread_spin_unlock(&queue->event_lock);
}

void event_queue_remove_cb(event_queue_t *queue, const char *src_name, event_cb_t cb, int code, void *user_data)
{
    event_src_t *src = get_src(src_name);
    if (!src) {
        fprintf(stderr, "event: src:%s not registed\n", src_name);
        return;
    }

    struct cb_data *res = NULL;

    pthread_spin_lock(&src->lock);
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &src->list) {
        struct cb_data *c = list_entry(pos, struct cb_data, link);
        if (c->cb == cb && c->code == code && c->user_data == user_data) {
            list_del_init(&c->link);
            res = c;
            break;
        }
    }
    pthread_spin_unlock(&src->lock);

    if (!res)
        return;

    pthread_spin_lock(&queue->lock);
    if (!list_empty(&res->link_queue)) {
        list_del_init(&res->link_queue);
        cb_data_unrefer(res);
    }
    pthread_spin_unlock(&queue->lock);

    remove_events(queue, cb, code, user_data);

    cb_data_unrefer(res);
}

void event_queue_remove_all_cb(event_queue_t *queue)
{
    while (1) {
        struct cb_data *c = NULL;
        pthread_spin_lock(&queue->lock);
        if (!list_empty(&queue->list)) {
            c = list_first_entry(&queue->list, struct cb_data, link_queue);
            list_del(&c->link_queue);
        }
        pthread_spin_unlock(&queue->lock);
        if (!c)
            break;

        event_src_t *src = c->src;
        pthread_spin_lock(&src->lock);
        if (!list_empty(&c->link)) {
            list_del_init(&c->link);
            cb_data_unrefer(c);
        }
        pthread_spin_unlock(&src->lock);

        remove_events(queue, c->cb, c->code, c->user_data);

        cb_data_unrefer(c);
    }
}

int event_queue_handle_events(event_queue_t *queue, int timeout_usecs)
{
    struct timespec tp;
    timespec_add_current_usecs(&tp, timeout_usecs);

    int count = 0;
    while (1) {
        if (sem_wait_until(&queue->sem, &tp))
            break;

        struct event_action *e = NULL;

        pthread_spin_lock(&queue->event_lock);
        if (!list_empty(&queue->event_list)) {
            e = list_first_entry(&queue->event_list, struct event_action, link);
            list_del(&e->link);
        }
        pthread_spin_unlock(&queue->event_lock);
        if (!e)
            continue;

        e->cb(&e->event->event, e->user_data);
        event_data_unrefer(e->event);
        free(e);

        count++;
    }

    return count;
}
