/**
 *
 * Helper for test components supporting fault injection.
 *
 */

#ifndef TEST_FAULT_H
#define TEST_FAULT_H

#include <stdbool.h>

/**
 * Information about a fault that should occurr in a component.
 */
struct test_fault
{
    /**
     * Trigger the fault when this counter gets to zero. Default is -1.
     */
    int countdown;
    /**
     * Repeat the fault this many times. Default is -1.
     */
    int repeat;
};

/**
 * Initialize a fault.
 */
void test_fault_init(struct test_fault *f);

/**
 * Advance the counters of the fault. Return true if the fault should be
 * triggered, false otherwise.
 */
bool test_fault_tick(struct test_fault *f);

#endif /* TEST_FAULT_H */
