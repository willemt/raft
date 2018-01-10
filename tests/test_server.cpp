#include <gtest/gtest.h>
extern "C"
{
#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
#include "mock_send_functions.h"
}
// TODO: leader doesn't timeout and cause election

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

int __raft_applylog(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    int idx
    )
{
    return 0;
}

int __raft_send_requestvote(raft_server_t* raft,
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

raft_cbs_t generic_funcs()
{
    raft_cbs_t f = {0};
    f.persist_term = __raft_persist_term;
    f.persist_vote = __raft_persist_vote;
    return f;
};

static int __raft_node_has_sufficient_logs(
    raft_server_t* raft,
    void *user_data,
    raft_node_t* node)
{
    int *flag = (int*)user_data;
    *flag += 1;
    return 0;
}

static int max_election_timeout(int election_timeout)
{
    return 2 * election_timeout;
}

TEST(TestServer, voted_for_records_who_we_voted_for)
{
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &generic_funcs(), NULL);
    raft_add_node(r, NULL, 2, 0);
    raft_vote(r, raft_get_node(r, 2));
    EXPECT_EQ(2, raft_get_voted_for(r));
}

TEST(TestServer, get_my_node)
{
    raft_server_t *r = raft_new();
    raft_node_t* me = raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    EXPECT_EQ(me, raft_get_my_node(r));
}

TEST(TestServer, idx_starts_at_1)
{
    raft_server_t *r = raft_new();
    EXPECT_EQ(0, raft_get_current_idx(r));

    raft_entry_t ety = {0};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    EXPECT_EQ(1, raft_get_current_idx(r));
}

TEST(TestServer, currentterm_defaults_to_0)
{
    raft_server_t *r = raft_new();
    EXPECT_EQ(0, raft_get_current_term(r));
}

TEST(TestServer, set_currentterm_sets_term)
{
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &generic_funcs(), NULL);
    raft_set_current_term(r, 5);
    EXPECT_EQ(5, raft_get_current_term(r));
}

TEST(TestServer, voting_results_in_voting)
{
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &generic_funcs(), NULL);
    raft_add_node(r, NULL, 1, 0);
    raft_add_node(r, NULL, 9, 0);

    raft_vote(r, raft_get_node(r, 1));
    EXPECT_EQ(1, raft_get_voted_for(r));
    raft_vote(r, raft_get_node(r, 9));
    EXPECT_EQ(9, raft_get_voted_for(r));
}

TEST(TestServer, add_node_makes_non_voting_node_voting)
{
    raft_server_t *r = raft_new();
    raft_node_t* n1 = raft_add_non_voting_node(r, NULL, 9, 0);

    EXPECT_FALSE(raft_node_is_voting(n1));
    raft_add_node(r, NULL, 9, 0);
    EXPECT_TRUE(raft_node_is_voting(n1));
    EXPECT_EQ(1, raft_get_num_nodes(r));
}

TEST(TestServer, add_node_with_already_existing_id_is_not_allowed)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 9, 0);
    raft_add_node(r, NULL, 11, 0);

    EXPECT_EQ(nullptr, raft_add_node(r, NULL, 9, 0));
    EXPECT_EQ(nullptr, raft_add_node(r, NULL, 11, 0));
}

TEST(TestServer, add_non_voting_node_with_already_existing_id_is_not_allowed)
{
    raft_server_t *r = raft_new();
    raft_add_non_voting_node(r, NULL, 9, 0);
    raft_add_non_voting_node(r, NULL, 11, 0);

    EXPECT_EQ(nullptr, raft_add_non_voting_node(r, NULL, 9, 0));
    EXPECT_EQ(nullptr, raft_add_non_voting_node(r, NULL, 11, 0));
}

TEST(TestServer, add_non_voting_node_with_already_existing_voting_id_is_not_allowed)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 9, 0);
    raft_add_node(r, NULL, 11, 0);

    EXPECT_EQ(nullptr, raft_add_non_voting_node(r, NULL, 9, 0));
    EXPECT_EQ(nullptr, raft_add_non_voting_node(r, NULL, 11, 0));
}

TEST(TestServer, remove_node)
{
    raft_server_t *r = raft_new();
    raft_node_t* n1 = raft_add_node(r, NULL, 1, 0);
    raft_node_t* n2 = raft_add_node(r, NULL, 9, 0);

    raft_remove_node(r, n1);
    EXPECT_EQ(nullptr, raft_get_node(r, 1));
    EXPECT_NE(nullptr, raft_get_node(r, 9));
    raft_remove_node(r, n2);
    EXPECT_EQ(nullptr, raft_get_node(r, 9));
}

TEST(TestServer, election_start_increments_term)
{
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &generic_funcs(), NULL);
    raft_set_current_term(r, 1);
    raft_election_start(r);
    EXPECT_EQ(2, raft_get_current_term(r));
}

TEST(TestServer, set_state)
{
    raft_server_t *r = raft_new();
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(RAFT_STATE_LEADER, raft_get_state(r));
}

TEST(TestServer, starts_as_follower)
{
    raft_server_t *r = raft_new();
    EXPECT_EQ(RAFT_STATE_FOLLOWER, raft_get_state(r));
}

TEST(TestServer, starts_with_election_timeout_of_1000ms)
{
    raft_server_t *r = raft_new();
    EXPECT_EQ(1000, raft_get_election_timeout(r));
}

TEST(TestServer, starts_with_request_timeout_of_200ms)
{
    raft_server_t *r = raft_new();
    EXPECT_EQ(200, raft_get_request_timeout(r));
}

TEST(TestServer, entry_append_increases_logidx)
{
    raft_entry_t ety = {0};
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    raft_server_t *r = raft_new();
    EXPECT_EQ(0, raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    EXPECT_EQ(1, raft_get_current_idx(r));
}

TEST(TestServer, append_entry_means_entry_gets_current_term)
{
    raft_entry_t ety = {0};
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    raft_server_t *r = raft_new();
    EXPECT_EQ(0, raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    EXPECT_EQ(1, raft_get_current_idx(r));
}

TEST(TestServer, append_entry_is_retrievable)
{
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &generic_funcs(), NULL);
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_current_term(r, 5);
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);

    raft_entry_t* kept =  raft_get_entry_from_idx(r, 1);
    EXPECT_NE(nullptr, kept->data.buf);
    EXPECT_EQ(ety.data.len, kept->data.len);
    EXPECT_EQ(kept->data.buf, ety.data.buf);
}

