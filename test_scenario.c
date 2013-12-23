
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "mock_send_functions.h"

void TestRaft_scenario_leader_appears(CuTest * tc)
{
    int i,j;
    raft_server_t *r[3];
    void* sender[3];
    raft_node_configuration_t cfg[] = {
                {(-1),(void*)1},
                {(-1),(void*)2},
                {(-1),(void*)3},
                {(-1),NULL}};

    senders_new();

    for (j=0;j<3;j++)
    {
        r[j] = raft_new();
        sender[j] = sender_new(cfg[j].udata_address);
        sender_set_raft(sender[j], r[j]);
        raft_set_election_timeout(r[j], 500);
        raft_set_configuration(r[j],cfg,j);
        raft_set_callbacks(r[j],&((raft_cbs_t) {
            .send = sender_send,
            .log = NULL
            }), sender[j]);
    }

    for (i=0;i<20;i++)
    {
        one_more_time:

        for (j=0;j<3;j++)
            sender_poll_msgs(sender[j]);

        for (j=0;j<3;j++)
            if (sender_msgs_available(sender[j]))
                    goto one_more_time;

        for (j=0;j<3;j++)
            raft_periodic(r[j], 100);
    }

    int leaders = 0;
    for (j=0;j<3;j++)
    {
        if (raft_is_leader(r[j]))
            leaders += 1;
    }

    CuAssertTrue(tc, 0 != leaders);
    CuAssertTrue(tc, 1 == leaders);
}

