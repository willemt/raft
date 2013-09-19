
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"

void TestRaft_peer_set_nextIdx(CuTest * tc)
{
    void *p;

    p = raft_peer_new();
    raft_peer_set_next_index(p,3);
    CuAssertTrue(tc, 3 == raft_peer_get_next_index(p));
}

