
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"

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
    CuAssertTrue(tc, 1 == raft_get_current_index(r));
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
    CuAssertTrue(tc, 500 == raft_get_election_timeout(r));
}

void TestRaft_server_increases_logindex_when_command_appended(CuTest* tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_index(r));
    raft_append_command(r,"aaa", 3);
    CuAssertTrue(tc, 1 == raft_get_current_index(r));
}

// If commitIndex > lastApplied: increment lastApplied, apply
// log[lastApplied] to state machine (§5.3)
void TestRaft_server_increment_lastApplied_when_lastApplied_lt_commitIndex(CuTest* tc)
{
    void *r;

    r = raft_new();
    raft_set_commit_index(r,5);
    raft_set_lastapplied_index(r, 4);
    raft_periodic(r,1);
    CuAssertTrue(tc, 5 == raft_get_lastapplied_index(r));
}

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

