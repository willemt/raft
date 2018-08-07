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

typedef struct
{
    /* size of array */
    raft_index_t size;

    /* the amount of elements in the array */
    raft_index_t count;

    /* position of the queue */
    raft_index_t front, back;

    /* we compact the log, and thus need to increment the Base Log Index */
    raft_index_t base;

    raft_entry_t* entries;

    /* callbacks */
    raft_cbs_t *cb;
    void* raft;
} log_private_t;

static int mod(raft_index_t a, raft_index_t b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

static int __ensurecapacity(log_private_t * me)
{
    raft_index_t i, j;
    raft_entry_t *temp;

    if ((me->count + n) <= me->size)
        return 0;

    temp = (raft_entry_t*)__raft_calloc(1, sizeof(raft_entry_t) * me->size * 2);
    if (!temp)
        return RAFT_ERR_NOMEM;

    if (0 < me->count)
    {
        int k = me->size - me->front;
        if (me->count <= k)
        {
            memcpy(&temp[0], &me->entries[me->front],
                   sizeof(raft_entry_t) * me->count);
        }
        else
        {
            memcpy(&temp[0], &me->entries[me->front],
                   sizeof(raft_entry_t) * k);
            memcpy(&temp[k], &me->entries[0],
                   sizeof(raft_entry_t) * (me->count - k));
        }
    }

    /* clean up old entries */
    __raft_free(me->entries);

    me->size = newsize;
    me->entries = temp;
    me->front = 0;
    return 0;
}

int log_load_from_snapshot(log_t *me_, raft_index_t idx, raft_term_t term)
{
    log_private_t* me = (log_private_t*)me_;

    log_clear(me_);
    me->base = idx;

    return 0;
}

log_t* log_alloc(raft_index_t initial_size)
{
    log_private_t* me = (log_private_t*)__raft_calloc(1, sizeof(log_private_t));
    if (!me)
        return NULL;
    me->size = initial_size;
    log_clear((log_t*)me);
    me->entries = (raft_entry_t*)__raft_calloc(1, sizeof(raft_entry_t) * me->size);
    if (!me->entries) {
        __raft_free(me);
        return NULL;
    }
    return (log_t*)me;
}

log_t* log_new(void)
{
    return log_alloc(INITIAL_CAPACITY);
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
    me->front = 0;
    me->base = 0;
}

static int has_idx(log_private_t* me, int idx)
{
    log_private_t* me = (log_private_t*)me_;
    raft_index_t idx = me->base + me->count + 1;
    int e;

    e = __ensurecapacity(me);
    if (e != 0)
        return e;

    memcpy(&me->entries[me->back], ety, sizeof(raft_entry_t));

/* Return the me->entries[] subscript for idx. */
static int subscript(log_private_t* me, int idx)
{
    return (me->front + (idx - (me->base + 1))) % me->size;
}

/* Return the maximal number of contiguous entries in me->entries[]
 * starting from and including idx up to at the most n entries. */
static int batch_up(log_private_t* me, int idx, int n)
{
    assert(n > 0);
    int lo = subscript(me, idx);
    int hi = subscript(me, idx + n - 1);
    return (lo <= hi) ?  (hi - lo + 1) : (me->size - lo);
}

/* Return the maximal number of contiguous entries in me->entries[]
 * starting from and including idx down to at the most n entries. */
static int batch_down(log_private_t* me, int idx, int n)
{
    assert(n > 0);
    int hi = subscript(me, idx);
    int lo = subscript(me, idx - n + 1);
    return (lo <= hi) ? (hi - lo + 1) : (hi + 1);
}

raft_entry_t* log_get_from_idx(log_t* me_, raft_index_t idx, int *n_etys)
{
    log_private_t* me = (log_private_t*)me_;
    raft_index_t i;

    assert(0 <= idx - 1);

    if (me->base + me->count < idx || idx <= me->base)
    {
        *n_etys = 0;
        return NULL;
    }

    *n_etys = batch_up(me, idx, (me->base + me->count) - idx + 1);
    return &me->entries[subscript(me, idx)];
}

raft_entry_t* log_get_at_idx(log_t* me_, raft_index_t idx)
{
    log_private_t* me = (log_private_t*)me_;
    raft_index_t i;

    if (idx == 0)
        return NULL;

    if (me->base + me->count < idx || idx <= me->base)
        return NULL;

    /* idx starts at 1 */
    idx -= 1;

    i = (me->front + idx - me->base) % me->size;
    return &me->entries[i];
}

raft_index_t log_count(log_t* me_)
{
    return ((log_private_t*)me_)->count;
}

int log_delete(log_t* me_, raft_index_t idx)
{
    log_private_t* me = (log_private_t*)me_;
    int e = 0;

    if (!has_idx(me, idx))
        return -1;

    while (idx <= me->base + me->count)
    {
        raft_index_t idx_tmp = me->base + me->count;
        raft_index_t back = mod(me->back - 1, me->size);

        if (me->cb && me->cb->log_pop)
            e = me->cb->log_pop(me->raft, raft_get_udata(me->raft),
                                 ptr, idx, &k);
        if (k > 0)
        {
            raft_pop_log(me->raft, ptr, k, idx);
            me->count -= k;
        }
        if (0 != e)
            return e;
        assert(k == batch_size);
    }
    return 0;
}

int log_poll(log_t* me_, int idx)
{
    log_private_t* me = (log_private_t*)me_;
    raft_index_t idx = me->base + 1;

    if (!has_idx(me, idx))
        return -1;

    while (me->base + 1 <= idx)
    {
        int k = batch_up(me, me->base + 1, idx - (me->base + 1) + 1);
        int batch_size = k;
        if (me->cb && me->cb->log_poll)
            e = me->cb->log_poll(me->raft, raft_get_udata(me->raft),
                                 &me->entries[me->front], me->base + 1, &k);
        if (k > 0)
        {
            me->front += k;
            me->front %= me->size;
            me->count -= k;
            me->base += k;
        }
        if (0 != e)
            return e;
        assert(k == batch_size);
    }

    return 0;
}

raft_entry_t *log_peektail(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    if (0 == me->count)
        return NULL;
    return &me->entries[subscript(me, me->base + me->count)];
}

void log_empty(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    me->front = 0;
    me->count = 0;
}

void log_free(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;

    __raft_free(me->entries);
    __raft_free(me);
}

raft_index_t log_get_current_idx(log_t* me_)
{
    log_private_t* me = (log_private_t*)me_;
    return log_count(me_) + me->base;
}

raft_index_t log_get_base(log_t* me_)
{
    return ((log_private_t*)me_)->base;
}
