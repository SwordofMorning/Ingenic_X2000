#ifndef __MESSAGE_QUEUE_H__
#define __MESSAGE_QUEUE_H__

#include <libutils2/list.h>

struct message {
    long type;
    long data;
    struct list_head link;
};

struct message_queue;

struct message_queue *message_queue_create(unsigned int msg_nums, unsigned int msg_data_size);

void message_queue_send(struct message_queue *queue, struct message *msg);

struct message *message_queue_receive_timeout(struct message_queue *queue, int msecs);

struct message *message_queue_receive(struct message_queue *queue);

int message_queue_is_empty(struct message_queue *queue);

int message_queue_size(struct message_queue *queue);

void message_queue_delete(struct message_queue *queue);

#endif /* __MESSAGE_QUEUE_H__ */
