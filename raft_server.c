
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

typedef struct {
    /* Array: For each server, index of the next log entry to send to
     * that server (initialized to leader last log index +1) */
    int next_index;

    /* Array: for each server, index of highest log entry known to be
     * replicated on server (initialized to 0, increases monotonically) */
    int match_index;

    /* The latest entry that each follower has acknowledged is the same as
     * the leader's. This is used to calculate commitIndex on the leader. */
    int last_agree_index;
} leader_t;

typedef struct {

    /* The set of servers from which the candidate has
     * received a RequestVote response in this term. */
    int votes_responded;

    /* The set of servers from which the candidate has received a vote in this 
     * term. */
    int votes_granted;

} candidate_t;

typedef struct {
    /* Persistent state: */

    /* the server's best guess of what the current term is
     * starts at zero */
    int current_term;

    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    int voted_for;

    /* the log which is replicated */
    log_t* log;

    /* Volatile state: */

    /* idx of highest log entry known to be committed */
    int commit_idx;

    /* idx of highest log entry applied to state machine */
    int last_applied_idx;

    /* follower/leader/candidate indicator */
    int state;

    /* most recently append idx, also indicates size of log */
    int current_idx;

    /* amount of time left till timeout */
    int timeout_elapsed;

    /* who has voted for me. This is an array with N = 'nnodes' elements */
    int *votes_for_me;

    raft_node_t* nodes;
    int nnodes;

    int election_timeout;
    int request_timeout;

    /* callbacks */
    raft_cbs_t cb;
    void* cb_ctx;

    int nodeid;

    union {
        leader_t leader;
        candidate_t candidate;
    };

} raft_server_private_t;

static void __log(raft_server_t *me_, void *src, const char *fmt, ...)
{
    raft_server_private_t* me = (void*)me_;
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    //printf("%d: %s\n", me->nodeid, buf);
    //__FUNC_log(bto,src,buf);
}

raft_server_t* raft_new()
{
    raft_server_private_t* me;

    if (!(me = calloc(1,sizeof(raft_server_private_t))))
        return NULL;

    me->current_term = 1;
    me->voted_for = -1;
    me->current_idx = 1;
    me->timeout_elapsed = 0;
    me->log = log_new();
    raft_set_state((void*)me,RAFT_STATE_FOLLOWER);
    me->request_timeout = 200;
    me->election_timeout = 1000;
    return (void*)me;
}

void raft_set_callbacks(raft_server_t* me_,
        raft_cbs_t* funcs, void* cb_ctx)
{
    raft_server_private_t* me = (void*)me_;

    memcpy(&me->cb, funcs, sizeof(raft_cbs_t));
    me->cb_ctx = cb_ctx;
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

    __log(me_, NULL, "election starting: %d %d, term: %d",
            me->election_timeout, me->timeout_elapsed, me->current_term);

    /* time to throw our hat in */
    raft_become_candidate(me_);
}

void raft_become_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    __log(me_, NULL, "becoming leader");
    raft_set_state(me_,RAFT_STATE_LEADER);
    me->voted_for = -1;
    for (i=0; i<me->nnodes; i++)
    {
        if (me->nodeid == i) continue;
        raft_node_t* p = raft_get_node(me_, i);
        raft_node_set_next_idx(p, raft_get_current_idx(me_)+1);
        raft_send_appendentries(me_, i);
    }
}

void raft_become_candidate(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    __log(me_, NULL, "becoming candidate");
    memset(me->votes_for_me, 0, sizeof(int) * me->nnodes);
    me->current_term += 1;
    raft_vote(me_, me->nodeid);
    raft_set_state(me_, RAFT_STATE_CANDIDATE);
    me->timeout_elapsed = rand() % 500;
    /* request votes from nodes */
    for (i=0; i<me->nnodes; i++)
    {
        if (me->nodeid == i) continue;
        raft_send_requestvote(me_, i);
    }
}

void raft_become_follower(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, NULL, "becoming follower");
    raft_set_state(me_, RAFT_STATE_FOLLOWER);
    me->voted_for = -1;
}

