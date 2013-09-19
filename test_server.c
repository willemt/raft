
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"

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

void TestRaft_add_peer(CuTest * tc)
{
    void *r;

    r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_num_peers(r));
    raft_add_peer(r,(void*)1);
    CuAssertTrue(tc, 1 == raft_get_num_peers(r));
}

void TestRaft_remove_peer(CuTest * tc)
{
    void *r;
    int peer;

    r = raft_new();
    peer = raft_add_peer(r,(void*)1);
    raft_remove_peer(r,peer);
    CuAssertTrue(tc, 0 == raft_get_num_peers(r));
}

void TestRaft_set_state(CuTest * tc)
{
    void *r;

    r = raft_new();
    raft_set_state(r,RAFT_STATE_LEADER);
    CuAssertTrue(tc, RAFT_STATE_LEADER == raft_get_state(r));
}

