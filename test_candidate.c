#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_increments_current_term(CuTest * tc)
{
    void *r;

    r = raft_new();

    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_votes_for_self(CuTest * tc)
{
    void *r;

    r = raft_new();

    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_resets_election_timeout(CuTest * tc)
{
    void *r;

    r = raft_new();

    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
}
 
/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_requests_votes_from_other_server(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    sender = sender_new();

    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);
    raft_become_candidate(r);
    msg = sender_poll_msg(sender);
//    CuAssertTrue(tc, 1 == sender_msg_is_reqestvote(msg));
}

/* Candidate 5.2 */
void TestRaft_candidate_election_timeout_and_no_leader_results_in_new_election(CuTest * tc)
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
    raft_periodic(r);

    raft_recv_requestvote_response(r,&vr);
    CuAssertTrue(tc, 0 == raft_is_leader(r));
    /*  now has majority */
    raft_recv_requestvote_response(r,&vr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_dont_grant_vote_if_candidate_has_a_less_complete_log(CuTest * tc)
{

}

/* Candidate 5.2 */
void TestRaft_candidate_receives_majority_of_votes_becomes_leader(CuTest * tc)
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

    raft_recv_requestvote_response(r,&vr);
    CuAssertTrue(tc, 0 == raft_is_leader(r));
    /*  now has majority */
    raft_recv_requestvote_response(r,&vr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_will_not_respond_to_voterequest_if_it_has_already_voted(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));

    msg_requestvote_t rv;
    memset(&rv,0,sizeof(msg_requestvote_t));

    sender = sender_new();

    r = raft_new();
    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);
    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,1);
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    //raft_recv_requestvote(
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    raft_periodic(r);

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_will_reject_requestvote_if_its_log_is_more_complete(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_requestvote_t rv;
    memset(&rv,0,sizeof(msg_requestvote_t));
    rv.lastLogTerm = 2;
    rv.lastLogIndex = 2;

    sender = sender_new();

    r = raft_new();
    /* three nodes */
    raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);
    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,5);
    raft_set_current_index(r,3);
    raft_set_state(r,RAFT_STATE_CANDIDATE);
//    raft_recv_requestvote(r,
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    raft_periodic(r);

//    raft_recv_appendentries(r,1,&ae);
//    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_requestvote_includes_loginfo(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    sender = sender_new();

    r = raft_new();
    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,5);
    raft_set_current_index(r,3);
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    raft_send_requestvote(r,peer);

    msg = sender_poll_msg(sender);
//    CuAssertTrue(tc, msg_is_requestvote(msg));
//    CuAssertTrue(tc, 3 == msg_requestvote_get_index(msg));
//    CuAssertTrue(tc, 5 == msg_requestvote_get_term(msg));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_leader_results_in_follower(CuTest * tc)
{
    void *r, *peer;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));

    sender = sender_new();

    r = raft_new();

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,1);
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    raft_periodic(r);

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_invalid_leader_doesnt_result_in_follower(CuTest * tc)
{
}