int raft_periodic(raft_server_t* me_, int msec_since_last_period)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, NULL, "periodic elapsed time: %d", me->timeout_elapsed);

    switch (me->state)
    {
    case RAFT_STATE_FOLLOWER:
        if (me->last_applied_idx < me->commit_idx)
        {
            if (0 == raft_apply_entry(me_))
                return 0;
        }
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
    else
    {
        if (me->election_timeout <= me->timeout_elapsed)
        {
            raft_election_start(me_);
        }
    }

    return 1;
}

raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int etyidx)
{
    raft_server_private_t* me = (void*)me_;
    return log_get_from_idx(me->log, etyidx);
}

int raft_recv_appendentries_response(raft_server_t* me_,
        int node, msg_appendentries_response_t* aer)
{
    raft_server_private_t* me = (void*)me_;
    raft_node_t* p;

    __log(me_, NULL, "received appendentries response from: %d", node);

    p = raft_get_node(me_, node);

    if (1 == aer->success)
    {
        int i;

        for (i=aer->first_idx; i<=aer->current_idx; i++)
            log_mark_node_has_committed(me->log, i);

        while (1)
        {
            raft_entry_t* e;

            e = log_get_from_idx(me->log, me->last_applied_idx + 1);

            /* majority has this */
            if (e && me->nnodes / 2 <= e->nnodes)
            {
                if (0 == raft_apply_entry(me_)) break;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        /* If AppendEntries fails because of log inconsistency:
           decrement nextIndex and retry (§5.3) */
        assert(0 <= raft_node_get_next_idx(p));
        // TODO does this have test coverage?
        // TODO can jump back to where node is different instead of iterating
        raft_node_set_next_idx(p, raft_node_get_next_idx(p)-1);
        raft_send_appendentries(me_, node);
    }

    return 1;
}

int raft_recv_appendentries(
        raft_server_t* me_,
        const int node,
        msg_appendentries_t* ae)
{
    raft_server_private_t* me = (void*)me_;
    msg_appendentries_response_t r;

    me->timeout_elapsed = 0;

    __log(me_, NULL, "received appendentries from: %d", node);

    r.term = me->current_term;

    /* we've found a leader who is legitimate */
    if (raft_is_leader(me_) && me->current_term <= ae->term)
        raft_become_follower(me_);

    /* 1. Reply false if term < currentTerm (§5.1) */
    if (ae->term < me->current_term)
    {
        __log(me_, NULL, "AE term is less than current term");
        r.success = 0;
        goto done;
    }

#if 0
    if (-1 != ae->prev_log_idx &&
         ae->prev_log_idx < raft_get_current_idx(me_))
    {
        __log(me_, NULL, "AE prev_idx is less than current idx");
        r.success = 0;
        goto done;
    }
#endif

    /* not the first appendentries we've received */
    if (0 != ae->prev_log_idx)
    {
        raft_entry_t* e;

        if ((e = raft_get_entry_from_idx(me_, ae->prev_log_idx)))
        {
            /* 2. Reply false if log doesn’t contain an entry at prevLogIndex
               whose term matches prevLogTerm (§5.3) */
            if (e->term != ae->prev_log_term)
            {
                __log(me_, NULL, "AE term doesn't match prev_idx");
                r.success = 0;
                goto done;
            }

            /* 3. If an existing entry conflicts with a new one (same index
            but different terms), delete the existing entry and all that
            follow it (§5.3) */
            raft_entry_t* e2;
            if ((e2 = raft_get_entry_from_idx(me_, ae->prev_log_idx+1)))
            {
                log_delete(me->log, ae->prev_log_idx+1);
            }
        }
        else
        {
            __log(me_, NULL, "AE no log at prev_idx");
            r.success = 0;
            goto done;
            //assert(0);
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
            while (1 == raft_apply_entry(me_));
        }
    }

    if (raft_is_candidate(me_))
        raft_become_follower(me_);

    raft_set_current_term(me_, ae->term);

    int i;
    
    /* append all entries to log */
    for (i=0; i<ae->n_entries; i++)
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
            __log(me_, NULL, "AE failure; couldn't append entry");
            r.success = 0;
            goto done;
        }
    }

    r.success = 1;
    r.current_idx = raft_get_current_idx(me_);
    r.first_idx = ae->prev_log_idx + 1;

done:
    if (me->cb.send)
        me->cb.send(me->cb_ctx, me, node, RAFT_MSG_APPENDENTRIES_RESPONSE,
                (void*)&r, sizeof(msg_appendentries_response_t));
    return 1;
}

