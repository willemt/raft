#include <stdlib.h>

#include "fault.h"
#include "heap.h"
#include "munit.h"

struct test__heap
{
    int n; /* Number of outstanding allocations. */
    struct test_fault fault;
};

static void test__heap_init(struct test__heap *t)
{
    t->n = 0;
    test_fault_init(&t->fault);
}

static void *test__heap_malloc(void *data, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    t->n++;

    return munit_malloc(size);
}

static void test__free(void *data, void *ptr)
{
    struct test__heap *t = data;

    t->n--;

    return free(ptr);
}

static void *test__calloc(void *data, size_t nmemb, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    t->n++;

    return munit_calloc(nmemb, size);
}

static void *test__realloc(void *data, void *ptr, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    t->n++;

    ptr = realloc(ptr, size);
    munit_assert_ptr_not_null(ptr);

    return ptr;
}

void test_heap_setup(const MunitParameter params[], struct raft_heap *h)
{
    struct test__heap *t = munit_malloc(sizeof *t);
    const char *delay = munit_parameters_get(params, TEST_HEAP_FAULT_DELAY);
    const char *repeat = munit_parameters_get(params, TEST_HEAP_FAULT_REPEAT);

    munit_assert_ptr_not_null(h);

    test__heap_init(t);

    if (delay != NULL) {
        t->fault.countdown = atoi(delay);
    }
    if (repeat != NULL) {
        t->fault.repeat = atoi(repeat);
    }

    h->data = t;
    h->malloc = test__heap_malloc;
    h->free = test__free;
    h->calloc = test__calloc;
    h->realloc = test__realloc;
}

void test_heap_tear_down(struct raft_heap *h)
{
    struct test__heap *t = h->data;

    if (t->n != 0) {
        munit_errorf("memory leak: %d outstanding allocations", t->n);
    }
}
