
/**
 * @file
 * @brief 
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "linked_list_hashmap.h"
#include "arrayqueue.h"
#include "raft.h"

typedef struct {
    /* Persistent state: */

    /* the server's best guess of what the current term is
     * starts at zero */
    int current_term;

    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    int voted_for;

    /* the log which is replicated */
    //void* log;
    arrayqueue_t* log;

    /* Volatile state: */

    /* Index of highest log entry known to be committed */
    int commit_index;

    /* Index of highest log entry applied to state machine */
    int last_applied_index;

    /*  follower/leader/candidate indicator */
    int state;

    raft_functions_t *func;

    /* callbacks */
    raft_external_functions_t *ext_func;
    void* caller;

    /* indicates size of log */
    int current_index;

    /* amount of time left till timeout */
    int timeout_elapased;

    raft_peer_t* peers;
    int npeers;

    int election_timeout;
    int request_timeout;

//    hashmap_t* peers;

//    int log_Size;

    /* who has voted for me */
    int *votes_for_me;
} raft_server_private_t;


static unsigned long __peer_hash(
    const void *obj
)
{
    return (unsigned long) obj;
}

static long __peer_compare(
    const void *obj,
    const void *other
)
{
    return obj - other;
}

raft_server_t* raft_new()
{
    raft_server_private_t* me;

    if (!(me = calloc(1,sizeof(raft_server_private_t))))
        return NULL;
    me->voted_for = -1;
    me->current_index = 0;
    me->timeout_elapased = 0;
    me->log = arrayqueue_new();
    raft_set_state((void*)me,RAFT_STATE_FOLLOWER);
    raft_set_request_timeout((void*)me, 500);
    raft_set_election_timeout((void*)me, 1000);
//    me->log_map = hashmap_new(__log_hash, __log_compare, 100);
//    me->peers = hashmap_new(__peer_hash, __peer_compare, 100);
    return (void*)me;
}

void raft_free(raft_server_t* me_)
{
    free(me_);
}

void raft_set_state(raft_server_t* me_, int state)
{
    raft_server_private_t* me = (void*)me_;

    me->state = state;
}

int raft_get_state(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    return me->state;
}

void raft_set_external_functions(raft_server_t* me_, raft_external_functions_t* funcs, void* caller)
{
    raft_server_private_t* me = (void*)me_;

    me->caller = caller;
    me->ext_func = funcs;
}

void raft_election_start(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    me->timeout_elapased = 0;
    /* time to throw our hat in */
    raft_become_candidate(me_);
}

/**
 * Candidate i transitions to leader. */
void raft_become_leader(raft_server_t* me_)
{
    raft_set_state(me_,RAFT_STATE_LEADER);
}

void raft_become_candidate(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
//    hashmap_iterator_t iter;
    void* p;
    int ii;

    memset(me->votes_for_me,0,sizeof(int) * me->npeers);
    me->current_term += 1;
    me->voted_for = 0;
    me->timeout_elapased = 0;

    raft_set_state(me_,RAFT_STATE_CANDIDATE);

//    for (hashmap_iterator(me->peers, &iter);
//         (p = hashmap_iterator_next_value(me->peers, &iter));)
    for (ii=0; ii<me->npeers; ii++)
    {
        msg_requestvote_t rv;

        rv.term = me->current_term;
        rv.candidate_id = 0;//me->current_term;
//        rv.last_log_index = me->log
        
        if (me->ext_func && me->ext_func->send)
            me->ext_func->send(me->caller,NULL, ii, (void*)&rv, sizeof(msg_requestvote_t));
    }

}

void raft_become_follower(raft_server_t* me_)
{
    raft_set_state(me_,RAFT_STATE_FOLLOWER);
}

/**
 * Convert to candidate if election timeout elapses without either
 *  Receiving valid AppendEntries RPC, or
 *  Granting vote to candidate
 */
#if 0
int raft_election_timeout_elapsed(raft_server_t* me_)
{
    raft_server_t* me_ = me_;

    if (nvalid_AEs_since_election == 0 || votes_granted_to_candidate_since_election == 0)
    {
        raft_become_candidate(me);
    }
}
#endif

int raft_get_timeout_elapsed(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->timeout_elapased;
}

int raft_periodic(raft_server_t* me_, int msec_since_last_period)
{
    raft_server_private_t* me = (void*)me_;

    me->timeout_elapased += msec_since_last_period;

    if (me->election_timeout < me->timeout_elapased)
    {
        raft_election_start(me_);
    }

    return 0;
}

/**
 * Invoked by leader to replicate log entries (§5.3); also used as heartbeat (§5.2). */
int raft_recv_appendentries(raft_server_t* me_, const int peer, msg_appendentries_t* ae)
{
    raft_server_private_t* me = (void*)me_;
    msg_appendentries_response_t r;

    r.term = raft_get_current_term(me_);

    if (
        ae->term < raft_get_current_term(me_) ||
        ae->prev_log_index < raft_get_current_index(me_))
    {
        r.success = 0;
        goto done;
    }

    if (raft_is_candidate(me_))
    {
        raft_become_follower(me_);
    }

    if (raft_get_current_term(me_) < ae->term)
    {
        raft_set_current_term(me_, ae->term);
    }

    int i;
    
    for (i=0; i<ae->n_entries; i++)
    {
        msg_command_t* cmd;
        raft_command_t* c;

        cmd = &ae->entries[i];

        c = malloc(sizeof(raft_command_t));
        c->term = raft_get_current_term(me_);
        c->len = cmd->len;
        c->id = cmd->id;
        c->data = malloc(cmd->len);
        memcpy(c->data,cmd->data,cmd->len);

        if (0 == raft_append_command(me_, c))
        {
            /* failure, we couldn't append it for some reason */
            r.success = 0;
            goto done;
        }
    }

    r.success = 1;

done:

    if (me->ext_func && me->ext_func->send)
        me->ext_func->send(me->caller, NULL, peer, (void*)&r,
                sizeof(msg_appendentries_response_t));

    return 0;
}