static int __raft_logentry_offer(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    int ety_idx
    )
{
    EXPECT_EQ(ety_idx, 1);
    ety->data.buf = udata;
    return 0;
}

TEST(TestServer, append_entry_user_can_set_data_buf)
{
    raft_cbs_t funcs = { 0 };
    funcs.log_offer = __raft_logentry_offer;
    funcs.persist_term = __raft_persist_term;
    char *buf = "aaa";

    raft_server_t *r = raft_new();
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_set_callbacks(r, &funcs, r);
    raft_set_current_term(r, 5);
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = buf;
    raft_append_entry(r, &ety);
    /* User's input entry is intact. */
    EXPECT_EQ(ety.data.buf, buf);
    raft_entry_t* kept =  raft_get_entry_from_idx(r, 1);
    EXPECT_NE(nullptr, kept->data.buf);
    /* Data buf is the one set by log_offer. */
    EXPECT_EQ(kept->data.buf, r);
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
    EXPECT_EQ(1, raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    raft_append_entry(r, &ety);
    EXPECT_EQ(2, raft_get_current_idx(r));

    /* different ID so we can be successful */
    ety.id = 2;
    raft_append_entry(r, &ety);
    EXPECT_EQ(3, raft_get_current_idx(r));
}
#endif

TEST(TestServer, entry_is_retrieveable_using_idx)
{
    raft_entry_t e1 = {0};
    raft_entry_t e2 = {0};
    raft_entry_t *ety_appended;
    char *str = "aaa";
    char *str2 = "bbb";

    raft_server_t *r = raft_new();

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

    ety_appended = raft_get_entry_from_idx(r, 2);
    EXPECT_NE(nullptr, ety_appended);
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, str2, 3));
}

TEST(TestServer, wont_apply_entry_if_we_dont_have_entry_to_apply)
{
    raft_server_t *r = raft_new();
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    EXPECT_EQ(0, raft_get_last_applied_idx(r));
    EXPECT_EQ(0, raft_get_commit_idx(r));
}

TEST(TestServer, wont_apply_entry_if_there_isnt_a_majority)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    EXPECT_EQ(0, raft_get_last_applied_idx(r));
    EXPECT_EQ(0, raft_get_commit_idx(r));

    char *str = "aaa";
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_apply_entry(r);
    /* Not allowed to be applied because we haven't confirmed a majority yet */
    EXPECT_EQ(0, raft_get_last_applied_idx(r));
    EXPECT_EQ(0, raft_get_commit_idx(r));
}

/* If commitidx > lastApplied: increment lastApplied, apply log[lastApplied]
 * to state machine (§5.3) */
TEST(TestServer, increment_lastApplied_when_lastApplied_lt_commitidx)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.applylog = __raft_applylog;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /* must be follower */
    raft_set_state(r, RAFT_STATE_FOLLOWER);
    raft_set_current_term(r, 1);
    raft_set_last_applied_idx(r, 0);

    /* need at least one entry */
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 1);

    /* let time lapse */
    raft_periodic(r, 1);
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
}

TEST(TestServer, apply_entry_increments_last_applied_idx)
{
    raft_cbs_t funcs = {0};
    funcs.applylog = __raft_applylog;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_set_last_applied_idx(r, 0);

    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);
    raft_apply_entry(r);
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
}

TEST(TestServer, periodic_elapses_election_timeout)
{
    raft_server_t *r = raft_new();
    /* we don't want to set the timeout to zero */
    raft_set_election_timeout(r, 1000);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    raft_periodic(r, 0);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    raft_periodic(r, 100);
    EXPECT_EQ(100, raft_get_timeout_elapsed(r));
}

TEST(TestServer, election_timeout_does_not_promote_us_to_leader_if_there_is_are_more_than_1_nodes)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    EXPECT_EQ(0, raft_is_leader(r));
}

TEST(TestServer, election_timeout_does_not_promote_us_to_leader_if_we_are_not_voting_node)
{
    raft_server_t *r = raft_new();
    raft_add_non_voting_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    EXPECT_EQ(0, raft_is_leader(r));
    EXPECT_EQ(0, raft_get_current_term(r));
}

TEST(TestServer, election_timeout_does_not_start_election_if_there_are_no_voting_nodes)
{
    raft_server_t *r = raft_new();
    raft_add_non_voting_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    EXPECT_EQ(0, raft_get_current_term(r));
}

TEST(TestServer, election_timeout_does_promote_us_to_leader_if_there_is_only_1_node)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    EXPECT_EQ(1, raft_is_leader(r));
}

TEST(TestServer, election_timeout_does_promote_us_to_leader_if_there_is_only_1_voting_node)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    EXPECT_EQ(1, raft_is_leader(r));
}

TEST(TestServer, recv_entry_auto_commits_if_we_are_the_only_node)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);
    raft_become_leader(r);
    EXPECT_EQ(0, raft_get_commit_idx(r));

    /* entry message */
    msg_entry_t ety = {0};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(1, raft_get_log_count(r));
    EXPECT_EQ(1, raft_get_commit_idx(r));
}

TEST(TestServer, recv_entry_fails_if_there_is_already_a_voting_change)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);
    raft_become_leader(r);
    EXPECT_EQ(0, raft_get_commit_idx(r));

    /* entry message */
    msg_entry_t ety = {0};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    EXPECT_EQ(0, raft_recv_entry(r, &ety, &cr));
    EXPECT_EQ(1, raft_get_log_count(r));

    ety.id = 2;
    EXPECT_EQ(RAFT_ERR_ONE_VOTING_CHANGE_ONLY, raft_recv_entry(r, &ety, &cr));
    EXPECT_EQ(1, raft_get_commit_idx(r));
}

TEST(TestServer, cfg_sets_num_nodes)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    EXPECT_EQ(2, raft_get_num_nodes(r));
}

TEST(TestServer, cant_get_node_we_dont_have)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    EXPECT_EQ(nullptr, raft_get_node(r, 0));
    EXPECT_NE(nullptr, raft_get_node(r, 1));
    EXPECT_NE(nullptr, raft_get_node(r, 2));
    EXPECT_EQ(nullptr, raft_get_node(r, 3));
}

/* If term > currentTerm, set currentTerm to term (step down if candidate or
 * leader) */
