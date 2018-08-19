#include "../../include/raft.h"

#include "../lib/heap.h"
#include "../lib/io.h"
#include "../lib/munit.h"

/**
 *
 * Helpers
 *
 */

struct fixture
{
    struct raft_heap heap;
    struct raft raft;
};

/**
 *
 * Setup and tear down
 *
 */

static void *setup(const MunitParameter params[], void *user_data)
{
    struct fixture *f = munit_malloc(sizeof *f);

    (void)user_data;

    test_heap_setup(params, &f->heap);
    return f;
}

static void tear_down(void *data)
{
    struct fixture *f = data;
    test_heap_tear_down(&f->heap);
}

/**
 *
 * raft_init
 *
 */

static MunitResult test_init(const MunitParameter params[], void *data)
{
    struct fixture *f = data;
    (void)params;

    return MUNIT_OK;
}

static MunitTest raft_init_tests[] = {
    {"/", test_init, setup, tear_down, 0, NULL},
    {NULL, NULL, NULL, NULL, 0, NULL},
};

/**
 *
 * Test suite
 *
 */

MunitSuite raft_suites[] = {
    {"_init", raft_init_tests, NULL, 1, 0},
    {NULL, NULL, NULL, 0, 0},
};
