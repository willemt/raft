#include <assert.h>

#include "../include/raft.h"

void raft_init(struct raft *r, struct raft_io *io)
{
    assert(r != NULL);
    assert(io != NULL);

    r->io = *io;
}

// int raft_close(struct raft *r) {}
