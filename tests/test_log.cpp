#include <gtest/gtest.h>

extern "C"
{
#include "linked_list_queue.h"
#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
}

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
    EXPECT_EQ(1, entry_idx);
    return 0;
}

static int __log_pop(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    int entry_idx
    )
{
    raft_entry_t* copy = (raft_entry_t*)malloc(sizeof(*entry));
    memcpy(copy, entry, sizeof(*entry));
    llqueue_offer((linked_list_queue_t*)user_data, copy);
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


TEST(TestLog, new_is_empty)
{
    log_t* l;

    l = log_new();
    EXPECT_EQ(0, log_count(l));
}

TEST(TestLog, append_is_not_empty)
{
    log_t* l;
    raft_entry_t e;

    raft_server_t *r = raft_new();

    memset(&e, 0, sizeof(raft_entry_t));

    e.id = 1;

    l = log_new();
    raft_cbs_t funcs = {0};
    funcs.log_offer = __log_offer;

    log_set_callbacks(l, &funcs, r);
    EXPECT_EQ(0, log_append_entry(l, &e));
    EXPECT_EQ(1, log_count(l));
}

TEST(TestLog, get_at_idx)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(e1.id, log_get_at_idx(l, 1)->id);
    EXPECT_EQ(e2.id, log_get_at_idx(l, 2)->id);
    EXPECT_EQ(e3.id, log_get_at_idx(l, 3)->id);
}

TEST(TestLog, get_at_idx_returns_null_where_out_of_bounds)
{
    log_t *l;
    raft_entry_t e1;

    memset(&e1, 0, sizeof(raft_entry_t));

    l = log_new();
    EXPECT_EQ(NULL, log_get_at_idx(l, 0));
    EXPECT_EQ(NULL, log_get_at_idx(l, 1));

    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(NULL, log_get_at_idx(l, 2));
}

TEST(TestLog, _delete)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;

    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(3, log_get_current_idx(l));

    log_delete(l, 3);
    EXPECT_EQ(2, log_count(l));
    EXPECT_EQ(e3.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    EXPECT_EQ(2, log_count(l));
    EXPECT_EQ(NULL, log_get_at_idx(l, 3));

    log_delete(l, 2);
    EXPECT_EQ(1, log_count(l));
    EXPECT_EQ(NULL, log_get_at_idx(l, 2));

    log_delete(l, 1);
    EXPECT_EQ(0, log_count(l));
    EXPECT_EQ(NULL, log_get_at_idx(l, 1));
}

TEST(TestLog, delete_onwards)
{
    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = { 0 };
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;

    raft_set_callbacks(r, &funcs, queue);

    log_t *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    log_set_callbacks(l, &funcs, r);
    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));

    /* even 3 gets deleted */
    log_delete(l, 2);
    EXPECT_EQ(1, log_count(l));
    EXPECT_EQ(e1.id, log_get_at_idx(l, 1)->id);
    EXPECT_EQ(NULL, log_get_at_idx(l, 2));
    EXPECT_EQ(NULL, log_get_at_idx(l, 3));
}

