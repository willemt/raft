#include "../lib/munit.h"

static MunitSuite unit__test_suites[] = {{NULL, NULL, NULL, 0, 0}};

static MunitSuite unit__test_suite = {(char *)"", NULL, unit__test_suites, 1, 0};

/* Test runner executable */
int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&unit__test_suite, (void *)"unit", argc, argv);
}
