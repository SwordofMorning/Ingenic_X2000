#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libutils2/message_queue.h>

struct message_queue {
    struct list_head list;
    pthread_mutex_t lock;
    pthread_mutexattr_t attr;
    sem_t sem;
};

struct message_queue *message_queue_create(unsigned int msg_nums, unsigned int msg_data_size)
{
    int msg_size = sizeof(struct message) + msg_data_size;
    int size = msg_nums * msg_size;
    struct message_queue *queue = malloc(sizeof(*queue) + size);
    if (!queue) {
        fprintf(stderr, "message_queue: failed to alloc queue: %d\n", size);
        return NULL;
    }

    if (sem_init(&queue->sem, 0, 0)) {
        fprintf(stderr, "message_queue: failed to init sem: %s\n", strerror(errno));
        goto free_queue;
    }

    pthread_mutexattr_init(&queue->attr);
    pthread_mutexattr_settype(&queue->attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&queue->lock, &queue->attr) != 0) {
        fprintf(stderr, "message_queue: failed to init mutex: %s\n", strerror(errno));
        goto destory_sem;
    }

    INIT_LIST_HEAD(&queue->list);

    int i;
    struct message *msg = (void*)&queue[1];
    for (i = 0; i < msg_nums; i++) {
        msg->type = 0;
        msg->data = 0;
        list_add_tail(&msg->link, &queue->list);
        msg = (void*)msg + msg_size;
        sem_post(&queue->sem);
    }

    return queue;
destory_sem:
    sem_destroy(&queue->sem);
free_queue:
    free(queue);
    return NULL;
}

void message_queue_send(struct message_queue *queue, struct message *msg)
{
    pthread_mutex_lock(&queue->lock);

    list_add_tail(&msg->link, &queue->list);

    pthread_mutex_unlock(&queue->lock);

    int ret = sem_post(&queue->sem);
    assert(!ret);
}

struct message *message_queue_receive_timeout(struct message_queue *queue, int msecs)
{
    struct message *msg = NULL;

    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    tp.tv_sec += msecs / 1000;
    tp.tv_nsec += (msecs % 1000)*1000*1000;
    if (tp.tv_nsec >= (1000*1000*1000)) {
        tp.tv_sec += tp.tv_nsec / (1000*1000*1000);
        tp.tv_nsec = tp.tv_nsec % (1000*1000*1000);
    }

    if (msecs == -1)
        sem_wait(&queue->sem);
    else
        sem_timedwait(&queue->sem, &tp);

    pthread_mutex_lock(&queue->lock);

    if (!list_empty(&queue->list)) {
        msg = list_first_entry(&queue->list, struct message, link);
        list_del(&msg->link);
    }

    pthread_mutex_unlock(&queue->lock);

    return msg;
}

struct message *message_queue_receive(struct message_queue *queue)
{
    return message_queue_receive_timeout(queue, -1);
}

int message_queue_size(struct message_queue *queue)
{
    int value = 0;
    sem_getvalue(&queue->sem, &value);
    return value;
}

int message_queue_is_empty(struct message_queue *queue)
{
    return message_queue_size(queue) == 0;
}

void message_queue_delete(struct message_queue *queue)
{
    pthread_mutex_destroy(&queue->lock);
    pthread_mutexattr_destroy(&queue->attr);
    sem_destroy(&queue->sem);
    free(queue);
}
