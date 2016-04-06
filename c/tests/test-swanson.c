/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include <string.h>

#include "swanson.h"
#include "test-cases.h"

/*-----------------------------------------------------------------------------
 * Helpers
 */

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

TEST_CASE_GROUP("S₀ names");

TEST_CASE("can create name from C string") {
    struct s0_name  *name;
    check_alloc(name, s0_name_new_str("hello"));
    check(s0_name_size(name) == 5);
    check(memcmp(s0_name_content(name), "hello", 5) == 0);
    s0_name_free(name);
}

TEST_CASE("can create name with explicit length") {
    struct s0_name  *name;
    check_alloc(name, s0_name_new(5, "hello"));
    check(s0_name_size(name) == 5);
    check(memcmp(s0_name_content(name), "hello", 5) == 0);
    s0_name_free(name);
}

TEST_CASE("can create name with embedded NUL") {
    struct s0_name  *name;
    check_alloc(name, s0_name_new(6, "hello\x00"));
    check(s0_name_size(name) == 6);
    check(memcmp(s0_name_content(name), "hello\x00", 6) == 0);
    s0_name_free(name);
}

TEST_CASE("can compare names") {
    struct s0_name  *n1;
    struct s0_name  *n2;
    struct s0_name  *n3;
    check_alloc(n1, s0_name_new_str("hello"));
    check_alloc(n2, s0_name_new(5, "hello"));
    check_alloc(n3, s0_name_new(6, "hello\x00"));
    check(s0_name_eq(n1, n2));
    check(!s0_name_eq(n1, n3));
    s0_name_free(n1);
    s0_name_free(n2);
    s0_name_free(n3);
}

/*-----------------------------------------------------------------------------
 * S₀: Name sets
 */

TEST_CASE_GROUP("S₀ name sets");

TEST_CASE("can create empty name set") {
    struct s0_name_set  *set;
    check_alloc(set, s0_name_set_new());
    s0_name_set_free(set);
}

TEST_CASE("empty name set has zero elements") {
    struct s0_name_set  *set;
    check_alloc(set, s0_name_set_new());
    check(s0_name_set_size(set) == 0);
    s0_name_set_free(set);
}

TEST_CASE("empty name set doesn't contain anything") {
    struct s0_name_set  *set;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check(!s0_name_set_contains(set, name));
    s0_name_free(name);
    s0_name_set_free(set);
}

TEST_CASE("can add names to set") {
    struct s0_name_set  *set;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    s0_name_set_free(set);
}

TEST_CASE("non-empty name set has accurate size") {
    struct s0_name_set  *set;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());

    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check(s0_name_set_size(set) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check0(s0_name_set_add(set, name));
    check(s0_name_set_size(set) == 2);

    s0_name_set_free(set);
}

TEST_CASE("can check which names belong to set") {
    struct s0_name_set  *set;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check_alloc(name, s0_name_new_str("b"));
    check0(s0_name_set_add(set, name));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_name_set_contains(set, name));
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_set_contains(set, name));
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(!s0_name_set_contains(set, name));
    s0_name_free(name);

    s0_name_set_free(set);
}

TEST_CASE("can iterate through names in set") {
    struct s0_name_set  *set;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check_alloc(name, s0_name_new_str("b"));
    check0(s0_name_set_add(set, name));

    /* This test assumes that the names are returned in the same order that they
     * were added to the set. */

    check_alloc(name, s0_name_new_str("a"));
    check(s0_name_eq(name, s0_name_set_at(set, 0)));
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_eq(name, s0_name_set_at(set, 1)));
    s0_name_free(name);

    s0_name_set_free(set);
}

/*-----------------------------------------------------------------------------
 * S₀: Name mappings
 */

TEST_CASE_GROUP("S₀ name mappings");

TEST_CASE("can create empty name mapping") {
    struct s0_name_mapping  *mapping;
    check_alloc(mapping, s0_name_mapping_new());
    s0_name_mapping_free(mapping);
}

TEST_CASE("empty name mapping has zero elements") {
    struct s0_name_mapping  *mapping;
    check_alloc(mapping, s0_name_mapping_new());
    check(s0_name_mapping_size(mapping) == 0);
    s0_name_mapping_free(mapping);
}

TEST_CASE("empty name mapping doesn't contain anything") {
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check(s0_name_mapping_get(mapping, from) == NULL);
    s0_name_free(from);
    s0_name_mapping_free(mapping);
}

TEST_CASE("can add names to mapping") {
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    s0_name_mapping_free(mapping);
}

