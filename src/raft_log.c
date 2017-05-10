/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @brief ADT for managing Raft log entries (aka entries)
 * @author Willem Thiart himself@willemthiart.com
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "raft.h"
#include "raft_private.h"
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

    /* we compact the log, and thus need to increment the Base Log Index */
    int base;

    raft_entry_t* entries;

    /* callbacks */
    raft_cbs_t *cb;
    void* raft;
} log_private_t;

static void __ensurecapacity(log_private_t * me)
{
    int i, j;
    raft_entry_t *temp;

    if (me->count < me->size)
        return;

    temp = (raft_entry_t*)calloc(1, sizeof(raft_entry_t) * me->size * 2);

    for (i = 0, j = me->front; i < me->count; i++, j++)
    {
        if (j == me->size)
            j = 0;
        memcpy(&temp[i], &me->entries[j], sizeof(raft_entry_t));
    }

    /* clean up old entries */
    free(me->entries);

    me->size *= 2;
    me->entries = temp;
    me->front = 0;
    me->back = me->count;
}

log_t* log_new()
{
    log_private_t* me = (log_private_t*)calloc(1, sizeof(log_private_t));
    if (!me)
        return NULL;
    me->size = INITIAL_CAPACITY;
    me->count = 0;
    me->back = in(me)->front = 0;
    me->entries = (raft_entry_t*)calloc(1, sizeof(raft_entry_t) * me->size);
    return (log_t*)me;
}

void log_set_callbacks(log_t* me_, raft_cbs_t* funcs, void* raft)
{
    log_private_t* me = (log_private_t*)me_;

    me->raft = raft;
    me->cb = funcs;
}

void log_clear(log_t* me_)
{
    log_private_t* me = (log_private_t*)me_;
    me->count = 0;
    me->back = 0;
    me->front = 0;
    me->base = 0;
}

int log_append_entry(log_t* me_, raft_entry_t* c)
{
    log_private_t* me = (log_private_t*)me_;
    int e = 0;

    __ensurecapacity(me);

    memcpy(&me->entries[me->back], c, sizeof(raft_entry_t));

    if (me->cb && me->cb->log_offer)
    {
        void* ud = raft_get_udata(me->raft);
        e = me->cb->log_offer(me->raft, ud, &me->entries[me->back], me->back);
        raft_offer_log(me->raft, &me->entries[me->back], me->back);
        if (e == RAFT_ERR_SHUTDOWN)
            return e;
    }

    me->count++;
    me->back++;

    return e;
}

raft_entry_t* log_get_from_idx(log_t* me_, int idx, int *n_etys)
{
    log_private_t* me = (log_private_t*)me_;
    int i;

    assert(0 <= idx - 1);

    if (me->base + me->count < idx || idx < me->base)
    {
        *n_etys = 0;
        return NULL;
    }

    /* idx starts at 1 */
    idx -= 1;

    i = (me->front + idx - me->base) % me->size;

    int logs_till_end_of_log;

    if (i < me->back)
        logs_till_end_of_log = me->back - i;
    else
        logs_till_end_of_log = me->size - i;

    *n_etys = logs_till_end_of_log;
    return &me->entries[i];
}

raft_entry_t* log_get_at_idx(log_t* me_, int idx)
{
    log_private_t* me = (log_private_t*)me_;
    int i;

    assert(0 <= idx - 1);

    if (me->base + me->count < idx || idx < me->base)
        return NULL;

    /* idx starts at 1 */
    idx -= 1;

    i = (me->front + idx - me->base) % me->size;
    return &me->entries[i];

}

int log_count(log_t* me_)
{
    return ((log_private_t*)me_)->count;
}

void log_delete(log_t* me_, int idx)
{
    log_private_t* me = (log_private_t*)me_;
    int end;

    /* idx starts at 1 */
    idx -= 1;
    idx -= me->base;

    for (end = log_count(me_); idx < end; idx++)
    {
        if (me->cb && me->cb->log_pop)
            me->cb->log_pop(me->raft, raft_get_udata(me->raft),
                            &me->entries[me->back - 1], me->back);
        raft_pop_log(me->raft, &me->entries[me->back - 1], me->back);
        me->back--;
        me->count--;
    }
}

void *log_poll(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    if (0 == log_count(me_))
        return NULL;

    const void *elem = &me->entries[me->front];
    if (me->cb && me->cb->log_poll)
        me->cb->log_poll(me->raft, raft_get_udata(me->raft),
                         &me->entries[me->front], me->front);
    me->front++;
    me->count--;
    me->base++;
    return (void*)elem;
}

raft_entry_t *log_peektail(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    if (0 == log_count(me_))
        return NULL;

    if (0 == me->back)
        return &me->entries[me->size - 1];
    else
        return &me->entries[me->back - 1];
}

void log_empty(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    me->front = 0;
    me->back = 0;
    me->count = 0;
}

void log_free(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    free(me->entries);
    free(me);
}

int log_get_current_idx(log_t* me_)
{
    log_private_t* me = (log_private_t*)me_;
    return log_count(me_) + me->base;
}
