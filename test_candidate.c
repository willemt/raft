#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

void TestRaft_candidate_becomes_candidate_is_candidate(CuTest * tc)
{
    void *r;

    r = raft_new();

    raft_become_candidate(r);
    CuAssertTrue(tc, raft_is_candidate(r));
}

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

    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 0 == raft_get_voted_for(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_resets_election_timeout(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 100);
    CuAssertTrue(tc, 100 == raft_get_timeout_elapsed(r));

    raft_become_candidate(r);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}
 
/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_requests_votes_from_other_servers(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    raft_peer_configuration_t cfg[] = {
            /* 2 peers */
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};
    msg_requestvote_t* rv;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* set term so we can check it gets included in the outbound message */
    raft_set_current_term(r,2);
    raft_set_current_index(r,5);

    /* becoming candidate triggers vote requests */
    raft_become_candidate(r);

    /* 2 peers = 2 vote requests */
    rv = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->term);
    /*  TODO: there should be more items */
    rv = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->term);
}

/* Candidate 5.2 */
void TestRaft_candidate_election_timeout_and_no_leader_results_in_new_election(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    msg_requestvote_response_t vr;

    memset(&vr,0,sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.vote_granted = 1;

    sender = sender_new();

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_external_functions(r,&funcs,sender);
    raft_set_election_timeout(r,1000);

    /* server wants to be leader, so becomes candidate */
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r,1001);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));

    /*  receiving this vote gives the server majority */
//    raft_recv_requestvote_response(r,1,&vr);
//    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_receives_majority_of_votes_becomes_leader(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),(void*)3},
                {(-1),(void*)4},
                {(-1),(void*)5},
                {(-1),NULL}};

    msg_requestvote_response_t vr;

    sender = sender_new();

    r = raft_new();
    raft_set_configuration(r,cfg);
    CuAssertTrue(tc, 5 == raft_get_npeers(r));
    raft_set_external_functions(r,&funcs,sender);
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    /* a vote for us */
    memset(&vr,0,sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.vote_granted = 1;

    /* get one vote */
    raft_recv_requestvote_response(r,1,&vr);
    CuAssertTrue(tc, 2 == raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 0 == raft_is_leader(r));

    /* get another vote
     * now has majority (ie. 3/5 votes) */
    raft_recv_requestvote_response(r,2,&vr);
    CuAssertTrue(tc, 3 == raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_will_not_respond_to_voterequest_if_it_has_already_voted(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    msg_requestvote_response_t* rvr;
    msg_requestvote_t rv;

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_external_functions(r,&funcs,sender);

    raft_vote(r,1);

    memset(&rv,0,sizeof(msg_requestvote_t));
    raft_recv_requestvote(r,1,&rv);

    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}

/* Candidate 5.2 */
void TestRaft_candidate_requestvote_includes_logIndex(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_requestvote_t* rv;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_state(r,RAFT_STATE_CANDIDATE);

    raft_set_external_functions(r,&funcs,sender);
    raft_set_current_term(r,5);
    raft_set_current_index(r,3);
    raft_send_requestvote(r,1);

    rv = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->last_log_index);
    CuAssertTrue(tc, 5 == rv->term);
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_leader_results_in_follower(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));

    sender = sender_new();

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_external_functions(r,&funcs,sender);

    raft_set_state(r,RAFT_STATE_CANDIDATE);
    CuAssertTrue(tc, 0 == raft_is_follower(r));

    raft_recv_appendentries(r,1,&ae);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_invalid_leader_doesnt_result_in_follower(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    msg_appendentries_t ae;

    sender = sender_new();
    r = raft_new();
    raft_set_external_functions(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* server's log is newer */
    raft_set_current_term(r,1);
    raft_set_current_index(r,2);

    /*  is a candidate */
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    CuAssertTrue(tc, 0 == raft_is_follower(r));

    /*  invalid leader determined by "leaders" old log */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_index = 1;
    ae.prev_log_term = 1;

    /* appendentry from invalid leader doesn't make candidate become follower */
    raft_recv_appendentries(r,1,&ae);
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

