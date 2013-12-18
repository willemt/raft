
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

#if 0
void T_estRaft_server_voted_for_records_who_we_voted_for(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_vote(r,2);
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
}
#endif

void TestRaft_server_idx_starts_at_1(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_set_currentterm_sets_term(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_set_current_term(r,5);
    CuAssertTrue(tc, 5 == raft_get_current_term(r));
}

void TestRaft_election_start_increments_term(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_set_current_term(r,1);
    raft_election_start(r);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
}

#if 0
void T_estRaft_add_peer(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_num_peers(r));
    raft_add_peer(r,(void*)1);
    CuAssertTrue(tc, 1 == raft_get_num_peers(r));
}

void T_estRaft_dont_add_duplicate_peers(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_num_peers(r));
    raft_add_peer(r,(void*)1);
    CuAssertTrue(tc, NULL == raft_add_peer(r,(void*)1));
    CuAssertTrue(tc, 1 == raft_get_num_peers(r));
}

void T_estRaft_remove_peer(CuTest * tc)
{
    void *r;
    void *peer;

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    CuAssertTrue(tc, 1 == raft_get_num_peers(r));
    raft_remove_peer(r,peer);
    CuAssertTrue(tc, 0 == raft_get_num_peers(r));
}
#endif

void TestRaft_set_state(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, RAFT_STATE_LEADER == raft_get_state(r));
}

void TestRaft_server_starts_as_follower(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, RAFT_STATE_FOLLOWER == raft_get_state(r));
}

void TestRaft_server_starts_with_election_timeout_of_1000ms(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 1000 == raft_get_election_timeout(r));
}

void TestRaft_server_starts_with_request_timeout_of_500ms(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 500 == raft_get_request_timeout(r));
}

void TestRaft_server_entry_append_cant_append_if_id_is_zero(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data = str;
    ety.len = 3;
    ety.id = 0;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r,&ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_entry_append_increases_logidx(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data = str;
    ety.len = 3;
    ety.id = 1;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r,&ety);
    CuAssertTrue(tc, 2 == raft_get_current_idx(r));
}

void TestRaft_server_append_entry_means_entry_gets_current_term(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data = str;
    ety.len = 3;
    ety.id = 1;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r,&ety);
    CuAssertTrue(tc, 2 == raft_get_current_idx(r));
}

#if 0
/* TODO: no support for duplicate detection yet */
void T_estRaft_server_append_entry_not_sucessful_if_entry_with_id_already_appended(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data = str;
    ety.len = 3;
    ety.id = 1;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r,&ety);
    raft_append_entry(r,&ety);
    CuAssertTrue(tc, 2 == raft_get_current_idx(r));

    /* different ID so we can be successful */
    ety.id = 2;
    raft_append_entry(r,&ety);
    CuAssertTrue(tc, 3 == raft_get_current_idx(r));
}
#endif

void TestRaft_server_entry_is_retrieveable_using_idx(CuTest* tc)
{
    void *r;
    raft_entry_t e1;
    raft_entry_t e2;
    raft_entry_t *ety_appended;
    char *str = "aaa";
    char *str2 = "bbb";

    r = raft_new();

    e1.term = 1;
    e1.id = 1;
    e1.data = str;
    e1.len = 3;
    raft_append_entry(r,&e1);

    /* different ID so we can be successful */
    e2.term = 1;
    e2.id = 2;
    e2.data = str2;
    e2.len = 3;
    raft_append_entry(r,&e2);

    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r,2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data,str2,3));
}

