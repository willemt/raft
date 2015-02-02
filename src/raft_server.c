/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @brief Implementation of a Raft server
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for varags */
#include <stdarg.h>

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

static void __log(raft_server_t *me_, const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);

#if 0 /* debugging */
    raft_server_private_t* me = (void*)me_;
    printf("%d: %s\n", me->nodeid, buf);
    __FUNC_log(bto, src, buf);
#endif
}

raft_server_t* raft_new()
{
    raft_server_private_t* me;

    if (!(me = calloc(1, sizeof(raft_server_private_t))))
        return NULL;

    me->current_term = 0;
    me->voted_for = -1;
    me->current_idx = 1;
    me->timeout_elapsed = 0;
    me->request_timeout = 200;
    me->election_timeout = 1000;
    me->log = log_new();
    raft_set_state((void*)me, RAFT_STATE_FOLLOWER);
    return (void*)me;
}

void raft_set_callbacks(raft_server_t* me_,
                        raft_cbs_t* funcs, void* udata)
{
    raft_server_private_t* me = (void*)me_;

    memcpy(&me->cb, funcs, sizeof(raft_cbs_t));
    me->udata = udata;
}

void raft_free(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    log_free(me->log);
    free(me_);
}

void raft_election_start(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, "election starting: %d %d, term: %d",
          me->election_timeout, me->timeout_elapsed, me->current_term);

    raft_become_candidate(me_);
}

void raft_become_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    __log(me_, "becoming leader");

    raft_set_state(me_, RAFT_STATE_LEADER);
    me->voted_for = -1;
    for (i = 0; i < me->num_nodes; i++)
    {
        if (me->nodeid == i)
            continue;
        raft_node_t* p = raft_get_node(me_, i);
        raft_node_set_next_idx(p, raft_get_current_idx(me_) + 1);
        raft_send_appendentries(me_, i);
    }
}

void raft_become_candidate(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    __log(me_, "becoming candidate");

    memset(me->votes_for_me, 0, sizeof(int) * me->num_nodes);
    me->current_term += 1;
    raft_vote(me_, me->nodeid);
    raft_set_state(me_, RAFT_STATE_CANDIDATE);

    /* we need a random factor here to prevent simultaneous candidates */
    me->timeout_elapsed = rand() % 500;

    for (i = 0; i < me->num_nodes; i++)
    {
        if (me->nodeid == i)
            continue;
        raft_send_requestvote(me_, i);
    }
}

void raft_become_follower(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, "becoming follower");

    raft_set_state(me_, RAFT_STATE_FOLLOWER);
    me->voted_for = -1;
}

int raft_periodic(raft_server_t* me_, int msec_since_last_period)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, "periodic elapsed time: %d", me->timeout_elapsed);

    switch (me->state)
    {
    case RAFT_STATE_FOLLOWER:
        if (me->last_applied_idx < me->commit_idx)
            if (0 == raft_apply_entry(me_))
                return 0;
        break;
    }

    me->timeout_elapsed += msec_since_last_period;

    if (me->state == RAFT_STATE_LEADER)
    {
        if (me->request_timeout <= me->timeout_elapsed)
        {
            raft_send_appendentries_all(me_);
            me->timeout_elapsed = 0;
        }
    }
    else if (me->election_timeout <= me->timeout_elapsed)
        raft_election_start(me_);


    return 1;
}

raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int etyidx)
{
    raft_server_private_t* me = (void*)me_;
    return log_get_from_idx(me->log, etyidx);
}

int raft_recv_appendentries_response(raft_server_t* me_,
                                     int node, msg_appendentries_response_t* r)
{
    raft_server_private_t* me = (void*)me_;
    raft_node_t* p;

    __log(me_, "received appendentries response from: %d", node);

    p = raft_get_node(me_, node);

    if (1 == r->success)
    {
        int i;

        for (i = r->first_idx; i <= r->current_idx; i++)
            log_mark_node_has_committed(me->log, i);

        while (1)
        {
            raft_entry_t* e;

            e = log_get_from_idx(me->log, me->last_applied_idx + 1);

            /* majority has this */
            if (e && me->num_nodes / 2 <= e->num_nodes)
            {
                if (0 == raft_apply_entry(me_))
                    break;
            }
            else
                break;
        }
    }
    else
    {
        /* If AppendEntries fails because of log inconsistency:
           decrement nextIndex and retry (§5.3) */
        assert(0 <= raft_node_get_next_idx(p));
        // TODO does this have test coverage?
        // TODO can jump back to where node is different instead of iterating
        raft_node_set_next_idx(p, raft_node_get_next_idx(p) - 1);
        raft_send_appendentries(me_, node);
    }

    return 1;
}

