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

static int __logentry_get_node_id(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    int ety_idx
    )
{
    return 0;
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

static int __log_pop_failing(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx
    )
{
    return -1;
}

raft_cbs_t funcs = {
    .log_pop = __log_pop,
    .log_get_node_id = __logentry_get_node_id
};

void* __set_up()
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, queue);
    return r;
}

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

    void *r = raft_new();

    memset(&e, 0, sizeof(raft_entry_t));

    e.id = 1;

    l = log_new();
    raft_cbs_t funcs = {
        .log_offer = __log_offer
    };
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    CuAssertIntEquals(tc, 1, log_count(l));
}

void TestLog_get_at_idx(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e1.id, log_get_at_idx(l, 1)->id);
    CuAssertIntEquals(tc, e2.id, log_get_at_idx(l, 2)->id);
    CuAssertIntEquals(tc, e3.id, log_get_at_idx(l, 3)->id);
}

void TestLog_get_at_idx_returns_null_where_out_of_bounds(CuTest * tc)
{
    void *l;
    raft_entry_t e1;

    memset(&e1, 0, sizeof(raft_entry_t));

    l = log_new();
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 0));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
}

void TestLog_delete(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    log_delete(l, 3);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, e3.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));

    log_delete(l, 2);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));

    log_delete(l, 1);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
}

void TestLog_delete_onwards(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    log_set_callbacks(l, &funcs, r);
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));

    /* even 3 gets deleted */
    log_delete(l, 2);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, e1.id, log_get_at_idx(l, 1)->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
}

void TestLog_delete_handles_log_pop_failure(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop_failing,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    CuAssertIntEquals(tc, -1, log_delete(l, 3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e3.id, ((raft_entry_t*)log_peektail(l))->id);
 }

void TestLog_delete_fails_for_idx_zero(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));
    CuAssertIntEquals(tc, log_delete(l, 0), -1);
}

void TestLog_poll(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_get_current_idx(l));

    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));

    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    raft_entry_t *ety;

    /* remove 1st */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 1, log_get_base(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 2nd */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, ety->id, 2);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 3rd */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, ety->id, 3);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));
}

void TestLog_peektail(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e3.id, log_peektail(l)->id);
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

void TestLog_load_from_snapshot(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    CuAssertIntEquals(tc, 0, log_get_current_idx(l));
    CuAssertIntEquals(tc, 0, log_load_from_snapshot(l, 10, 5));
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));

    /* this is just a marker
     * it should never be sent to any nodes because it is part of a snapshot */
    CuAssertIntEquals(tc, 1, log_count(l));
}

void TestLog_load_from_snapshot_clears_log(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));

    CuAssertIntEquals(tc, 0, log_load_from_snapshot(l, 10, 5));
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));
}

void TestLog_front_pushes_across_boundary(CuTest * tc)
{
    void* r = __set_up();

    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 2);
}

void TestLog_front_and_back_pushed_across_boundary_with_enlargement_required(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);

    raft_entry_t* ety;

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 2);

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 3);
}

void TestLog_delete_after_polling(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);

    raft_entry_t* ety;

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 0, log_count(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_delete(l, 1), 0);
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_delete_after_polling_from_double_append(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 1, log_count(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_delete(l, 1), 0);
    CuAssertIntEquals(tc, 0, log_count(l));
}
