#ifndef TXN_QUEUE_INCLUDED
#define TXN_QUEUE_INCLUDED
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct TXNQueueNode {
    uint8_t     mode;
    uint8_t     op_type;
    uint32_t    lock_id;
    struct TXNQueueNode *next;
} txn_queue_node;

typedef struct TXNQueueList {
    int size_of_queue;
    txn_queue_node *head;
    txn_queue_node *tail;
} txn_queue_list;

void txn_queue_init(txn_queue_list *q);

int txn_enqueue(txn_queue_list *q, uint8_t mode, uint8_t opt, uint32_t lock_id);

void txn_dequeue(txn_queue_list *q, uint8_t* mode, uint32_t* lock_id);

void txn_queue_clear(txn_queue_list *q);

int txn_get_queue_size(txn_queue_list *q);

#endif

