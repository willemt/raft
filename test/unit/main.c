#include "../lib/munit.h"

extern MunitSuite raft_suites[];

static MunitSuite suites[] = {
    {"raft", NULL, raft_suites, 1, 0},
    {NULL, NULL, NULL, 0, 0},
};

static MunitSuite suite = {(char *)"", NULL, suites, 1, 0};

/* Test runner executable */
int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&suite, (void *)"unit", argc, argv);
}
