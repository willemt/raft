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

static raft_node_id_t __logentry_get_node_id(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t ety_idx
    )
{
    return 0;
}

static raft_node_id_t __logentry_get_node_id_check(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t ety_idx
    )
{
    assert(ety->data.buf);
    return atoi(ety->data.buf);
}

static int __log_offer(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entries,
    raft_index_t entry_idx,
    int *n_entries
    )
{
    CuAssertIntEquals((CuTest*)raft, 1, entry_idx);
    return 0;
}

static int __log_pop(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx,
    int *n_entries
    )
{
    int i;
    for (i = *n_entries - 1; i >= 0; i--)
    {
        assert(entry_idx + i == entry[i].id);
        raft_entry_t* copy = malloc(sizeof(*copy));
        assert(copy);
        memcpy(copy, &entry[i], sizeof(*copy));
        entry[i].data.buf = NULL;
        entry[i].data.len = 0;
        llqueue_offer(user_data, copy);
    }
    return 0;
}

static int __log_pop_failing(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx,
    int *n_entries
    )
{
    *n_entries = 0;
    return -1;
}

raft_cbs_t g_funcs = {
    .log_pop = __log_pop,
    .log_get_node_id = __logentry_get_node_id
};

static int log_append_entry(log_t* me_, raft_entry_t* ety)
{
    int k = 1;
    return log_append(me_, ety, &k);
}

void* __set_up()
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_set_callbacks(r, &g_funcs, queue);
    return r;
}

void TestLog_new_is_empty(CuTest * tc)
{
    void *l;

    l = log_new();
    CuAssertTrue(tc, 0 == log_count(l));
    log_free(l);
}

void TestLog_append_is_not_empty(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    memset(&e, 0, sizeof(raft_entry_t));

    e.id = 1;

    l = log_new();
    raft_cbs_t funcs = {
        .log_offer = __log_offer
    };
    log_set_callbacks(l, &funcs, tc);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    CuAssertIntEquals(tc, 1, log_count(l));
    log_free(l);
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
    log_free(l);
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
    log_free(l);
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
    CuAssertIntEquals(tc, e2.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));

    log_delete(l, 1);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, e1.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    log_free(l);
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
    CuAssertIntEquals(tc, e3.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertIntEquals(tc, e2.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
    log_free(l);
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
    log_free(l);
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
    log_free(l);
}

/* This test also verifies that an entry is processed before it is popped
   via the "buf" assertion in __logentry_get_node_id_check. */
void TestLog_delete_processes_cfg_entries(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id_check
    };
    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e, 0, sizeof(raft_entry_t));

    e.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));

    e.id = 2;
    e.type = RAFT_LOGTYPE_ADD_NONVOTING_NODE;
    e.data.buf = "2";
    e.data.len = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    raft_node_t* node = raft_get_node(r, 2);
    CuAssertPtrNotNull(tc, node);
    CuAssertTrue(tc, !raft_node_is_voting(node));

    e.id = 3;
    e.type = RAFT_LOGTYPE_PROMOTE_NODE;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    node = raft_get_node(r, 2);
    CuAssertPtrNotNull(tc, node);
    CuAssertTrue(tc, raft_node_is_voting(node));

    e.id = 4;
    e.type = RAFT_LOGTYPE_DEMOTE_NODE;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    node = raft_get_node(r, 2);
    CuAssertPtrNotNull(tc, node);
    CuAssertTrue(tc, !raft_node_is_voting(node));

    e.id = 5;
    e.type = RAFT_LOGTYPE_REMOVE_NONVOTING_NODE;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    CuAssertPtrEquals(tc, NULL, raft_get_node(r, 2));

    CuAssertIntEquals(tc, 5, log_get_current_idx(l));

    /* Delete the last two entries. */
    CuAssertIntEquals(tc, 0, log_delete(l, 4));
    CuAssertIntEquals(tc, 3, log_count(l));
    node = raft_get_node(r, 2);
    CuAssertPtrNotNull(tc, node);
    CuAssertTrue(tc, raft_node_is_voting(node));

    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* Delete the last two entries. */
    CuAssertIntEquals(tc, 0, log_delete(l, 2));
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertPtrEquals(tc, NULL, raft_get_node(r, 2));

    log_free(l);
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
    e1.term = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_get_current_idx(l));

    e2.id = 2;
    e2.term = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));

    e3.id = 3;
    e3.term = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 1st */
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, 1, log_get_base(l));
    CuAssertIntEquals(tc, 1, log_get_base_term(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 2nd and 3rd */
    CuAssertIntEquals(tc, 0, log_poll(l, 3));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_base(l));
    CuAssertIntEquals(tc, 3, log_get_base_term(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));
    log_free(l);
}

void TestLog_delete_batches(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e;

    memset(&e, 0, sizeof(raft_entry_t));

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    /* Fill the internal array. */
    int i;
    int n = log_size(l);
    for (i = 0; i < n; i++)
    {
        e.id = i + 1;
        CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    }
    CuAssertIntEquals(tc, log_size(l), log_count(l));

    /* Poll one entry, then append one entry, which occupies array index 0. */
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, log_size(l) - 1, log_count(l));
    e.id = n + 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    CuAssertIntEquals(tc, log_size(l), log_count(l));

    /* Delete the last two entries, which should end up in two batches. The
       "index == id" assertion in __log_pop should catch bugs that mess up
       batch indices. */
    CuAssertIntEquals(tc, 0, log_delete(l, n));
    CuAssertIntEquals(tc, n - 2, log_count(l));
    CuAssertIntEquals(tc, n + 1, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertIntEquals(tc, n, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, n));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, n + 1));
    log_free(l);
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
    log_free(l);
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
    log_free(l);
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
    log_load_from_snapshot(l, 10, 5);
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));
    CuAssertIntEquals(tc, 0, log_count(l));
    log_free(l);
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

    log_load_from_snapshot(l, 10, 5);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));
    log_free(l);
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
    log_set_callbacks(l, &g_funcs, r);

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 1, log_get_base(l));

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 0, log_poll(l, 2));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_base(l));
    log_free(l);
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

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));

    /* poll */
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 1, log_get_base(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));

    /* poll */
    CuAssertIntEquals(tc, 0, log_poll(l, 2));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_base(l));

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));

    /* poll */
    CuAssertIntEquals(tc, 0, log_poll(l, 3));
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_base(l));
    log_free(l);
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

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 1, log_get_base(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* delete */
    CuAssertIntEquals(tc, 0, log_delete(l, 2));
    CuAssertIntEquals(tc, 0, log_count(l));
    log_free(l);
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

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, 0, log_poll(l, 1));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* delete */
    CuAssertIntEquals(tc, 0, log_delete(l, 2));
    CuAssertIntEquals(tc, 0, log_count(l));
    log_free(l);
}

void TestLog_get_from_idx_with_base_off_by_one(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, 1), 0);
    CuAssertIntEquals(tc, 1, log_count(l));

    /* get off-by-one index */
    int n_etys;
    CuAssertPtrEquals(tc, log_get_from_idx(l, 1, &n_etys), NULL);
    CuAssertIntEquals(tc, n_etys, 0);

    /* now get the correct index */
    ety = log_get_from_idx(l, 2, &n_etys);
    CuAssertPtrNotNull(tc, ety);
    CuAssertIntEquals(tc, n_etys, 1);
    CuAssertIntEquals(tc, ety->id, 2);
    log_free(l);
}
