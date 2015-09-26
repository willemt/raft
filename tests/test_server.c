
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

// TODO: leader doesn't timeout and cause election

void TestRaft_server_voted_for_records_who_we_voted_for(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 2, 0);
    raft_vote(r, raft_get_node(r, 2));
    CuAssertTrue(tc, 2 == raft_get_voted_for(r));
}

void TestRaft_server_idx_starts_at_1(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));

    raft_entry_t ety;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_currentterm_defaults_to_0(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_term(r));
}

void TestRaft_server_set_currentterm_sets_term(CuTest * tc)
{
    void *r = raft_new();
    raft_set_current_term(r, 5);
    CuAssertTrue(tc, 5 == raft_get_current_term(r));
}

void TestRaft_server_voting_results_in_voting(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 0);
    raft_add_node(r, NULL, 9, 0);

    raft_vote(r, raft_get_node(r, 1));
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
    raft_vote(r, raft_get_node(r, 9));
    CuAssertTrue(tc, 9 == raft_get_voted_for(r));
}

void TestRaft_election_start_increments_term(CuTest * tc)
{
    void *r = raft_new();
    raft_set_current_term(r, 1);
    raft_election_start(r);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
}

void TestRaft_set_state(CuTest * tc)
{
    void *r = raft_new();
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, RAFT_STATE_LEADER == raft_get_state(r));
}

void TestRaft_server_starts_as_follower(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, RAFT_STATE_FOLLOWER == raft_get_state(r));
}

void TestRaft_server_starts_with_election_timeout_of_1000ms(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 1000 == raft_get_election_timeout(r));
}

void TestRaft_server_starts_with_request_timeout_of_200ms(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 200 == raft_get_request_timeout(r));
}

void TestRaft_server_entry_append_cant_append_if_id_is_zero(CuTest* tc)
{
    raft_entry_t ety;
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 0;
    ety.term = 1;

    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
}

void TestRaft_server_entry_append_increases_logidx(CuTest* tc)
{
    raft_entry_t ety;
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_append_entry_means_entry_gets_current_term(CuTest* tc)
{
    raft_entry_t ety;
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

static int __raft_logentry_offer(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    int ety_idx
    )
{
    ety->data.buf = udata;
    return 0;
}

void TestRaft_server_append_entry_is_retrievable(CuTest * tc)
{
    raft_cbs_t funcs = {
        .log_offer = __raft_logentry_offer,
    };

    char *data = "xxx";

    void *r = raft_new();
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_callbacks(r, &funcs, data);
    raft_set_current_term(r, 5);
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);

    raft_entry_t* kept =  raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, kept->data.buf == data);
}

#if 0
/* TODO: no support for duplicate detection yet */
void
T_estRaft_server_append_entry_not_sucessful_if_entry_with_id_already_appended(
    CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 2 == raft_get_current_idx(r));

    /* different ID so we can be successful */
    ety.id = 2;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 3 == raft_get_current_idx(r));
}
#endif

void TestRaft_server_entry_is_retrieveable_using_idx(CuTest* tc)
{
    raft_entry_t e1;
    raft_entry_t e2;
    raft_entry_t *ety_appended;
    char *str = "aaa";
    char *str2 = "bbb";

    void *r = raft_new();

    e1.term = 1;
    e1.id = 1;
    e1.data.buf = str;
    e1.data.len = 3;
    raft_append_entry(r, &e1);

    /* different ID so we can be successful */
    e2.term = 1;
    e2.id = 2;
    e2.data.buf = str2;
    e2.data.len = 3;
    raft_append_entry(r, &e2);

    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str2, 3));
}

void TestRaft_server_wont_apply_entry_if_we_dont_have_entry_to_apply(CuTest* tc)
{
    void *r = raft_new();
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));
}

void TestRaft_server_wont_apply_entry_if_there_isnt_a_majority(CuTest* tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    char *str = "aaa";
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_apply_entry(r);
    /* Not allowed to be applied because we haven't confirmed a majority yet */
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));
}

/* If commitidx > lastApplied: increment lastApplied, apply log[lastApplied]
 * to state machine (§5.3) */
