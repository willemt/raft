
#ifndef RAFT_PRIVATE_H_
#define RAFT_PRIVATE_H_

/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

typedef struct {
    /* Persistent state: */

    /* the server's best guess of what the current term is
     * starts at zero */
    int current_term;

    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    int voted_for;

    /* the log which is replicated */
    void* log;

    /* Volatile state: */

    /* idx of highest log entry known to be committed */
    int commit_idx;

    /* idx of highest log entry applied to state machine */
    int last_applied_idx;

    /* follower/leader/candidate indicator */
    int state;

    /* amount of time left till timeout */
    int timeout_elapsed;

    /* who has voted for me. This is an array with N = 'num_nodes' elements */
    int *votes_for_me;

    raft_node_t* nodes;
    int num_nodes;

    int election_timeout;
    int request_timeout;

    /* what this node thinks is the node ID of the current leader, or -1 if
     * there isn't a known current leader. */
    int current_leader;

    /* callbacks */
    raft_cbs_t cb;
    void* udata;

    /* my node ID */
    int nodeid;
} raft_server_private_t;

void raft_election_start(raft_server_t* me);

void raft_become_leader(raft_server_t* me);

void raft_become_candidate(raft_server_t* me);

void raft_become_follower(raft_server_t* me);

void raft_vote(raft_server_t* me, int node);

void raft_set_current_term(raft_server_t* me,int term);

/**
 * @return 0 on error */
int raft_send_requestvote(raft_server_t* me, int node);

void raft_send_appendentries(raft_server_t* me, int node);

void raft_send_appendentries_all(raft_server_t* me_);

/**
 * Apply entry at lastApplied + 1. Entry becomes 'committed'.
 * @return 1 if entry committed, 0 otherwise */
int raft_apply_entry(raft_server_t* me_);

/**
 * Appends entry using the current term.
 * Note: we make the assumption that current term is up-to-date
 * @return 0 if unsuccessful */
int raft_append_entry(raft_server_t* me_, raft_entry_t* c);

void raft_set_commit_idx(raft_server_t* me, int commit_idx);
int raft_get_commit_idx(raft_server_t* me_);

void raft_set_last_applied_idx(raft_server_t* me, int idx);

void raft_set_state(raft_server_t* me_, int state);

int raft_get_state(raft_server_t* me_);

raft_node_t* raft_node_new(void* udata);

void raft_node_set_next_idx(raft_node_t* node, int nextIdx);

void raft_node_set_match_idx(raft_node_t* node, int matchIdx);

int raft_node_get_match_idx(raft_node_t* me_);

int raft_votes_is_majority(const int nnodes, const int nvotes);

#endif /* RAFT_PRIVATE_H_ */
