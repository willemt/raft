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

#include "raft_types.h"

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
    raft_term_t current_term;

    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    raft_node_id_t voted_for;

    /* the log which is replicated */
    void* log;

    /* Volatile state: */

    /* idx of highest log entry known to be committed */
    raft_index_t commit_idx;

    /* idx of highest log entry applied to state machine */
    raft_index_t last_applied_idx;

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

    /* the log which has a voting cfg change, otherwise -1 */
    raft_index_t voting_cfg_change_log_idx;

    /* Our membership with the cluster is confirmed (ie. configuration log was
     * committed) */
    int connected;

    int snapshot_in_progress;
    int snapshot_flags;

    /* Last compacted snapshot */
    raft_index_t snapshot_last_idx;
    raft_term_t snapshot_last_term;

    /* Previous index/term values stored during snapshot,
     * which are restored if the operation is cancelled.
     */
    raft_index_t saved_snapshot_last_idx;
    raft_term_t saved_snapshot_last_term;
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

void raft_set_last_applied_idx(raft_server_t* me, raft_index_t idx);

void raft_set_state(raft_server_t* me_, int state);

int raft_get_state(raft_server_t* me_);

raft_node_t* raft_node_new(void* udata, raft_node_id_t id);

void raft_node_free(raft_node_t* me_);

void raft_node_set_next_idx(raft_node_t* node, raft_index_t nextIdx);

void raft_node_set_match_idx(raft_node_t* node, raft_index_t matchIdx);

raft_index_t raft_node_get_match_idx(raft_node_t* me_);

void raft_node_vote_for_me(raft_node_t* me_, const int vote);

int raft_node_has_vote_for_me(raft_node_t* me_);

void raft_node_set_has_sufficient_logs(raft_node_t* me_);

int raft_votes_is_majority(const int nnodes, const int nvotes);

void raft_offer_log(raft_server_t* me_, raft_entry_t* ety, const raft_index_t idx);

void raft_pop_log(raft_server_t* me_, raft_entry_t* ety, const raft_index_t idx);

raft_index_t raft_get_num_snapshottable_logs(raft_server_t* me_);

int raft_node_is_active(raft_node_t* me_);

void raft_node_set_voting_committed(raft_node_t* me_, int voting);

void raft_node_set_addition_committed(raft_node_t* me_, int committed);

/* Heap functions */
extern void *(*__raft_malloc)(size_t size);
extern void *(*__raft_calloc)(size_t nmemb, size_t size);
extern void *(*__raft_realloc)(void *ptr, size_t size);
extern void (*__raft_free)(void *ptr);

#endif /* RAFT_PRIVATE_H_ */