void TestRaft_server_wont_apply_entry_if_we_dont_have_entry_to_apply(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    raft_entry_t *ety_appended;
    char *str = "aaa";

    r = raft_new();
    raft_set_commit_idx(r,0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    ety.term = 1;
    ety.id = 1;
    ety.data = str;
    ety.len = 3;
    raft_append_entry(r,&ety);
    raft_apply_entry(r);
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
}

// If commitidx > lastApplied: increment lastApplied, apply
// log[lastApplied] to state machine (§5.3)
void TestRaft_server_increment_lastApplied_when_lastApplied_lt_commitidx(CuTest* tc)
{
    void *r;
    raft_entry_t ety;

    r = raft_new();
    /* must be follower */
    raft_set_state(r,RAFT_STATE_FOLLOWER);
    raft_set_commit_idx(r,1);
    raft_set_last_applied_idx(r, 0);

    /* need at least one entry */
    ety.term = 1;
    ety.id = 1;
    ety.data = "aaa";
    ety.len = 3;
    raft_append_entry(r,&ety);

    /* let time lapse */
    raft_periodic(r,1);
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
}

void TestRaft_server_apply_entry_increments_last_applied_idx(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    raft_entry_t *ety_appended;
    char *str = "aaa";

    ety.term = 1;

    r = raft_new();
    raft_set_commit_idx(r,1);
    raft_set_last_applied_idx(r, 0);

    ety.id = 1;
    ety.data = str;
    ety.len = 3;
    raft_append_entry(r,&ety);
    raft_apply_entry(r);
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
}

#if 0
/* Not incorrect assertion.
 * The leader will always have its commit idx = last applied idx */
void T_estRaft_server_apply_entry_wont_apply_if_lastapplied_equalto_commit_index(CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    raft_entry_t *ety_appended;
    char *str = "aaa";

    ety.term = 1;

    r = raft_new();
    raft_set_commit_idx(r,5);
    raft_set_last_applied_idx(r, 5);

    ety.id = 1;
    ety.data = str;
    ety.len = 3;
    raft_append_entry(r,&ety);
    raft_apply_entry(r);
    CuAssertTrue(tc, 5 == raft_get_last_applied_idx(r));
}
#endif

void TestRaft_server_periodic_elapses_election_timeout(CuTest * tc)
{
    void *r;

    r = raft_new();
    /* we don't want to set the timeout to zero */
    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r,0);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r,100);
    CuAssertTrue(tc, 100 == raft_get_timeout_elapsed(r));
}

void TestRaft_server_election_timeout_sets_to_zero_when_elapsed_time_greater_than_timeout(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_set_election_timeout(r, 1000);

    /* greater than 1000 */
    raft_periodic(r,2000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}

void TestRaft_server_cfg_sets_npeers(CuTest * tc)
{
    void *r;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);

    CuAssertTrue(tc, 2 == raft_get_npeers(r));
}

void TestRaft_server_cant_get_peer_we_dont_have(CuTest * tc)
{
    void *r;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);

    CuAssertTrue(tc, NULL != raft_get_peer(r,0));
    CuAssertTrue(tc, NULL != raft_get_peer(r,1));
    CuAssertTrue(tc, NULL == raft_get_peer(r,2));
}

/* If term > currentTerm, set currentTerm to term (step down if candidate or leader) */
//void TestRaft_when_recv_requestvote_step_down_if_term_is_greater(CuTest * tc)
void TestRaft_votes_are_majority_is_true(
    CuTest * tc
)
{
    /* 1 of 3 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(3,1));

    /* 2 of 3 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(3,2));

    /* 2 of 5 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(5,2));

    /* 3 of 5 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(5,3));

    /* 2 of 1?? This is an error */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(1,2));
}

