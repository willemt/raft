/******************************************************************************
 *
 * Test implementation of the raft_store interface.
 *
 *****************************************************************************/

#ifndef RAFT_TEST_STORE_H
#define RAFT_TEST_STORE_H

#include "../../include/raft.h"

#include "munit.h"

/* Munit parameter defining after how many API calls the test store should start
   failing and return errors. The default is -1, meaning that no failure will
   ever occur. */
#define TEST_STORE_FAIL_DELAY_PARAM "store-fail-delay"

/* Munit parameter defining the how many consecutive times API calls against the
   test store should fail after they started failing. This parameter has an
   effect only if 'store-fail-delay' is 0 or greater. The default is 1. */
#define TEST_STORE_FAIL_REPEAT_PARAM "store-FAIL-repeat"

void test_store_setup(const MunitParameter params[], raft_store *s);

void test_store_tear_down(raft_store *s);

#endif /* RAFT_TEST_STORE_H */