TEST(TestServer, votes_are_majority_is_true)
{
    /* 1 of 3 = lose */
    EXPECT_EQ(0, raft_votes_is_majority(3, 1));

    /* 2 of 3 = win */
    EXPECT_EQ(1, raft_votes_is_majority(3, 2));

    /* 2 of 5 = lose */
    EXPECT_EQ(0, raft_votes_is_majority(5, 2));

    /* 3 of 5 = win */
    EXPECT_EQ(1, raft_votes_is_majority(5, 3));

    /* 2 of 1?? This is an error */
    EXPECT_EQ(0, raft_votes_is_majority(1, 2));
}

TEST(TestServer, recv_requestvote_response_dont_increase_votes_for_me_when_not_granted)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 0;
    int e = raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_EQ(0, e);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));
}

TEST(TestServer, recv_requestvote_response_dont_increase_votes_for_me_when_term_is_not_equal)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 3);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));
}

TEST(TestServer, recv_requestvote_response_increase_votes_for_me)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));
    EXPECT_EQ(1, raft_get_current_term(r));

    raft_become_candidate(r);
    EXPECT_EQ(2, raft_get_current_term(r));
    EXPECT_EQ(1, raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 1;
    int e = raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_EQ(0, e);
    EXPECT_EQ(2, raft_get_nvotes_for_me(r));
}

TEST(TestServer, recv_requestvote_response_must_be_candidate_to_receive)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));

    raft_become_leader(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_EQ(0, raft_get_nvotes_for_me(r));
}

/* Reply false if term < currentTerm (§5.1) */
TEST(TestServer, recv_requestvote_reply_false_if_term_less_than_current_term)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 2);

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    int e = raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(0, e);
    EXPECT_EQ(0, rvr.vote_granted);
}

TEST(TestServer, leader_recv_requestvote_does_not_step_down)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_vote(r, raft_get_node(r, 1));
    raft_become_leader(r);
    EXPECT_EQ(1, raft_is_leader(r));

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(1, raft_get_current_leader(r));
}

/* Reply true if term >= currentTerm (§5.1) */
TEST(TestServer, recv_requestvote_reply_true_if_term_greater_than_or_equal_to_current_term)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* term is less than current term */
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    EXPECT_EQ(1, rvr.vote_granted);
}

TEST(TestServer, recv_requestvote_reset_timeout)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_set_election_timeout(r, 1000);
    raft_periodic(r, 900);

    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(1, rvr.vote_granted);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));
}

TEST(TestServer, recv_requestvote_candidate_step_down_if_term_is_higher_than_current_term)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_become_candidate(r);
    raft_set_current_term(r, 1);
    EXPECT_EQ(1, raft_get_voted_for(r));

    /* current term is less than term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.candidate_id = 2;
    rv.term = 2;
    rv.last_log_idx = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(1, raft_is_follower(r));
    EXPECT_EQ(2, raft_get_current_term(r));
    EXPECT_EQ(2, raft_get_voted_for(r));
}

TEST(TestServer, recv_requestvote_depends_on_candidate_id)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_become_candidate(r);
    raft_set_current_term(r, 1);
    EXPECT_EQ(1, raft_get_voted_for(r));

    /* current term is less than term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.candidate_id = 3;
    rv.term = 2;
    rv.last_log_idx = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, NULL, &rv, &rvr);
    EXPECT_EQ(1, raft_is_follower(r));
    EXPECT_EQ(2, raft_get_current_term(r));
    EXPECT_EQ(3, raft_get_voted_for(r));
}

/* If votedFor is null or candidateId, and candidate's log is at
 * least as up-to-date as local log, grant vote (§5.2, §5.4) */
TEST(TestServer, recv_requestvote_dont_grant_vote_if_we_didnt_vote_for_this_candidate)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 0, 0);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* vote for self */
    raft_vote_for_nodeid(r, 1);

    msg_requestvote_t rv = {0};
    rv.term = 1;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(0, rvr.vote_granted);

    /* vote for ID 0 */
    raft_vote_for_nodeid(r, 0);
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(0, rvr.vote_granted);
}

TEST(TestFollower, becomes_follower_is_follower)
{
    raft_server_t *r = raft_new();
    raft_become_follower(r);
    EXPECT_TRUE(raft_is_follower(r));
}

TEST(TestFollower, becomes_follower_does_not_clear_voted_for)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);

    raft_vote(r, raft_get_node(r, 1));
    EXPECT_EQ(1, raft_get_voted_for(r));
    raft_become_follower(r);
    EXPECT_EQ(1, raft_get_voted_for(r));
}

/* 5.1 */
TEST(TestFollower, recv_appendentries_reply_false_if_term_less_than_currentterm)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    /* no leader known at this point */
    EXPECT_EQ(-1, raft_get_current_leader(r));

    /* term is low */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;

    /*  higher current term */
    raft_set_current_term(r, 5);
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    EXPECT_FALSE(aer.success);
    /* rejected appendentries doesn't change the current leader. */
    EXPECT_EQ(-1, raft_get_current_leader(r));
}

TEST(TestFollower, recv_appendentries_does_not_need_node)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    msg_appendentries_t ae = {0};
    ae.term = 1;
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, NULL, &ae, &aer);
    EXPECT_TRUE(aer.success);
}

/* TODO: check if test case is needed */
TEST(TestFollower, recv_appendentries_updates_currentterm_if_term_gt_currentterm)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  older currentterm */
    raft_set_current_term(r, 1);
    EXPECT_EQ(-1, raft_get_current_leader(r));

    /*  newer term for appendentry */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    /* no prev log idx */
    ae.prev_log_idx = 0;
    ae.term = 2;

    /*  appendentry has newer term, so we change our currentterm */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_TRUE(aer.success);
    EXPECT_EQ(2, aer.term);
    /* term has been updated */
    EXPECT_EQ(2, raft_get_current_term(r));
    /* and leader has been updated */
    EXPECT_EQ(2, raft_get_current_leader(r));
}

TEST(TestFollower, recv_appendentries_does_not_log_if_no_entries_are_specified)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    EXPECT_EQ(0, raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    ae.n_entries = 0;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(0, raft_get_log_count(r));
}

