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

#define ok0(call, desc) \
    do { \
        ok((call) == 0, desc); \
    } while (0)

static struct s0_block *
create_empty_block(void)
{
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *inputs;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;

    inputs = s0_name_mapping_new();
    statements = s0_statement_list_new();
    src = s0_name_new_str("x");
    branch = s0_name_new_str("body");
    invocation = s0_invoke_closure_new(src, branch);
    return s0_block_new(inputs, statements, invocation);
}

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
 * S₀: Name sets
 */

#define TEST_COUNT_S0_NAME_SETS  32

static void
test_s0_name_sets(void)
{
    struct s0_name_set  *set;
    struct s0_name  *name;

    diag("S₀ name sets");

#define check_size(expected) \
    ok(s0_name_set_size(set) == expected, \
       "s0_name_set_size(set) == " #expected);

    ok_alloc(set, s0_name_set_new());
    check_size(0);

    ok_alloc(name, s0_name_new_str("a"));
    ok(!s0_name_set_contains(set, name),
       "!s0_name_set_contains(set, \"a\")");
    check_size(0);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("b"));
    ok(!s0_name_set_contains(set, name),
       "!s0_name_set_contains(set, \"b\")");
    check_size(0);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("a"));
    ok0(s0_name_set_add(set, name),
        "s0_name_set_add(set, \"a\")");
    check_size(1);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_name_set_contains(set, name),
       "s0_name_set_contains(set, \"a\")");
    check_size(1);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("b"));
    ok(!s0_name_set_contains(set, name),
       "!s0_name_set_contains(set, \"b\")");
    check_size(1);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("b"));
    ok0(s0_name_set_add(set, name),
        "s0_name_set_add(set, \"b\")");
    check_size(2);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_name_set_contains(set, name),
       "s0_name_set_contains(set, \"a\")");
    check_size(2);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_name_set_contains(set, name),
       "s0_name_set_contains(set, \"b\")");
    check_size(2);
    s0_name_free(name);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_name_eq(name, s0_name_set_at(set, 0)),
       "s0_name_set_at(set, 0) == \"a\"");
    s0_name_free(name);
    check_size(2);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_name_eq(name, s0_name_set_at(set, 1)),
       "s0_name_set_at(set, 1) == \"b\"");
    s0_name_free(name);
    check_size(2);

    s0_name_set_free(set);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * S₀: Name mappings
 */

#define TEST_COUNT_S0_NAME_MAPPINGS  28

