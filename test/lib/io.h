/**
 *
 * Test implementation of the raft_io interface, with fault injection.
 *
 */

#ifndef TEST_IO_H
#define TEST_IO_H

#include "../../include/raft.h"

#include "munit.h"

/**
 * Munit parameter defining after how many API calls the test raft_io
 * implementation should start failing and return errors. The default is -1,
 * meaning that no failure will ever occur.
 */
#define TEST_IO_FAULT_DELAY "io-fault-delay"

/**
 * Munit parameter defining how many consecutive times API calls against the
 * test raft_io implementation should keep failingo after they started
 * failing. This parameter has an effect only if 'store-fail-delay' is 0 or
 * greater. The default is 1, and -1 means "keep failing forever".
 */
#define TEST_IO_FAULT_REPEAT "io-fault-repeat"

void test_io_setup(const MunitParameter params[], struct raft_io *io);

void test_io_tear_down(struct raft_io *io);

/**
 * Fire all pending I/O events.
 */
void test_io_flush(struct raft_io *io);

#endif /* TEST_IO_H */