TEST(TestFollower, recv_appendentries_increases_log)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_entry_t ety = {0};
    msg_appendentries_response_t aer;
    char *str = "aaa";

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    EXPECT_EQ(0, raft_get_log_count(r));

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
    EXPECT_TRUE(aer.success);
    EXPECT_EQ(1, raft_get_log_count(r));
    raft_entry_t* log = raft_get_entry_from_idx(r, 1);
    EXPECT_EQ(2, log->term);
}

/*  5.3 */
TEST(TestFollower, recv_appendentries_reply_false_if_doesnt_have_log_at_prev_log_idx_which_matches_prev_log_term)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_t ety = {0};
    char *str = "aaa";

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

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
    EXPECT_FALSE(aer.success);
}

static raft_entry_t* __create_mock_entries_for_conflict_tests(raft_server_t* r, char** strs)
{
    raft_entry_t ety = {0};
    raft_entry_t *ety_appended;

    /* increase log size */
    char *str1 = strs[0];
    ety.data.buf = str1;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    EXPECT_EQ(1, raft_get_log_count(r));

    /* this log will be overwritten by a later appendentries */
    char *str2 = strs[1];
    ety.data.buf = str2;
    ety.data.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    EXPECT_EQ(2, raft_get_log_count(r));
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 2)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, str2, 3));

    /* this log will be overwritten by a later appendentries */
    char *str3 = strs[2];
    ety.data.buf = str3;
    ety.data.len = 3;
    ety.id = 3;
    ety.term = 1;
    raft_append_entry(r, &ety);
    EXPECT_EQ(3, raft_get_log_count(r));
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 3)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, str3, 3));

    return ety_appended;
}

/* 5.3 */
TEST(TestFollower, recv_appendentries_delete_entries_if_conflict_with_new_entries_via_prev_log_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __create_mock_entries_for_conflict_tests(r, strs);

    /* pass a appendentry that is newer  */
    msg_entry_t mety = {0};

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    /* entries from 2 onwards will be overwritten by this appendentries message */
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

    /* str4 has overwritten the last 2 entries */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(2, raft_get_log_count(r));
    /* str1 is still there */
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 1)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, strs[0], 3));
    /* str4 has overwritten the last 2 entries */
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 2)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, str4, 3));
}

TEST(TestFollower, recv_appendentries_delete_entries_if_conflict_with_new_entries_via_prev_log_idx_at_idx_0)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __create_mock_entries_for_conflict_tests(r, strs);

    /* pass a appendentry that is newer  */
    msg_entry_t mety = {0};

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    /* ALL append entries will be overwritten by this appendentries message */
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    /* include one entry */
    memset(&mety, 0, sizeof(msg_entry_t));
    char *str4 = "444";
    mety.data.buf = str4;
    mety.data.len = 3;
    mety.id = 4;
    ae.entries = &mety;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(1, raft_get_log_count(r));
    /* str1 is gone */
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 1)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, str4, 3));
}

TEST(TestFollower, recv_appendentries_delete_entries_if_conflict_with_new_entries_greater_than_prev_log_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended;
    
    __create_mock_entries_for_conflict_tests(r, strs);
    EXPECT_EQ(3, raft_get_log_count(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t) * 1);
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(2, raft_get_log_count(r));
    EXPECT_NE(nullptr, (ety_appended = raft_get_entry_from_idx(r, 1)));
    EXPECT_EQ(0, strncmp((const char*)ety_appended->data.buf, strs[0], 3));
}

// TODO: add TestRaft_follower_recv_appendentries_delete_entries_if_term_is_different

TEST(TestFollower, recv_appendentries_add_new_entries_not_already_in_log)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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

    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(2, raft_get_log_count(r));
}

TEST(TestFollower, recv_appendentries_does_not_add_dupe_entries_already_in_log)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(1, raft_get_log_count(r));

    /* lets get the server to append 2 now! */
    e[1].id = 2;
    ae.n_entries = 2;
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(2, raft_get_log_count(r));
}

typedef enum {
    __RAFT_NO_ERR = 0,
    __RAFT_LOG_OFFER_ERR,
    __RAFT_LOG_POP_ERR
} __raft_error_type_e;

typedef struct {
    __raft_error_type_e type;
    int idx;
} __raft_error_t;

static int __raft_log_offer_error(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx)
{
    __raft_error_t *error = (__raft_error_t*)user_data;

    if (__RAFT_LOG_OFFER_ERR == error->type && entry_idx == error->idx)
        return RAFT_ERR_NOMEM;
    return 0;
}

static int __raft_log_pop_error(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx)
{
    __raft_error_t *error = (__raft_error_t*)user_data;

    if (__RAFT_LOG_POP_ERR == error->type && entry_idx == error->idx)
        return RAFT_ERR_NOMEM;
    return 0;
}

TEST(TestFollower, recv_appendentries_partial_failures)
{
    raft_cbs_t funcs = { 0 };

    funcs.persist_term = __raft_persist_term;
    funcs.log_offer = __raft_log_offer_error;
    funcs.log_pop = __raft_log_pop_error;

    raft_server_t *r = raft_new();
    __raft_error_t error = {};
    raft_set_callbacks(r, &funcs, &error);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* Append entry 1 and 2 of term 1. */
    raft_entry_t ety = {};
    ety.data.buf = "1aa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    ety.data.buf = "1bb";
    ety.data.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    EXPECT_EQ(2, raft_get_current_idx(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* To be received: entry 2 and 3 of term 2. */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].term = 2;
    e[0].id = 2;
    e[1].term = 2;
    e[1].id = 3;
    ae.entries = e;
    ae.n_entries = 2;

    /* Ask log_pop to fail at entry 2. */
    error.type = __RAFT_LOG_POP_ERR;
    error.idx = 2;
    memset(&aer, 0, sizeof(aer));
    int err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(RAFT_ERR_NOMEM, err);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(1, aer.current_idx);
    EXPECT_EQ(2, raft_get_current_idx(r));
    raft_entry_t *tmp = raft_get_entry_from_idx(r, 2);
    EXPECT_NE(nullptr, tmp);
    EXPECT_EQ(1, tmp->term);

    /* Ask log_offer to fail at entry 3. */
    error.type = __RAFT_LOG_OFFER_ERR;
    error.idx = 3;
    memset(&aer, 0, sizeof(aer));
    err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(RAFT_ERR_NOMEM, err);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(2, aer.current_idx);
    EXPECT_EQ(2, raft_get_current_idx(r));
    tmp = raft_get_entry_from_idx(r, 2);
    EXPECT_NE(nullptr, tmp);
    EXPECT_EQ(2, tmp->term);

    /* No more errors. */
    memset(&error, 0, sizeof(error));
    memset(&aer, 0, sizeof(aer));
    err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_EQ(0, err);
    EXPECT_EQ(1, aer.success);
    EXPECT_EQ(3, aer.current_idx);
    EXPECT_EQ(3, raft_get_current_idx(r));
}

/* If leaderCommit > commitidx, set commitidx =
 *  min(leaderCommit, last log idx) */
TEST(TestFollower, recv_appendentries_set_commitidx_to_prevLogIdx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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

    EXPECT_EQ(1, aer.success);
    /* set to 4 because commitIDX is lower */
    EXPECT_EQ(4, raft_get_commit_idx(r));
}

TEST(TestFollower, recv_appendentries_set_commitidx_to_LeaderCommit)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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

    EXPECT_EQ(1, aer.success);
    /* set to 3 because leaderCommit is lower */
    EXPECT_EQ(3, raft_get_commit_idx(r));
}

