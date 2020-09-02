#include <txn_queue.h>

void txn_queue_init(txn_queue_list *q) {
    q->size_of_queue = 0;
    q->head = NULL;
    q->tail = NULL;
}

int txn_enqueue(txn_queue_list *q, uint8_t mode, uint8_t opt, uint32_t lock_id) {
    txn_queue_node *new_node = (txn_queue_node *) malloc(sizeof(txn_queue_node));
    if (new_node == NULL) {
        fprintf(stderr, "MALLOC ERROR.\n");
        return -1;
    }

    new_node->mode = mode;
    new_node->op_type = opt;
    new_node->lock_id = lock_id;
    
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

void txn_dequeue(txn_queue_list *q, uint8_t* mode, uint32_t* lock_id) {
    if ((q != NULL) && (q->size_of_queue > 0)) {
        txn_queue_node *temp = q->head;
        (*mode) = temp->mode;
        (*lock_id) = temp->lock_id;
        
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

void txn_queue_clear(txn_queue_list *q) {
    txn_queue_node *temp;
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

int txn_get_queue_size(txn_queue_list *q) {
    if (q == NULL)
        return 0;
    return q->size_of_queue;
}

