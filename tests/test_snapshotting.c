#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
#include "mock_send_functions.h"

static int __raft_persist_term(
    raft_server_t* raft,
    void *udata,
    raft_term_t term,
    int vote
    )
{
    return 0;
}

static int __raft_persist_vote(
    raft_server_t* raft,
    void *udata,
    int vote
    )
{
    return 0;
}

static int __raft_applylog(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t idx
    )
{
    return 0;
}

static int __raft_send_requestvote(raft_server_t* raft,
                            void* udata,
                            raft_node_t* node,
                            msg_requestvote_t* msg)
{
    return 0;
}

static int __raft_send_appendentries(raft_server_t* raft,
                              void* udata,
                              raft_node_t* node,
                              msg_appendentries_t* msg)
{
    return 0;
}

static int __raft_send_appendentries_capture(raft_server_t* raft,
                              void* udata,
                              raft_node_t* node,
                              msg_appendentries_t* msg)
{
    msg_appendentries_t* msg_captured = (msg_appendentries_t*)udata;
    memcpy(msg_captured, msg, sizeof(msg_appendentries_t));
    return 0;
}

static int __raft_send_snapshot_increment(raft_server_t* raft,
        void* udata,
        raft_node_t* node)
{
    int *counter = udata;

    (*counter)++;
    return 0;
}

/* static raft_cbs_t generic_funcs = { */
/*     .persist_term = __raft_persist_term, */
/*     .persist_vote = __raft_persist_vote, */
/* }; */

static int max_election_timeout(int election_timeout)
{
	return 2 * election_timeout;
}

// TODO: don't apply logs while snapshotting
// TODO: don't cause elections while snapshotting

void TestRaft_leader_begin_snapshot_fails_if_no_logs_to_compact(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    CuAssertIntEquals(tc, -1, raft_begin_snapshot(r, 0));

    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
}

void TestRaft_leader_will_not_apply_entry_if_snapshot_is_in_progress(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 1;
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
    raft_set_commit_idx(r, 2);
    CuAssertIntEquals(tc, -1, raft_apply_entry(r));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
}

void TestRaft_leader_snapshot_end_fails_if_snapshot_not_in_progress(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, -1, raft_end_snapshot(r));
}

void TestRaft_leader_snapshot_begin_fails_if_less_than_2_logs_to_compact(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));
    CuAssertIntEquals(tc, -1, raft_begin_snapshot(r, 0));
}

void TestRaft_leader_snapshot_end_succeeds_if_log_compacted(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    CuAssertIntEquals(tc, 1, raft_get_num_snapshottable_logs(r));

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));

    raft_entry_t* _ety;
    int i = raft_get_first_entry_idx(r);
    for (; i < raft_get_commit_idx(r); i++)
        CuAssertIntEquals(tc, 0, raft_poll_entry(r, &_ety));

    CuAssertIntEquals(tc, 0, raft_end_snapshot(r));
    CuAssertIntEquals(tc, 0, raft_get_num_snapshottable_logs(r));
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
    CuAssertIntEquals(tc, 0, raft_periodic(r, 1000));
}

void TestRaft_leader_snapshot_end_succeeds_if_log_compacted2(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    ety.id = 3;
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 2);
    CuAssertIntEquals(tc, 3, raft_get_log_count(r));
    CuAssertIntEquals(tc, 2, raft_get_num_snapshottable_logs(r));

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));

    raft_entry_t* _ety;
    int i = raft_get_first_entry_idx(r);
    for (; i <= raft_get_commit_idx(r); i++)
        CuAssertIntEquals(tc, 0, raft_poll_entry(r, &_ety));

    CuAssertIntEquals(tc, 0, raft_end_snapshot(r));
    CuAssertIntEquals(tc, 0, raft_get_num_snapshottable_logs(r));
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
    CuAssertIntEquals(tc, 2, raft_get_last_applied_idx(r));
    CuAssertIntEquals(tc, 0, raft_periodic(r, 1000));
}

