#include <stdbool.h>

#include "store.h"

/* Set to 1 to enable logging. */
#if 0
#define __log(MSG_logf(MUNIT_LOG_DEBUG, MSG__)
#define __logf(MSG, ...) munit_logf(MUNIT_LOG_DEBUG, MSG, __VA_ARGS__)
#else
#define __log(MSG)
#define __logf(MSG, ...)
#endif

struct test_store__data
{
    /* Term and vote */
    uint64_t term;
    uint32_t voted_for;

    /* Log */
    raft_entry *entries; /* Entries array */
    uint64_t first;      /* Index of the first entry */
    size_t n;            /* Size of the entries array */

    /* Failure state */
    int counter;     /* Number of times one the store APIs was called. */
    int fail_delay;  /* Value of 'store-fail-delay', or -1 */
    int fail_repeat; /* Value of 'store-fail-repeat', or 1 */
};

/* Initialize the stora data. */
static void __init(struct test_store__data *sd)
{
    sd->term = 0;
    sd->voted_for = 0;

    sd->entries = NULL;
    sd->first = 0;
    sd->n = 0;

    sd->counter = 0;
    sd->fail_delay = -1;
    sd->fail_repeat = -1;
}

/* Return whether a failure should be triggered. */
static bool __should_fail(struct test_store__data *sd)
{
    return (sd->counter >= sd->fail_delay &&
            sd->counter - sd->fail_delay < sd->fail_repeat);
}

static void __set_term(void *data, uint64_t term, void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;

    munit_assert(data != NULL);

    if (__should_fail(sd)) {
        __logf("store: fail to set term %ld", term);
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __logf("store: set term %ld", term);
    sd->term = term;

out:
    sd->counter++;
    cb(rv);
}

static void __set_vote(void *data, uint64_t node_id, void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;

    munit_assert(data != NULL);

    if (__should_fail(sd)) {
        __logf("store: fail to set vote for %ld", node_id);
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __logf("store: set vote for %ld", node_id);
    sd->voted_for = node_id;

out:
    sd->counter++;
    cb(rv);
}

static void __append_entries(void *data,
                             raft_entry *entries,
                             size_t n,
                             void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    raft_entry *new_entries;
    int rv = 0;

    munit_assert(data != NULL);
    munit_assert_int(n, >, 0);

    if (__should_fail(sd)) {
        __logf("store: fail to append %d log entries", n);
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __logf("store: append %d log entries", n);

    if (sd->first == 0) {
        sd->first = 1;
    }

    new_entries = munit_malloc((sd->n + n) * sizeof *new_entries);

    if (sd->n > 0) {
        munit_assert_ptr_not_null(sd->entries);
        memcpy(new_entries, sd->entries, sd->n * sizeof *sd->entries);
    }

    memcpy(new_entries + sd->n, entries, n * sizeof *entries);

    sd->entries = new_entries;
    sd->n += n;

out:
    sd->counter++;
    cb(rv);
}

static void __delete_entries(void *data, uint64_t index, void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;

    munit_assert(data != NULL);
    munit_assert(index >= sd->first);

    if (__should_fail(sd)) {
        __logf("store: fail to delete log entries from %ld onward", index);
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __logf("store: delete log entries from %ld onward", index);

    sd->n = index - 1;

    if (sd->n > 0) {
        raft_entry *new_entries;
        new_entries = munit_malloc((index - 1) * sizeof *new_entries);
        memcpy(new_entries, sd->entries, sd->n * sizeof *sd->entries);
	sd->entries = new_entries;
    } else {
        sd->entries = NULL;
    }

out:
    sd->counter++;
    cb(rv);
}

static void __get_entries(void *data,
                          uint64_t index,
                          size_t n,
                          raft_entry *entries,
                          void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;
    size_t i;

    munit_assert(data != NULL);
    munit_assert(index >= sd->first);
    munit_assert(n <= sd->n);

    if (__should_fail(sd)) {
        __logf("store: fail to get %s log entries starting at %ld", n, index);
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __logf("store: get %s log entries starting at %ld", n, index);

    for (i = 0; i < n; i++) {
        *(entries + i) = *(sd->entries + index - 1 + i);
    }

out:
    sd->counter++;
    cb(rv);
}

static void __first_index(void *data, uint64_t *index, void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;

    munit_assert(data != NULL);

    if (__should_fail(sd)) {
        __log("store: fail to get first index");
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __log("store: get first index");

    *index = sd->first;

 out:
    sd->counter++;
    cb(rv);
}

static void __entries_count(void *data, size_t *n, void (*cb)(int rv))
{
    struct test_store__data *sd = data;
    int rv = 0;

    munit_assert(data != NULL);

    if (__should_fail(sd)) {
        __log("store: fail to get entries count");
        rv = RAFT_ERR_SHUTDOWN;
        goto out;
    }

    __log("store: get entries count");

    *n = sd->n;

 out:
    sd->counter++;
    cb(rv);
}

void test_store_setup(const MunitParameter params[], raft_store *s)
{
    struct test_store__data *sd = munit_malloc(sizeof *sd);
    const char *fail_delay = munit_parameters_get(params, "store-fail-delay");
    const char *fail_repeat = munit_parameters_get(params, "store-repeat-delay");

    munit_assert_ptr_not_null(s);

    __init(sd);

    if (fail_delay != NULL) {
        sd->fail_delay = atoi(fail_delay);
    }
    if (fail_repeat != NULL) {
        sd->fail_repeat = atoi(fail_repeat);
    }

    s->version = 1;
    s->data = sd;
    s->set_term = __set_term;
    s->set_vote = __set_vote;
    s->append_entries = __append_entries;
    s->delete_entries = __delete_entries;
    s->get_entries = __get_entries;
    s->first_index = __first_index;
    s->entries_count = __entries_count;
}

void test_store_tear_down(raft_store *s)
{
    munit_assert_ptr_not_null(s);

}
