
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "raft.h"

typedef struct {
    int pass;
} raft_peer_t;

void* raft_peer_new()
{
    return NULL;
}

int raft_peer_is_leader(void* peer)
{
    return 0;
}

int raft_peer_get_next_index(void* peer)
{
    return 0;
}

void raft_peer_set_next_index(void* peer, int nextIdx)
{

}
