#ifndef _EVENT_QUEUE_H_
#define _EVENT_QUEUE_H_

#include <pthread.h>
#include <semaphore.h>
#include "libutils2/refer.h"
#include "libutils2/list.h"

// 表示接收所有事件,在回调添加的时候
#define EVENT_ALL 0xffffffff

struct event;
struct event_src;

/**
 * 事件回调函数,在回调函数中不要更改 struct event e中的值,
 * user_data 是注册回调时传入的user_data
 */
typedef void (*event_cb_t)(struct event *e, void *user_data);

/**
 * 事件
 */
typedef struct event {
    struct event_src *src; // 事件源(谁发的事件)
    int code;              // 事件值
    int size;              // 事件发送数据的大小
    void *data;            // 事件发送的数据
} event_t;

/**
 * 事件源,负责发送事件
 * 可以往注册的回调函数对应的事件队列发送事件
 */
typedef struct event_src {
    refer_t refer;
    struct list_head link;
    struct list_head list;
    pthread_spinlock_t lock;
    const char *name;
} event_src_t;

/**
 * 事件队列,负责接收事件
 * 可以往不同的事件源注册回调函数,接收其发送的事件
 */
typedef struct event_queue {
    refer_t refer;
    struct list_head list;
    pthread_spinlock_t lock;
    struct list_head event_list;
    pthread_spinlock_t event_lock;
    sem_t sem;
} event_queue_t;

/**
 * 获取事件源,通过名字查找,所以的事件源名字都是唯一的
 * 需要调用 event_src_put 解除占用
 */
event_src_t *event_src_get(const char *name);

/**
 * 引用/占用事件源
 * 需要调用 event_src_put 解除占用
 */
void event_src_get_(event_src_t *src);

/**
 * 解除引用/占用事件源
 */
void event_src_put(event_src_t *src);

/**
 * 引用/占用事件队列
 */
void event_queue_get(event_queue_t *queue);

/**
 * 解除引用/占用事件队列
 */
void event_queue_put(event_queue_t *queue);

/**
 * 初始化事件源
 * @param src 事件源
 * @param name 事件源的名字
 * @return 0 表示成功, < 0 表示失败,一般是已经有同名的事件源注册了
 */
int event_src_init(event_src_t *src, const char *name);

/**
 * 解初始化事件源
 * 在事件源的引用归零时,会释放事件源的注册以及其占用的资源
 */
void event_src_deinit(event_src_t *src);

/**
 * 发送事件数据
 * @param src 事件源,会往该事件源注册的回调的事件队列上发送事件
 * @param code 事件值,匹配该事件值,或者事件值是EVENT_ALL的回调会收到该事件
 * @param data 事件数据,不会拷贝
 * @param size 事件数据大小
 * @param data_free_cb 当事件回调完成时,会调用此函数释放数据data所占用的资源
 */
void event_src_send_event_ptr(event_src_t *src, int code, void *data, int size, refer_delete_cb_t data_free_cb);

/**
 * 发送事件数据,相当于如下写法
 * if (size == 0)
 *     event_src_send_event_ptr(src, code, data, 0, NULL);
 * else
 *      event_src_send_event_ptr(src, code, malloc_and_copy(data, size), free);
 * @param src 事件源,会往该事件源注册的回调的事件队列上发送事件
 * @param code 事件值,匹配该事件值,或者事件值是EVENT_ALL的回调会收到该事件
 * @param data 事件数据,size > 0时会被拷贝, size = 0时不会拷贝
 * @param size 事件数据大小
 */
void event_src_send_event(event_src_t *src, int code, void *data, int size);

/**
 * 初始化事件队列
 */
void event_queue_init(event_queue_t *queue);

/**
 * 解初始化事件队列
 * 在事件队列的引用归零时,会释放事件源的注册以及其占用的资源
 */
void event_queue_deinit(event_queue_t *queue);

/**
 * 注册事件回调
 * 会引用 事件队列queue 和 src_name 对应的事件源
 * @param queue 绑定该事件队列
 * @param src_name 查找该名字的事件源,并且注册事件回调
 * @param cb 事件回调函数
 * @param code 事件值,等于此值或者此值等于EVENT_ALL时,事件可以被回调,否则忽略
 * @param user_data 事件回调的私有数据
 * @return 0 表示成功, < 0表示失败,一般是事件源找不到
 */