void TestRaft_server_increment_lastApplied_when_lastApplied_lt_commitidx(
    CuTest* tc)
{
    void *r = raft_new();

    /* must be follower */
    raft_set_state(r, RAFT_STATE_FOLLOWER);
    raft_set_current_term(r, 1);
    raft_set_last_applied_idx(r, 0);

    /* need at least one entry */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 1);

    /* let time lapse */
    raft_periodic(r, 1);
    CuAssertTrue(tc, 0 != raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
}

void TestRaft_server_apply_entry_increments_last_applied_idx(CuTest* tc)
{
    void *r = raft_new();
    raft_set_last_applied_idx(r, 0);

    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);
    raft_apply_entry(r);
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
}

void TestRaft_server_periodic_elapses_election_timeout(CuTest * tc)
{
    void *r = raft_new();
    /* we don't want to set the timeout to zero */
    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 0);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 100);
    CuAssertTrue(tc, 100 == raft_get_timeout_elapsed(r));
}

void TestRaft_server_election_timeout_promotes_us_to_leader_if_there_is_only_1_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

void TestRaft_server_recv_entry_auto_commits_if_we_are_the_only_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);
    raft_become_leader(r);
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    /* entry message */
    msg_entry_t ety;
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, raft_get_node(r, 1), &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
}

void TestRaft_server_cfg_sets_num_nodes(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertTrue(tc, 2 == raft_get_num_nodes(r));
}

void TestRaft_server_cant_get_node_we_dont_have(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertTrue(tc, NULL == raft_get_node(r, 0));
    CuAssertTrue(tc, NULL != raft_get_node(r, 1));
    CuAssertTrue(tc, NULL != raft_get_node(r, 2));
    CuAssertTrue(tc, NULL == raft_get_node(r, 3));
}

/* If term > currentTerm, set currentTerm to term (step down if candidate or
 * leader) */
void TestRaft_votes_are_majority_is_true(
    CuTest * tc
    )
{
    /* 1 of 3 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(3, 1));

    /* 2 of 3 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(3, 2));

    /* 2 of 5 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(5, 2));

    /* 3 of 5 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(5, 3));

    /* 2 of 1?? This is an error */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(1, 2));
}

void TestRaft_server_recv_requestvote_response_dont_increase_votes_for_me_when_not_granted(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 0;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_dont_increase_votes_for_me_when_term_is_not_equal(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 3);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_increase_votes_for_me(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    raft_become_candidate(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_must_be_candidate_to_receive(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    raft_become_leader(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

/* Reply false if term < currentTerm (§5.1) */
void TestRaft_server_recv_requestvote_reply_false_if_term_less_than_current_term(
    CuTest * tc
    )
{
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 2);

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

void TestRaft_leader_recv_requestvote_does_not_step_down(
    CuTest * tc
    )
{
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_vote(r, raft_get_node(r, 1));
    raft_become_leader(r);
    CuAssertIntEquals(tc, 1, raft_is_leader(r));

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, raft_get_current_leader(r));
}

/* Reply true if term >= currentTerm (§5.1) */
void TestRaft_server_recv_requestvote_reply_true_if_term_greater_than_or_equal_to_current_term(
    CuTest * tc
    )
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* term is less than current term */
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    CuAssertTrue(tc, 1 == rvr.vote_granted);
}

void TestRaft_server_recv_requestvote_reset_timeout(
    CuTest * tc
    )
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_set_election_timeout(r, 1000);
    raft_periodic(r, 900);

    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 1 == rvr.vote_granted);
    CuAssertIntEquals(tc, 0, raft_get_timeout_elapsed(r));
}

void TestRaft_server_recv_requestvote_candidate_step_down_if_term_is_higher_than_current_term(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_become_candidate(r);
    raft_set_current_term(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));

    /* current term is less than term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, raft_is_follower(r));
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));
}

/* If votedFor is null or candidateId, and candidate's log is at
 * least as up-to-date as local log, grant vote (§5.2, §5.4) */
void TestRaft_server_dont_grant_vote_if_we_didnt_vote_for_this_candidate(
    CuTest * tc
    )
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_vote(r, raft_get_node(r, 1));

    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

void TestRaft_follower_becomes_follower_is_follower(CuTest * tc)
{
    void *r = raft_new();
    raft_become_follower(r);
    CuAssertTrue(tc, raft_is_follower(r));
}

void TestRaft_follower_becomes_follower_does_not_clear_voted_for(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);

    raft_vote(r, raft_get_node(r, 1));
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));
    raft_become_follower(r);
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));
}

