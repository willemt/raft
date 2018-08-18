#include "heap.h"
#include "fault.h"
#include "stdlib.h"

struct test__heap
{
    struct test_fault fault;
};

static void test__heap_init(struct test__heap *t)
{
    test_fault_init(&t->fault);
}

static void *test__heap_malloc(void *data, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    return malloc(size);
}

static void test__free(void *data, void *ptr)
{
    (void)data;
    return free(ptr);
}

static void *test__calloc(void *data, size_t nmemb, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    return calloc(nmemb, size);
}

static void *test__realloc(void *data, void *ptr, size_t size)
{
    struct test__heap *t = data;

    if (test_fault_tick(&t->fault)) {
        return NULL;
    }

    return realloc(ptr, size);
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
    (void)h;
}
