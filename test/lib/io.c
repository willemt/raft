#include <stdbool.h>

#include "fault.h"
#include "io.h"

/* Set to 1 to enable logging. */
#if 0
#define __log(MSG_logf(MUNIT_LOG_DEBUG, MSG__)
#define __logf(MSG, ...) munit_logf(MUNIT_LOG_DEBUG, MSG, __VA_ARGS__)
#else
#define __log(MSG)
#define __logf(MSG, ...)
#endif

/* Maximum number of pending I/O events in the queue. This should be enough for
   testing purposes. */
#define TEST__IO_EVENT_QUEUE_SIZE 64

/* Hold a raft callback to be fired at some point in the future. */
struct test__io_event
{
    raft_io_cb cb;
    struct raft *raft;
};

/* Queue of pending events */
struct test__io_event_queue
{
    struct test__io_event events[64]; /* Events buffer */
    int n; /* Number of pending events in the buffer */
};

void test__io_event_queue_init(struct test__io_event_queue *q)
{
    int i;
    for (i = 0; i < TEST__IO_EVENT_QUEUE_SIZE; i++) {
        q->events[i].cb = NULL;
        q->events[i].raft = NULL;
    }
    q->n = 0;
}

/* Enqueue a pending events. */
void test__io_event_queue_push(struct test__io_event_queue *q,
                               raft_io_cb cb,
                               struct raft *raft)
{
    /* Make sure that there's enough capacity in the events buffer. */
    munit_assert_int(q->n, <, TEST__IO_EVENT_QUEUE_SIZE);

    munit_assert_ptr_null(q->events[q->n].cb);
    munit_assert_ptr_null(q->events[q->n].raft);

    q->events[q->n].cb = cb;
    q->events[q->n].raft = raft;

    q->n++;
}

/* Fire all pending events. */
void test__io_event_queue_flush(struct test__io_event_queue *q,
                                struct test_fault *fault)
{
    int i;
    int n = q->n;

    for (i = 0; i < n; i++) {
        raft_io_cb cb = q->events[i].cb;
        struct raft *raft = q->events[i].raft;
        int rv = 0;

        munit_assert_ptr_not_null(cb);
        munit_assert_ptr_not_null(raft);

        if (test_fault_tick(fault)) {
            rv = 1;
        }

        q->events[i].cb = NULL;
        q->events[i].raft = NULL;
        q->n--;

        cb(raft, rv);
    }

    munit_assert_int(q->n, ==, 0);
}

struct test__io
{
    /* Term and vote */
    uint64_t term;
    uint32_t voted_for;

    /* Log */
    struct raft_entry *entries; /* Entries array */
    uint64_t first;             /* Index of the first entry */
    size_t n;                   /* Size of the entries array */

    /* Internal state */
    struct test__io_event_queue queue; /* Event queue. */
    struct test_fault fault;           /* Fault injection state */
};

/* Initialize the in-memory I/O implementation data. */
static void test__io_init(struct test__io *t)
{
    t->term = 0;
    t->voted_for = 0;

    t->entries = NULL;
    t->first = 0;
    t->n = 0;

    test__io_event_queue_init(&t->queue);
    test_fault_init(&t->fault);
}