/* 5.1 */
void TestRaft_follower_recv_appendentries_reply_false_if_term_less_than_currentterm(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    /* no leader known at this point */
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));

    /* term is low */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;

    /*  higher current term */
    raft_set_current_term(r, 5);
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 0 == aer.success);
    /* rejected appendentries doesn't change the current leader. */
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
}

/* TODO: check if test case is needed */
void TestRaft_follower_recv_appendentries_updates_currentterm_if_term_gt_currentterm(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  older currentterm */
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));

    /*  newer term for appendentry */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    /* no prev log idx */
    ae.prev_log_idx = 0;
    ae.term = 2;

    /*  appendentry has newer term, so we change our currentterm */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    /* term has been updated */
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
    /* and leader has been updated */
    CuAssertIntEquals(tc, 2, raft_get_current_leader(r));
}

void TestRaft_follower_recv_appendentries_does_not_log_if_no_entries_are_specified(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    ae.n_entries = 0;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));
}

void TestRaft_follower_recv_appendentries_increases_log(CuTest * tc)
{
    msg_appendentries_t ae;
    msg_entry_t ety;
    msg_appendentries_response_t aer;
    char *str = "aaa";

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 3;
    ae.prev_log_term = 1;
    /* first appendentries msg */
    ae.prev_log_idx = 0;
    ae.leader_commit = 5;
    /* include one entry */
    memset(&ety, 0, sizeof(msg_entry_t));
    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    /* check that old terms are passed onto the log */
    ety.term = 2;
    ae.entries = &ety;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    raft_entry_t* log = raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, 2 == log->term);
}

/*  5.3 */
void TestRaft_follower_recv_appendentries_reply_false_if_doesnt_have_log_at_prev_log_idx_which_matches_prev_log_term(
    CuTest * tc)
{
    msg_entry_t ety;
    char *str = "aaa";

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* term is different from appendentries */
    raft_set_current_term(r, 2);
    // TODO at log manually?

    /* log idx that server doesn't have */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    /* prev_log_term is less than current term (ie. 2) */
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&ety, 0, sizeof(msg_entry_t));
    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ae.entries = &ety;
    ae.n_entries = 1;

    /* trigger reply */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* reply is false */
    CuAssertTrue(tc, 0 == aer.success);
}

static raft_entry_t* __entries_for_conflict_tests(
        CuTest * tc,
        raft_server_t* r,
        char** strs)
{
    raft_entry_t ety;
    raft_entry_t *ety_appended;

    /* increase log size */
    char *str1 = strs[0];
    ety.data.buf = str1;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* this log will be overwritten by the appendentries below */
    char *str2 = strs[1];
    ety.data.buf = str2;
    ety.data.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str2, 3));

    /* this log will be overwritten by the appendentries below */
    char *str3 = strs[2];
    ety.data.buf = str3;
    ety.data.len = 3;
    ety.id = 3;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 3 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 3)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str3, 3));

    return ety_appended;
}

/* 5.3 */
void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __entries_for_conflict_tests(tc, r, strs);

    /* pass a appendentry that is newer  */
    msg_entry_t mety;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&mety, 0, sizeof(msg_entry_t));
    char *str4 = "444";
    mety.data.buf = str4;
    mety.data.len = 3;
    mety.id = 4;
    ae.entries = &mety;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, strs[0], 3));
}

void TestRaft_follower_recv_appendentries_delete_entries_if_current_idx_greater_than_prev_log_idx(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __entries_for_conflict_tests(tc, r, strs);

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    ae.entries = NULL;
    ae.n_entries = 0;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, strs[0], 3));
}

