
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include "linked_list_queue.h"

void TestLLQueue_offer(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    CuAssertTrue(tc, 1 == llqueue_count(qu));
    llqueue_free(qu);
}

void TestLLQueue_cant_poll_with_no_contents(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem";

    qu = llqueue_new();
    llqueue_offer(qu, item);
    CuAssertTrue(tc, item == llqueue_poll(qu));
    CuAssertTrue(tc, 0 == llqueue_count(qu));
    llqueue_free(qu);
}

void TestLLQueue_offer_and_poll_item(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    CuAssertTrue(tc, item == llqueue_poll(qu));
    llqueue_free(qu);
}

void TestLLQueue_fifo(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem", *item2 = "testitem2";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    llqueue_offer(qu, item2);
    CuAssertTrue(tc, item == llqueue_poll(qu));
    CuAssertTrue(tc, item2 == llqueue_poll(qu));
    llqueue_free(qu);
}

void TestLLQueue_remove_item_is_null_when_not_available(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem", *item2 = "testitem2";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    CuAssertTrue(tc, NULL == llqueue_remove_item(qu, item2));
    llqueue_free(qu);
}

void TestLLQueue_remove_item(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem", *item2 = "testitem2";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    llqueue_offer(qu, item2);
    CuAssertTrue(tc, item2 == llqueue_remove_item(qu, item2));
    llqueue_free(qu);
}

void TestLLQueue_remove_item_when_it_is_head(
    CuTest * tc
)
{
    void *qu;

    char *item = "testitem", *item2 = "testitem2";

    qu = llqueue_new();

    llqueue_offer(qu, item);
    llqueue_offer(qu, item2);
    CuAssertTrue(tc, item == llqueue_remove_item(qu, item));
    llqueue_free(qu);
}