void TestRaft_joinee_needs_to_get_snapshot(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    CuAssertIntEquals(tc, 1, raft_get_num_snapshottable_logs(r));

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
    CuAssertIntEquals(tc, -1, raft_apply_entry(r));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
}

void TestRaft_follower_load_from_snapshot(CuTest * tc)
{
    raft_cbs_t funcs = {
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 5, 5));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, 0, raft_get_num_snapshottable_logs(r));
    CuAssertIntEquals(tc, 5, raft_get_commit_idx(r));
    CuAssertIntEquals(tc, 5, raft_get_last_applied_idx(r));

    CuAssertIntEquals(tc, 0, raft_periodic(r, 1000));

    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 7);
    /* last_included_index conflicts with committed entry */
    CuAssertIntEquals(tc, RAFT_ERR_SHUTDOWN, raft_begin_load_snapshot(r, 6, 5));
    CuAssertIntEquals(tc, 7, raft_get_commit_idx(r));
}

void TestRaft_follower_load_from_snapshot_fails_if_term_is_0(CuTest * tc)
{
    raft_cbs_t funcs = {
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, -1, raft_begin_load_snapshot(r, 0, 5));
}

void TestRaft_follower_load_from_snapshot_fails_if_already_loaded(CuTest * tc)
{
    raft_cbs_t funcs = {
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 5, 5));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertIntEquals(tc, 0, raft_get_num_snapshottable_logs(r));
    CuAssertIntEquals(tc, 5, raft_get_commit_idx(r));
    CuAssertIntEquals(tc, 5, raft_get_last_applied_idx(r));

    CuAssertIntEquals(tc, RAFT_ERR_SNAPSHOT_ALREADY_LOADED, raft_begin_load_snapshot(r, 5, 5));
}

void TestRaft_follower_load_from_snapshot_does_not_break_cluster_safety(CuTest * tc)
{
    raft_cbs_t funcs = {
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");
    raft_append_entry(r, &ety);

    ety.id = 2;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");
    raft_append_entry(r, &ety);

    ety.id = 3;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");
    raft_append_entry(r, &ety);

    /* log mismatch */
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 2));
}

void TestRaft_follower_load_from_snapshot_fails_if_log_is_newer(CuTest * tc)
{
    raft_cbs_t funcs = {
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    raft_set_last_applied_idx(r, 5);

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    CuAssertIntEquals(tc, -1, raft_begin_load_snapshot(r, 2, 2));
}

void TestRaft_leader_sends_snapshot_when_node_next_index_was_compacted(CuTest* tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries_capture,
    };

    msg_appendentries_t ae;

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, &ae);

    raft_node_t* node;
    raft_add_node(r, NULL, 1, 1);
    node = raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* entry 1 */
    char *str = "aaa";
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    /* entry 2 */
    ety.term = 1;
    ety.id = 2;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    /* entry 3 */
    ety.term = 1;
    ety.id = 3;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 3, raft_get_current_idx(r));

    /* compact entry 1 & 2 */
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 3));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));
    CuAssertIntEquals(tc, 3, raft_get_current_idx(r));

    /* node wants an entry that was compacted */
    raft_node_set_next_idx(node, raft_get_current_idx(r));

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    CuAssertIntEquals(tc, RAFT_ERR_NEEDS_SNAPSHOT, raft_send_appendentries(r, node));
    // CuAssertIntEquals(tc, 2, ae.term);
    // CuAssertIntEquals(tc, 3, ae.prev_log_idx);
    // CuAssertIntEquals(tc, 2, ae.prev_log_term);
}

void TestRaft_recv_entry_fails_if_snapshot_in_progress(CuTest* tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));

    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));

    ety.id = 3;
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    CuAssertIntEquals(tc, RAFT_ERR_SNAPSHOT_IN_PROGRESS, raft_recv_entry(r, &ety, &cr));
}