int event_queue_add_cb(event_queue_t *queue, const char *src_name, event_cb_t cb, int code, void *user_data);

/**
 * 移除事件回调
 * @param queue 解除绑定该事件队列
 * @param src_name 查找该名字的事件源,并且移除事件回调
 * @param cb 事件回调函数
 * @param code 事件值
 * @param user_data 事件回调的私有数据
 * 移除的条件是 cb code user_data 都相等
 * 如果有相同的多个事件,那么只会移除第一个
 */
void event_queue_remove_cb(event_queue_t *queue, const char *src_name, event_cb_t cb, int code, void *user_data);

/**
 * 移除事件队列绑定的所有回调
 */
void event_queue_remove_all_cb(event_queue_t *queue);

/**
 * 回调当前事件队列的缓存的事件
 * @param queue 事件队列
 * @param timeout_usecs 等待事件到来的超时时间
 * @return 0 表示没有事件 > 0 表示回调的事件的个数
 */
int event_queue_handle_events(event_queue_t *queue, int timeout_usecs);


/**
 * 这是一个列子
 * wifi 的状态 和 tp 的报点在不同的线程
 * 统一归结到 main 函数的线程处理

#include <unistd.h>

enum {
    wifi_connecting,
    wifi_disconnected,
    wifi_connected,
};

void *wifi_event_thread(void *data)
{
    event_src_t *src = event_src_get("wifi");
    assert(src);
    int count = (long) data;

    while (count--) {
        // usleep(300*1000);
        event_src_send_event(src, wifi_connecting, NULL, 0);
        // usleep(2*1000);
        event_src_send_event(src, wifi_disconnected, NULL, 0);
        // usleep(2*1000);
        event_src_send_event(src, wifi_connected, NULL, 0);
        usleep(100);
    }

    event_src_put(src);
    return NULL;
}

enum {
    tp_pressed,
    tp_unpressed,
    tp_moving,
};

void *tp_event_thread(void *data)
{
    event_src_t *src = event_src_get("tp");
    assert(src);

    int count = (long) data;
    int i = 0;
    while (count--) {
        int pos[2] = {i, i+1};
        i++;
        // usleep(300*1000);
        event_src_send_event(src, tp_pressed, pos, sizeof(pos));
        // usleep(2*1000);
        event_src_send_event(src, tp_unpressed, pos, sizeof(pos));
        // usleep(2*1000);
        event_src_send_event(src, tp_moving, pos, sizeof(pos));
        usleep(100);
    }

    event_src_put(src);
    return NULL;
}

void wifi_cb(struct event *e, void *user_data)
{
    printf("wifi: %d\n", e->code);
}

void tp_cb(struct event *e, void *user_data)
{
    int *pos = (void *)e->data;
    printf("tp: %d %d,%d\n", e->code, pos[0], pos[1]);
}

int main(int argc, char *argv[])
{
    mtrace();

    int i = 0;
    while (1) {
        event_src_t wifi_src;
        event_src_t tp_src;
        event_src_init(&wifi_src, "wifi");
        event_src_init(&tp_src, "tp");

        static event_queue_t queue;
        event_queue_init(&queue);

        pthread_t thread0, thread1;
        pthread_create(&thread0, NULL, wifi_event_thread, (void *)1000);
        pthread_create(&thread1, NULL, tp_event_thread, (void *)2000);

        event_queue_add_cb(&queue, "wifi", wifi_cb, EVENT_ALL, NULL);
        event_queue_add_cb(&queue, "tp", tp_cb, EVENT_ALL, NULL);

        while (1) {
            int count = event_queue_handle_events(&queue, 10000);
            if (count == 0)
                break;
        }

        event_src_deinit(&wifi_src);
        event_src_deinit(&tp_src);
        // event_queue_remove_cb(&queue, "wifi", wifi_cb, EVENT_ALL, NULL);
        // event_queue_remove_cb(&queue, "tp", tp_cb, EVENT_ALL, NULL);
        event_queue_remove_all_cb(&queue);
        event_queue_deinit(&queue);
        printf("i: ------------------------------------ %d\n", ++i);

        pthread_join(thread0, NULL);
        pthread_join(thread1, NULL);

        if (i == 5)
            break;
    }

    // muntrace();

    return 0;
}
*/

#endif /* _EVENT_QUEUE_H_ */