static void
test_s0_name_mappings(void)
{
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_name_mapping_entry  entry;

    diag("S₀ name mappings");

#define check_size(expected) \
    ok(s0_name_mapping_size(mapping) == expected, \
       "s0_name_mapping_size(mapping) == " #expected);

    ok_alloc(mapping, s0_name_mapping_new());
    check_size(0);

    ok_alloc(from, s0_name_new_str("a"));
    ok_alloc(to, s0_name_new_str("b"));
    ok0(s0_name_mapping_add(mapping, from, to),
        "s0_name_mapping_add(\"a\", \"b\")");
    check_size(1);

    ok_alloc(from, s0_name_new_str("c"));
    ok_alloc(to, s0_name_new_str("d"));
    ok0(s0_name_mapping_add(mapping, from, to),
        "s0_name_mapping_add(\"c\", \"d\")");
    check_size(2);

    ok_alloc(from, s0_name_new_str("a"));
    ok_alloc(to, s0_name_new_str("b"));
    ok(s0_name_eq(s0_name_mapping_get(mapping, from), to),
       "s0_name_mapping_get(mapping, \"a\") == \"b\"");
    s0_name_free(from);
    s0_name_free(to);
    check_size(2);

    ok_alloc(from, s0_name_new_str("c"));
    ok_alloc(to, s0_name_new_str("d"));
    ok(s0_name_eq(s0_name_mapping_get(mapping, from), to),
       "s0_name_mapping_get(mapping, \"c\") == \"d\"");
    s0_name_free(from);
    s0_name_free(to);
    check_size(2);

    entry = s0_name_mapping_at(mapping, 0);
    ok_alloc(from, s0_name_new_str("a"));
    ok_alloc(to, s0_name_new_str("b"));
    ok(s0_name_eq(entry.from, from),
       "s0_name_mapping_at(mapping, 0).from == \"a\"");
    ok(s0_name_eq(entry.to, to),
       "s0_name_mapping_at(mapping, 0).to == \"b\"");
    s0_name_free(from);
    s0_name_free(to);
    check_size(2);

    entry = s0_name_mapping_at(mapping, 1);
    ok_alloc(from, s0_name_new_str("c"));
    ok_alloc(to, s0_name_new_str("d"));
    ok(s0_name_eq(entry.from, from),
       "s0_name_mapping_at(mapping, 1).from == \"c\"");
    ok(s0_name_eq(entry.to, to),
       "s0_name_mapping_at(mapping, 1).to == \"d\"");
    s0_name_free(from);
    s0_name_free(to);
    check_size(2);

    s0_name_mapping_free(mapping);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * S₀: Named blocks
 */

#define TEST_COUNT_S0_NAMED_BLOCKS  16

static void
test_s0_named_blocks(void)
{
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;
    struct s0_block  *block1;
    struct s0_block  *block2;

    diag("S₀ named blocks");

#define check_size(expected) \
    ok(s0_named_blocks_size(blocks) == expected, \
       "s0_named_blocks_size(blocks) == " #expected);

    ok_alloc(blocks, s0_named_blocks_new());
    check_size(0);

    ok_alloc(name, s0_name_new_str("a"));
    ok_alloc(block1, create_empty_block());
    ok0(s0_named_blocks_add(blocks, name, block1),
        "s0_named_blocks_add(\"a\", block1)");
    check_size(1);

    ok_alloc(name, s0_name_new_str("b"));
    ok_alloc(block2, create_empty_block());
    ok0(s0_named_blocks_add(blocks, name, block2),
        "s0_named_blocks_add(\"b\", block2)");
    check_size(2);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_named_blocks_get(blocks, name) == block1,
       "s0_named_blocks_get(blocks, \"a\") == block1");
    s0_name_free(name);
    check_size(2);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_named_blocks_get(blocks, name) == block2,
       "s0_named_blocks_get(blocks, \"b\") == block2");
    s0_name_free(name);
    check_size(2);

    s0_named_blocks_free(blocks);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * S₀: Statements
 */

#define TEST_COUNT_S0_STATEMENTS  31

static void
test_s0_statements(void)
{
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;
    struct s0_name  *self_input;
    struct s0_block  *body;
    struct s0_statement  *stmt;

    diag("S₀ statements");

    /* create_atom */

    ok_alloc(dest, s0_name_new_str("a"));
    ok_alloc(stmt, s0_create_atom_new(dest));
    ok(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_ATOM,
       "type(stmt) == create_atom");

    ok_alloc(dest, s0_name_new_str("a"));
    ok(s0_name_eq(s0_create_atom_dest(stmt), dest), "dest(stmt) == \"a\"");
    s0_name_free(dest);

    s0_statement_free(stmt);

    /* create_closure */

    ok_alloc(dest, s0_name_new_str("a"));
    ok_alloc(closed_over, s0_name_set_new());
    ok_alloc(branches, s0_named_blocks_new());
    ok_alloc(stmt, s0_create_closure_new(dest, closed_over, branches));
    ok(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_CLOSURE,
       "type(stmt) == create_closure");

    ok_alloc(dest, s0_name_new_str("a"));
    ok(s0_name_eq(s0_create_closure_dest(stmt), dest), "dest(stmt) == \"a\"");
    s0_name_free(dest);

    ok(s0_create_closure_closed_over(stmt) == closed_over,
       "closed_over(stmt) == closed_over");
    ok(s0_create_closure_branches(stmt) == branches,
       "branches(stmt) == branches");

    s0_statement_free(stmt);

    /* create_literal */

    ok_alloc(dest, s0_name_new_str("a"));
    ok_alloc(stmt, s0_create_literal_new(dest, 5, "hello"));
    ok(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_LITERAL,
       "type(stmt) == create_literal");

    ok_alloc(dest, s0_name_new_str("a"));
    ok(s0_name_eq(s0_create_literal_dest(stmt), dest), "dest(stmt) == \"a\"");
    s0_name_free(dest);

    ok(s0_create_literal_size(stmt) == 5,
       "[create_literal(5, \"hello\")] == 5");
    ok(memcmp(s0_create_literal_content(stmt), "hello", 5) == 0,
       "create_literal(5, \"hello\") == \"hello\"");

    s0_statement_free(stmt);

    /* create_method */

    ok_alloc(dest, s0_name_new_str("a"));
    ok_alloc(self_input, s0_name_new_str("self"));
    ok_alloc(body, create_empty_block());
    ok_alloc(stmt, s0_create_method_new(dest, self_input, body));
    ok(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_METHOD,
       "type(stmt) == create_method");

    ok_alloc(dest, s0_name_new_str("a"));
    ok(s0_name_eq(s0_create_method_dest(stmt), dest), "dest(stmt) == \"a\"");
    s0_name_free(dest);

    ok_alloc(self_input, s0_name_new_str("self"));
    ok(s0_name_eq(s0_create_method_self_input(stmt), self_input),
       "self_input(stmt) == \"self\"");
    s0_name_free(self_input);

    ok(s0_create_method_body(stmt) == body, "body(stmt) == body");

    s0_statement_free(stmt);
}

/*-----------------------------------------------------------------------------
 * S₀: Statement lists
 */

#define TEST_COUNT_S0_STATEMENT_LISTS  14

static void
test_s0_statement_lists(void)
{
    struct s0_statement_list  *list;
    struct s0_name  *dest;
    struct s0_statement  *stmt1;
    struct s0_statement  *stmt2;

    diag("S₀ statement list");

#define check_size(expected) \
    ok(s0_statement_list_size(list) == expected, \
       "s0_statement_list_size(list) == " #expected);

    ok_alloc(list, s0_statement_list_new());
    check_size(0);

    ok_alloc(dest, s0_name_new_str("a"));
    ok_alloc(stmt1, s0_create_atom_new(dest));
    ok0(s0_statement_list_add(list, stmt1),
        "s0_statement_list_add(list, create_atom(\"a\"))");
    check_size(1);

    ok_alloc(dest, s0_name_new_str("b"));
    ok_alloc(stmt2, s0_create_atom_new(dest));
    ok0(s0_statement_list_add(list, stmt2),
        "s0_statement_list_add(list, create_atom(\"b\"))");
    check_size(2);

    ok(s0_statement_list_at(list, 0) == stmt1,
       "s0_statement_list_at(list, 0) == create_atom(\"a\")");
    check_size(2);

    ok(s0_statement_list_at(list, 1) == stmt2,
       "s0_statement_list_at(list, 2) == create_atom(\"b\")");
    check_size(2);

    s0_statement_list_free(list);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * S₀: Invocations
 */

#define TEST_COUNT_S0_INVOCATIONS  16

static void
test_s0_invocations(void)
{
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name  *method;
    struct s0_invocation  *invocation;

    diag("S₀ invocations");

    /* invoke_closure */

    ok_alloc(src, s0_name_new_str("a"));
    ok_alloc(branch, s0_name_new_str("body"));
    ok_alloc(invocation, s0_invoke_closure_new(src, branch));
    ok(s0_invocation_type(invocation) == S0_INVOCATION_TYPE_INVOKE_CLOSURE,
       "type(invocation) == invoke_closure");

    ok_alloc(src, s0_name_new_str("a"));
    ok(s0_name_eq(s0_invoke_closure_src(invocation), src),
       "src(invocation) == \"a\"");
    s0_name_free(src);

    ok_alloc(branch, s0_name_new_str("body"));
    ok(s0_name_eq(s0_invoke_closure_branch(invocation), branch),
       "branch(invocation) == \"body\"");
    s0_name_free(branch);

    s0_invocation_free(invocation);

    /* invoke_method */

    ok_alloc(src, s0_name_new_str("a"));
    ok_alloc(method, s0_name_new_str("run"));
    ok_alloc(invocation, s0_invoke_method_new(src, method));
    ok(s0_invocation_type(invocation) == S0_INVOCATION_TYPE_INVOKE_METHOD,
       "type(invocation) == invoke_method");

    ok_alloc(src, s0_name_new_str("a"));
    ok(s0_name_eq(s0_invoke_method_src(invocation), src),
       "src(invocation) == \"a\"");
    s0_name_free(src);

    ok_alloc(method, s0_name_new_str("run"));
    ok(s0_name_eq(s0_invoke_method_method(invocation), method),
       "method(invocation) == \"run\"");
    s0_name_free(method);

    s0_invocation_free(invocation);
}

/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

#define TEST_COUNT_S0_BLOCKS  9

static void
test_s0_blocks(void)
{
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *inputs;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;
    struct s0_block  *block;

    diag("S₀ blocks");

    ok_alloc(inputs, s0_name_mapping_new());
    ok_alloc(statements, s0_statement_list_new());
    ok_alloc(src, s0_name_new_str("x"));
    ok_alloc(branch, s0_name_new_str("body"));
    ok_alloc(invocation, s0_invoke_closure_new(src, branch));
    ok_alloc(block, s0_block_new(inputs, statements, invocation));

    ok(s0_block_inputs(block) == inputs, "inputs(block) == inputs");
    ok(s0_block_statements(block) == statements,
       "statements(block) == statements");
    ok(s0_block_invocation(block) == invocation,
       "invocation(block) == invocation");

    s0_block_free(block);
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
 * S₀: Closures
 */

#define TEST_COUNT_S0_CLOSURES  6

static void
test_s0_closures(void)
{
    struct s0_entity  *closure;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;

    diag("S₀ closures");

    ok_alloc(env, s0_environment_new());
    ok_alloc(blocks, s0_named_blocks_new());
    ok_alloc(closure, s0_closure_new(env, blocks));

    ok(s0_entity_type(closure) == S0_ENTITY_TYPE_CLOSURE,
       "type(closure) == closure");

    ok(s0_closure_environment(closure) == env, "env(closure) == env");
    ok(s0_closure_named_blocks(closure) == blocks, "blocks(closure) == blocks");

    s0_entity_free(closure);
}

/*-----------------------------------------------------------------------------
 * S₀: Literals
 */

#define TEST_COUNT_S0_LITERALS  12

static void
test_s0_literals(void)
{
    struct s0_entity  *l1;
    struct s0_entity  *l2;
    struct s0_entity  *l3;

    diag("S₀ literals");

    ok_alloc(l1, s0_literal_new_str("hello"));
    ok_alloc(l2, s0_literal_new(5, "hello"));
    /* content includes NUL terminator */
    ok_alloc(l3, s0_literal_new(6, "hello"));

    ok(s0_entity_type(l1) == S0_ENTITY_TYPE_LITERAL,
       "type(literal1) == literal");
    ok(s0_entity_type(l2) == S0_ENTITY_TYPE_LITERAL,
       "type(literal2) == literal");
    ok(s0_entity_type(l3) == S0_ENTITY_TYPE_LITERAL,
       "type(literal3) == literal");

    ok(s0_literal_size(l1) == 5, "[literal(5, \"hello\")] == 5");
    ok(s0_literal_size(l2) == 5, "[literal(5, \"hello\")] == 5");
    ok(s0_literal_size(l3) == 6, "[literal(6, \"hello\\x00\")] == 6");

    ok(memcmp(s0_literal_content(l1), "hello", 5) == 0,
       "literal(5, \"hello\") == \"hello\"");
    ok(memcmp(s0_literal_content(l2), "hello", 5) == 0,
       "literal(5, \"hello\") == \"hello\"");
    ok(memcmp(s0_literal_content(l3), "hello\x00", 6) == 0,
       "literal(6, \"hello\\x00\") == \"hello\\x00\"");

    s0_entity_free(l1);
    s0_entity_free(l2);
    s0_entity_free(l3);
}

/*-----------------------------------------------------------------------------
 * S₀: Methods
 */

#define TEST_COUNT_S0_METHODS  7

static void
test_s0_methods(void)
{
    struct s0_entity  *method;
    struct s0_name  *self_name;
    struct s0_block  *block;

    diag("S₀ methods");

    ok_alloc(self_name, s0_name_new_str("self"));
    ok_alloc(block, create_empty_block());
    ok_alloc(method, s0_method_new(self_name, block));

    ok(s0_entity_type(method) == S0_ENTITY_TYPE_METHOD,
       "type(method) == method");

    ok_alloc(self_name, s0_name_new_str("self"));
    ok(s0_name_eq(s0_method_self_name(method), self_name),
       "self_name(method) == self_name");
    s0_name_free(self_name);

    ok(s0_method_block(method) == block, "block(method) == block");

    s0_entity_free(method);
}

/*-----------------------------------------------------------------------------
 * S₀: Objects
 */

#define TEST_COUNT_S0_OBJECTS  25

static void
test_s0_objects(void)
{
    struct s0_entity  *obj;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;
    struct s0_object_entry  entry;

    diag("S₀ objects");

#define check_size(expected) \
    ok(s0_object_size(obj) == expected, \
       "s0_object_size(obj) == " #expected);

    ok_alloc(obj, s0_object_new());
    check_size(0);
    ok(s0_entity_type(obj) == S0_ENTITY_TYPE_OBJECT, "type(obj) == object");

    ok_alloc(name, s0_name_new_str("a"));
    ok_alloc(atom1, s0_atom_new());
    ok0(s0_object_add(obj, name, atom1),
        "s0_object_add(\"a\", atom1)");
    check_size(1);

    ok_alloc(name, s0_name_new_str("b"));
    ok_alloc(atom2, s0_atom_new());
    ok0(s0_object_add(obj, name, atom2),
        "s0_object_add(\"b\", atom2)");
    check_size(2);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_object_get(obj, name) == atom1,
       "s0_object_get(obj, \"a\") == atom1");
    s0_name_free(name);
    check_size(2);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_object_get(obj, name) == atom2,
       "s0_object_get(obj, \"b\") == atom2");
    s0_name_free(name);
    check_size(2);

    entry = s0_object_at(obj, 0);
    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_name_eq(entry.name, name), "s0_object_at(obj, 0) == \"a\"");
    s0_name_free(name);
    ok(entry.entity == atom1, "s0_object_at(obj, 0) == atom1");
    check_size(2);

    entry = s0_object_at(obj, 1);
    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_name_eq(entry.name, name), "s0_object_at(obj, 1) == \"b\"");
    s0_name_free(name);
    ok(entry.entity == atom2, "s0_object_at(obj, 1) == atom2");
    check_size(2);

    s0_entity_free(obj);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * S₀: Environments
 */

#define TEST_COUNT_S0_ENVIRONMENTS  28

static void
test_s0_environments(void)
{
    struct s0_environment  *env;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;

    diag("S₀ environments");

#define check_size(expected) \
    ok(s0_environment_size(env) == expected, \
       "s0_environment_size(env) == " #expected);

    ok_alloc(env, s0_environment_new());
    check_size(0);

    ok_alloc(name, s0_name_new_str("a"));
    ok_alloc(atom1, s0_atom_new());
    ok0(s0_environment_add(env, name, atom1),
        "s0_environment_add(\"a\", atom1)");
    check_size(1);

    ok_alloc(name, s0_name_new_str("b"));
    ok_alloc(atom2, s0_atom_new());
    ok0(s0_environment_add(env, name, atom2),
        "s0_environment_add(\"b\", atom2)");
    check_size(2);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_environment_get(env, name) == atom1,
       "s0_environment_get(env, \"a\") == atom1");
    s0_name_free(name);
    check_size(2);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_environment_get(env, name) == atom2,
       "s0_environment_get(env, \"b\") == atom2");
    s0_name_free(name);
    check_size(2);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_environment_delete(env, name) == atom1,
       "s0_environment_delete(env, \"a\") == atom1");
    s0_name_free(name);
    s0_entity_free(atom1);
    check_size(1);

    ok_alloc(name, s0_name_new_str("a"));
    ok(s0_environment_get(env, name) == NULL,
       "s0_environment_get(env, \"a\") == NULL");
    s0_name_free(name);
    check_size(1);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_environment_delete(env, name) == atom2,
       "s0_environment_delete(env, \"b\") == atom2");
    s0_name_free(name);
    s0_entity_free(atom2);
    check_size(0);

    ok_alloc(name, s0_name_new_str("b"));
    ok(s0_environment_get(env, name) == NULL,
       "s0_environment_get(env, \"b\") == NULL");
    s0_name_free(name);
    check_size(0);

    s0_environment_free(env);
#undef check_size
}