int raft_recv_appendentries(
    raft_server_t* me_,
    const int node,
    msg_appendentries_t* ae,
    msg_appendentries_response_t *r
    )
{
    raft_server_private_t* me = (void*)me_;

    me->timeout_elapsed = 0;

    __log(me_, "received appendentries from: %d", node);

    r->term = me->current_term;

    /* we've found a leader who is legitimate */
    if (raft_is_leader(me_) && me->current_term <= ae->term)
        raft_become_follower(me_);

    /* 1. Reply false if term < currentTerm (§5.1) */
    if (ae->term < me->current_term)
    {
        __log(me_, "AE term is less than current term");
        r->success = 0;
        return 1;
    }

#if 0
    if (-1 != ae->prev_log_idx &&
        ae->prev_log_idx < raft_get_current_idx(me_))
    {
        __log(me_, "AE prev_idx is less than current idx");
        r->success = 0;
        return 1;
    }
#endif

    /* not the first appendentries we've received */
    if (0 != ae->prev_log_idx)
    {
        raft_entry_t* e;

        if ((e = raft_get_entry_from_idx(me_, ae->prev_log_idx)))
        {
            /* 2. Reply false if log doesn't contain an entry at prevLogIndex
               whose term matches prevLogTerm (§5.3) */
            if (e->term != ae->prev_log_term)
            {
                __log(me_, "AE term doesn't match prev_idx");
                r->success = 0;
                return 1;
            }

            /* 3. If an existing entry conflicts with a new one (same index
               but different terms), delete the existing entry and all that
               follow it (§5.3) */
            raft_entry_t* e2;
            if ((e2 = raft_get_entry_from_idx(me_, ae->prev_log_idx + 1)))
                log_delete(me->log, ae->prev_log_idx + 1);
        }
        else
        {
            __log(me_, "AE no log at prev_idx");
            r->success = 0;
            return 1;
        }
    }

    /* 5. If leaderCommit > commitIndex, set commitIndex =
        min(leaderCommit, last log index) */
    if (raft_get_commit_idx(me_) < ae->leader_commit)
    {
        raft_entry_t* e;

        if ((e = log_peektail(me->log)))
        {
            raft_set_commit_idx(me_, e->id < ae->leader_commit ?
                                e->id : ae->leader_commit);
            while (1 == raft_apply_entry(me_))
                ;
        }
    }

    if (raft_is_candidate(me_))
        raft_become_follower(me_);

    raft_set_current_term(me_, ae->term);

    int i;

    /* append all entries to log */
    for (i = 0; i < ae->n_entries; i++)
    {
        msg_entry_t* cmd;
        raft_entry_t* c;

        cmd = &ae->entries[i];

        /* TODO: replace malloc with mempoll/arena */
        c = malloc(sizeof(raft_entry_t));
        c->term = me->current_term;
        c->len = cmd->len;
        c->id = cmd->id;
        c->data = malloc(cmd->len);
        memcpy(c->data, cmd->data, cmd->len);
        if (0 == raft_append_entry(me_, c))
        {
            __log(me_, "AE failure; couldn't append entry");
            r->success = 0;
            return 1;
        }
    }

    r->success = 1;
    r->current_idx = raft_get_current_idx(me_);
    r->first_idx = ae->prev_log_idx + 1;
    return 1;
}

int raft_recv_requestvote(raft_server_t* me_, int node, msg_requestvote_t* vr,
                          msg_requestvote_response_t *r)
{
    raft_server_private_t* me = (void*)me_;

    if (raft_get_current_term(me_) < vr->term)
        me->voted_for = -1;

    if (vr->term < raft_get_current_term(me_) ||
        /* we've already voted */
        -1 != me->voted_for ||
        /* we have a more up-to-date log */
        vr->last_log_idx < me->current_idx)
        r->vote_granted = 0;
    else
    {
        raft_vote(me_, node);
        r->vote_granted = 1;
    }

    __log(me_, "node requested vote: %d replying: %s",
          node, r->vote_granted == 1 ? "granted" : "not granted");

    r->term = raft_get_current_term(me_);
    return 0;
}

