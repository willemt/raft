#include <gtest/gtest.h>
extern "C"
{
#include "raft.h"
#include "raft_private.h"
}

TEST(TestNode, is_voting_by_default)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    EXPECT_TRUE(raft_node_is_voting(p));
}

TEST(TestNode, node_set_nextIdx)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_next_idx(p, 3);
    EXPECT_EQ(3, raft_node_get_next_idx(p));
}