/*-----------------------------------------------------------------------------
 * Harness
 */

#define TEST_COUNT_TOTAL \
    TEST_COUNT_S0_NAMES + \
    TEST_COUNT_S0_NAME_SETS + \
    TEST_COUNT_S0_NAME_MAPPINGS + \
    TEST_COUNT_S0_NAMED_BLOCKS + \
    TEST_COUNT_S0_STATEMENTS + \
    TEST_COUNT_S0_STATEMENT_LISTS + \
    TEST_COUNT_S0_INVOCATIONS + \
    TEST_COUNT_S0_BLOCKS + \
    TEST_COUNT_S0_ATOMS + \
    TEST_COUNT_S0_CLOSURES + \
    TEST_COUNT_S0_LITERALS + \
    TEST_COUNT_S0_METHODS + \
    TEST_COUNT_S0_OBJECTS + \
    TEST_COUNT_S0_ENVIRONMENTS + \
    0

int main(void)
{
    plan_tests(TEST_COUNT_TOTAL);
    test_s0_names();
    test_s0_name_sets();
    test_s0_name_mappings();
    test_s0_named_blocks();
    test_s0_statements();
    test_s0_statement_lists();
    test_s0_invocations();
    test_s0_blocks();
    test_s0_atoms();
    test_s0_closures();
    test_s0_literals();
    test_s0_methods();
    test_s0_objects();
    test_s0_environments();
    return exit_status();
}
