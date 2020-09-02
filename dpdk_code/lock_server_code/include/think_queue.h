#ifndef THINK_QUEUE_INCLUDED
#define THINK_QUEUE_INCLUDED
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct ThinkQueueNode {
    uint8_t     mode;
    uint8_t     client_id;
    uint32_t    lock_id;
    uint32_t    txnID;
    uint32_t    ip_src;
    uint64_t    timestamp;
    uint64_t    grant_time;
    struct ThinkQueueNode *next;
} think_queue_node;

typedef struct ThinkQueueList {
    int size_of_queue;
    think_queue_node *head;
    think_queue_node *tail;
} think_queue_list;

void think_queue_init(think_queue_list *q);

int think_enqueue(think_queue_list *q, uint8_t mode, uint32_t lock_id, uint32_t txn_id, uint8_t client_id, uint64_t grant_time);

void think_dequeue(think_queue_list *q, uint8_t* mode, uint32_t* lock_id, uint32_t* txn_id, uint8_t* client_id);

void think_queue_clear(think_queue_list *q);

int think_get_queue_size(think_queue_list *q);

#endif

