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
void TestRaft_follower_recv_appendentries_reply_false_if_term_less_than_currentterm(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    sender = sender_new();
    r = raft_new();
    peer = raft_add_peer(r,(void*)1);

    /* term is low */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;

    /*  higher current term */
    raft_set_current_term(r,5);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,peer,&ae);

    /*  response is false */
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

void TestRaft_follower_recv_appendentries_updates_currentterm_if_term_gt_currentterm(CuTest * tc)
{
    void *r, *peer;
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    /*  newer term */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);

    /*  older currentterm */
    raft_set_current_term(r,1);

    /*  appendentry has newer term, so we change our currentterm */
    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));

}

/*  5.3 */
void TestRaft_follower_recv_appendentries_reply_false_if_doesnt_have_log_at_prev_log_index(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    sender = sender_new();
    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_external_functions(r,&funcs,sender);

    /* log index that server doesn't have */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 5;
    ae.prev_log_index = 5;

    /* current term is old */
    raft_set_current_term(r,5);

    /* trigger reply */
    raft_recv_appendentries(r,peer,&ae);
    aer = sender_poll_msg(sender);

    /* reply is false */
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

/* 5.3 */
void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries(CuTest * tc)
{
    void *r, *peer;
    msg_appendentries_t ae;

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);

    raft_set_current_term(r,2);

    /* first append entry */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_index = 0;
    ae.prev_log_term = 0;

    /* increase log size */
    raft_append_command(r, "111", 3);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* pass a appendentry that is newer  */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));
}

void TestRaft_follower_recv_appendentries_add_new_entries_not_already_in_log(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    /* appendentries has multiple entries */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;

    sender = sender_new();

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);

    raft_set_current_term(r,1);
    raft_set_external_functions(r,&funcs,sender);
    raft_recv_appendentries(r,peer,&ae);

    CuAssertTrue(tc, 0);
//    msg = sender_poll_msg(sender);
//    CuAssertTrue(tc, aer);
//    CuAssertTrue(tc, 1 == sender_msg_is_appendentries(msg));
//    CuAssertTrue(tc, 1 == sender_msg_is_false(msg));
}

//If leaderCommit > commitIndex, set commitIndex =
//min(leaderCommit, last log index)
void TestRaft_follower_recv_appendentries_set_commitindex_to_prevLogIdx(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    sender = sender_new();
    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_external_functions(r,&funcs,sender);

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_index = 4;
    ae.leader_commit = 5;

    /* receipt of appendentries changes commit index */
    raft_recv_appendentries(r,peer,&ae);
    /* set to 4 because prevLogIdx is lower */
    CuAssertTrue(tc, 4 == raft_get_commit_index(r));
}

void TestRaft_follower_recv_appendentries_set_commitindex_to_LeaderCommit(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;

    sender = sender_new();
    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_set_external_functions(r,&funcs,sender);

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_index = 4;
    ae.leader_commit = 3;

    /* receipt of appendentries changes commit index */
    raft_recv_appendentries(r,peer,&ae);
    /* set to 3 because leaderCommit is lower */
    CuAssertTrue(tc, 3 == raft_get_commit_index(r));
}

void TestRaft_follower_increases_log_after_appendentry(CuTest * tc)
{
    void *r, *peer;

    msg_appendentries_t ae;

    /* appendentry */
    memset(&ae,0,sizeof(msg_appendentries_t));

    r = raft_new();

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_state(r,RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

void TestRaft_follower_rejects_appendentries_if_idx_and_term_dont_match_preceding_ones(CuTest * tc)
{
    void *r, *peer;

    msg_appendentries_t ae;

    r = raft_new();

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_current_term(r,1);

    /* first append entry */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_index = 0;
    ae.prev_log_term = 0;

    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

#if 0
void T_estRaft_follower_resends_command_if_request_from_leader_timesout(CuTest * tc)
{
    void *r, *peer;

    msg_appendentries_t ae;

    /* appendentry */
    memset(&ae,0,sizeof(msg_appendentries_t));

    r = raft_new();

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_state(r,RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}
#endif

void TestRaft_follower_becomes_candidate_when_election_timeout_occurs(CuTest * tc)
{
    void *r, *peer;

    r = raft_new();

    /*  1 second election timeout */
    raft_set_election_timeout(r, 1000);

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /*  1.001 seconds have passed */
    raft_periodic(r, 1001);

    /* is a candidate now */
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_dont_grant_vote_if_candidate_has_a_less_complete_log(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_requestvote_t rv;
    msg_requestvote_response_t *rvr;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    /*  request vote */
    /*  vote indicates candidate's log is not complete compared to follower */
    memset(&rv,0,sizeof(msg_requestvote_t));
    rv.term = 1;
    rv.candidate_id = 0;
    rv.last_log_index = 1;
    rv.last_log_term = 1;

    /* server's term and index are more up-to-date */
    raft_set_current_term(r,1);
    raft_set_current_index(r,2);

    /* vote not granted */
    raft_recv_requestvote(r,peer,&rv);
    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rvr);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}