void TestRaft_server_dont_increase_votes_for_me_when_receive_request_vote_response_is_not_granted(
    CuTest * tc
)
{
    void *r;
    msg_requestvote_response_t rvr;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_current_term(r,1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 0;
    raft_recv_requestvote_response(r,1,&rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_increase_votes_for_me_when_receive_request_vote_response(
    CuTest * tc
)
{
    void *r;
    msg_requestvote_response_t rvr;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_current_term(r,1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r,1,&rvr);
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
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

    r = raft_new();
    raft_set_configuration(r,cfg);
    sender = sender_new();
    raft_set_callbacks(r,&funcs,sender);

    raft_set_current_term(r,1);

    memset(&rv,0,sizeof(msg_requestvote_t));
    rv.term = 2;
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

    raft_set_callbacks(r,&funcs,sender);
    raft_set_current_term(r,1);
    raft_vote(r,1);
    raft_recv_requestvote(r,1,&rv);
    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rvr);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}

void TestRaft_follower_becomes_follower_is_follower(CuTest * tc)
{
    void *r;

    r = raft_new();

    raft_become_follower(r);
    CuAssertTrue(tc, raft_is_follower(r));
}

/* 5.1 */
void TestRaft_follower_recv_appendentries_reply_false_if_term_less_than_currentterm(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    r = raft_new();
    raft_set_configuration(r,cfg);
    sender = sender_new();
    raft_set_callbacks(r,&funcs,sender);

    /* term is low */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;

    /*  higher current term */
    raft_set_current_term(r,5);
    raft_recv_appendentries(r,1,&ae);

    /*  response is false */
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

/* TODO: check if test case is needed */
void TestRaft_follower_recv_appendentries_updates_currentterm_if_term_gt_currentterm(CuTest * tc)
{
    void *r;
    void *sender;
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;

    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);
    sender = sender_new();
    raft_set_callbacks(r,&funcs,sender);

    /*  older currentterm */
    raft_set_current_term(r,1);

    /*  newer term for appendentry */
    memset(&ae,0,sizeof(msg_appendentries_t));
    /* no prev log idx */
    ae.prev_log_idx = 0;
    ae.term = 2;

    /*  appendentry has newer term, so we change our currentterm */
    raft_recv_appendentries(r,1,&ae);
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    /* term has been updated */
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
}

void TestRaft_follower_doesnt_log_after_appendentry_if_no_entries_are_specified(CuTest * tc)
{
    void *r;

    msg_appendentries_t ae;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_state(r,RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    ae.n_entries = 0;

    raft_recv_appendentries(r,1,&ae);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));
}

void TestRaft_follower_increases_log_after_appendentry(CuTest * tc)
{
    void *r;
    void *sender;
    msg_appendentries_t ae;
    msg_entry_t ety;
    msg_appendentries_response_t *aer;
    char *str = "aaa";

    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    r = raft_new();
    raft_set_configuration(r,cfg);
    sender = sender_new();
    raft_set_callbacks(r,&funcs,sender);

    raft_set_state(r,RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    /* first appendentries msg */
    ae.prev_log_idx = 0;
    ae.leader_commit = 5;
    /* include one entry */
    memset(&ety,0,sizeof(msg_entry_t));
    ety.data = str;
    ety.len = 3;
    ety.id = 1;
    ae.entries = &ety;
    ae.n_entries = 1;

    raft_recv_appendentries(r,1,&ae);
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

/*  5.3 */
void TestRaft_follower_recv_appendentries_reply_false_if_doesnt_have_log_at_prev_log_idx_which_matches_prev_log_term(CuTest * tc)
{
    void *r;
    void *sender;
    void *msg;
    msg_entry_t ety;
    char *str = "aaa";
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
    msg_appendentries_response_t *aer;

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_callbacks(r,&funcs,sender);

    /* term is different from appendentries */
    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 1);
    raft_set_last_applied_idx(r, 1);
    // TODO at log manually?

    /* log idx that server doesn't have */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    /* prev_log_term is less than current term (ie. 2) */
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&ety,0,sizeof(msg_entry_t));
    ety.data = str;
    ety.len = 3;
    ety.id = 1;
    ae.entries = &ety;
    ae.n_entries = 1;

    /* trigger reply */
    raft_recv_appendentries(r,1,&ae);
    aer = sender_poll_msg(sender);

    /* reply is false */
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 0 == aer->success);
}

/* 5.3 */
void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries(CuTest * tc)
{
    void *r;
    void *sender;
    msg_appendentries_t ae;
    msg_appendentries_response_t *aer;
    raft_entry_t *ety_appended;

    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    r = raft_new();
    raft_set_configuration(r,cfg);
    sender = sender_new();
    raft_set_callbacks(r,&funcs,sender);

    raft_set_current_term(r,1);

    raft_entry_t ety;

    /* increase log size */
    char *str1 = "111";
    ety.data = str1;
    ety.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* this log will be overwritten by the appendentries below */
    char *str2 = "222";
    ety.data = str2;
    ety.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r,2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data,str2,3));

    /* pass a appendentry that is newer  */
    msg_entry_t mety;

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&mety,0,sizeof(msg_entry_t));
    char *str3 = "333";
    mety.data = str3;
    mety.len = 3;
    mety.id = 3;
    ae.entries = &mety;
    ae.n_entries = 1;

    raft_recv_appendentries(r,1,&ae);
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r,1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data,str1,3));
}

void TestRaft_follower_recv_appendentries_add_new_entries_not_already_in_log(CuTest * tc)
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


    r = raft_new();
    sender = sender_new();
    raft_set_configuration(r,cfg);
    raft_set_current_term(r,1);
    raft_set_callbacks(r,&funcs,sender);

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[2];
    memset(&e,0,sizeof(msg_entry_t) * 2);
    e[0].id = 1;
    e[1].id = 2;
    ae.entries = e;
    ae.n_entries = 2;
    raft_recv_appendentries(r,1,&ae);

    msg_appendentries_response_t *aer;
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
}

