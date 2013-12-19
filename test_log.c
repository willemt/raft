
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "raft_log.h"

void TestLog_new_is_empty(CuTest * tc)
{
    void *l;

    l = log_new();
    CuAssertTrue(tc, 0 == log_count(l));
}

void TestLog_append_is_not_empty(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    e.id = 1;

    l = log_new();
    CuAssertTrue(tc, 1 == log_append_entry(l, &e));
    CuAssertTrue(tc, 1 == log_count(l));
}

void TestLog_get_at_idx(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    CuAssertTrue(tc, 3 == log_count(l));
    CuAssertTrue(tc, e2.id == log_get_from_idx(l,2)->id);
}

void TestLog_get_at_idx_returns_null_where_out_of_bounds(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e1));
    CuAssertTrue(tc, NULL == log_get_from_idx(l,2));
}

void TestLog_mark_peer_has_committed_adds_peers(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    log_append_entry(l, &e1);
    CuAssertTrue(tc, 0 == log_get_from_idx(l,1)->npeers);
    log_mark_peer_has_committed(l, 1);
    CuAssertTrue(tc, 1 == log_get_from_idx(l,1)->npeers);
    log_mark_peer_has_committed(l, 1);
    CuAssertTrue(tc, 2 == log_get_from_idx(l,1)->npeers);
}

void TestLog_delete(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    log_delete(l,3);
    CuAssertTrue(tc, 2 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_from_idx(l,3));
    log_delete(l,2);
    CuAssertTrue(tc, 1 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_from_idx(l,2));
    log_delete(l,1);
    CuAssertTrue(tc, 0 == log_count(l));
    CuAssertTrue(tc, NULL == log_get_from_idx(l,1));
}

void TestLog_delete_onwards(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e3));
    CuAssertTrue(tc, 3 == log_count(l));

    /* even 3 gets deleted */
    log_delete(l,2);
    CuAssertTrue(tc, 1 == log_count(l));
    CuAssertTrue(tc, e1.id == log_get_from_idx(l,1)->id);
    CuAssertTrue(tc, NULL == log_get_from_idx(l,2));
    CuAssertTrue(tc, NULL == log_get_from_idx(l,3));
}

void TestLog_peektail(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertTrue(tc, 1 == log_append_entry(l, &e3));
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

