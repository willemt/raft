
/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief ADT for managing Raft log entries (aka commands)
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
    arrayqueue_t* log;
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
    me->log = arrayqueue_new();
//    me->log_map = hashmap_new(__entry_hash, __entry_compare, 100);
    return (void*)me;
}

void raft_log_append_entry(raft_log_t* me_, raft_command_t* c)
{
    raft_log_private_t* me = (void*)me_;
    arrayqueue_offer(me->log, c);
}

int raft_log_count(raft_log_t* me_)
{
    raft_log_private_t* me = (void*)me_;
    return arrayqueue_count(me->log);
}
