#include <gtest/gtest.h>
extern "C"
{
#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
#include "mock_send_functions.h"
}

static int __raft_persist_term(
    raft_server_t* raft,
    void *udata,
    const int val
    )
{
    return 0;
}

static int __raft_persist_vote(
    raft_server_t* raft,
    void *udata,
    const int val
    )
{
    return 0;
}

TEST(TestScenario, leader_appears)
{
    unsigned long i, j;
    raft_server_t *r[3];
    void* sender[3];

    raft_cbs_t funcs = { 0 };
    funcs.send_requestvote = sender_requestvote;
    funcs.send_appendentries = sender_appendentries;
    funcs.persist_term = __raft_persist_term;
    funcs.persist_vote = __raft_persist_vote;

    senders_new();

    for (j = 0; j < 3; j++)
        sender[j] = sender_new((void*)j);

    for (j = 0; j < 3; j++)
    {
        r[j] = raft_new();
        sender_set_raft(sender[j], r[j]);
        raft_set_election_timeout(r[j], 500);
        raft_add_node(r[j], sender[0], 1, j==0);
        raft_add_node(r[j], sender[1], 2, j==1);
        raft_add_node(r[j], sender[2], 3, j==2);
        raft_set_callbacks(r[j], &funcs, sender[j]);
    }

    /* NOTE: important for 1st node to send vote request before others */
    raft_periodic(r[0], 1000);

    for (i = 0; i < 20; i++)
    {
one_more_time:

        for (j = 0; j < 3; j++)
            sender_poll_msgs(sender[j]);

        for (j = 0; j < 3; j++)
            if (sender_msgs_available(sender[j]))
                goto one_more_time;

        for (j = 0; j < 3; j++)
            raft_periodic(r[j], 100);
    }

    int leaders = 0;
    for (j = 0; j < 3; j++)
        if (raft_is_leader(r[j]))
            leaders += 1;

    EXPECT_NE(0, leaders);
    EXPECT_EQ(1, leaders);
}