TEST(TestLog, delete_handles_log_pop_failure)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop_failing;
    funcs.log_get_node_id = __logentry_get_node_id;

    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(3, log_get_current_idx(l));

    EXPECT_EQ(-1, log_delete(l, 3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(e3.id, ((raft_entry_t*)log_peektail(l))->id);
 }

TEST(TestLog, delete_fails_for_idx_zero)
{
    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;

    raft_set_callbacks(r, &funcs, queue);

    log_t *l;
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
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(0, log_append_entry(l, &e4));
    EXPECT_EQ(log_delete(l, 0), -1);
}

TEST(TestLog, poll)
{
    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;

    raft_set_callbacks(r, &funcs, queue);

    log_t *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(1, log_get_current_idx(l));

    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(2, log_get_current_idx(l));

    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(3, log_get_current_idx(l));

    raft_entry_t *ety;

    /* remove 1st */
    ety = nullptr;
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_NE(nullptr, ety);
    EXPECT_EQ(2, log_count(l));
    EXPECT_EQ(ety->id, 1);
    EXPECT_EQ(1, log_get_base(l));
    EXPECT_EQ(nullptr, log_get_at_idx(l, 1));
    EXPECT_NE(nullptr, log_get_at_idx(l, 2));
    EXPECT_NE(nullptr, log_get_at_idx(l, 3));
    EXPECT_EQ(3, log_get_current_idx(l));

    /* remove 2nd */
    ety = nullptr;
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_NE(nullptr, ety);
    EXPECT_EQ(1, log_count(l));
    EXPECT_EQ(ety->id, 2);
    EXPECT_EQ(nullptr, log_get_at_idx(l, 1));
    EXPECT_EQ(nullptr, log_get_at_idx(l, 2));
    EXPECT_NE(nullptr, log_get_at_idx(l, 3));
    EXPECT_EQ(3, log_get_current_idx(l));

    /* remove 3rd */
    ety = nullptr;
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_NE(nullptr, ety);
    EXPECT_EQ(0, log_count(l));
    EXPECT_EQ(ety->id, 3);
    EXPECT_EQ(nullptr, log_get_at_idx(l, 1));
    EXPECT_EQ(nullptr, log_get_at_idx(l, 2));
    EXPECT_EQ(nullptr, log_get_at_idx(l, 3));
    EXPECT_EQ(3, log_get_current_idx(l));
}

TEST(TestLog, peektail)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(e3.id, log_peektail(l)->id);
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

TEST(TestLog, load_from_snapshot)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    EXPECT_EQ(0, log_get_current_idx(l));
    EXPECT_EQ(0, log_load_from_snapshot(l, 10, 5));
    EXPECT_EQ(10, log_get_current_idx(l));

    /* this is just a marker
     * it should never be sent to any nodes because it is part of a snapshot */
    EXPECT_EQ(1, log_count(l));
}

TEST(TestLog, load_from_snapshot_clears_log)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();

    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(2, log_count(l));
    EXPECT_EQ(2, log_get_current_idx(l));

    EXPECT_EQ(0, log_load_from_snapshot(l, 10, 5));
    EXPECT_EQ(1, log_count(l));
    EXPECT_EQ(10, log_get_current_idx(l));
}

TEST(TestLog, front_pushes_across_boundary)
{
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;

    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_set_callbacks(r, &funcs, queue);

    log_t *l;
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

    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 1);
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 2);
}

TEST(TestLog, front_and_back_pushed_across_boundary_with_enlargement_required)
{
    log_t *l;
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
    EXPECT_EQ(0, log_append_entry(l, &e1));

    /* poll */
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 1);

    /* append */
    EXPECT_EQ(0, log_append_entry(l, &e2));

    /* poll */
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 2);

    /* append append */
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(0, log_append_entry(l, &e4));

    /* poll */
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 3);
}

TEST(TestLog, delete_after_polling)
{
    log_t *l;
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
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(1, log_count(l));

    /* poll */
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 1);
    EXPECT_EQ(0, log_count(l));

    /* append */
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(1, log_count(l));

    /* poll */
    EXPECT_EQ(log_delete(l, 1), 0);
    EXPECT_EQ(0, log_count(l));
}

TEST(TestLog, delete_after_polling_from_double_append)
{
    linked_list_queue_t* queue = (linked_list_queue_t*)llqueue_new();
    raft_server_t *r = raft_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;
    funcs.log_get_node_id = __logentry_get_node_id;
    raft_set_callbacks(r, &funcs, queue);

    log_t *l;
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
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(0, log_append_entry(l, &e2));
    EXPECT_EQ(2, log_count(l));

    /* poll */
    EXPECT_EQ(log_poll(l, (void**)&ety), 0);
    EXPECT_EQ(ety->id, 1);
    EXPECT_EQ(1, log_count(l));

    /* append */
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(2, log_count(l));

    /* poll */
    EXPECT_EQ(log_delete(l, 1), 0);
    EXPECT_EQ(0, log_count(l));
}