//If leaderCommit > commitidx, set commitidx =
//min(leaderCommit, last log idx)
void TestRaft_follower_recv_appendentries_set_commitidx_to_prevLogIdx(CuTest * tc)
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

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_callbacks(r,&funcs,sender);

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e,0,sizeof(msg_entry_t) * 4);
    e[0].id = 1;
    e[1].id = 2;
    e[2].id = 3;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r,1,&ae);

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r,1,&ae);

    msg_appendentries_response_t *aer;
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    /* set to 4 because commitIDX is lower */
    printf("%d\n", raft_get_commit_idx(r));
    CuAssertTrue(tc, 4 == raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_set_commitidx_to_LeaderCommit(CuTest * tc)
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

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_callbacks(r,&funcs,sender);

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e,0,sizeof(msg_entry_t) * 4);
    e[0].id = 1;
    e[1].id = 2;
    e[2].id = 3;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r,1,&ae);

    /* receive an appendentry with commit */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 3;
    ae.leader_commit = 3;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r,1,&ae);

    msg_appendentries_response_t *aer;
    aer = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != aer);
    CuAssertTrue(tc, 1 == aer->success);
    /* set to 3 because leaderCommit is lower */
    CuAssertTrue(tc, 3 == raft_get_commit_idx(r));
}

#if 0
void T_estRaft_follower_rejects_appendentries_if_idx_and_term_dont_match_preceding_ones(CuTest * tc)
{
    void *r;

    msg_appendentries_t ae;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_current_term(r,1);

    /* first append entry */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;

    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_appendentries(r,1,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}
#endif

#if 0
void T_estRaft_follower_resends_entry_if_request_from_leader_timesout(CuTest * tc)
{
    void *r;

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
    void *r;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    r = raft_new();

    /*  1 second election timeout */
    raft_set_election_timeout(r, 1000);

    raft_set_configuration(r,cfg);

    /*  1.001 seconds have passed */
    raft_periodic(r, 1001);

    /* is a candidate now */
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_dont_grant_vote_if_candidate_has_a_less_complete_log(CuTest * tc)
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


    msg_requestvote_t rv;
    msg_requestvote_response_t *rvr;

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /*  request vote */
    /*  vote indicates candidate's log is not complete compared to follower */
    memset(&rv,0,sizeof(msg_requestvote_t));
    rv.term = 1;
    rv.candidate_id = 0;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;

    /* server's term and idx are more up-to-date */
    raft_set_current_term(r,1);
    raft_set_current_idx(r,2);

    /* vote not granted */
    raft_recv_requestvote(r,1,&rv);
    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rvr);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}


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
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* set term so we can check it gets included in the outbound message */
    raft_set_current_term(r,2);
    raft_set_current_idx(r,5);

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
    raft_set_callbacks(r,&funcs,sender);
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
    raft_set_callbacks(r,&funcs,sender);
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
    raft_set_callbacks(r,&funcs,sender);

    raft_vote(r,1);

    memset(&rv,0,sizeof(msg_requestvote_t));
    raft_recv_requestvote(r,1,&rv);

    rvr = sender_poll_msg(sender);
    CuAssertTrue(tc, 0 == rvr->vote_granted);
}

/* Candidate 5.2 */
void TestRaft_candidate_requestvote_includes_logidx(CuTest * tc)
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

    raft_set_callbacks(r,&funcs,sender);
    raft_set_current_term(r,5);
    raft_set_current_idx(r,3);
    raft_send_requestvote(r,1);

    rv = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->last_log_idx);
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
    raft_set_callbacks(r,&funcs,sender);

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
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* server's log is newer */
    raft_set_current_term(r,1);
    raft_set_current_idx(r,2);

    /*  is a candidate */
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    CuAssertTrue(tc, 0 == raft_is_follower(r));

    /*  invalid leader determined by "leaders" old log */
    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;

    /* appendentry from invalid leader doesn't make candidate become follower */
    raft_recv_appendentries(r,1,&ae);
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

void TestRaft_leader_becomes_leader_is_leader(CuTest * tc)
{
    void *r;

    r = raft_new();

    raft_become_leader(r);
    CuAssertTrue(tc, raft_is_leader(r));
}

void TestRaft_leader_when_becomes_leader_all_peers_have_nextidx_equal_to_lastlog_idx_plus_1(CuTest * tc)
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

    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* candidate to leader */
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    int i;
    for (i=0; i<raft_get_npeers(r); i++)
    {
        raft_peer_t* p = raft_get_peer(r,i);
        CuAssertTrue(tc, raft_get_current_idx(r) + 1 ==
                raft_peer_get_next_idx(p));
    }
}

