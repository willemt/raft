/**
 *
 * Wrapper around stdlib's malloc() family.
 *
 */

#ifndef RAFT_HEAP_H
#define RAFT_HEAP_H

#include "../include/raft.h"

void raft__heap_init(struct raft_heap *h);

#endif /* RAFT_HEAP_H */
