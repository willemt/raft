#ifndef LINKED_LIST_QUEUE_H
#define LINKED_LIST_QUEUE_H

typedef struct llqnode_s llqnode_t;

struct llqnode_s
{
    llqnode_t *next;
    void *item;
};

typedef struct
{
    llqnode_t *head, *tail;
    int count;
} linked_list_queue_t;

void *llqueue_new(
);

void llqueue_free(
    linked_list_queue_t * qu
);

void *llqueue_poll(
    linked_list_queue_t * qu
);

void llqueue_offer(
    linked_list_queue_t * qu,
    void *item
);

/**
 * remove this item, by comparing the memory address of the item */
void *llqueue_remove_item(
    linked_list_queue_t * qu,
    const void *item
);

int llqueue_count(
    const linked_list_queue_t * qu
);

/**
 * remove this item, by using the supplied compare function */
void *llqueue_remove_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    int (*cmp)(const void*, const void*));

/**
 * get this item, by using the supplied compare function */
void *llqueue_get_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    long (*cmp)(const void*, const void*));

#endif /* LINKED_LIST_QUEUE_H */
