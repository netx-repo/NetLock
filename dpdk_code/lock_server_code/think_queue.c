#include <think_queue.h>

void think_queue_init(think_queue_list *q) {
    q->size_of_queue = 0;
    q->head = NULL;
    q->tail = NULL;
}

int think_enqueue(think_queue_list *q, uint8_t mode, uint32_t lock_id, uint32_t txn_id, uint8_t client_id, uint64_t grant_time) {
    think_queue_node *new_node = (think_queue_node *) malloc(sizeof(think_queue_node));
    if (new_node == NULL) {
        fprintf(stderr, "MALLOC ERROR.\n");
        return -1;
    }

    new_node->mode = mode;
    new_node->client_id = client_id;
    new_node->lock_id = lock_id;
    new_node->txnID = txn_id;
    // new_node->ip_src = ip_src_addr;
    // new_node->timestamp = timestamp;
    new_node->grant_time = grant_time;
    new_node->next = NULL;

    if (q->size_of_queue == 0) {
        q->head = new_node;
        q->tail = new_node;
    }
    else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->size_of_queue ++;
    return 0;
}

void think_dequeue(think_queue_list *q, uint8_t* mode, uint32_t* lock_id, uint32_t* txn_id, uint8_t* client_id) {
    if ((q != NULL) && (q->size_of_queue > 0)) {
        think_queue_node *temp = q->head;
        (*mode) = temp->mode;
        (*client_id) = temp->client_id;
        (*lock_id) = temp->lock_id;
        (*txn_id) = temp->txnID;
        // (*ip_src_addr) = temp->ip_src;
        // (*timestamp) = temp->timestamp;
        free(temp);
        if (q->size_of_queue > 1) {
            q->head = q->head->next;
        }
        else {
            q->head = NULL;
            q->tail = NULL;
        }

        q->size_of_queue --;
        
    }
    return;
}

void think_queue_clear(think_queue_list *q) {
    think_queue_node *temp;
    while (q->size_of_queue > 0) {
        temp = q->head;
        q->head = temp->next;
        free(temp);
        q->size_of_queue --;
    }

    q->head = NULL;
    q->tail = NULL;
    return;
}

int think_get_queue_size(think_queue_list *q) {
    if (q == NULL)
        return 0;
    return q->size_of_queue;
}