raft_command_t* raft_get_command_from_index(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;

    return NULL;
}

int raft_recv_appendentries_response(raft_server_t* me_, int peer, msg_appendentries_response_t* ae)
{
    return 0;
}

int raft_recv_requestvote(raft_server_t* me_, int peer, msg_requestvote_t* vr)
{
    raft_server_private_t* me = (void*)me_;
    msg_requestvote_response_t r;

    if (vr->term < raft_get_current_term(me_))
    {
        r.vote_granted = 0;
    }
    /* TODO: what's this for? */
    else if (-1 == me->voted_for)
    {
        r.vote_granted = 0;
    }
    else
    {
        me->voted_for = peer;
        r.vote_granted = 1;
    }

    r.term = raft_get_current_term(me_);
    if (me->ext_func && me->ext_func->send)
        me->ext_func->send(me->caller, NULL, peer, (void*)&r,
                sizeof(msg_requestvote_response_t));

    return 0;
}

int raft_votes_is_majority(const int npeers, const int nvotes)
{
    int half;

    if (npeers < nvotes)
        return 0;
    
    half = npeers / 2;
    return half + 1 <= nvotes;
}

/**
 * @param peer The peer this response was sent by */
int raft_recv_requestvote_response(raft_server_t* me_, int peer, msg_requestvote_response_t* r)
{
    raft_server_private_t* me = (void*)me_;

    assert(peer < me->npeers);

    if (r->term != raft_get_current_term(me_))
        return 0;

    if (1 == r->vote_granted)
    {
        int votes;

        me->votes_for_me[peer] = 1;

        votes = raft_get_nvotes_for_me(me_);

        if (raft_votes_is_majority(me->npeers, votes))
        {
            raft_become_leader(me_);
        }
    }

    return 0;
}

void raft_execute_command(raft_server_t* me_)
{
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

int raft_get_election_timeout(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->election_timeout;
}

int raft_get_request_timeout(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->request_timeout;
}

int raft_vote(raft_server_t* me_, int peer)
{
    return 0;
}

#if 0
raft_peer_t* raft_add_peer(raft_server_t* me_, int peer_udata)
{
    raft_server_private_t* me = (void*)me_;
    raft_peer_t* p;

    if (hashmap_get(me->peers,peer_udata))
    {
        return NULL;
    }

    p = raft_peer_new(peer_udata);
    hashmap_put(me->peers, peer_udata, p);
    return p;
}

int raft_remove_peer(raft_server_t* me_, int peer)
{
    return 0;
}
#endif

int raft_get_num_peers(raft_server_t* me_)
{

    return 0;
}

int raft_recv_command(raft_server_t* me_, int peer, msg_command_t* cmd)
{
    return 0;
}

/**
 * @return number of items within log */
int raft_get_log_count(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return arrayqueue_count(me->log);
}

int raft_get_voted_for(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->voted_for;
}

void raft_set_current_term(raft_server_t* me_, int term)
{
    raft_server_private_t* me = (void*)me_;
    me->current_term = term;
}

int raft_get_current_term(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->current_term;
}

void raft_set_current_index(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;
    me->current_index = idx;
}

int raft_get_current_index(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->current_index;
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
    return 0;
}

int raft_send_requestvote(raft_server_t* me_, int peer)
{
    raft_server_private_t* me = (void*)me_;
    msg_requestvote_t rv;

    rv.term = raft_get_current_term(me_);
    rv.last_log_index = raft_get_current_index(me_);
    if (me->ext_func && me->ext_func->send)
        me->ext_func->send(me->caller,NULL, peer, (void*)&rv, sizeof(msg_requestvote_t));

    return 0;
}

/**
 * Appends command using the current term.
 * Note: we make the assumption that current term is up-to-date
 * @return 0 if unsuccessful */
int raft_append_command(raft_server_t* me_, raft_command_t* c)
{
    raft_server_private_t* me = (void*)me_;

    arrayqueue_offer(me->log, c);
    me->current_index += 1;
    return 1;
}

void raft_set_commit_index(raft_server_t* me_, int commit_idx)
{
}

void raft_set_last_applied_index(raft_server_t* me_, int idx)
{
    raft_server_private_t* me = (void*)me_;
    me->last_applied_index = idx;
}

int raft_get_last_applied_index(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->last_applied_index;
}

int raft_get_commit_index(raft_server_t* me_)
{
    return 0;
}

void raft_commit_command(raft_server_t* me_, int logIndex)
{

}

void raft_send_appendentries(raft_server_t* me_, int peer)
{

}

void raft_set_configuration(raft_server_t* me_, raft_peer_configuration_t* peers)
{
    raft_server_private_t* me = (void*)me_;
    int npeers = 0;

    /* TODO: one allocation only please */
    while (peers->udata_address)
    {
        npeers++;
        me->peers = realloc(me->peers,sizeof(raft_peer_t*) * npeers);
        me->npeers = npeers;
        me->peers[npeers-1] = raft_peer_new(peers);
        peers++;
    }

    me->votes_for_me = calloc(npeers, sizeof(int));
}

/**
 * @return number of peers that this server has */
int raft_get_npeers(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;

    return me->npeers;
}

/**
 * @return number of votes this server has received this election */
int raft_get_nvotes_for_me(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    int i, votes;

    for (i=0, votes=0; i<me->npeers; i++)
    {
        if (1 == me->votes_for_me[i])
            votes += 1;
    }

    if (raft_get_voted_for(me_) == raft_get_my_id(me_))
        votes += 1;

    return votes;
}
