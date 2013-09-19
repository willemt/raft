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
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_requestvote_response_t vr;

    memset(&vr,0,sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.voteGranted = 1;

    sender = sender_new();

    r = raft_new();
    /* three nodes */
    raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);
    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,1);
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    raft_become_leader(r);
}

/* 5.2 */
void TestRaft_leader_responds_to_command_msg_when_command_is_committed(CuTest * tc)
{

}

/* 5.3 */
void TestRaft_leader_sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx(CuTest * tc)
{

}

/* 5.3 */
void TestRaft_leader_retries_appendentries_with_decremented_NextIdx_log_inconsistency(CuTest * tc)
{

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
    CuAssertTrue(tc, 0 == raft_get_log_size(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_size(r));
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
    CuAssertTrue(tc, 0 == raft_get_log_size(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_size(r));

    raft_recv_command(r,peer,&cmd);
    CuAssertTrue(tc, 1 == raft_get_log_size(r));
}

void TestRaft_leader_increase_commitno_when_majority_have_entry_and_atleast_one_newer_entry(CuTest * tc)
{

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

