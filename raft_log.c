
/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief ADT for managing Raft log entries (aka entrys)
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "raft.h"
#include "raft_log.h"

#define INITIAL_CAPACITY 10
#define in(x) ((log_private_t*)x)

typedef struct
{
    /* size of array */
    int size;              

    /* the amount of elements in the array */
    int count;             

    /* position of the queue */
    int front, back;       

    /* we compact the log, and thus need to increment the base idx */
    int base_log_idx;

    raft_entry_t* entries;
} log_private_t;

static unsigned long __entry_hash(
    const void *obj
)
{
    return (unsigned long) obj;
}

static long __entry_compare(
    const void *obj,
    const void *other
)
{
    return obj - other;
}

static void __ensurecapacity(
    log_private_t * me
)
{
    int i, j;
    raft_entry_t *temp;

    if (me->count < me->size)
        return;

    temp = calloc(1,sizeof(raft_entry_t) * me->size * 2);

    for (i = 0, j = me->front; i < me->count; i++, j++)
    {
        if (j == me->size)
            j = 0;
        memcpy(&temp[i], &me->entries[j], sizeof(raft_entry_t));
    }

    me->size *= 2;
    me->entries = temp;
    me->front = 0;
    me->back = me->count;

    /* clean up old entries */
    free(me->entries);
}

log_t* log_new()
{
    log_private_t* me;

    me = calloc(1,sizeof(log_private_t));
    me->size = INITIAL_CAPACITY;
    me->count = 0;
    me->back = in(me)->front = 0;
    me->entries = calloc(1,sizeof(raft_entry_t) * me->size);
    return (void*)me;
}

/**
 * Add entry to log.
 * Don't add entry if we've already added this entry (based off ID)
 * Don't add entries with ID=0 
 * @return 0 if unsucessful; 1 otherwise */
int log_append_entry(log_t* me_, raft_entry_t* c)
{
    log_private_t* me = (void*)me_;

    if (0 == c->id)
        return 0;

    __ensurecapacity(me);

//    if (hashmap_get(me->log_map, (void*)c->id+1))
//        return 0;

    memcpy(&me->entries[me->back],c,sizeof(raft_entry_t));
    me->entries[me->back].npeers = 0;
    me->count++;
    me->back++;
    return 1;
}

raft_entry_t* log_get_from_idx(log_t* me_, int idx)
{
    log_private_t* me = (void*)me_;
    int i;

    if (me->base_log_idx + me->count < idx || idx < me->base_log_idx)
        return NULL;

    /* idx starts at 1 */
    idx -= 1;
    i = (me->front + idx - me->base_log_idx) % me->size;
    return &me->entries[i];
}

int log_count(log_t* me_)
{
    log_private_t* me = (void*)me_;
    return me->count;
}

/**
 * Delete all logs from this log onwards */
void log_delete(log_t* me_, int idx)
{
    log_private_t* me = (void*)me_;
    int end, i;

    /* idx starts at 1 */
    idx -= 1;
    idx -= me->base_log_idx;

    for (end = log_count(me_); idx<end; idx++)
    {
        me->back--;
        me->count--;
    }

#if 0
    const void *elem;

    if (arrayqueue_is_empty(me))
        return NULL;

//    __checkwrapping(me);
    in(me)->back--;
    in(me)->count--;
    if (-1 == in(me)->back)
        in(me)->back = in(me)->size;
    elem = me->entries[in(me)->back];

    return (void *) elem;
#endif
}

/**
 * Remove oldest entry
 * @return oldest entry */
void *log_poll(log_t * me_)
{
    log_private_t* me = (void*)me_;
    const void *elem;

    if (0 == log_count(me_))
        return NULL;
    elem = &me->entries[me->front];
    me->front++;
    me->count--;
    me->base_log_idx++;
    return (void *) elem;
}

/*
 * @return youngest entry */
raft_entry_t *log_peektail(log_t * me_)
{
    log_private_t* me = (void*)me_;
    const void *elem;
    int i;

    if (0 == log_count(me_))
        return NULL;

    if (0 == me->back)
        return &me->entries[me->size-1];
    else
        return &me->entries[me->back-1];
}

/**
 * Empty the queue. */
void log_empty(log_t * me_)
{
    log_private_t* me = (void*)me_;

    me->front = 0;
    me->back = 0;
    me->count = 0;
}

void log_free(log_t * me_)
{
    log_private_t* me = (void*)me_;

    free(me->entries);
    free(me);
}

void log_mark_peer_has_committed(log_t* me_, int idx)
{
    log_private_t* me = (void*)me_;
    raft_entry_t* e;

    if ((e = log_get_from_idx(me_,idx)))
    {
        e->npeers += 1;
    }
}

