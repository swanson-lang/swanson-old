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
 * S₀: Atoms
 */

#define TEST_COUNT_S0_ATOMS  12

static void
test_s0_atoms(void)
{
    struct s0_entity  *a1;
    struct s0_entity  *a2;
    struct s0_entity  *a3;

    diag("S₀ atoms");

    ok_alloc(a1, s0_atom_new());
    ok_alloc(a2, s0_atom_new());
    ok_alloc(a3, s0_atom_new());

    ok(s0_entity_type(a1) == S0_ENTITY_TYPE_ATOM, "type(atom1) == atom");
    ok(s0_entity_type(a2) == S0_ENTITY_TYPE_ATOM, "type(atom2) == atom");
    ok(s0_entity_type(a3) == S0_ENTITY_TYPE_ATOM, "type(atom3) == atom");

    ok(s0_atom_eq(a1, a1),  "atom1 == atom1");
    ok(s0_atom_eq(a2, a2),  "atom2 == atom2");
    ok(s0_atom_eq(a3, a3),  "atom3 == atom3");

    ok(!s0_atom_eq(a1, a2),  "atom1 != atom2");
    ok(!s0_atom_eq(a1, a3),  "atom1 != atom3");
    ok(!s0_atom_eq(a2, a3),  "atom2 != atom3");

    s0_entity_free(a1);
    s0_entity_free(a2);
    s0_entity_free(a3);
}

/*-----------------------------------------------------------------------------
 * Harness
 */

#define TEST_COUNT_TOTAL \
    TEST_COUNT_S0_NAMES + \
    TEST_COUNT_S0_ATOMS + \
    0

int main(void)
{
    plan_tests(TEST_COUNT_TOTAL);
    test_s0_names();
    test_s0_atoms();
    return exit_status();
}