TEST(TestFollower, recv_appendentries_failure_includes_current_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_entry_t ety = {0};
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

    EXPECT_FALSE(aer.success);
    EXPECT_EQ(1, aer.current_idx);

    /* try again with a higher current_idx */
    memset(&aer, 0, sizeof(aer));
    ety.id = 2;
    raft_append_entry(r, &ety);
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_FALSE(aer.success);
    EXPECT_EQ(2, aer.current_idx);
}

TEST(TestFollower, becomes_candidate_when_election_timeout_occurs)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /*  1 second election timeout */
    raft_set_election_timeout(r, 1000);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  max election timeout have passed */
    raft_periodic(r, max_election_timeout(1000) + 1);

    /* is a candidate now */
    EXPECT_TRUE(raft_is_candidate(r));
}

/* Candidate 5.2 */
TEST(TestFollower, dont_grant_vote_if_candidate_has_a_less_complete_log)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    ety.term = 2;
    raft_append_entry(r, &ety);

    /* vote not granted */
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_FALSE(rvr.vote_granted);

    /* approve vote, because last_log_term is higher */
    raft_set_current_term(r, 2);
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 3;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    EXPECT_EQ(1, rvr.vote_granted);
}

TEST(TestFollower, recv_appendentries_heartbeat_does_not_overwrite_logs)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* The server sends a follow up AE
     * NOTE: the server has received a response from the last AE so
     * prev_log_idx has been incremented */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive a heartbeat
     * NOTE: the leader hasn't received the response to the last AE so it can 
     * only assume prev_Log_idx is still 1 */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 1;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    EXPECT_TRUE(aer.success);
    EXPECT_EQ(5, raft_get_current_idx(r));
}

TEST(TestFollower, recv_appendentries_does_not_deleted_commited_entries)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[5];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* Follow up AE. Node responded with success */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    ae.entries = e;
    ae.n_entries = 4;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* The server sends a follow up AE */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 5);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    e[4].term = 1;
    e[4].id = 6;
    ae.entries = e;
    ae.n_entries = 5;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    EXPECT_TRUE(aer.success);
    EXPECT_EQ(6, raft_get_current_idx(r));
    EXPECT_EQ(4, raft_get_commit_idx(r));

    /* The server sends a follow up AE.
     * This appendentry forces the node to check if it's going to delete
     * commited logs */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 3;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 5);
    e[0].id = 1;
    e[0].id = 5;
    e[1].term = 1;
    e[1].id = 6;
    e[2].term = 1;
    e[2].id = 7;
    ae.entries = e;
    ae.n_entries = 3;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    EXPECT_TRUE(aer.success);
    EXPECT_EQ(6, raft_get_current_idx(r));
}

TEST(TestCandidate, becomes_candidate_is_candidate)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_become_candidate(r);
    EXPECT_TRUE(raft_is_candidate(r));
}

/* Candidate 5.2 */
TEST(TestFollower, becoming_candidate_increments_current_term)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    EXPECT_EQ(0, raft_get_current_term(r));
    raft_become_candidate(r);
    EXPECT_EQ(1, raft_get_current_term(r));
}

/* Candidate 5.2 */
TEST(TestFollower, becoming_candidate_votes_for_self)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    EXPECT_EQ(-1, raft_get_voted_for(r));
    raft_become_candidate(r);
    EXPECT_EQ(raft_get_nodeid(r), raft_get_voted_for(r));
    EXPECT_EQ(1, raft_get_nvotes_for_me(r));
}

/* Candidate 5.2 */
TEST(TestFollower, becoming_candidate_resets_election_timeout)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_election_timeout(r, 1000);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    raft_periodic(r, 900);
    EXPECT_EQ(900, raft_get_timeout_elapsed(r));

    raft_become_candidate(r);
    /* time is selected randomly */
    EXPECT_TRUE(raft_get_timeout_elapsed(r) < 1000);
}

TEST(TestFollower, recv_appendentries_resets_election_timeout)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_election_timeout(r, 1000);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 1);

    raft_periodic(r, 900);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 1), &ae, &aer);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));
}

/* Candidate 5.2 */
TEST(TestFollower, becoming_candidate_requests_votes_from_other_servers)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = sender_requestvote;

    msg_requestvote_t* rv;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* set term so we can check it gets included in the outbound message */
    raft_set_current_term(r, 2);

    /* becoming candidate triggers vote requests */
    raft_become_candidate(r);

    /* 2 nodes = 2 vote requests */
    rv = (msg_requestvote_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, rv);
    EXPECT_NE(2, rv->term);
    EXPECT_EQ(3, rv->term);
    /*  TODO: there should be more items */
    rv = (msg_requestvote_t *)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, rv);
    EXPECT_EQ(3, rv->term);
}

/* Candidate 5.2 */
TEST(TestCandidate, election_timeout_and_no_leader_results_in_new_election)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t vr;
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 0;
    vr.vote_granted = 1;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* server wants to be leader, so becomes candidate */
    raft_become_candidate(r);
    EXPECT_EQ(1, raft_get_current_term(r));

    /* clock over (ie. max election timeout + 1), causing new election */
    raft_periodic(r, max_election_timeout(1000) + 1);
    EXPECT_EQ(2, raft_get_current_term(r));

    /*  receiving this vote gives the server majority */