void TestRaft_follower_recv_appendentries_add_new_entries_not_already_in_log(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].id = 1;
    e[1].id = 2;
    ae.entries = e;
    ae.n_entries = 2;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
}

void TestRaft_follower_recv_appendentries_does_not_add_dupe_entries_already_in_log(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include 1 entry */
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    /* still successful even when no raft_append_entry() happened! */
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));

    /* lets get the server to append 2 now! */
    e[1].id = 2;
    ae.n_entries = 2;
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
}

/* If leaderCommit > commitidx, set commitidx =
 *  min(leaderCommit, last log idx) */
void TestRaft_follower_recv_appendentries_set_commitidx_to_prevLogIdx(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    e[3].term = 1;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    /* set to 4 because commitIDX is lower */
    CuAssertIntEquals(tc, 4, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_set_commitidx_to_LeaderCommit(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    e[3].term = 1;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 3;
    ae.leader_commit = 3;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    /* set to 3 because leaderCommit is lower */
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_failure_includes_current_idx(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_entry_t ety;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    /* receive an appendentry with commit */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    /* lower term means failure */
    ae.term = 0;
    ae.prev_log_term = 0;
    ae.prev_log_idx = 0;
    ae.leader_commit = 0;
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 0 == aer.success);
    CuAssertIntEquals(tc, 1, aer.current_idx);

    /* try again with a higher current_idx */
    memset(&aer, 0, sizeof(aer));
    ety.id = 2;
    raft_append_entry(r, &ety);
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == aer.success);
    CuAssertIntEquals(tc, 2, aer.current_idx);
}

void TestRaft_follower_becomes_candidate_when_election_timeout_occurs(
    CuTest * tc)
{
    void *r = raft_new();

    /*  1 second election timeout */
    raft_set_election_timeout(r, 1000);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  1.001 seconds have passed */
    raft_periodic(r, 1001);

    /* is a candidate now */
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_dont_grant_vote_if_candidate_has_a_less_complete_log(
    CuTest * tc)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  request vote */
    /*  vote indicates candidate's log is not complete compared to follower */
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;

    raft_set_current_term(r, 1);

    /* server's idx are more up-to-date */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    raft_append_entry(r, &ety);

    /* vote not granted */
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);

    /* approve vote, because last_log_term is higher */
    raft_set_current_term(r, 2);
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 2;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 1 == rvr.vote_granted);
}

void TestRaft_candidate_becomes_candidate_is_candidate(CuTest * tc)
{
    void *r = raft_new();
    raft_become_candidate(r);
    CuAssertTrue(tc, raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_increments_current_term(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_votes_for_self(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, raft_get_nodeid(r) == raft_get_voted_for(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_resets_election_timeout(CuTest * tc)
{
    void *r = raft_new();
    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 900);
    CuAssertTrue(tc, 900 == raft_get_timeout_elapsed(r));

    raft_become_candidate(r);
    /* time is selected randomly */
    CuAssertTrue(tc, raft_get_timeout_elapsed(r) < 1000);
}

void TestRaft_follower_recv_appendentries_resets_election_timeout(
    CuTest * tc)
{
    void *r = raft_new();
    raft_set_election_timeout(r, 1000);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 1);

    raft_periodic(r, 900);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 1), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_requests_votes_from_other_servers(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_requestvote = sender_requestvote,
    };
    msg_requestvote_t* rv;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* set term so we can check it gets included in the outbound message */
    raft_set_current_term(r, 2);

    /* becoming candidate triggers vote requests */
    raft_become_candidate(r);

    /* 2 nodes = 2 vote requests */
    rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 2 != rv->term);
    CuAssertTrue(tc, 3 == rv->term);
    /*  TODO: there should be more items */
    rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->term);
}

/* Candidate 5.2 */
void TestRaft_candidate_election_timeout_and_no_leader_results_in_new_election(
    CuTest * tc)
{
    msg_requestvote_response_t vr;
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 0;
    vr.vote_granted = 1;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* server wants to be leader, so becomes candidate */
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));

    /*  receiving this vote gives the server majority */