int raft_recv_requestvote(raft_server_t* me_, int node, msg_requestvote_t* vr)
{
    raft_server_private_t* me = (void*)me_;
    msg_requestvote_response_t r;

    if (raft_get_current_term(me_) < vr->term)
    {
        me->voted_for = -1;
    }

    if (vr->term < raft_get_current_term(me_) ||
        /* we've already voted */
        -1 != me->voted_for ||
        /* we have a more up-to-date log */
        vr->last_log_idx < me->current_idx)
    {
        r.vote_granted = 0;
    }
    else
    {
        raft_vote(me_,node);
        r.vote_granted = 1;
    }

    __log(me_, NULL, "node requested vote: %d replying: %s",
            node, r.vote_granted == 1 ? "granted" : "not granted");

    r.term = raft_get_current_term(me_);
    if (me->cb.send)
        me->cb.send(me->cb_ctx, me, node, RAFT_MSG_REQUESTVOTE_RESPONSE,
                (void*)&r, sizeof(msg_requestvote_response_t));

    return 0;
}

int raft_votes_is_majority(const int nnodes, const int nvotes)
{
    int half;

    if (nnodes < nvotes)
        return 0;
    half = nnodes / 2;
    return half + 1 <= nvotes;
}

int raft_recv_requestvote_response(raft_server_t* me_, int node,
        msg_requestvote_response_t* r)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, NULL, "node responded to requestvote: %d status: %s",
            node, r->vote_granted == 1 ? "granted" : "not granted");

    if (raft_is_leader(me_))
        return 0;

    assert(node < me->nnodes);

//    if (r->term != raft_get_current_term(me_))
//        return 0;

    if (1 == r->vote_granted)
    {
        int votes;

        me->votes_for_me[node] = 1;
        votes = raft_get_nvotes_for_me(me_);
        if (raft_votes_is_majority(me->nnodes, votes))
            raft_become_leader(me_);
    }

    return 0;
}

int raft_send_entry_response(raft_server_t* me_,
        int node, int etyid, int was_committed)
{
    raft_server_private_t* me = (void*)me_;
    msg_entry_response_t res;

    __log(me_, NULL, "send entry response to: %d", node);

    res.id = etyid;
    res.was_committed = was_committed;
    if (me->cb.send)
        me->cb.send(me->cb_ctx, me, node, RAFT_MSG_ENTRY_RESPONSE,
                (void*)&res, sizeof(msg_entry_response_t));
    return 0;
}

int raft_recv_entry(raft_server_t* me_, int node, msg_entry_t* cmd)
{
    raft_server_private_t* me = (void*)me_;
    raft_entry_t ety;
    int res, i;

    __log(me_, NULL, "received entry from: %d", node);

    ety.term = me->current_term;
    ety.id = cmd->id;
    ety.data = cmd->data;
    ety.len = cmd->len;
    res = raft_append_entry(me_, &ety);
    raft_send_entry_response(me_, node, cmd->id, res);
    for (i=0; i<me->nnodes; i++)
    {
        if (me->nodeid == i) continue;
        raft_send_appendentries(me_,i);
    }
    return 0;
}

int raft_send_requestvote(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;
    msg_requestvote_t rv;

    __log(me_, NULL, "sending requestvote to: %d", node);

    rv.term = me->current_term;
    rv.last_log_idx = raft_get_current_idx(me_);
    if (me->cb.send)
        me->cb.send(me->cb_ctx, me, node, RAFT_MSG_REQUESTVOTE,
                (void*)&rv, sizeof(msg_requestvote_t));
    return 1;
}

int raft_append_entry(raft_server_t* me_, raft_entry_t* c)
{
    raft_server_private_t* me = (void*)me_;

    if (1 == log_append_entry(me->log,c))
    {
        me->current_idx += 1;
        return 1;
    }
    return 0;
}