//    raft_recv_requestvote_response(r,1,&vr);
//    EXPECT_TRUE(raft_is_leader(r));
}

/* Candidate 5.2 */
TEST(TestCandidate, receives_majority_of_votes_becomes_leader)
{
    msg_requestvote_response_t vr;

    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    EXPECT_EQ(5, raft_get_num_nodes(r));

    /* vote for self */
    raft_become_candidate(r);
    EXPECT_EQ(1, raft_get_current_term(r));
    EXPECT_EQ(1, raft_get_nvotes_for_me(r));

    /* a vote for us */
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.vote_granted = 1;
    /* get one vote */
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &vr);
    EXPECT_EQ(2, raft_get_nvotes_for_me(r));
    EXPECT_FALSE(raft_is_leader(r));

    /* get another vote
     * now has majority (ie. 3/5 votes) */
    raft_recv_requestvote_response(r, raft_get_node(r, 3), &vr);
    EXPECT_TRUE(raft_is_leader(r));
}

/* Candidate 5.2 */
TEST(TestCandidate, will_not_respond_to_voterequest_if_it_has_already_voted)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_vote(r, raft_get_node(r, 1));

    memset(&rv, 0, sizeof(msg_requestvote_t));
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    /* we've vote already, so won't respond with a vote granted... */
    EXPECT_FALSE(rvr.vote_granted);
}

/* Candidate 5.2 */
TEST(TestCandidate, requestvote_includes_logidx)
{
    raft_cbs_t funcs = {0};
    funcs.send_requestvote = sender_requestvote;
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_callbacks(r, &funcs, sender);
    raft_set_current_term(r, 5);
    /* 3 entries */
    raft_entry_t ety = {0};
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

    msg_requestvote_t* rv = (msg_requestvote_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, rv);
    EXPECT_EQ(3, rv->last_log_idx);
    EXPECT_EQ(5, rv->term);
    EXPECT_EQ(3, rv->last_log_term);
    EXPECT_EQ(1, rv->candidate_id);
}

TEST(TestCandidate, recv_requestvote_response_becomes_follower_if_current_term_is_less_than_term)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    EXPECT_FALSE(raft_is_follower(r));
    EXPECT_EQ(-1, raft_get_current_leader(r));
    EXPECT_EQ(1, raft_get_current_term(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 0;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_TRUE(raft_is_follower(r));
    EXPECT_EQ(2, raft_get_current_term(r));
    EXPECT_EQ(-1, raft_get_voted_for(r));
}

/* Candidate 5.2 */
TEST(TestCandidate, recv_appendentries_frm_leader_results_in_follower)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    EXPECT_FALSE(raft_is_follower(r));
    EXPECT_EQ(-1, raft_get_current_leader(r));
    EXPECT_EQ(0, raft_get_current_term(r));

    /* receive recent appendentries */
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_TRUE(raft_is_follower(r));
    /* after accepting a leader, it's available as the last known leader */
    EXPECT_EQ(2, raft_get_current_leader(r));
    EXPECT_EQ(1, raft_get_current_term(r));
    EXPECT_EQ(-1, raft_get_voted_for(r));
}

/* Candidate 5.2 */
TEST(TestCandidate, recv_appendentries_from_same_term_results_in_step_down)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;
    funcs.send_requestvote = __raft_send_requestvote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);
    raft_become_candidate(r);
    EXPECT_FALSE(raft_is_follower(r));
    EXPECT_EQ(1, raft_get_voted_for(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    EXPECT_FALSE(raft_is_candidate(r));

    /* The election algorithm requires that votedFor always contains the node
     * voted for in the current term (if any), which is why it is persisted.
     * By resetting that to -1 we have the following problem:
     *
     *  Node self, other1 and other2 becomes candidates
     *  Node other1 wins election
     *  Node self gets appendentries
     *  Node self resets votedFor
     *  Node self gets requestvote from other2
     *  Node self votes for Other2
    */
    EXPECT_EQ(1, raft_get_voted_for(r));
}

TEST(TestLeader, becomes_leader_is_leader)
{
    raft_server_t *r = raft_new();
    raft_become_leader(r);
    EXPECT_TRUE(raft_is_leader(r));
}

TEST(TestLeader, becomes_leader_does_not_clear_voted_for)
{
    raft_cbs_t funcs = { 0 };
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_vote(r, raft_get_node(r, 1));
    EXPECT_EQ(1, raft_get_voted_for(r));
    raft_become_leader(r);
    EXPECT_EQ(1, raft_get_voted_for(r));
}

TEST(TestLeader, when_becomes_leader_all_nodes_have_nextidx_equal_to_lastlog_idx_plus_1)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

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
        EXPECT_EQ(raft_get_current_idx(r) + 1, raft_node_get_next_idx(p));
    }
}

/* 5.2 */
TEST(TestLeader, when_it_becomes_a_leader_sends_empty_appendentries)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
}

/* 5.2
 * Note: commit means it's been appended to the log, not applied to the FSM */
TEST(TestLeader, responds_to_entry_msg_when_entry_is_committed)
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
    msg_entry_t ety = {0};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(1, raft_get_log_count(r));

    /* trigger response through commit */
    raft_apply_entry(r);
}

void TestRaft_non_leader_recv_entry_msg_fails()
{
    msg_entry_response_t cr;

    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /* entry message */
    msg_entry_t ety = {0};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    int e = raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(RAFT_ERR_NOT_LEADER, e);
}

/* 5.3 */
TEST(TestLeader, sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_node_t* p = raft_get_node(r, 2);
    raft_node_set_next_idx(p, 4);

    /* receive appendentries messages */
    raft_send_appendentries(r, p);
    msg_appendentries_t* ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
}

TEST(TestLeader, sends_appendentries_with_leader_commit)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    int i;

    for (i=0; i<10; i++)
    {
        raft_entry_t ety = {0};
        ety.term = 1;
        ety.id = 1;
        ety.data.buf = "aaa";
        ety.data.len = 3;
        raft_append_entry(r, &ety);
    }

    raft_set_commit_idx(r, 10);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    EXPECT_EQ(10, ae->leader_commit);
}

