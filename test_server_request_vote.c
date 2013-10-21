
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

/* If term > currentTerm, set currentTerm to term (step down if candidate or leader) */
//void TestRaft_when_recv_requestvote_step_down_if_term_is_greater(CuTest * tc)
void TestRaft_votes_are_majority_is_true(
    CuTest * tc
)
{
    /* 1 of 3 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(3,1));

    /* 2 of 3 = win */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(3,2));

    /* 2 of 5 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(5,2));

    /* 3 of 5 = win */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(5,3));

    /* 2 of 1?? This is an error */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(1,2));
}

void TestRaft_server_increase_votes_for_me_when_receive_request_vote_response(
    CuTest * tc
)
{
    void *r;
    msg_requestvote_t rv;
    msg_requestvote_response_t *rvr;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    memset(&rv,0,sizeof(msg_requestvote_response_t));
    rv.term = 1;

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_current_term(r,1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    raft_recv_requestvote(r,1,&rv);
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
    raft_vote(r,1);
    CuAssertTrue(tc, 2 == raft_get_nvotes_for_me(r));
}


// Reply false if term < currentTerm (§5.1)
void TestRaft_server_recv_requestvote_reply_false_if_term_less_than_current_term(
    CuTest * tc
)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_requestvote_t rv;
    msg_requestvote_response_t *rvr;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    memset(&rv,0,sizeof(msg_requestvote_t));
    rv.term = 2;

    sender = sender_new();

    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_current_term(r,1);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_requestvote(r,1,&rv);
    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rvr);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}

// If votedFor is null or candidateId, and candidate's log is at
// least as up-to-date as local log, grant vote (§5.2, §5.4)
void TestRaft_server_dont_grant_vote_if_we_didnt_vote_for_this_candidate(
    CuTest * tc
)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_requestvote_t rv;
    msg_requestvote_response_t *rvr;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    memset(&rv,0,sizeof(msg_requestvote_response_t));
    rv.term = 1;

    sender = sender_new();

    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,1);
    raft_vote(r,1);
    raft_recv_requestvote(r,1,&rv);
    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rvr);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}



