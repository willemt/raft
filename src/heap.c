#include "stdlib.h"

#include "../include/raft.h"

static void *raft__heap_malloc(void *data, size_t size)
{
  (void)data;
    return malloc(size);
}

static void raft__free(void *data, void *ptr)
{
    (void)data;
    return free(ptr);
}

static void *raft__calloc(void *data, size_t nmemb, size_t size)
{
  (void)data;
    return calloc(nmemb, size);
}

static void *raft__realloc(void *data, void *ptr, size_t size)
{
  (void)data;
    return realloc(ptr, size);
}

void raft__heap_init(struct raft_heap *h)
{
    h->data = NULL;
    h->malloc = raft__heap_malloc;
    h->free = raft__free;
    h->calloc = raft__calloc;
    h->realloc = raft__realloc;
}