/* 5.2 */
void TestRaft_leader_when_it_becomes_a_leader_sends_empty_appendentries(CuTest * tc)
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

    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* candidate to leader */
    raft_set_state(r,RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both peers */
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.2
 * Note: commit means it's been appended to the log, not applied to the FSM 
 **/
void TestRaft_leader_responds_to_entry_msg_when_entry_is_committed(CuTest * tc)
{
    void *r, *sender;
    msg_entry_response_t *cr;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* I am the leader */
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety;
    ety.id = 1;
    ety.data = "entry";
    ety.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r,1,&ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* trigger response through commit */
    raft_apply_entry(r);

    /* leader sent response to entry message */
    cr = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != cr);
}

/* 5.3 */
void TestRaft_leader_sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx(CuTest * tc)
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


    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* i'm leader */
    raft_set_state(r,RAFT_STATE_LEADER);

    void* p;
    p = raft_get_peer(r,0);
    raft_peer_set_next_idx(p, 4);

    /* receive appendentries messages */
    raft_send_appendentries(r,0);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.3 */
void TestRaft_leader_retries_appendentries_with_decremented_NextIdx_log_inconsistency(CuTest * tc)
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


    msg_appendentries_t* ae;

    sender = sender_new();
    r = raft_new();
    raft_set_callbacks(r,&funcs,sender);
    raft_set_configuration(r,cfg);

    /* i'm leader */
    raft_set_state(r,RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r,0);
    ae = sender_poll_msg(sender);
    CuAssertTrue(tc, NULL != ae);
}

/*
If there exists an N such that N > commitidx, a majority
of matchidx[i] = N, and log[N].term == currentTerm:
set commitidx = N (§5.2, §5.4).
*/

void TestRaft_leader_append_entry_to_log_increases_idxno(CuTest * tc)
{
    void *r;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};


    msg_entry_t ety;
    ety.id = 1;
    ety.data = "entry";
    ety.len = strlen("entry");

    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r,1,&ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

#if 0
// TODO no support for duplicates
void T_estRaft_leader_doesnt_append_entry_if_unique_id_is_duplicate(CuTest * tc)
{
    void *r;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};

    msg_entry_t ety;
    ety.id = 1;
    ety.data = "entry";
    ety.len = strlen("entry");

    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r,1,&ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    raft_recv_entry(r,1,&ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}
#endif

void TestRaft_leader_increase_commit_idx_when_majority_have_entry_and_atleast_one_newer_entry(CuTest * tc)
{
    void *r;
    void *sender;
    raft_external_functions_t funcs = {
        .send = sender_send,
        .log = NULL
    };
    msg_appendentries_response_t aer;

    /* 2 peers */
    raft_peer_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),NULL}};
    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);
    raft_set_callbacks(r,&funcs,sender);

    /* I'm the leader */
    raft_set_state(r,RAFT_STATE_LEADER);
    raft_set_current_term(r,1);
    raft_set_current_idx(r,0);
    raft_set_commit_idx(r,0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r,0);

    /* append entries - we need two */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data = "aaaa";
    ety.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);

    memset(&aer,0,sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, 0);
    raft_send_appendentries(r, 1);
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r,1,&aer);
    raft_recv_appendentries_response(r,2,&aer);
    /* leader will now have majority followers who have appended this log */
    printf("last applied idx: %d\n", raft_get_last_applied_idx(r));
    printf("commit idx: %d\n", raft_get_commit_idx(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));

    /* SECOND entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, 0);
    raft_send_appendentries(r, 1);
    /* receive mock success responses */
    aer.term = 5;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r,1,&aer);
    raft_recv_appendentries_response(r,2,&aer);
    /* leader will now have majority followers who have appended this log */
    printf("last applied idx: %d\n", raft_get_last_applied_idx(r));
    printf("commit idx: %d\n", raft_get_commit_idx(r));
    CuAssertTrue(tc, 2 == raft_get_commit_idx(r));
    CuAssertTrue(tc, 2 == raft_get_last_applied_idx(r));
}

void TestRaft_leader_steps_down_if_received_appendentries_is_newer_than_itself(CuTest * tc)
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

    memset(&ae,0,sizeof(msg_appendentries_t));
    ae.term = 5;
    ae.prev_log_idx = 6;
    ae.prev_log_term = 5;

    sender = sender_new();
    r = raft_new();
    raft_set_configuration(r,cfg);

    raft_set_state(r,RAFT_STATE_LEADER);
    raft_set_current_term(r,5);
    raft_set_current_idx(r,5);
    raft_set_callbacks(r,&funcs,sender);
    raft_recv_appendentries(r,1,&ae);

    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

/* TODO: If a server receives a request with a stale term number, it rejects the request. */


#if 0
void T_estRaft_leader_sends_appendentries_when_receive_entry_msg(CuTest * tc)
#endif
