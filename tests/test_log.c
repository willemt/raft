#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "linked_list_queue.h"

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

void TestLog_new_is_empty(CuTest * tc)
{
    void *l;

    l = log_new();
    CuAssertTrue(tc, 0 == log_count(l));
}

static int __log_offer(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx
    )
{
    CuAssertIntEquals((CuTest*)raft, 1, entry_idx);
    return 0;
}

void TestLog_append_is_not_empty(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    e.id = 1;

    l = log_new();
    raft_cbs_t funcs = {
        .log_offer = __log_offer
    };
    log_set_callbacks(l, &funcs, tc);
    CuAssertTrue(tc, 0 == log_append_entry(l, &e));
    CuAssertTrue(tc, 1 == log_count(l));
}

void TestLog_get_at_idx(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    CuAssertTrue(tc, 3 == log_count(l));
    CuAssertTrue(tc, e2.id == log_get_at_idx(l, 2)->id);
}

void TestLog_get_at_idx_returns_null_where_out_of_bounds(CuTest * tc)
{
    void *l;
    raft_entry_t e1;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
}

static int __log_pop(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx
    )
{
    raft_entry_t* copy = malloc(sizeof(*entry));
    memcpy(copy, entry, sizeof(*entry));
    llqueue_offer(user_data, copy);
    return 0;
}

void TestLog_delete(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    void *r = raft_new();

    l = log_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop
    };
    raft_set_callbacks(r, &funcs, queue);
    log_set_callbacks(l, &funcs, r);

    e1.id = 1;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    log_delete(l, 3);
    CuAssertTrue(tc, ((raft_entry_t*)llqueue_poll(queue))->id == e3.id);

    CuAssertTrue(tc, 2 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
    log_delete(l, 2);
    CuAssertTrue(tc, 1 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    log_delete(l, 1);
    CuAssertTrue(tc, 0 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
}

void TestLog_delete_onwards(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    /* even 3 gets deleted */
    log_delete(l, 2);
    CuAssertTrue(tc, 1 == log_count(l));
    CuAssertTrue(tc, e1.id == log_get_at_idx(l, 1)->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
}

void TestLog_peektail(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 0 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));
    CuAssertTrue(tc, e3.id == log_peektail(l)->id);
}

#if 0
// TODO: duplicate testing not implemented yet
void T_estlog_cant_append_duplicates(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    e.id = 1;

    l = log_new();
    CuAssertTrue(tc, 1 == log_append_entry(l, &e));
    CuAssertTrue(tc, 1 == log_count(l));
}
#endif

