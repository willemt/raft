/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 */

#ifndef RAFT_PRIVATE_H_
#define RAFT_PRIVATE_H_

enum {
    RAFT_NODE_STATUS_DISCONNECTED,
    RAFT_NODE_STATUS_CONNECTED,
    RAFT_NODE_STATUS_CONNECTING,
    RAFT_NODE_STATUS_DISCONNECTING
};

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
 
    raft_node_t* nodes;
    int num_nodes;

    int election_timeout;
    int election_timeout_rand;
    int request_timeout;

    /* what this node thinks is the node ID of the current leader, or NULL if
     * there isn't a known current leader. */
    raft_node_t* current_leader;

    /* callbacks */
    raft_cbs_t cb;
    void* udata;

    /* my node ID */
    raft_node_t* node;

    /* log entry which has a voting cfg change, otherwise -1 */
    int voting_cfg_change_log_idx;

    /* Our membership with the cluster is confirmed (ie. configuration log
     * entry was committed) */
    int connected;

    int snapshot_in_progress;

    /* Last compacted snapshot */
    int snapshot_last_idx;
    int snapshot_last_term;
} raft_server_private_t;

int raft_election_start(raft_server_t* me);

int raft_become_candidate(raft_server_t* me);

void raft_randomize_election_timeout(raft_server_t* me_);

/**
 * @return 0 on error */
int raft_send_requestvote(raft_server_t* me, raft_node_t* node);

int raft_send_appendentries(raft_server_t* me, raft_node_t* node);

int raft_send_appendentries_all(raft_server_t* me_);

/**
 * Apply entry at lastApplied + 1. Entry becomes 'committed'.
 * @return 1 if entry committed, 0 otherwise */
int raft_apply_entry(raft_server_t* me_);

/**
 * Appends entry using the current term.
 * Note: we make the assumption that current term is up-to-date
 * @return 0 if unsuccessful */
int raft_append_entry(raft_server_t* me_, raft_entry_t* c);

void raft_set_last_applied_idx(raft_server_t* me, int idx);

void raft_set_state(raft_server_t* me_, int state);

int raft_get_state(raft_server_t* me_);

raft_node_t* raft_node_new(void* udata, int id);

void raft_node_free(raft_node_t* me_);

void raft_node_set_next_idx(raft_node_t* node, int nextIdx);

void raft_node_set_match_idx(raft_node_t* node, int matchIdx);

int raft_node_get_match_idx(raft_node_t* me_);

void raft_node_vote_for_me(raft_node_t* me_, const int vote);

int raft_node_has_vote_for_me(raft_node_t* me_);

void raft_node_set_has_sufficient_entries(raft_node_t* me_);

int raft_votes_is_majority(const int nnodes, const int nvotes);

/* DEPRECATED */
void raft_offer_log(raft_server_t* me_, raft_entry_t* ety, const int idx);

/* DEPRECATED */
void raft_pop_log(raft_server_t* me_, raft_entry_t* ety, const int idx);

void raft_offer_entries(raft_server_t* me_, raft_entry_t* ety, const int idx, int len);

void raft_pop_entries(raft_server_t* me_, raft_entry_t* ety, const int idx, int len);

int raft_get_num_snapshottable_entries(raft_server_t* me_);

int raft_node_is_active(raft_node_t* me_);

void raft_node_set_voting_committed(raft_node_t* me_, int voting);

int raft_node_set_addition_committed(raft_node_t* me_, int committed);

#endif /* RAFT_PRIVATE_H_ */
