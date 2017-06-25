#include <gtest/gtest.h>

extern "C"
{
#include "linked_list_queue.h"
#include "raft.h"
#include "raft_log.h"
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

    e.id = 1;

    l = log_new();
    EXPECT_EQ(0, log_append_entry(l, &e));
    EXPECT_EQ(1, log_count(l));
}

TEST(TestLog, get_at_idx)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));

    EXPECT_EQ(3, log_count(l));
    EXPECT_EQ(e2.id, log_get_at_idx(l, 2)->id);
}

TEST(TestLog, get_at_idx_returns_null_where_out_of_bounds)
{
    log_t *l;
    raft_entry_t e1;

    l = log_new();
    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    EXPECT_EQ(NULL, log_get_at_idx(l, 2));
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

TEST(TestLog, delete)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    raft_server_t *r = raft_new();

    l = log_new();
    raft_cbs_t funcs = {0};
    funcs.log_pop = __log_pop;

    raft_set_callbacks(r, &funcs, queue);
    log_set_callbacks(l, &funcs, r);

    e1.id = 1;
    EXPECT_EQ(0, log_append_entry(l, &e1));
    e2.id = 2;
    EXPECT_EQ(0, log_append_entry(l, &e2));
    e3.id = 3;
    EXPECT_EQ(0, log_append_entry(l, &e3));
    EXPECT_EQ(3, log_count(l));

    log_delete(l, 3);
    unsigned int id = ((raft_entry_t*)llqueue_poll((linked_list_queue_t*)queue))->id;
    EXPECT_EQ(id, e3.id);

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
    log_t *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
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

TEST(TestLog, peektail)
{
    log_t *l;
    raft_entry_t e1, e2, e3;

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
    log_t *l;
    raft_entry_t e;

    e.id = 1;

    l = log_new();
    EXPECT_EQ(1, log_append_entry(l, &e));
    EXPECT_EQ(1, log_count(l));
}
#endif