TEST(TestLeader, sends_appendentries_with_prevLogIdx)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1); /* me */
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    EXPECT_EQ(0, ae->prev_log_idx);

    raft_node_t* n = raft_get_node(r, 2);

    /* add 1 entry */
    /* receive appendentries messages */
    raft_entry_t ety = {0};
    ety.term = 2;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_node_set_next_idx(n, 1);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    EXPECT_EQ(0, ae->prev_log_idx);
    EXPECT_EQ(1, ae->n_entries);
    EXPECT_EQ(100, ae->entries[0].id);
    EXPECT_EQ(2, ae->entries[0].term);

    /* set next_idx */
    /* receive appendentries messages */
    raft_node_set_next_idx(n, 2);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    EXPECT_EQ(1, ae->prev_log_idx);
}

TEST(TestLeader, sends_appendentries_when_node_has_next_idx_of_0)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = (msg_appendentries_t*)sender_poll_msg_data(sender);

    /* add an entry */
    /* receive appendentries messages */
    raft_node_t* n = raft_get_node(r, 2);
    raft_node_set_next_idx(n, 1);
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_send_appendentries(r, n);
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    EXPECT_EQ(0, ae->prev_log_idx);
}

/* 5.3 */
TEST(TestLeader, retries_appendentries_with_decremented_NextIdx_log_inconsistency)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t* ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
}

/*
 * If there exists an N such that N > commitidx, a majority
 * of matchidx[i] = N, and log[N].term == currentTerm:
 * set commitidx = N (§5.2, §5.4).  */
TEST(TestLeader, append_entry_to_log_increases_idxno)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;
    msg_entry_t ety = {0};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_LEADER);
    EXPECT_EQ(0, raft_get_log_count(r));

    raft_recv_entry(r, &ety, &cr);
    EXPECT_EQ(1, raft_get_log_count(r));
}

#if 0
// TODO no support for duplicates
void T_estRaft_leader_doesnt_append_entry_if_unique_id_is_duplicate()
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
    EXPECT_EQ(0, raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    EXPECT_EQ(1, raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    EXPECT_EQ(1, raft_get_log_count(r));
}
#endif

TEST(TestLeader, recv_appendentries_response_increase_commit_idx_when_majority_have_entry_and_atleast_one_newer_entry)
{
    raft_cbs_t funcs = {0};
    funcs.applylog = __raft_applylog;
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
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
    raft_entry_t ety = {0};
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
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    EXPECT_EQ(1, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(1, raft_get_last_applied_idx(r));

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
    EXPECT_EQ(1, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    EXPECT_EQ(2, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(2, raft_get_last_applied_idx(r));
}

TEST(TestLeader, recv_appendentries_response_set_has_sufficient_logs_for_node)
{
    raft_cbs_t funcs = { 0 };
    funcs.applylog = __raft_applylog;
    funcs.persist_term = __raft_persist_term;
    funcs.node_has_sufficient_logs = __raft_node_has_sufficient_logs;
    msg_appendentries_response_t aer;

    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_node_t* node = raft_add_node(r, NULL, 5, 0);

    int has_sufficient_logs_flag = 0;
    raft_set_callbacks(r, &funcs, &has_sufficient_logs_flag);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
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

    raft_send_appendentries(r, raft_get_node(r, 5));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 1;

    raft_node_set_voting(node, 0);
    raft_recv_appendentries_response(r, node, &aer);
    EXPECT_EQ(1, has_sufficient_logs_flag);

    raft_recv_appendentries_response(r, node, &aer);
    EXPECT_EQ(1, has_sufficient_logs_flag);
}

TEST(TestLeader, recv_appendentries_response_increase_commit_idx_using_voting_nodes_majority)
{
    raft_cbs_t funcs = {0};
    funcs.applylog = __raft_applylog;
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_non_voting_node(r, NULL, 4, 0);
    raft_add_non_voting_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(1, raft_get_commit_idx(r));
    /* leader will now have majority followers who have appended this log */
    raft_periodic(r, 1);
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
}

TEST(TestLeader, recv_appendentries_response_duplicate_does_not_decrement_match_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
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
    raft_entry_t ety = {0};
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

    /* receive msg 1 */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(1, raft_node_get_match_idx(raft_get_node(r, 2)));

    /* receive msg 2 */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(2, raft_node_get_match_idx(raft_get_node(r, 2)));

    /* receive msg 1 - because of duplication ie. unreliable network */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(2, raft_node_get_match_idx(raft_get_node(r, 2)));
}

TEST(TestLeader, recv_appendentries_response_do_not_increase_commit_idx_because_of_old_terms_with_majority)
{
    raft_cbs_t funcs = {0};
    funcs.applylog = __raft_applylog;
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();

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
    raft_entry_t ety = {0};
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
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(0, raft_get_last_applied_idx(r));

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
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(0, raft_get_last_applied_idx(r));

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
    EXPECT_EQ(0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    EXPECT_EQ(3, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(1, raft_get_last_applied_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(2, raft_get_last_applied_idx(r));
    raft_periodic(r, 1);
    EXPECT_EQ(3, raft_get_last_applied_idx(r));
}

TEST(TestLeader, recv_appendentries_response_jumps_to_lower_next_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {0};
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
    EXPECT_EQ(5, raft_node_get_next_idx(node));
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));
    EXPECT_EQ(4, ae->prev_log_term);
    EXPECT_EQ(4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(2, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));
    EXPECT_EQ(1, ae->prev_log_term);
    EXPECT_EQ(1, ae->prev_log_idx);

    EXPECT_EQ(nullptr, sender_poll_msg_data(sender));
}

TEST(TestLeader, recv_appendentries_response_decrements_to_lower_next_idx)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {0};
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
    EXPECT_EQ(5, raft_node_get_next_idx(node));
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));
    EXPECT_EQ(4, ae->prev_log_term);
    EXPECT_EQ(4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(4, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));
    EXPECT_EQ(3, ae->prev_log_term);
    EXPECT_EQ(3, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(3, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    EXPECT_NE(nullptr, (ae = (msg_appendentries_t*)sender_poll_msg_data(sender)));
    EXPECT_EQ(2, ae->prev_log_term);
    EXPECT_EQ(2, ae->prev_log_idx);

    EXPECT_EQ(nullptr, sender_poll_msg_data(sender));
}

TEST(TestLeader, recv_appendentries_response_retry_only_if_leader)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
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
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));

    EXPECT_NE(nullptr, sender_poll_msg_data(sender));
    EXPECT_NE(nullptr, sender_poll_msg_data(sender));

    raft_become_follower(r);

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    EXPECT_EQ(RAFT_ERR_NOT_LEADER, raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer));
    EXPECT_EQ(nullptr, sender_poll_msg_data(sender));
}