//    raft_recv_requestvote_response(r,1,&vr);
//    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_receives_majority_of_votes_becomes_leader(CuTest * tc)
{
    msg_requestvote_response_t vr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    CuAssertTrue(tc, 5 == raft_get_num_nodes(r));

    /* vote for self */
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));

    /* a vote for us */
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.vote_granted = 1;
    /* get one vote */
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &vr);
    CuAssertTrue(tc, 2 == raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 0 == raft_is_leader(r));

    /* get another vote
     * now has majority (ie. 3/5 votes) */
    raft_recv_requestvote_response(r, raft_get_node(r, 3), &vr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_will_not_respond_to_voterequest_if_it_has_already_voted(
    CuTest * tc)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_vote(r, raft_get_node(r, 1));

    memset(&rv, 0, sizeof(msg_requestvote_t));
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    /* we've vote already, so won't respond with a vote granted... */
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

/* Candidate 5.2 */
void TestRaft_candidate_requestvote_includes_logidx(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_requestvote = sender_requestvote,
        .log              = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_callbacks(r, &funcs, sender);
    raft_set_current_term(r, 5);
    /* 3 entries */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    raft_append_entry(r, &ety);
    ety.id = 102;
    ety.term = 3;
    raft_append_entry(r, &ety);
    raft_send_requestvote(r, raft_get_node(r, 2));

    msg_requestvote_t* rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertIntEquals(tc, 3, rv->last_log_idx);
    CuAssertIntEquals(tc, 5, rv->term);
    CuAssertIntEquals(tc, 3, rv->last_log_term);
    CuAssertIntEquals(tc, 1, rv->candidate_id);
}

void TestRaft_candidate_recv_requestvote_response_becomes_follower_if_current_term_is_less_than_term(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 0;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_leader_results_in_follower(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
    CuAssertTrue(tc, 0 == raft_get_current_term(r));

    /* receive recent appendentries */
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    /* after accepting a leader, it's available as the last known leader */
    CuAssertTrue(tc, 2 == raft_get_current_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_from_same_term_results_in_step_down(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 2);

    raft_set_state(r, RAFT_STATE_CANDIDATE);
    CuAssertTrue(tc, 0 == raft_is_follower(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_is_candidate(r));
}

void TestRaft_leader_becomes_leader_is_leader(CuTest * tc)
{
    void *r = raft_new();
    raft_become_leader(r);
    CuAssertTrue(tc, raft_is_leader(r));
}

void TestRaft_leader_becomes_leader_does_not_clear_voted_for(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_vote(r, raft_get_node(r, 1));
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
    raft_become_leader(r);
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
}

void TestRaft_leader_when_becomes_leader_all_nodes_have_nextidx_equal_to_lastlog_idx_plus_1(
    CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    int i;
    for (i = 2; i <= 3; i++)
    {
        raft_node_t* p = raft_get_node(r, i);
        CuAssertTrue(tc, raft_get_current_idx(r) + 1 ==
                     raft_node_get_next_idx(p));
    }
}

/* 5.2 */
void TestRaft_leader_when_it_becomes_a_leader_sends_empty_appendentries(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.2
 * Note: commit means it's been appended to the log, not applied to the FSM */
void TestRaft_leader_responds_to_entry_msg_when_entry_is_committed(CuTest * tc)
{
    msg_entry_response_t cr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety;
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, raft_get_node(r, 2), &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* trigger response through commit */
    raft_apply_entry(r);
}

void TestRaft_non_leader_recv_entry_msg_fails(CuTest * tc)
{
    msg_entry_response_t cr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /* entry message */
    msg_entry_t ety;
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    int e = raft_recv_entry(r, raft_get_node(r, 2), &ety, &cr);
    CuAssertTrue(tc, -1 == e);
}

/* 5.3 */
void TestRaft_leader_sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_node_t* p = raft_get_node(r, 2);
    raft_node_set_next_idx(p, 4);

    /* receive appendentries messages */
    raft_send_appendentries(r, p);
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

void TestRaft_leader_sends_appendentries_with_leader_commit(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    int i;

    for (i=0; i<10; i++)
    {
        raft_entry_t ety;
        ety.term = 1;
        ety.id = 1;
        ety.data.buf = "aaa";
        ety.data.len = 3;
        raft_append_entry(r, &ety);
    }

    raft_set_commit_idx(r, 10);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->leader_commit == 10);
}

void TestRaft_leader_sends_appendentries_with_prevLogIdx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1); /* me */
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);

    raft_node_t* n = raft_get_node(r, 2);

    /* add 1 entry */
    /* receive appendentries messages */
    raft_entry_t ety;
    ety.term = 2;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_node_set_next_idx(n, 1);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);
    CuAssertTrue(tc, ae->n_entries == 1);
    CuAssertTrue(tc, ae->entries[0].id == 100);
    CuAssertTrue(tc, ae->entries[0].term == 2);

    /* set next_idx */
    /* receive appendentries messages */
    raft_node_set_next_idx(n, 2);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 1);
}

