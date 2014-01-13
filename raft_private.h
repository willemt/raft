/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

enum {
    RAFT_STATE_NONE,
    RAFT_STATE_FOLLOWER,
    RAFT_STATE_CANDIDATE,
    RAFT_STATE_LEADER
};


void raft_election_start(raft_server_t* me);

void raft_become_leader(raft_server_t* me);

void raft_become_candidate(raft_server_t* me);

void raft_vote(raft_server_t* me, int node);

void raft_set_current_term(raft_server_t* me,int term);

void raft_set_current_idx(raft_server_t* me,int idx);

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

void raft_set_last_applied_idx(raft_server_t* me, int idx);

void raft_set_state(raft_server_t* me_, int state);

raft_node_t* raft_node_new(void* udata);

void raft_node_set_next_idx(raft_node_t* node, int nextIdx);

int raft_votes_is_majority(const int nnodes, const int nvotes);

