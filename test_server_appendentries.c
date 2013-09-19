#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

/* 5.1 */
void TestRaft_server_recv_appendentries_reply_false_when_term_less_than_currentterm(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;

    sender = sender_new();
    r = raft_new();

    /*  higher current term */
    raft_set_current_term(r,5);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,1,&ae);

    /*  response is false */
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

//Reply false if log doesn’t contain an entry at prevLogIndex
//whose term matches prevLogTerm
// TODO
/*  5.3 */
void TestRaft_server_recv_appendentries_reply_false_if_log_does_not_contain_entry_at_prevLogIndex(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 5;
    ae.prevLogIndex = 5;

    sender = sender_new();

    r = raft_new();
    raft_set_current_term(r,5);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,1,&ae);
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

/* 5.3 */
void TestRaft_server_recv_appendentries_delete_entries_if_conflict_with_new_entries(CuTest * tc)
{
    void *r;
    msg_appendentries_t ae;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    r = raft_new();
    raft_set_current_term(r,1);
    raft_recv_appendentries(r,1,&ae);
}

void TestRaft_server_recv_appendentries_add_new_entries_not_already_in_log(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    sender = sender_new();

    r = raft_new();
    raft_set_current_term(r,1);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,1,&ae);

//    msg = sender_poll_msg(sender);
//    CuAssertTrue(tc, aer);
//    CuAssertTrue(tc, 1 == sender_msg_is_appendentries(msg));
//    CuAssertTrue(tc, 1 == sender_msg_is_false(msg));
}

//If leaderCommit > commitIndex, set commitIndex =
//min(leaderCommit, last log index)
void TestRaft_server_recv_appendentries_set_commitindex(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    sender = sender_new();

    r = raft_new();
    raft_set_current_term(r,1);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,1,&ae);

//    msg = sender_poll_msg(sender);
//    CuAssertTrue(tc, aer);
//    CuAssertTrue(tc, 1 == sender_msg_is_appendentries(msg));
//    CuAssertTrue(tc, 1 == sender_msg_is_false(msg));
}