int raft_votes_is_majority(const int num_nodes, const int nvotes)
{
    int half;

    if (num_nodes < nvotes)
        return 0;
    half = num_nodes / 2;
    return half + 1 <= nvotes;
}

int raft_recv_requestvote_response(raft_server_t* me_, int node,
                                   msg_requestvote_response_t* r)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, "node responded to requestvote: %d status: %s",
          node, r->vote_granted == 1 ? "granted" : "not granted");

    if (raft_is_leader(me_))
        return 0;

    assert(node < me->num_nodes);

//    if (r->term != raft_get_current_term(me_))
//        return 0;

    if (1 == r->vote_granted)
    {
        int votes;

        me->votes_for_me[node] = 1;
        votes = raft_get_nvotes_for_me(me_);
        if (raft_votes_is_majority(me->num_nodes, votes))
            raft_become_leader(me_);
    }

    return 0;
}

int raft_recv_entry(raft_server_t* me_, int node, msg_entry_t* e,
                    msg_entry_response_t *r)
{
    raft_server_private_t* me = (void*)me_;
    raft_entry_t ety;
    int res, i;

    __log(me_, "received entry from: %d", node);

    ety.term = me->current_term;
    ety.id = e->id;
    ety.data = e->data;
    ety.len = e->len;
    res = raft_append_entry(me_, &ety);
    for (i = 0; i < me->num_nodes; i++)
        if (me->nodeid != i)
            raft_send_appendentries(me_, i);

    r->id = e->id;
    r->was_committed = res;
    return 0;
}

int raft_send_requestvote(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;
    msg_requestvote_t rv;

    __log(me_, "sending requestvote to: %d", node);

    rv.term = me->current_term;
    rv.last_log_idx = raft_get_current_idx(me_);
    if (me->cb.send_requestvote)
        me->cb.send_requestvote(me_, me->udata, node, &rv);
    return 1;
}

int raft_append_entry(raft_server_t* me_, raft_entry_t* c)
{
    raft_server_private_t* me = (void*)me_;

    if (1 == log_append_entry(me->log, c))
    {
        me->current_idx += 1;
        return 1;
    }
    return 0;
}

int raft_apply_entry(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    raft_entry_t* e;

    if (!(e = log_get_from_idx(me->log, me->last_applied_idx + 1)))
        return 0;

    __log(me_, "applying log: %d", me->last_applied_idx);

    me->last_applied_idx++;
    if (me->commit_idx < me->last_applied_idx)
        me->commit_idx = me->last_applied_idx;
    if (me->cb.applylog)
        me->cb.applylog(me_, me->udata, e->data, e->len);
    return 1;
}

void raft_send_appendentries(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, "sending appendentries to: %d", node);

    if (!(me->cb.send_appendentries))
        return;

    msg_appendentries_t ae;
    raft_node_t* p = raft_get_node(me_, node);

    ae.term = me->current_term;
    ae.leader_id = me->nodeid;
    ae.prev_log_term = raft_node_get_next_idx(p);
    // TODO:
    ae.prev_log_idx = 0;
    ae.n_entries = 0;
    if (me->cb.send_appendentries)
        me->cb.send_appendentries(me_, me->udata, node, &ae);
}

void raft_send_appendentries_all(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    for (i = 0; i < me->num_nodes; i++)
    {
        if (me->nodeid == i)
            continue;
        raft_send_appendentries(me_, i);
    }
}

void raft_set_configuration(raft_server_t* me_,
                            raft_node_configuration_t* nodes, int my_idx)
{
    raft_server_private_t* me = (void*)me_;
    int num_nodes;

    /* TODO: one memory allocation only please */
    for (num_nodes = 0; nodes->udata_address; nodes++)
    {
        num_nodes++;
        me->nodes = realloc(me->nodes, sizeof(raft_node_t*) * num_nodes);
        me->num_nodes = num_nodes;
        me->nodes[num_nodes - 1] = raft_node_new(nodes->udata_address);
    }
    me->votes_for_me = calloc(num_nodes, sizeof(int));
    me->nodeid = my_idx;
}

int raft_get_nvotes_for_me(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i, votes;

    for (i = 0, votes = 0; i < me->num_nodes; i++)
    {
        if (me->nodeid == i)
            continue;
        if (1 == me->votes_for_me[i])
            votes += 1;
    }

    if (me->voted_for == me->nodeid)
        votes += 1;

    return votes;
}

void raft_vote(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;
    me->voted_for = node;
}