static int test__io_write_term(struct raft *r, uint64_t term, raft_io_cb cb)
{
    struct test__io *t = r->io.data;

    munit_assert_ptr_not_null(t);

    if (test_fault_tick(&t->fault)) {
        __logf("io: fail to set term %ld", term);
        return RAFT_ERR_SHUTDOWN;
    }

    __logf("io: set term %ld", term);
    t->term = term;

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

static int test__io_write_vote(struct raft *r, uint64_t node_id, raft_io_cb cb)
{
    struct test__io *t = r->io.data;

    munit_assert_ptr_not_null(t);

    if (test_fault_tick(&t->fault)) {
        __logf("io: fail to set vote for %ld", node_id);
        return RAFT_ERR_SHUTDOWN;
    }

    __logf("io: set vote for %ld", node_id);
    t->voted_for = node_id;

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

static int test__io_write_log(struct raft *r,
                              struct raft_entry *entries,
                              size_t n,
                              raft_io_cb cb)
{
    struct test__io *t = r->io.data;
    struct raft_entry *new_entries;

    munit_assert_ptr_not_null(t);
    munit_assert_int(n, >, 0);

    if (test_fault_tick(&t->fault)) {
        __logf("io: fail to append %d log entries", n);
        return RAFT_ERR_SHUTDOWN;
    }

    __logf("io: append %d log entries", n);

    if (t->first == 0) {
        t->first = 1;
    }

    new_entries = munit_malloc((t->n + n) * sizeof *new_entries);

    if (t->n > 0) {
        munit_assert_ptr_not_null(t->entries);
        memcpy(new_entries, t->entries, t->n * sizeof *t->entries);
    }

    memcpy(new_entries + t->n, entries, n * sizeof *entries);

    t->entries = new_entries;
    t->n += n;

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

static int test__io_truncate_log(struct raft *r, uint64_t index, raft_io_cb cb)
{
    struct test__io *t = r->io.data;

    munit_assert_ptr_not_null(t);
    munit_assert_true(index >= t->first);

    if (test_fault_tick(&t->fault)) {
        __logf("io: fail to delete log entries from %ld onward", index);
        return RAFT_ERR_SHUTDOWN;
    }

    __logf("io: delete log entries from %ld onward", index);

    t->n = index - 1;

    if (t->n > 0) {
        struct raft_entry *new_entries;
        new_entries = munit_malloc((index - 1) * sizeof *new_entries);
        memcpy(new_entries, t->entries, t->n * sizeof *t->entries);
        t->entries = new_entries;
    } else {
        t->entries = NULL;
    }

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

static int test__io_read_log(struct raft *r,
                             uint64_t index,
                             size_t n,
                             struct raft_entry *entries,
                             raft_io_cb cb)
{
    struct test__io *t = r->io.data;
    size_t i;

    munit_assert(t != NULL);
    munit_assert(index >= t->first);
    munit_assert(n <= t->n);

    if (test_fault_tick(&t->fault)) {
        __logf("io: fail to get %s log entries starting at %ld", n, index);
        return RAFT_ERR_SHUTDOWN;
    }

    __logf("io: get %s log entries starting at %ld", n, index);

    for (i = 0; i < n; i++) {
        *(entries + i) = *(t->entries + index - 1 + i);
    }

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

static int test__io_read_log_meta(struct raft *r,
                                  size_t *n,
                                  uint64_t *first_index,
                                  raft_io_cb cb)
{
    struct test__io *t = r->io.data;

    munit_assert(t != NULL);

    if (test_fault_tick(&t->fault)) {
        __log("io: fail to get first index");
        return RAFT_ERR_SHUTDOWN;
    }

    __log("io: get first index");

    *first_index = t->first;
    *n = t->n;

    test__io_event_queue_push(&t->queue, cb, r);

    return 0;
}

void test_io_setup(const MunitParameter params[], struct raft_io *io)
{
    struct test__io *t = munit_malloc(sizeof *t);
    const char *delay = munit_parameters_get(params, TEST_IO_FAULT_DELAY);
    const char *repeat = munit_parameters_get(params, TEST_IO_FAULT_REPEAT);

    munit_assert_ptr_not_null(io);

    test__io_init(t);

    if (delay != NULL) {
        t->fault.countdown = atoi(delay);
    }
    if (repeat != NULL) {
        t->fault.repeat = atoi(repeat);
    }

    io->version = 1;
    io->data = t;
    io->write_term = test__io_write_term;
    io->write_vote = test__io_write_vote;
    io->write_log = test__io_write_log;
    io->truncate_log = test__io_truncate_log;
    io->read_log = test__io_read_log;
    io->read_log_meta = test__io_read_log_meta;
}

void test_io_tear_down(struct raft_io *io)
{
    munit_assert_ptr_not_null(io);
}

void test_io_flush(struct raft_io *io)
{
    struct test__io *t = io->data;
    test__io_event_queue_flush(&t->queue, &t->fault);
}