TEST_CASE("non-empty name mapping has accurate size") {
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    check_alloc(mapping, s0_name_mapping_new());

    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check(s0_name_mapping_size(mapping) == 1);

    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));
    check(s0_name_mapping_size(mapping) == 2);

    s0_name_mapping_free(mapping);
}

TEST_CASE("can check which names belong to mapping") {
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));

    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check(s0_name_eq(s0_name_mapping_get(mapping, from), to));
    s0_name_free(from);
    s0_name_free(to);

    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check(s0_name_eq(s0_name_mapping_get(mapping, from), to));
    s0_name_free(from);
    s0_name_free(to);

    check_alloc(from, s0_name_new_str("e"));
    check(s0_name_mapping_get(mapping, from) == NULL);
    s0_name_free(from);

    s0_name_mapping_free(mapping);
}

TEST_CASE("can iterate through names in mapping") {
    struct s0_name_mapping  *mapping;
    struct s0_name_mapping_entry  entry;
    struct s0_name  *from;
    struct s0_name  *to;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));

    /* This test assumes that the names are returned in the same order that they
     * were added to the mapping. */

    entry = s0_name_mapping_at(mapping, 0);
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check(s0_name_eq(entry.from, from));
    check(s0_name_eq(entry.to, to));
    s0_name_free(from);
    s0_name_free(to);

    entry = s0_name_mapping_at(mapping, 1);
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check(s0_name_eq(entry.from, from));
    check(s0_name_eq(entry.to, to));
    s0_name_free(from);
    s0_name_free(to);

    s0_name_mapping_free(mapping);
}

/*-----------------------------------------------------------------------------
 * S₀: Named blocks
 */

TEST_CASE_GROUP("S₀ named blocks");

TEST_CASE("can create empty named blocks") {
    struct s0_named_blocks  *blocks;
    check_alloc(blocks, s0_named_blocks_new());
    s0_named_blocks_free(blocks);
}

TEST_CASE("empty named blocks has zero elements") {
    struct s0_named_blocks  *blocks;
    check_alloc(blocks, s0_named_blocks_new());
    check(s0_named_blocks_size(blocks) == 0);
    s0_named_blocks_free(blocks);
}

TEST_CASE("empty named blocks doesn't contain anything") {
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(name, s0_name_new_str("a"));
    check(s0_named_blocks_get(blocks, name) == NULL);
    s0_name_free(name);
    s0_named_blocks_free(blocks);
}

TEST_CASE("can add names to blocks") {
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;
    struct s0_block  *block;
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(block, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block));
    s0_named_blocks_free(blocks);
}

TEST_CASE("non-empty named blocks has accurate size") {
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;
    struct s0_block  *block;
    check_alloc(blocks, s0_named_blocks_new());

    check_alloc(name, s0_name_new_str("a"));
    check_alloc(block, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block));
    check(s0_named_blocks_size(blocks) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check_alloc(block, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block));
    check(s0_named_blocks_size(blocks) == 2);

    s0_named_blocks_free(blocks);
}

TEST_CASE("can check which names belong to blocks") {
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;
    struct s0_block  *block1;
    struct s0_block  *block2;
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(block1, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(block2, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_named_blocks_get(blocks, name) == block1);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_named_blocks_get(blocks, name) == block2);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_named_blocks_get(blocks, name) == NULL);
    s0_name_free(name);

    s0_named_blocks_free(blocks);
}

/*-----------------------------------------------------------------------------
 * S₀: Statements
 */

TEST_CASE_GROUP("S₀ statements");

TEST_CASE("can create `create-atom` statement") {
    struct s0_name  *dest;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_ATOM);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_atom_dest(stmt), dest));
    s0_name_free(dest);

    s0_statement_free(stmt);
}

TEST_CASE("can create `create-closure` statement") {
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(closed_over, s0_name_set_new());
    check_alloc(branches, s0_named_blocks_new());
    check_alloc(stmt, s0_create_closure_new(dest, closed_over, branches));
    check(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_CLOSURE);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_closure_dest(stmt), dest));
    s0_name_free(dest);

    check(s0_create_closure_closed_over(stmt) == closed_over);
    check(s0_create_closure_branches(stmt) == branches);

    s0_statement_free(stmt);
}

TEST_CASE("can create `create-literal` statement") {
    struct s0_name  *dest;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_literal_new(dest, 5, "hello"));
    check(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_LITERAL);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_literal_dest(stmt), dest));
    s0_name_free(dest);

    check(s0_create_literal_size(stmt) == 5);
    check(memcmp(s0_create_literal_content(stmt), "hello", 5) == 0);

    s0_statement_free(stmt);
}

