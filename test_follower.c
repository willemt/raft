#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

void TestRaft_follower_increases_log_after_appendentry(CuTest * tc)
{
    void *r, *peer;

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));

    r = raft_new();

    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_current_term(r,1);
    raft_set_state(r,RAFT_STATE_FOLLOWER);

    CuAssertTrue(tc, 0 == raft_get_log_size(r));

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_size(r));
}

void TestRaft_follower_rejects_appendentries_if_idx_and_term_dont_match_preceding_ones(CuTest * tc)
{
    void *r, *peer;

    msg_appendentries_t ae;
    memset(&ae,0,sizeof(msg_appendentries_t));

    r = raft_new();
    /* three nodes */
    peer = raft_add_peer(r,(void*)1);
    raft_add_peer(r,(void*)2);

    raft_set_current_term(r,1);
    raft_set_state(r,RAFT_STATE_FOLLOWER);

    CuAssertTrue(tc, 0 == raft_get_log_size(r));

    raft_recv_appendentries(r,peer,&ae);
    CuAssertTrue(tc, 1 == raft_get_log_size(r));
}

void TestRaft_follower_deletes_logentries_if_revealed_to_be_extraneous_by_new_appendentries(CuTest * tc)
{

}

void TestRaft_follower_resends_command_if_request_from_leader_times_out(CuTest * tc)
{

}

