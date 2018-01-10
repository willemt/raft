#include <gtest/gtest.h>

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern "C"
{
#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
#include "mock_send_functions.h"
}

static int __raft_persist_term(
    raft_server_t* raft,
    void *udata,
    int term,
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
    int idx
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

TEST(TestSnapshooting, leader_begin_snapshot_fails_if_no_logs_to_compact)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(2, raft_get_log_count(r));
    EXPECT_EQ(-1, raft_begin_snapshot(r));

    raft_set_commit_idx(r, 1);
    EXPECT_EQ(0, raft_begin_snapshot(r));
}

TEST(TestSnapshooting, leader_will_not_apply_entry_if_snapshot_is_in_progress)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

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
    EXPECT_EQ(2, raft_get_log_count(r));

    EXPECT_EQ(0, raft_begin_snapshot(r));
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
    raft_set_commit_idx(r, 2);
    EXPECT_EQ(-1, raft_apply_entry(r));
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
}

TEST(TestSnapshooting, leader_snapshot_end_fails_if_snapshot_not_in_progress)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));
    EXPECT_EQ(-1, raft_end_snapshot(r));
}

TEST(TestSnapshooting, leader_snapshot_begin_fails_if_less_than_2_logs_to_compact)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    raft_set_commit_idx(r, 1);
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(-1, raft_begin_snapshot(r));
}

TEST(TestSnapshooting, leader_snapshot_end_succeeds_if_log_compacted)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    EXPECT_EQ(0, raft_get_log_count(r));

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
    EXPECT_EQ(2, raft_get_log_count(r));
    EXPECT_EQ(1, raft_get_num_snapshottable_logs(r));

    EXPECT_EQ(0, raft_begin_snapshot(r));

    raft_entry_t* _ety;
    int i = raft_get_first_entry_idx(r);
    for (; i < raft_get_commit_idx(r); i++)
        EXPECT_EQ(0, raft_poll_entry(r, &_ety));

    EXPECT_EQ(0, raft_end_snapshot(r));
    EXPECT_EQ(0, raft_get_num_snapshottable_logs(r));
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(1, raft_get_commit_idx(r));
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
    EXPECT_EQ(0, raft_periodic(r, 1000));
}

TEST(TestSnapshooting, leader_snapshot_end_succeeds_if_log_compacted2)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    EXPECT_EQ(0, raft_get_log_count(r));

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
    EXPECT_EQ(3, raft_get_log_count(r));
    EXPECT_EQ(2, raft_get_num_snapshottable_logs(r));

    EXPECT_EQ(0, raft_begin_snapshot(r));

    raft_entry_t* _ety;
    int i = raft_get_first_entry_idx(r);
    for (; i <= raft_get_commit_idx(r); i++)
        EXPECT_EQ(0, raft_poll_entry(r, &_ety));

    EXPECT_EQ(0, raft_end_snapshot(r));
    EXPECT_EQ(0, raft_get_num_snapshottable_logs(r));
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(2, raft_get_commit_idx(r));
    EXPECT_EQ(2, raft_get_last_applied_idx(r));
    EXPECT_EQ(0, raft_periodic(r, 1000));
}

TEST(TestSnapshooting, joinee_needs_to_get_snapshot)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

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
    EXPECT_EQ(2, raft_get_log_count(r));
    EXPECT_EQ(1, raft_get_num_snapshottable_logs(r));

    EXPECT_EQ(0, raft_begin_snapshot(r));
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
    EXPECT_EQ(-1, raft_apply_entry(r));
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
}

TEST(TestSnapshooting, follower_load_from_snapshot)
{
    raft_cbs_t funcs = {0};

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    EXPECT_EQ(0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    EXPECT_EQ(0, raft_get_log_count(r));
    EXPECT_EQ(0, raft_begin_load_snapshot(r, 5, 5));
    EXPECT_EQ(0, raft_end_load_snapshot(r));
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(0, raft_get_num_snapshottable_logs(r));
    EXPECT_EQ(5, raft_get_commit_idx(r));
    EXPECT_EQ(5, raft_get_last_applied_idx(r));

    EXPECT_EQ(0, raft_periodic(r, 1000));

    /* current idx means snapshot was unnecessary */
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 7);
    EXPECT_EQ(-1, raft_begin_load_snapshot(r, 6, 5));
    EXPECT_EQ(7, raft_get_commit_idx(r));
}

TEST(TestSnapshooting, follower_load_from_snapshot_fails_if_already_loaded)
{
    raft_cbs_t funcs = { 0 };

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    EXPECT_EQ(0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    EXPECT_EQ(0, raft_get_log_count(r));
    EXPECT_EQ(0, raft_begin_load_snapshot(r, 5, 5));
    EXPECT_EQ(0, raft_end_load_snapshot(r));
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(0, raft_get_num_snapshottable_logs(r));
    EXPECT_EQ(5, raft_get_commit_idx(r));
    EXPECT_EQ(5, raft_get_last_applied_idx(r));

    EXPECT_EQ(RAFT_ERR_SNAPSHOT_ALREADY_LOADED, raft_begin_load_snapshot(r, 5, 5));
}

TEST(TestSnapshooting, follower_load_from_snapshot_does_not_break_cluster_safety)
{
    raft_cbs_t funcs = { 0 };

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    EXPECT_EQ(0, raft_get_log_count(r));

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

    EXPECT_EQ(-1, raft_begin_load_snapshot(r, 2, 2));
}

TEST(TestSnapshooting, follower_load_from_snapshot_fails_if_log_is_newer)
{
    raft_cbs_t funcs = { 0 };

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);
    EXPECT_EQ(0, raft_get_log_count(r));

    raft_set_last_applied_idx(r, 5);

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    EXPECT_EQ(-1, raft_begin_load_snapshot(r, 2, 2));
}

TEST(TestSnapshooting, leader_sends_appendentries_when_node_next_index_was_compacted)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries_capture;

    msg_appendentries_t ae;

    raft_server_t *r = raft_new();
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
    EXPECT_EQ(3, raft_get_current_idx(r));

    /* compact entry 1 & 2 */
    EXPECT_EQ(0, raft_begin_load_snapshot(r, 2, 3));
    EXPECT_EQ(0, raft_end_load_snapshot(r));
    EXPECT_EQ(3, raft_get_current_idx(r));

    /* node wants an entry that was compacted */
    raft_node_set_next_idx(node, raft_get_current_idx(r));

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    EXPECT_EQ(0, raft_send_appendentries(r, node));
    EXPECT_EQ(2, ae.term);
    EXPECT_EQ(2, ae.prev_log_idx);
    EXPECT_EQ(2, ae.prev_log_term);
}

TEST(TestSnapshooting, recv_entry_fails_if_snapshot_in_progress)
{
    raft_cbs_t funcs = { 0 };
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    ety.id = 2;
    raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(2, raft_get_log_count(r));

    raft_set_commit_idx(r, 1);
    EXPECT_EQ(0, raft_begin_snapshot(r));

    ety.id = 3;
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    EXPECT_EQ(RAFT_ERR_SNAPSHOT_IN_PROGRESS, raft_recv_entry(r, &ety, &cr));
}
