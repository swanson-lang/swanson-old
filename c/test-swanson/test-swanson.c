/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include <string.h>

#include "ccan/tap/tap.h"
#include "swanson.h"

/*-----------------------------------------------------------------------------
 * Helpers
 */

#define ok_alloc(var, call) \
    do { \
        var = call; \
        ok(var != NULL, #call " != NULL"); \
    } while (0)

/*-----------------------------------------------------------------------------
 * S₀: Names
 */

#define TEST_COUNT_S0_NAMES  11

static void
test_s0_names(void)
{
    struct s0_name  *n1;
    struct s0_name  *n2;
    struct s0_name  *n3;

    diag("S₀ names");

    ok_alloc(n1, s0_name_new_str("hello"));
    ok_alloc(n2, s0_name_new(5, "hello"));
    /* content includes NUL terminator */
    ok_alloc(n3, s0_name_new(6, "hello"));

    ok(s0_name_size(n1) == 5, "[name(5, \"hello\")] == 5");
    ok(s0_name_size(n2) == 5, "[name(5, \"hello\")] == 5");
    ok(s0_name_size(n3) == 6, "[name(6, \"hello\\x00\")] == 6");

    ok(memcmp(s0_name_content(n1), "hello", 5) == 0,
       "name(5, \"hello\") == \"hello\"");
    ok(memcmp(s0_name_content(n2), "hello", 5) == 0,
       "name(5, \"hello\") == \"hello\"");
    ok(memcmp(s0_name_content(n3), "hello\x00", 6) == 0,
       "name(6, \"hello\\x00\") == \"hello\\x00\"");

    ok(s0_name_eq(n1, n2),  "name(5, \"hello\") == name(5, \"hello\")");
    ok(!s0_name_eq(n1, n3), "name(5, \"hello\") != name(6, \"hello\\x00\")");

    s0_name_free(n1);
    s0_name_free(n2);
    s0_name_free(n3);
}

/*-----------------------------------------------------------------------------
 * Harness
 */

#define TEST_COUNT_TOTAL \
    TEST_COUNT_S0_NAMES + \
    0

int main(void)
{
    plan_tests(TEST_COUNT_TOTAL);
    test_s0_names();
    return exit_status();
}