TEST_CASE("can create `create-method` statement") {
    struct s0_name  *dest;
    struct s0_name  *self_input;
    struct s0_block  *body;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(self_input, s0_name_new_str("self"));
    check_alloc(body, create_empty_block());
    check_alloc(stmt, s0_create_method_new(dest, self_input, body));
    check(s0_statement_type(stmt) == S0_STATEMENT_TYPE_CREATE_METHOD);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_method_dest(stmt), dest));
    s0_name_free(dest);

    check_alloc(self_input, s0_name_new_str("self"));
    check(s0_name_eq(s0_create_method_self_input(stmt), self_input));
    s0_name_free(self_input);

    check(s0_create_method_body(stmt) == body);

    s0_statement_free(stmt);
}

/*-----------------------------------------------------------------------------
 * S₀: Statement lists
 */

TEST_CASE_GROUP("S₀ statement lists");

TEST_CASE("can create empty statement list") {
    struct s0_statement_list  *list;
    check_alloc(list, s0_statement_list_new());
    s0_statement_list_free(list);
}

TEST_CASE("empty statement list has zero elements") {
    struct s0_statement_list  *list;
    check_alloc(list, s0_statement_list_new());
    check(s0_statement_list_size(list) == 0);
    s0_statement_list_free(list);
}

TEST_CASE("can add statements to list") {
    struct s0_statement_list  *list;
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    check_alloc(list, s0_statement_list_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt));
    s0_statement_list_free(list);
}

TEST_CASE("non-empty statement list has accurate size") {
    struct s0_statement_list  *list;
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    check_alloc(list, s0_statement_list_new());

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt));
    check(s0_statement_list_size(list) == 1);

    check_alloc(dest, s0_name_new_str("b"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt));
    check(s0_statement_list_size(list) == 2);

    s0_statement_list_free(list);
}

TEST_CASE("can iterate through statements in list") {
    struct s0_statement_list  *list;
    struct s0_name  *dest;
    struct s0_statement  *stmt1;
    struct s0_statement  *stmt2;
    check_alloc(list, s0_statement_list_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt1, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt1));
    check_alloc(dest, s0_name_new_str("b"));
    check_alloc(stmt2, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt2));

    check(s0_statement_list_at(list, 0) == stmt1);
    check(s0_statement_list_at(list, 1) == stmt2);

    s0_statement_list_free(list);
}

/*-----------------------------------------------------------------------------
 * S₀: Invocations
 */

TEST_CASE_GROUP("S₀ invocations");

TEST_CASE("can create `invoke-closure` invocation") {
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_invocation  *invocation;

    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(invocation, s0_invoke_closure_new(src, branch));
    check(s0_invocation_type(invocation) == S0_INVOCATION_TYPE_INVOKE_CLOSURE);

    check_alloc(src, s0_name_new_str("a"));
    check(s0_name_eq(s0_invoke_closure_src(invocation), src));
    s0_name_free(src);

    check_alloc(branch, s0_name_new_str("body"));
    check(s0_name_eq(s0_invoke_closure_branch(invocation), branch));
    s0_name_free(branch);

    s0_invocation_free(invocation);
}

TEST_CASE("can create `invoke-method` invocation") {
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_invocation  *invocation;

    check_alloc(src, s0_name_new_str("a"));
    check_alloc(method, s0_name_new_str("run"));
    check_alloc(invocation, s0_invoke_method_new(src, method));
    check(s0_invocation_type(invocation) == S0_INVOCATION_TYPE_INVOKE_METHOD);

    check_alloc(src, s0_name_new_str("a"));
    check(s0_name_eq(s0_invoke_method_src(invocation), src));
    s0_name_free(src);

    check_alloc(method, s0_name_new_str("run"));
    check(s0_name_eq(s0_invoke_method_method(invocation), method));
    s0_name_free(method);

    s0_invocation_free(invocation);
}

/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

TEST_CASE_GROUP("S₀ blocks");

