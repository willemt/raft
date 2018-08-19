#include <assert.h>

#include "../include/raft.h"

#include "heap.h"

void raft_init(struct raft *r, struct raft_io *io)
{
    assert(r != NULL);
    assert(io != NULL);

    r->io = *io;
    raft__heap_init(&r->heap);

    r->current_term = 0;
    r->voted_for = 0;
}

// int raft_close(struct raft *r) {}