void TestRaft_recv_entry_succeeds_if_snapshot_nonblocking_apply_is_set(CuTest* tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));

    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, RAFT_SNAPSHOT_NONBLOCKING_APPLY));

    ety.id = 3;
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));
}


void TestRaft_follower_recv_appendentries_is_successful_when_previous_log_idx_equals_snapshot_last_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 2));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 3;
    ae.prev_log_idx = 2;
    ae.prev_log_term = 2;
    /* include entries */
    msg_entry_t e[5];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 3;
    e[0].id = 3;
    ae.entries = e;
    ae.n_entries = 1;
    CuAssertIntEquals(tc, 0, raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer));
    CuAssertIntEquals(tc, 1, aer.success);
}

void TestRaft_leader_not_send_appendentries_when_snapshotted(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    CuAssertTrue(tc, NULL != raft_add_node(r, NULL, 1, 1));
    CuAssertTrue(tc, NULL != raft_add_node(r, NULL, 2, 0));

    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 4));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    CuAssertTrue(tc, NULL != raft_add_node(r, NULL, 2, 0));
    raft_node_t* p = raft_get_node_from_idx(r, 1);
    CuAssertTrue(tc, NULL != p);
    raft_node_set_next_idx(p, 4);

    /* receive appendentries messages */
    raft_send_appendentries(r, p);
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL == ae);
    // CuAssertIntEquals(tc, 2, ae->prev_log_term);
    // CuAssertIntEquals(tc, 4, ae->prev_log_idx);
}

void TestRaft_cancel_snapshot_restores_state(CuTest* tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    /* single entry */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");
    raft_recv_entry(r, &ety, &cr);

    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 2);

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 0, raft_end_snapshot(r));

    /* more entries  */
    ety.id = 3;
    raft_recv_entry(r, &ety, &cr);
    ety.id = 4;
    raft_recv_entry(r, &ety, &cr);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    CuAssertIntEquals(tc, 2, raft_get_snapshot_last_idx(r));

    /* begin and cancel another snapshot */
    raft_set_commit_idx(r, 4);
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 1, raft_snapshot_is_in_progress(r));
    CuAssertIntEquals(tc, 0, raft_cancel_snapshot(r));

    /* snapshot no longer in progress, index must not have changed */
    CuAssertIntEquals(tc, 0, raft_snapshot_is_in_progress(r));
    CuAssertIntEquals(tc, 2, raft_get_snapshot_last_idx(r));
}

void TestRaft_leader_sends_snapshot_if_log_was_compacted(CuTest* tc)
{
    raft_cbs_t funcs = {
        .send_snapshot = __raft_send_snapshot_increment,
        .send_appendentries = __raft_send_appendentries
    };

    int send_snapshot_count = 0;

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, &send_snapshot_count);

    raft_node_t* node;
    raft_add_node(r, NULL, 1, 1);
    node = raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    /* entry 1 */
    char *str = "aaa";
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    /* entry 2 */
    ety.term = 1;
    ety.id = 2;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    /* entry 3 */
    ety.term = 1;
    ety.id = 3;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 3, raft_get_current_idx(r));

    /* compact entry 1 & 2 */
    raft_set_commit_idx(r, 2);
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 0, raft_end_snapshot(r));
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));

    /* at this point a snapshot should have been sent; we'll continue
     * assuming the node was not available to get it.
     */
    CuAssertIntEquals(tc, 2, send_snapshot_count);
    send_snapshot_count = 0;

    /* node wants an entry that was compacted */
    raft_node_set_match_idx(node, 1);
    raft_node_set_next_idx(node, 2);

    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 0;
    aer.current_idx = 1;
    aer.first_idx = 4;

    raft_recv_appendentries_response(r, node, &aer);
    CuAssertIntEquals(tc, 0, send_snapshot_count);

    /* It is ok to send snapshot when request timeout */
    raft_periodic(r, ((raft_server_private_t*)r)->request_timeout);
    CuAssertIntEquals(tc, 2, send_snapshot_count);
}
