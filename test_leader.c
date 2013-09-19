#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

/* 5.2 */
void TestRaft_leader_when_it_becomes_a_leader_sends_empty_appendentries(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);

    /* three nodes */
    raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /* candidate to leader */
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both peers */
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.2 */
void TestRaft_leader_responds_to_command_msg_when_command_is_committed(CuTest * tc)
{
    void *r, *peer, *sender;
    msg_command_response_t *cr;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /* I am the leader */
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* command message */
    msg_command_t cmd;
    cmd.id = 1;
    cmd.data = "command";
    cmd.len = strlen("command");

    /* receive command */
    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* trigger response through commit */
    raft_commit_command(r, 1);

    /* leader sent response to command message */
    cr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != cr);
}

/* 5.3 */
void TestRaft_leader_sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /* i'm leader */
    raft_set_state(r,RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r,peer);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.3 */
void TestRaft_leader_retries_appendentries_with_decremented_NextIdx_log_inconsistency(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /* i'm leader */
    raft_set_state(r,RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r,peer);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/*
If there exists an N such that N > commitIndex, a majority
of matchIndex[i] = N, and log[N].term == currentTerm:
set commitIndex = N (§5.2, §5.4).
*/

void TestRaft_leader_append_command_to_log_increases_idxno(CuTest * tc)
{
    void *r, *peer;

    msg_command_t cmd;
    cmd.id = 1;
    cmd.data = "command";
    cmd.len = strlen("command");

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

void TestRaft_leader_doesnt_append_command_if_unique_id_is_duplicate(CuTest * tc)
{
    void *r, *peer;

    msg_command_t cmd;
    cmd.id = 1;
    cmd.data = "command";
    cmd.len = strlen("command");

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

void TestRaft_leader_increase_commitno_when_majority_have_entry_and_atleast_one_newer_entry(CuTest * tc)
{
    void *p1, *p2;
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_response_t aer;

    sender = sender_new();
    r = raft_new();

    /* 3 nodes */
    p1 = raft_add_peer(r,(void*)1);
    p2 = raft_add_peer(r,(void*)2);

    /* I'm the leader */
    raft_set_state(r,RAFT_STATE_LEADER);

    /* the commit index will became 5 */
    raft_set_current_term(r,5);
    raft_set_current_index(r,5);
    raft_set_commit_index(r,4);
    raft_set_external_functions(r,&funcs,sender);

    raft_send_appendentries(r, p1);
    raft_send_appendentries(r, p2);

    /* respond that we have the appendentries */
    memset(&aer,0,sizeof(msg_appendentries_response_t));
    aer.term = 5;
    aer.success = 1;

    /* announce to leader that the majority have appended this log */
    raft_recv_appendentries_response(r,p1,&aer);
    raft_recv_appendentries_response(r,p2,&aer);
    CuAssertTrue(tc, 5 == raft_get_commit_index(r));
}

void TestRaft_leader_steps_down_if_received_appendentries_is_newer_than_itself(CuTest * tc)
{
    void* peer;
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 5;
    ae.prevLogIndex = 6;
    ae.prevLogTerm = 5;

    sender = sender_new();
    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_state(r,RAFT_STATE_LEADER);
    raft_set_current_term(r,5);
    raft_set_current_index(r,5);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,peer,&ae);

    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