TEST_CASE("can create block") {
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *inputs;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    check_alloc(inputs, s0_name_mapping_new());
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("x"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(invocation, s0_invoke_closure_new(src, branch));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check(s0_block_inputs(block) == inputs);
    check(s0_block_statements(block) == statements);
    check(s0_block_invocation(block) == invocation);
    s0_block_free(block);
}

/*-----------------------------------------------------------------------------
 * S₀: Atoms
 */

TEST_CASE_GROUP("S₀ atoms");

TEST_CASE("can create atom") {
    struct s0_entity  *atom;
    check_alloc(atom, s0_atom_new());
    check(s0_entity_type(atom) == S0_ENTITY_TYPE_ATOM);
    s0_entity_free(atom);
}

TEST_CASE("atoms should equal themselves") {
    struct s0_entity  *a1;
    struct s0_entity  *a2;
    struct s0_entity  *a3;
    check_alloc(a1, s0_atom_new());
    check_alloc(a2, s0_atom_new());
    check_alloc(a3, s0_atom_new());
    check(s0_atom_eq(a1, a1));
    check(s0_atom_eq(a2, a2));
    check(s0_atom_eq(a3, a3));
    s0_entity_free(a1);
    s0_entity_free(a2);
    s0_entity_free(a3);
}

TEST_CASE("atoms should not equal any others") {
    struct s0_entity  *a1;
    struct s0_entity  *a2;
    struct s0_entity  *a3;
    check_alloc(a1, s0_atom_new());
    check_alloc(a2, s0_atom_new());
    check_alloc(a3, s0_atom_new());
    check(!s0_atom_eq(a1, a2));
    check(!s0_atom_eq(a1, a3));
    check(!s0_atom_eq(a2, a3));
    s0_entity_free(a1);
    s0_entity_free(a2);
    s0_entity_free(a3);
}

/*-----------------------------------------------------------------------------
 * S₀: Closures
 */

TEST_CASE_GROUP("S₀ closures");

TEST_CASE("can create closure") {
    struct s0_entity  *closure;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(closure, s0_closure_new(env, blocks));
    check(s0_entity_type(closure) == S0_ENTITY_TYPE_CLOSURE);
    check(s0_closure_environment(closure) == env);
    check(s0_closure_named_blocks(closure) == blocks);
    s0_entity_free(closure);
}

/*-----------------------------------------------------------------------------
 * S₀: Literals
 */

TEST_CASE_GROUP("S₀ literals");

TEST_CASE("can create literal from C string") {
    struct s0_entity  *literal;
    check_alloc(literal, s0_literal_new_str("hello"));
    check(s0_entity_type(literal) == S0_ENTITY_TYPE_LITERAL);
    check(s0_literal_size(literal) == 5);
    check(memcmp(s0_literal_content(literal), "hello", 5) == 0);
    s0_entity_free(literal);
}

TEST_CASE("can create literal with explicit length") {
    struct s0_entity  *literal;
    check_alloc(literal, s0_literal_new(5, "hello"));
    check(s0_entity_type(literal) == S0_ENTITY_TYPE_LITERAL);
    check(s0_literal_size(literal) == 5);
    check(memcmp(s0_literal_content(literal), "hello", 5) == 0);
    s0_entity_free(literal);
}

TEST_CASE("can create literal with embedded NUL") {
    struct s0_entity  *literal;
    check_alloc(literal, s0_literal_new(6, "hello\x00"));
    check(s0_entity_type(literal) == S0_ENTITY_TYPE_LITERAL);
    check(s0_literal_size(literal) == 6);
    check(memcmp(s0_literal_content(literal), "hello\x00", 6) == 0);
    s0_entity_free(literal);
}

/*-----------------------------------------------------------------------------
 * S₀: Methods
 */

TEST_CASE_GROUP("S₀ methods");

TEST_CASE("can create method") {
    struct s0_entity  *method;
    struct s0_name  *self_name;
    struct s0_block  *block;
    check_alloc(self_name, s0_name_new_str("self"));
    check_alloc(block, create_empty_block());
    check_alloc(method, s0_method_new(self_name, block));
    check(s0_entity_type(method) == S0_ENTITY_TYPE_METHOD);
    check_alloc(self_name, s0_name_new_str("self"));
    check(s0_name_eq(s0_method_self_name(method), self_name));
    s0_name_free(self_name);
    check(s0_method_block(method) == block);
    s0_entity_free(method);
}

/*-----------------------------------------------------------------------------
 * S₀: Objects
 */

TEST_CASE_GROUP("S₀ objects");

TEST_CASE("can create empty object") {
    struct s0_entity  *obj;
    check_alloc(obj, s0_object_new());
    s0_entity_free(obj);
}

TEST_CASE("empty object has zero elements") {
    struct s0_entity  *obj;
    check_alloc(obj, s0_object_new());
    check(s0_object_size(obj) == 0);
    s0_entity_free(obj);
}

TEST_CASE("empty object doesn't contain anything") {
    struct s0_entity  *obj;
    struct s0_name  *name;
    check_alloc(obj, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check(s0_object_get(obj, name) == NULL);
    s0_name_free(name);
    s0_entity_free(obj);
}

TEST_CASE("can add entries to object") {
    struct s0_entity  *obj;
    struct s0_name  *name;
    struct s0_entity  *atom;
    check_alloc(obj, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_object_add(obj, name, atom));
    s0_entity_free(obj);
}

TEST_CASE("non-empty object has accurate size") {
    struct s0_entity  *obj;
    struct s0_name  *name;
    struct s0_entity  *atom;
    check_alloc(obj, s0_object_new());

    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_object_add(obj, name, atom));
    check(s0_object_size(obj) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_object_add(obj, name, atom));
    check(s0_object_size(obj) == 2);

    s0_entity_free(obj);
}

TEST_CASE("can check which names belong to object") {
    struct s0_entity  *obj;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;
    check_alloc(obj, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom1, s0_atom_new());
    check0(s0_object_add(obj, name, atom1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom2, s0_atom_new());
    check0(s0_object_add(obj, name, atom2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_object_get(obj, name) == atom1);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_object_get(obj, name) == atom2);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_object_get(obj, name) == NULL);
    s0_name_free(name);

    s0_entity_free(obj);
}

TEST_CASE("can iterate through names in obj") {
    struct s0_entity  *obj;
    struct s0_object_entry  entry;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;
    check_alloc(obj, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom1, s0_atom_new());
    check0(s0_object_add(obj, name, atom1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom2, s0_atom_new());
    check0(s0_object_add(obj, name, atom2));

    /* This test assumes that the entries are returned in the same order that
     * they were added to the obj. */

    entry = s0_object_at(obj, 0);
    check_alloc(name, s0_name_new_str("a"));
    check(s0_name_eq(entry.name, name));
    check(entry.entity == atom1);
    s0_name_free(name);

    entry = s0_object_at(obj, 1);
    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_eq(entry.name, name));
    check(entry.entity == atom2);
    s0_name_free(name);

    s0_entity_free(obj);
}

/*-----------------------------------------------------------------------------
 * S₀: Environments
 */

TEST_CASE_GROUP("S₀ environments");

TEST_CASE("can create empty environment") {
    struct s0_environment  *env;
    check_alloc(env, s0_environment_new());
    s0_environment_free(env);
}

TEST_CASE("empty environment has zero elements") {
    struct s0_environment  *env;
    check_alloc(env, s0_environment_new());
    check(s0_environment_size(env) == 0);
    s0_environment_free(env);
}

TEST_CASE("empty environment doesn't contain anything") {
    struct s0_environment  *env;
    struct s0_name  *name;
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_get(env, name) == NULL);
    s0_name_free(name);
    s0_environment_free(env);
}

