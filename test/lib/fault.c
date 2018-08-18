#include "fault.h"
#include "munit.h"

void test_fault_init(struct test_fault *f)
{
    f->countdown = -1;
    f->repeat = -1;
}

bool test_fault_tick(struct test_fault *f)
{
    /* If the initial delay parameter was set to -1, then never fail. This is
     * the most common case. */
    if MUNIT_LIKELY(f->countdown < 0) {
	return false;
    }

    /* If we did not yet reach 'delay' ticks, then just decrease the countdown.
     */
    if (f->countdown > 0) {
        f->countdown--;
        return false;
    }

    munit_assert_int(f->countdown, ==, 0);

    /* We reached 'delay' ticks, let's see how many times we have to trigger the
     * fault, if any. */

    if (f->repeat < 0) {
        /* Trigger the fault forever. */
        return true;
    }

    if (f->repeat > 0) {
        /* Trigger the fault at least this time. */
        f->repeat--;
        return true;
    }

    munit_assert_int(f->repeat, ==, 0);

    /* We reached 'repeat' ticks, let's stop triggering the fault. */
    f->countdown--;

    return false;
}