void TestRaft_leader_sends_appendentries_when_node_has_next_idx_of_0(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);

    /* add an entry */
    /* receive appendentries messages */
    raft_node_t* n = raft_get_node(r, 2);
    raft_node_set_next_idx(n, 1);
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_send_appendentries(r, n);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);
}

/* 5.3 */
void TestRaft_leader_retries_appendentries_with_decremented_NextIdx_log_inconsistency(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/*
 * If there exists an N such that N > commitidx, a majority
 * of matchidx[i] = N, and log[N].term == currentTerm:
 * set commitidx = N (§5.2, §5.4).  */
void TestRaft_leader_append_entry_to_log_increases_idxno(CuTest * tc)
{
    msg_entry_t ety;
    msg_entry_response_t cr;

    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r, raft_get_node(r, 2), &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

#if 0
// TODO no support for duplicates
void T_estRaft_leader_doesnt_append_entry_if_unique_id_is_duplicate(CuTest * tc)
{
    void *r;

    /* 2 nodes */
    raft_node_configuration_t cfg[] = {
        { (void*)1 },
        { (void*)2 },
        { NULL     }
    };

    msg_entry_t ety;
    ety.id = 1;
    ety.data = "entry";
    ety.data.len = strlen("entry");

    r = raft_new();
    raft_set_configuration(r, cfg, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}
#endif

void TestRaft_leader_recv_appendentries_response_increase_commit_idx_when_majority_have_entry_and_atleast_one_newer_entry(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));

    /* SECOND entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_last_applied_idx(r));
}

void TestRaft_leader_recv_appendentries_response_do_not_increase_commit_idx_because_of_old_terms_with_majority(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_last_applied_idx(r));

    /* SECOND entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_last_applied_idx(r));

    /* THIRD entry log application */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses
     * let's say that the nodes have majority within leader's current term */
    aer.term = 2;
    aer.success = 1;
    aer.current_idx = 3;
    aer.first_idx = 3;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_last_applied_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 3, raft_get_last_applied_idx(r));
}