TEST(TestLeader, recv_appendentries_response_without_node_fails)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 0;
    aer.first_idx = 0;
    EXPECT_EQ(-1, raft_recv_appendentries_response(r, NULL, &aer));
}

TEST(TestLeader, recv_entry_resets_election_timeout)
{
    raft_server_t *r = raft_new();
    raft_set_election_timeout(r, 1000);
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_periodic(r, 900);

    /* entry message */
    msg_entry_t mety = {0};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));
}

TEST(TestLeader, recv_entry_is_committed_returns_0_if_not_committed)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {0};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    EXPECT_EQ(0, raft_msg_entry_response_committed(r, &cr));

    raft_set_commit_idx(r, 1);
    EXPECT_EQ(1, raft_msg_entry_response_committed(r, &cr));
}

TEST(TestLeader, recv_entry_is_committed_returns_neg_1_if_invalidated)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {0};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    EXPECT_EQ(0, raft_msg_entry_response_committed(r, &cr));
    EXPECT_EQ(1, cr.term);
    EXPECT_EQ(1, cr.idx);
    EXPECT_EQ(1, raft_get_current_idx(r));
    EXPECT_EQ(0, raft_get_commit_idx(r));

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
    EXPECT_TRUE(aer.success);
    EXPECT_EQ(1, raft_get_current_idx(r));
    EXPECT_EQ(1, raft_get_commit_idx(r));
    EXPECT_EQ(-1, raft_msg_entry_response_committed(r, &cr));
}

TEST(TestLeader, recv_entry_fails_if_prevlogidx_less_than_commit)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = __raft_send_appendentries;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    EXPECT_EQ(0, raft_msg_entry_response_committed(r, &cr));
    EXPECT_EQ(2, cr.term);
    EXPECT_EQ(1, cr.idx);
    EXPECT_EQ(1, raft_get_current_idx(r));
    EXPECT_EQ(0, raft_get_commit_idx(r));

    raft_set_commit_idx(r, 1);

    /* append entry that invalidates entry message */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.leader_commit = 1;
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
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
    EXPECT_EQ(0, aer.success);
}

TEST(TestLeader, recv_entry_does_not_send_new_appendentries_to_slow_nodes)
{
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* make the node slow */
    raft_node_set_next_idx(raft_get_node(r, 2), 1);

    /* append entries */
    raft_entry_t ety = {0};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    /* entry message */
    msg_entry_t mety = {0};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);

    /* check if the slow node got sent this appendentries */
    msg_appendentries_t* ae;
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_EQ(nullptr, ae);
}

TEST(TestLeader, recv_appendentries_response_failure_does_not_set_node_nextid_to_0)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {0};
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
    EXPECT_EQ(1, raft_node_get_next_idx(p));
    raft_recv_appendentries_response(r, p, &aer);
    EXPECT_EQ(1, raft_node_get_next_idx(p));
}

TEST(TestLeader, recv_appendentries_response_increment_idx_of_node)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    raft_node_t* p = raft_get_node(r, 2);
    EXPECT_EQ(1, raft_node_get_next_idx(p));

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 0;
    aer.first_idx = 0;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(1, raft_node_get_next_idx(p));
}

TEST(TestLeader, recv_appendentries_response_drop_message_if_term_is_old)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);

    raft_node_t* p = raft_get_node(r, 2);
    EXPECT_EQ(1, raft_node_get_next_idx(p));

    /* receive OLD mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(1, raft_node_get_next_idx(p));
}

TEST(TestLeader, recv_appendentries_response_steps_down_if_term_is_newer)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);

    raft_node_t* p = raft_get_node(r, 2);
    EXPECT_EQ(1, raft_node_get_next_idx(p));

    /* receive NEW mock failed responses */
    msg_appendentries_response_t aer;
    aer.term = 3;
    aer.success = 0;
    aer.current_idx = 2;
    aer.first_idx = 0;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    EXPECT_EQ(1, raft_is_follower(r));
    EXPECT_EQ(-1, raft_get_current_leader(r));
}

TEST(TestLeader, recv_appendentries_steps_down_if_newer)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);
    /* check that node 1 considers itself the leader */
    EXPECT_EQ(1, raft_is_leader(r));
    EXPECT_EQ(1, raft_get_current_leader(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 6;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* after more recent appendentries from node 2, node 1 should
    * consider node 2 the leader. */
    EXPECT_EQ(1, raft_is_follower(r));
    EXPECT_EQ(2, raft_get_current_leader(r));
}

TEST(TestLeader, recv_appendentries_steps_down_if_newer_term)
{
    raft_cbs_t funcs = {0};
    funcs.persist_term = __raft_persist_term;

    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 5;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    EXPECT_TRUE(raft_is_follower(r));
}

TEST(TestLeader, sends_empty_appendentries_every_request_timeout)
{
    raft_cbs_t funcs = {0};
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae;
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);

    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_EQ(nullptr, ae);

    /* force request timeout */
    raft_periodic(r, 501);
    ae = (msg_appendentries_t*)sender_poll_msg_data(sender);
    EXPECT_NE(nullptr, ae);
}

/* TODO: If a server receives a request with a stale term number, it rejects the request. */
#if 0
void T_estRaft_leader_sends_appendentries_when_receive_entry_msg()
#endif

TEST(TestLeader, recv_requestvote_responds_without_granting)
{
    raft_cbs_t funcs = {0};
    funcs.persist_vote = __raft_persist_vote;
    funcs.persist_term = __raft_persist_term;
    funcs.send_requestvote = __raft_send_requestvote;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_TRUE(raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    EXPECT_FALSE(rvr.vote_granted);
}

TEST(TestLeader, recv_requestvote_responds_with_granting_if_term_is_higher)
{
    raft_cbs_t funcs = {0};
    funcs.persist_vote = __raft_persist_vote;
    funcs.persist_term = __raft_persist_term;
    funcs.send_requestvote = __raft_send_requestvote;
    funcs.send_appendentries = sender_appendentries;

    void *sender = sender_new(NULL);
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    EXPECT_EQ(0, raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    EXPECT_TRUE(raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    EXPECT_TRUE(raft_is_follower(r));
}