TEST_CASE("can add entries to environment") {
    struct s0_environment  *env;
    struct s0_name  *name;
    struct s0_entity  *atom;
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    s0_environment_free(env);
}

TEST_CASE("non-empty environment has accurate size") {
    struct s0_environment  *env;
    struct s0_name  *name;
    struct s0_entity  *atom;
    check_alloc(env, s0_environment_new());

    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check(s0_environment_size(env) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check(s0_environment_size(env) == 2);

    s0_environment_free(env);
}

TEST_CASE("can check which names belong to environment") {
    struct s0_environment  *env;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom1, s0_atom_new());
    check0(s0_environment_add(env, name, atom1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom2, s0_atom_new());
    check0(s0_environment_add(env, name, atom2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_get(env, name) == atom1);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_get(env, name) == atom2);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_environment_get(env, name) == NULL);
    s0_name_free(name);

    s0_environment_free(env);
}

TEST_CASE("can delete entries from environment") {
    struct s0_environment  *env;
    struct s0_name  *name;
    struct s0_entity  *atom1;
    struct s0_entity  *atom2;

    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom1, s0_atom_new());
    check0(s0_environment_add(env, name, atom1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom2, s0_atom_new());
    check0(s0_environment_add(env, name, atom2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_delete(env, name) == atom1);
    s0_name_free(name);
    s0_entity_free(atom1);
    check(s0_environment_size(env) == 1);

    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_get(env, name) == NULL);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_delete(env, name) == atom2);
    s0_name_free(name);
    s0_entity_free(atom2);
    check(s0_environment_size(env) == 0);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_get(env, name) == NULL);
    s0_name_free(name);

    s0_environment_free(env);
}

/*-----------------------------------------------------------------------------
 * Harness
 */

int main(void)
{
    run_tests();
    return exit_status();
}