void TestRaft_leader_recv_appendentries_response_jumps_to_lower_next_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 3;
    ety.id = 3;
    raft_append_entry(r, &ety);
    ety.term = 4;
    ety.id = 4;
    raft_append_entry(r, &ety);

    msg_appendentries_t* ae;

    /* become leader sets next_idx to current_idx */
    raft_become_leader(r);
    raft_node_t* node = raft_get_node(r, 2);
    CuAssertIntEquals(tc, 5, raft_node_get_next_idx(node));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 4, ae->prev_log_term);
    CuAssertIntEquals(tc, 4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 2, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 1, ae->prev_log_term);
    CuAssertIntEquals(tc, 1, ae->prev_log_idx);

    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_appendentries_response_decrements_to_lower_next_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 3;
    ety.id = 3;
    raft_append_entry(r, &ety);
    ety.term = 4;
    ety.id = 4;
    raft_append_entry(r, &ety);

    msg_appendentries_t* ae;

    /* become leader sets next_idx to current_idx */
    raft_become_leader(r);
    raft_node_t* node = raft_get_node(r, 2);
    CuAssertIntEquals(tc, 5, raft_node_get_next_idx(node));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 4, ae->prev_log_term);
    CuAssertIntEquals(tc, 4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 4, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 3, ae->prev_log_term);
    CuAssertIntEquals(tc, 3, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 3, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 2, ae->prev_log_term);
    CuAssertIntEquals(tc, 2, ae->prev_log_idx);

    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_appendentries_response_retry_only_if_leader(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));

    CuAssertTrue(tc, NULL != sender_poll_msg_data(sender));
    CuAssertTrue(tc, NULL != sender_poll_msg_data(sender));

    raft_become_follower(r);

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    CuAssertTrue(tc, -1 == raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer));
    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_entry_resets_election_timeout(
    CuTest * tc)
{
    void *r = raft_new();
    raft_set_election_timeout(r, 1000);
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_periodic(r, 900);

    /* entry message */
    msg_entry_t mety;
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, NULL, &mety, &cr);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}

void TestRaft_leader_recv_entry_is_committed_returns_0_if_not_committed(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety;
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, raft_get_node(r, 2), &mety, &cr);
    CuAssertTrue(tc, 0 == raft_msg_entry_response_committed(r, &cr));

    raft_set_commit_idx(r, 1);
    CuAssertTrue(tc, 1 == raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_leader_recv_entry_is_committed_returns_neg_1_if_invalidated(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety;
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, raft_get_node(r, 2), &mety, &cr);
    CuAssertTrue(tc, 0 == raft_msg_entry_response_committed(r, &cr));
    CuAssertTrue(tc, cr.term == 1);
    CuAssertTrue(tc, cr.idx == 1);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    /* append entry that invalidates entry message */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.leader_commit = 1;
    ae.term = 2;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    msg_appendentries_response_t aer;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t) * 1);
    e[0].term = 2;
    e[0].id = 999;
    e[0].data.buf = "aaa";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
    CuAssertTrue(tc, -1 == raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_leader_recv_entry_does_not_send_new_appendentries_to_slow_nodes(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* make the node slow */
    raft_node_set_next_idx(raft_get_node(r, 2), 1);

    /* append entries */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    /* entry message */
    msg_entry_t mety;
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, NULL, &mety, &cr);

    /* check if the slow node got sent this appendentries */
    msg_appendentries_t* ae;
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL == ae);
}

void TestRaft_leader_recv_appendentries_response_failure_does_not_set_node_nextid_to_0(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety;
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));

    /* receive mock success response */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 0;
    aer.current_idx = 0;
    aer.first_idx = 0;
    raft_node_t* p = raft_get_node(r, 2);
    raft_recv_appendentries_response(r, p, &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
    raft_recv_appendentries_response(r, p, &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_response_increment_idx_of_node(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    raft_node_t* p = raft_get_node(r, 2);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 0;
    aer.first_idx = 0;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_response_drop_message_if_term_is_old(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);

    raft_node_t* p = raft_get_node(r, 2);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));

    /* receive OLD mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_steps_down_if_newer(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);
    /* check that node 0 considers itself the leader */
    CuAssertTrue(tc, 1 == raft_is_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_leader(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 6;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* after more recent appendentries from node 1, node 0 should
     * consider node 1 the leader. */
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    CuAssertTrue(tc, 1 == raft_get_current_leader(r));
}

void TestRaft_leader_recv_appendentries_steps_down_if_newer_term(
    CuTest * tc)
{
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 5;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

void TestRaft_leader_sends_empty_appendentries_every_request_timeout(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae;
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);

    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL == ae);

    /* force request timeout */
    raft_periodic(r, 501);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* TODO: If a server receives a request with a stale term number, it rejects the request. */
#if 0
void T_estRaft_leader_sends_appendentries_when_receive_entry_msg(CuTest * tc)
#endif

void TestRaft_leader_recv_requestvote_responds_without_granting(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

void TestRaft_leader_recv_requestvote_responds_with_granting_if_term_is_higher(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}