int raft_apply_entry(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    if (!log_get_from_idx(me->log, me->last_applied_idx+1))
        return 0;

    __log(me_, NULL, "applying log: %d", me->last_applied_idx);

    me->last_applied_idx++;
    if (me->commit_idx < me->last_applied_idx)
        me->commit_idx = me->last_applied_idx;

    // TODO: callback for state machine application goes here

    return 1;
}

void raft_send_appendentries(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;

    __log(me_, NULL, "sending appendentries to: %d", node);

    if (!(me->cb.send))
        return;

    msg_appendentries_t ae;
    raft_node_t* p = raft_get_node(me_, node);

    ae.term = me->current_term;
    ae.leader_id = raft_get_my_id(me_);
    ae.prev_log_term = raft_node_get_next_idx(p);
    // TODO:
    ae.prev_log_idx = 0;
    ae.n_entries = 0;
    me->cb.send(me->cb_ctx, me, node, RAFT_MSG_APPENDENTRIES,
            (void*)&ae, sizeof(msg_appendentries_t));
}

void raft_send_appendentries_all(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i;

    for (i=0; i<me->nnodes; i++)
    {
        if (me->nodeid == i) continue;
        raft_send_appendentries(me_, i);
    }
}

void raft_set_configuration(raft_server_t* me_,
        raft_node_configuration_t* nodes, int me_idx)
{
    raft_server_private_t* me = (void*)me_;
    int nnodes;

    /* TODO: one memory allocation only please */
    for (nnodes=0; nodes->udata_address; nodes++)
    {
        nnodes++;
        me->nodes = realloc(me->nodes,sizeof(raft_node_t*) * nnodes);
        me->nnodes = nnodes;
        me->nodes[nnodes-1] = raft_node_new(nodes);
    }
    me->votes_for_me = calloc(nnodes, sizeof(int));
    me->nodeid = me_idx;
}

int raft_get_nvotes_for_me(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i, votes;

    for (i=0, votes=0; i<me->nnodes; i++)
    {
        if (me->nodeid == i) continue;
        if (1 == me->votes_for_me[i])
            votes += 1;
    }

    if (raft_get_voted_for(me_) == raft_get_my_id(me_))
        votes += 1;

    return votes;
}

void raft_set_election_timeout(raft_server_t* me_, int millisec)
{
    raft_server_private_t* me = (void*)me_;
    me->election_timeout = millisec;
}

void raft_set_request_timeout(raft_server_t* me_, int millisec)
{
    raft_server_private_t* me = (void*)me_;
    me->request_timeout = millisec;
}

int raft_get_nodeid(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->nodeid;
}

int raft_get_election_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->election_timeout;
}

int raft_get_request_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->request_timeout;
}

void raft_vote(raft_server_t* me_, int node)
{
    raft_server_private_t* me = (void*)me_;
    me->voted_for = node;
}

int raft_get_num_nodes(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->nnodes;
}

void raft_set_state(raft_server_t* me_, int state)
{
    raft_server_private_t* me = (void*)me_;
    me->state = state;
}

int raft_get_state(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->state;
}

raft_node_t* raft_get_node(raft_server_t *me_, int nodeid)
{
    raft_server_private_t* me = (void*)me_;

    if (nodeid < 0 || me->nnodes <= nodeid)
        return NULL;
    return me->nodes[nodeid];
}

int raft_get_timeout_elapsed(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->timeout_elapsed;
}

int raft_get_log_count(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return log_count(me->log);
}

int raft_get_voted_for(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->voted_for;
}

void raft_set_current_term(raft_server_t* me_, int term)
{
    raft_server_private_t* me = (void*)me_;
    me->current_term = term;
}

int raft_get_current_term(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->current_term;
}

void raft_set_current_idx(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;
    me->current_idx = idx;
}

int raft_get_current_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->current_idx;
}

int raft_is_follower(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_FOLLOWER;
}

int raft_is_leader(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_LEADER;
}

int raft_is_candidate(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_CANDIDATE;
}

int raft_get_my_id(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->nodeid;
}

void raft_set_commit_idx(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;
    me->commit_idx = idx;
}

void raft_set_last_applied_idx(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;
    me->last_applied_idx = idx;
}

int raft_get_last_applied_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->last_applied_idx;
}

int raft_get_commit_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->commit_idx;
}

/*--------------------------------------------------------------79-characters-*/
