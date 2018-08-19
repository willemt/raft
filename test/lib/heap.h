/**
 *
 * Add support for fault injection and leak detection to stdlib's malloc() family.
 *
 */

#ifndef TEST_HEAP_H
#define TEST_HEAP_H

#include "../../include/raft.h"

#include "munit.h"

/**
 * Munit parameter defining after how many API calls the test raft_heap
 * implementation should start failing and return errors. The default is -1,
 * meaning that no failure will ever occur.
 */
#define TEST_HEAP_FAULT_DELAY "heap-fault-delay"

/**
 * Munit parameter defining how many consecutive times API calls against the
 * test raft_heap implementation should keep failing after they started
 * failing. This parameter has an effect only if 'store-fail-delay' is 0 or
 * greater. The default is 1, and -1 means "keep failing forever".
 */
#define TEST_HEAP_FAULT_REPEAT "heap-fault-repeat"

void test_heap_setup(const MunitParameter params[], struct raft_heap *h);

void test_heap_tear_down(struct raft_heap *h);

#endif /* TEST_HEAP_H */
