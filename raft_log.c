
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

#include "linked_list_hashmap.h"
#include "arrayqueue.h"
#include "raft.h"
#include "raft_log.h"


typedef struct {
    arrayqueue_t* log_qu;
    hashmap_t* log_map;
} raft_log_private_t;

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

raft_log_t* raft_log_new()
{
    raft_log_private_t* me;

    me = calloc(1,sizeof(raft_log_private_t));
    me->log_qu = arrayqueue_new();
    me->log_map = hashmap_new(__entry_hash, __entry_compare, 100);
    return (void*)me;
}

/**
 * Add entry to log.
 * Don't add entry if we've already added this entry (based off ID)
 * Don't add entries with ID=0 
 * @return 0 if unsucessful; 1 otherwise */
int raft_log_append_entry(raft_log_t* me_, raft_entry_t* c)
{
    raft_log_private_t* me = (void*)me_;

    if (0 == c->id)
        return 0;

    if (hashmap_get(me->log_map, (void*)c->id))
        return 0;

    arrayqueue_offer(me->log_qu, c);
    hashmap_put(me->log_map, (void*)c->id, c);
    assert(arrayqueue_count(me->log_qu) == hashmap_count(me->log_map));
    return 1;
}

raft_entry_t* raft_log_get_from_index(raft_log_t* me_, int idx)
{
    raft_log_private_t* me = (void*)me_;

    return hashmap_get(me->log_map, (void*)idx);
}

int raft_log_count(raft_log_t* me_)
{
    raft_log_private_t* me = (void*)me_;
    return arrayqueue_count(me->log_qu);
}
