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
    struct s0_environment_type  *inputs;
    struct s0_name_mapping  *params;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;
    inputs = s0_environment_type_new();
    statements = s0_statement_list_new();
    src = s0_name_new_str("x");
    branch = s0_name_new_str("body");
    params = s0_name_mapping_new();
    invocation = s0_invoke_closure_new(src, branch, params);
    return s0_block_new(inputs, statements, invocation);
}

#define YAML "%TAG !s0! tag:swanson-lang.org,2016:\n---\n"

static struct s0_entity_type *
entity_type(const char *str)
{
    struct s0_yaml_stream  *stream;
    struct s0_yaml_node  node;
    struct s0_entity_type  *type;

    stream = s0_yaml_stream_new_from_string(str);
    if (unlikely(stream == NULL)) {
        return NULL;
    }

    node = s0_yaml_stream_parse_document(stream);
    if (s0_yaml_node_is_error(node)) {
        fail(s0_yaml_stream_last_error(stream));
        s0_yaml_stream_free(stream);
        return NULL;
    }

    type = s0_yaml_document_parse_entity_type(node);
    if (type == NULL) {
        fail(s0_yaml_stream_last_error(stream));
    }
    s0_yaml_stream_free(stream);
    return type;
}

static struct s0_environment_type *
environment_type(const char *str)
{
    struct s0_yaml_stream  *stream;
    struct s0_yaml_node  node;
    struct s0_environment_type  *type;

    stream = s0_yaml_stream_new_from_string(str);
    if (unlikely(stream == NULL)) {
        return NULL;
    }

    node = s0_yaml_stream_parse_document(stream);
    if (s0_yaml_node_is_error(node)) {
        fail(s0_yaml_stream_last_error(stream));
        s0_yaml_stream_free(stream);
        return NULL;
    }

    type = s0_yaml_document_parse_environment_type(node);
    if (type == NULL) {
        fail(s0_yaml_stream_last_error(stream));
    }
    s0_yaml_stream_free(stream);
    return type;
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

TEST_CASE("can create copy of name") {
    struct s0_name  *name1;
    struct s0_name  *name2;
    check_alloc(name1, s0_name_new(6, "hello\x00"));
    check_alloc(name2, s0_name_new_copy(name1));
    check(s0_name_size(name2) == 6);
    check(memcmp(s0_name_content(name2), "hello\x00", 6) == 0);
    check(s0_name_eq(name1, name2));
    s0_name_free(name1);
    s0_name_free(name2);
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

TEST_CASE("can create a copy of a name set") {
    struct s0_name_set  *set;
    struct s0_name_set  *copy;
    struct s0_name  *name;
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check_alloc(name, s0_name_new_str("b"));
    check0(s0_name_set_add(set, name));
    check_alloc(copy, s0_name_set_new_copy(set));
    check(s0_name_set_eq(set, copy));
    s0_name_set_free(set);
    s0_name_set_free(copy);
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
    const struct s0_name  *actual;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));

    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check_nonnull(actual = s0_name_mapping_get(mapping, from));
    check(s0_name_eq(actual, to));
    s0_name_free(from);
    s0_name_free(to);

    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check_nonnull(actual = s0_name_mapping_get(mapping, from));
    check(s0_name_eq(actual, to));
    s0_name_free(from);
    s0_name_free(to);

    check_alloc(from, s0_name_new_str("e"));
    check(s0_name_mapping_get(mapping, from) == NULL);
    s0_name_free(from);

    s0_name_mapping_free(mapping);
}

TEST_CASE("can create a copy of a name mapping") {
    struct s0_name_mapping  *mapping;
    struct s0_name_mapping  *copy;
    struct s0_name  *from;
    struct s0_name  *to;
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(copy, s0_name_mapping_new_copy(mapping));
    check(s0_name_mapping_eq(mapping, copy));
    s0_name_mapping_free(mapping);
    s0_name_mapping_free(copy);
}

TEST_CASE("can iterate through names in mapping") {
    struct s0_name_mapping  *mapping;
    const struct s0_name_mapping_entry  *entry;
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

    check_nonnull(entry = s0_name_mapping_at(mapping, 0));
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check(s0_name_eq(entry->from, from));
    check(s0_name_eq(entry->to, to));
    s0_name_free(from);
    s0_name_free(to);

    check_nonnull(entry = s0_name_mapping_at(mapping, 1));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check(s0_name_eq(entry->from, from));
    check(s0_name_eq(entry->to, to));
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

TEST_CASE("can create a copy of named blocks") {
    struct s0_named_blocks  *blocks;
    struct s0_named_blocks  *copy;
    struct s0_name  *name;
    struct s0_block  *block;
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(block, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(block, create_empty_block());
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(copy, s0_named_blocks_new_copy(blocks));
    check(s0_named_blocks_eq(blocks, copy));
    s0_named_blocks_free(blocks);
    s0_named_blocks_free(copy);
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
    check(s0_statement_kind(stmt) == S0_STATEMENT_KIND_CREATE_ATOM);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_atom_dest(stmt), dest));
    s0_name_free(dest);

    s0_statement_free(stmt);
}

TEST_CASE("can copy `create-atom` statement") {
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    struct s0_statement  *copy;
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check_alloc(copy, s0_statement_new_copy(stmt));
    check(s0_statement_eq(stmt, copy));
    s0_statement_free(stmt);
    s0_statement_free(copy);
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
    check(s0_statement_kind(stmt) == S0_STATEMENT_KIND_CREATE_CLOSURE);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_closure_dest(stmt), dest));
    s0_name_free(dest);

    check(s0_create_closure_closed_over(stmt) == closed_over);
    check(s0_create_closure_branches(stmt) == branches);

    s0_statement_free(stmt);
}

TEST_CASE("can copy `create-closure` statement") {
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;
    struct s0_statement  *stmt;
    struct s0_statement  *copy;
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(closed_over, s0_name_set_new());
    check_alloc(branches, s0_named_blocks_new());
    check_alloc(stmt, s0_create_closure_new(dest, closed_over, branches));
    check_alloc(copy, s0_statement_new_copy(stmt));
    check(s0_statement_eq(stmt, copy));
    s0_statement_free(stmt);
    s0_statement_free(copy);
}

TEST_CASE("can create `create-literal` statement") {
    struct s0_name  *dest;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_literal_new(dest, 5, "hello"));
    check(s0_statement_kind(stmt) == S0_STATEMENT_KIND_CREATE_LITERAL);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_literal_dest(stmt), dest));
    s0_name_free(dest);

    check(s0_create_literal_size(stmt) == 5);
    check(memcmp(s0_create_literal_content(stmt), "hello", 5) == 0);

    s0_statement_free(stmt);
}

TEST_CASE("can copy a `create-literal` statement") {
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    struct s0_statement  *copy;
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_literal_new(dest, 5, "hello"));
    check_alloc(copy, s0_statement_new_copy(stmt));
    check(s0_statement_eq(stmt, copy));
    s0_statement_free(stmt);
    s0_statement_free(copy);
}

TEST_CASE("can create `create-method` statement") {
    struct s0_name  *dest;
    struct s0_block  *body;
    struct s0_statement  *stmt;

    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(body, create_empty_block());
    check_alloc(stmt, s0_create_method_new(dest, body));
    check(s0_statement_kind(stmt) == S0_STATEMENT_KIND_CREATE_METHOD);

    check_alloc(dest, s0_name_new_str("a"));
    check(s0_name_eq(s0_create_method_dest(stmt), dest));
    s0_name_free(dest);

    check(s0_create_method_body(stmt) == body);

    s0_statement_free(stmt);
}

TEST_CASE("can copy a `create-method` statement") {
    struct s0_name  *dest;
    struct s0_block  *body;
    struct s0_statement  *stmt;
    struct s0_statement  *copy;
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(body, create_empty_block());
    check_alloc(stmt, s0_create_method_new(dest, body));
    check_alloc(copy, s0_statement_new_copy(stmt));
    check(s0_statement_eq(stmt, copy));
    s0_statement_free(stmt);
    s0_statement_free(copy);
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

TEST_CASE("can create a copy of a statement list") {
    struct s0_statement_list  *list;
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    struct s0_statement_list  *copy;
    check_alloc(list, s0_statement_list_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt));
    check_alloc(dest, s0_name_new_str("b"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_statement_list_add(list, stmt));
    check_alloc(copy, s0_statement_list_new_copy(list));
    check(s0_statement_list_eq(list, copy));
    s0_statement_list_free(list);
    s0_statement_list_free(copy);
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
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;

    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check(s0_invocation_kind(invocation) == S0_INVOCATION_KIND_INVOKE_CLOSURE);

    check_alloc(src, s0_name_new_str("a"));
    check(s0_name_eq(s0_invoke_closure_src(invocation), src));
    s0_name_free(src);

    check_alloc(branch, s0_name_new_str("body"));
    check(s0_name_eq(s0_invoke_closure_branch(invocation), branch));
    s0_name_free(branch);

    check(s0_invoke_closure_params(invocation) == params);

    s0_invocation_free(invocation);
}

TEST_CASE("can copy a `invoke-closure` invocation") {
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;
    struct s0_invocation  *copy;
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(copy, s0_invocation_new_copy(invocation));
    check(s0_invocation_eq(invocation, copy));
    s0_invocation_free(invocation);
    s0_invocation_free(copy);
}

TEST_CASE("can create `invoke-method` invocation") {
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;

    check_alloc(src, s0_name_new_str("a"));
    check_alloc(method, s0_name_new_str("run"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_method_new(src, method, params));
    check(s0_invocation_kind(invocation) == S0_INVOCATION_KIND_INVOKE_METHOD);

    check_alloc(src, s0_name_new_str("a"));
    check(s0_name_eq(s0_invoke_method_src(invocation), src));
    s0_name_free(src);

    check_alloc(method, s0_name_new_str("run"));
    check(s0_name_eq(s0_invoke_method_method(invocation), method));
    s0_name_free(method);

    check(s0_invoke_method_params(invocation) == params);

    s0_invocation_free(invocation);
}

TEST_CASE("can copy a `invoke-method` invocation") {
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;
    struct s0_invocation  *copy;
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(method, s0_name_new_str("run"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_method_new(src, method, params));
    check_alloc(copy, s0_invocation_new_copy(invocation));
    check(s0_invocation_eq(invocation, copy));
    s0_invocation_free(invocation);
    s0_invocation_free(copy);
}

/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

TEST_CASE_GROUP("S₀ blocks");

TEST_CASE("can create block") {
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_environment_type  *inputs;
    struct s0_statement_list  *statements;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("x"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check(s0_block_inputs(block) == inputs);
    check(s0_block_statements(block) == statements);
    check(s0_block_invocation(block) == invocation);
    s0_block_free(block);
}

TEST_CASE("can copy a block") {
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_environment_type  *inputs;
    struct s0_statement_list  *statements;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_block  *copy;
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("x"));
    check_alloc(branch, s0_name_new_str("body"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(copy, s0_block_new_copy(block));
    check(s0_block_eq(block, copy));
    s0_block_free(block);
    s0_block_free(copy);
}

/*-----------------------------------------------------------------------------
 * S₀: Atoms
 */

TEST_CASE_GROUP("S₀ atoms");

TEST_CASE("can create atom") {
    struct s0_entity  *atom;
    check_alloc(atom, s0_atom_new());
    check(s0_entity_kind(atom) == S0_ENTITY_KIND_ATOM);
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
    check(s0_entity_kind(closure) == S0_ENTITY_KIND_CLOSURE);
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
    check(s0_entity_kind(literal) == S0_ENTITY_KIND_LITERAL);
    check(s0_literal_size(literal) == 5);
    check(memcmp(s0_literal_content(literal), "hello", 5) == 0);
    s0_entity_free(literal);
}

TEST_CASE("can create literal with explicit length") {
    struct s0_entity  *literal;
    check_alloc(literal, s0_literal_new(5, "hello"));
    check(s0_entity_kind(literal) == S0_ENTITY_KIND_LITERAL);
    check(s0_literal_size(literal) == 5);
    check(memcmp(s0_literal_content(literal), "hello", 5) == 0);
    s0_entity_free(literal);
}

TEST_CASE("can create literal with embedded NUL") {
    struct s0_entity  *literal;
    check_alloc(literal, s0_literal_new(6, "hello\x00"));
    check(s0_entity_kind(literal) == S0_ENTITY_KIND_LITERAL);
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
    struct s0_block  *body;
    check_alloc(body, create_empty_block());
    check_alloc(method, s0_method_new(body));
    check(s0_entity_kind(method) == S0_ENTITY_KIND_METHOD);
    check(s0_method_body(method) == body);
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
 * S₀: Entity types: Any
 */

TEST_CASE_GROUP("S₀ entity types: any");

TEST_CASE("can load * from string") {
    struct s0_entity_type  *expected;
    struct s0_entity_type  *actual;
    /* expected = * */
    check_alloc(expected, s0_any_entity_type_new());
    /* Load type from YAML string */
    check_alloc(actual, entity_type(YAML "!s0!any {}"));
    /* Verify actual == expected */
    check(s0_entity_type_equiv(actual, expected));
    /* Free everything */
    s0_entity_type_free(actual);
    s0_entity_type_free(expected);
}

TEST_CASE("can copy *") {
    struct s0_entity_type  *type1;
    struct s0_entity_type  *type2;
    /* type1 = * */
    check_alloc(type1, s0_any_entity_type_new());
    /* type2 = type1 */
    check_alloc(type2, s0_entity_type_new_copy(type1));
    /* Verify type1 == type2 */
    check(s0_entity_type_equiv(type1, type2));
    /* Free everything */
    s0_entity_type_free(type1);
    s0_entity_type_free(type2);
}

TEST_CASE("⋄ ∈ *") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = * */
    check_alloc(type, s0_any_entity_type_new());
    /* entity = ⋄ */
    check_alloc(entity, s0_atom_new());
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⤿ ⦃a:*⦄ ...⟩ ∈ *") {
    struct s0_entity_type  *type;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_name_mapping  *params;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    /* type = * */
    check_alloc(type, s0_any_entity_type_new());
    /* entity = ⟨⤿ ⦃a:*⦄ ...⟩ */
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(entity, s0_closure_new(env, blocks));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("literal ∈ *") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = * */
    check_alloc(type, s0_any_entity_type_new());
    /* entity = literal */
    check_alloc(entity, s0_literal_new_str("hello"));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⊶ ⦃a:*⦄ ...⟩ ∈ *") {
    struct s0_entity_type  *type;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *body;
    struct s0_entity  *entity;
    /* type = * */
    check_alloc(type, s0_any_entity_type_new());
    /* entity = ⟨⊶ ⦃a:*⦄ ...⟩ */
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(body, s0_block_new(inputs, statements, invocation));
    check_alloc(entity, s0_method_new(body));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("object ∈ *") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = * */
    check_alloc(type, s0_any_entity_type_new());
    /* entity = object */
    check_alloc(entity, s0_object_new());
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

/*-----------------------------------------------------------------------------
 * S₀: Entity types: Closures
 */

TEST_CASE_GROUP("S₀ entity types: closures");

TEST_CASE("can load ⤿ ⦃a:*⦄ from string") {
    struct s0_environment_type_mapping  *branches;
    struct s0_environment_type  *branch_type;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_entity_type  *expected;
    struct s0_entity_type  *actual;
    /* expected = ⤿ ⦃a:*⦄ */
    check_alloc(branches, s0_environment_type_mapping_new());
    check_alloc(branch_type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(branch_type, name, input_type));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_environment_type_mapping_add(branches, name, branch_type));
    check_alloc(expected, s0_closure_entity_type_new(branches));
    /* Load type from YAML string */
    check_alloc(actual, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* Verify actual == expected */
    check(s0_entity_type_equiv(actual, expected));
    /* Free everything */
    s0_entity_type_free(actual);
    s0_entity_type_free(expected);
}

TEST_CASE("can copy ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type1;
    struct s0_entity_type  *type2;
    /* type1 = ⤿ ⦃a:*⦄ */
    check_alloc(type1, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* type2 = type1 */
    check_alloc(type2, s0_entity_type_new_copy(type1));
    /* Verify type1 == type2 */
    check(s0_entity_type_equiv(type1, type2));
    /* Free everything */
    s0_entity_type_free(type1);
    s0_entity_type_free(type2);
}

TEST_CASE("⋄ ∉ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = ⋄ */
    check_alloc(entity, s0_atom_new());
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("literal ∉ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = literal */
    check_alloc(entity, s0_literal_new_str("hello"));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⊶ ⦃a:*⦄ ...⟩ ∉ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *body;
    struct s0_entity  *entity;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = ⟨⊶ ⦃a:*⦄ ...⟩ */
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(body, s0_block_new(inputs, statements, invocation));
    check_alloc(entity, s0_method_new(body));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("object ∉ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = object */
    check_alloc(entity, s0_object_new());
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⤿ ⦃a:*⦄ ...⟩ ∈ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = ⟨⤿ ⦃a:*⦄ ...⟩ */
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(entity, s0_closure_new(env, blocks));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("type of ⟨⤿ ⦃a:*⦄ ...⟩ = ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    struct s0_entity_type  *calculated_type;
    /* type = ⤿ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* entity = ⟨⤿ ⦃a:*⦄ ...⟩ */
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(entity, s0_closure_new(env, blocks));
    /* Verify type(entity) = type */
    check_alloc(calculated_type, s0_entity_type_new_from_entity(entity));
    check(s0_entity_type_equiv(type, calculated_type));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
    s0_entity_type_free(calculated_type);
}

/*-----------------------------------------------------------------------------
 * S₀: Entity types: Methods
 */

TEST_CASE_GROUP("S₀ entity types: methods");

TEST_CASE("can load ⊶ ⦃a:*⦄ from string") {
    struct s0_environment_type  *body_type;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_entity_type  *expected;
    struct s0_entity_type  *actual;
    /* expected = ⊶ ⦃a:*⦄ */
    check_alloc(body_type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(body_type, name, input_type));
    check_alloc(expected, s0_method_entity_type_new(body_type));
    /* Load type from YAML string */
    check_alloc(actual, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* Verify actual == expected */
    check(s0_entity_type_equiv(actual, expected));
    /* Free everything */
    s0_entity_type_free(actual);
    s0_entity_type_free(expected);
}

TEST_CASE("can copy ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type1;
    struct s0_entity_type  *type2;
    /* type1 = ⊶ ⦃a:*⦄ */
    check_alloc(type1, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* type2 = type1 */
    check_alloc(type2, s0_entity_type_new_copy(type1));
    /* Verify type1 == type2 */
    check(s0_entity_type_equiv(type1, type2));
    /* Free everything */
    s0_entity_type_free(type1);
    s0_entity_type_free(type2);
}

TEST_CASE("⋄ ∉ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = ⋄ */
    check_alloc(entity, s0_atom_new());
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⤿ ⦃a:*⦄ ...⟩ ∉ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = ⟨⤿ ⦃a:*⦄ ...⟩ */
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(entity, s0_closure_new(env, blocks));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("literal ∉ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = literal */
    check_alloc(entity, s0_literal_new_str("hello"));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("object ∉ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = object */
    check_alloc(entity, s0_object_new());
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⊶ ⦃a:*⦄ ...⟩ ∈ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *body;
    struct s0_entity  *entity;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = ⟨⊶ ⦃a:*⦄ ...⟩ */
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(body, s0_block_new(inputs, statements, invocation));
    check_alloc(entity, s0_method_new(body));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("type of ⟨⊶ ⦃a:*⦄ ...⟩ = ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *type;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *body;
    struct s0_entity  *entity;
    struct s0_entity_type  *calculated_type;
    /* type = ⊶ ⦃a:*⦄ */
    check_alloc(type, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* entity = ⟨⊶ ⦃a:*⦄ ...⟩ */
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(body, s0_block_new(inputs, statements, invocation));
    check_alloc(entity, s0_method_new(body));
    /* Verify type(entity) = type */
    check_alloc(calculated_type, s0_entity_type_new_from_entity(entity));
    check(s0_entity_type_equiv(type, calculated_type));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
    s0_entity_type_free(calculated_type);
}

/*-----------------------------------------------------------------------------
 * S₀: Entity types: Objects
 */

TEST_CASE_GROUP("S₀ entity types: objects");

TEST_CASE("can load ⟪a:*⟫ from string") {
    struct s0_environment_type  *elements_type;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_entity_type  *expected;
    struct s0_entity_type  *actual;
    /* expected = ⟪a:*⟫ */
    check_alloc(elements_type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(elements_type, name, input_type));
    check_alloc(expected, s0_object_entity_type_new(elements_type));
    /* Load type from YAML string */
    check_alloc(actual, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* Verify actual == expected */
    check(s0_entity_type_equiv(actual, expected));
    /* Free everything */
    s0_entity_type_free(actual);
    s0_entity_type_free(expected);
}

TEST_CASE("can copy ⟪a:*⟫") {
    struct s0_entity_type  *type1;
    struct s0_entity_type  *type2;
    /* type1 = ⟪a:*⟫ */
    check_alloc(type1, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* type2 = type1 */
    check_alloc(type2, s0_entity_type_new_copy(type1));
    /* Verify type1 == type2 */
    check(s0_entity_type_equiv(type1, type2));
    /* Free everything */
    s0_entity_type_free(type1);
    s0_entity_type_free(type2);
}

TEST_CASE("⋄ ∉ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⋄ */
    check_alloc(entity, s0_atom_new());
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⤿ ⦃a:*⦄ ...⟩ ∉ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⟨⤿ ⦃a:*⦄ ...⟩ */
    check_alloc(env, s0_environment_new());
    check_alloc(blocks, s0_named_blocks_new());
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(name, s0_name_new_str("cont"));
    check0(s0_named_blocks_add(blocks, name, block));
    check_alloc(entity, s0_closure_new(env, blocks));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("literal ∉ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = literal */
    check_alloc(entity, s0_literal_new_str("hello"));
    /* Verify entity ∉ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨⊶ ⦃a:*⦄ ...⟩ ∉ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_entity_type  *input_type;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *block;
    struct s0_entity  *entity;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⟨⊶ ⦃a:*⦄ ...⟩ */
    check_alloc(inputs, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(input_type, s0_any_entity_type_new());
    check0(s0_environment_type_add(inputs, name, input_type));
    check_alloc(statements, s0_statement_list_new());
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, branch, params));
    check_alloc(block, s0_block_new(inputs, statements, invocation));
    check_alloc(entity, s0_method_new(block));
    /* Verify entity ∈ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨a:⋄⟩ ∈ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    struct s0_name  *name;
    struct s0_entity  *element;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⟨a:⋄⟩ */
    check_alloc(entity, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(element, s0_atom_new());
    check0(s0_object_add(entity, name, element));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨a:⋄,b:⋄⟩ ∈ ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    struct s0_name  *name;
    struct s0_entity  *element;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⟨a:⋄,b:⋄⟩ */
    check_alloc(entity, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(element, s0_atom_new());
    check0(s0_object_add(entity, name, element));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(element, s0_atom_new());
    check0(s0_object_add(entity, name, element));
    /* Verify entity ∈ type */
    check(s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("⟨a:⋄⟩ ∉ ⟪a:*,b:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    struct s0_name  *name;
    struct s0_entity  *element;
    /* type = ⟪a:*,b:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* entity = ⟨a:⋄⟩ */
    check_alloc(entity, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(element, s0_atom_new());
    check0(s0_object_add(entity, name, element));
    /* Verify entity ∈ type */
    check(!s0_entity_type_satisfied_by(type, entity));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
}

TEST_CASE("type of ⟨a:⋄⟩ = ⟪a:*⟫") {
    struct s0_entity_type  *type;
    struct s0_entity  *entity;
    struct s0_name  *name;
    struct s0_entity  *element;
    struct s0_entity_type  *calculated_type;
    /* type = ⟪a:*⟫ */
    check_alloc(type, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* entity = ⟨a:⋄⟩ */
    check_alloc(entity, s0_object_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(element, s0_atom_new());
    check0(s0_object_add(entity, name, element));
    /* Verify type(entity) = type */
    check_alloc(calculated_type, s0_entity_type_new_from_entity(entity));
    check(s0_entity_type_equiv(type, calculated_type));
    /* Free everything */
    s0_entity_free(entity);
    s0_entity_type_free(type);
    s0_entity_type_free(calculated_type);
}

/*-----------------------------------------------------------------------------
 * S₀: Entity types: Subtyping
 */

TEST_CASE_GROUP("S₀ entity types: subtyping");

TEST_CASE("* ⊆ *") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = * */
    check_alloc(requires, s0_any_entity_type_new());
    /* have = * */
    check_alloc(have, s0_any_entity_type_new());
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⤿ ⦃a:*⦄ ⊆ *") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = * */
    check_alloc(requires, s0_any_entity_type_new());
    /* have = ⤿ ⦃a:*⦄ */
    check_alloc(have, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("* ⊈ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⤿ ⦃a:*⦄ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* have = * */
    check_alloc(have, s0_any_entity_type_new());
    /* Verify have ⊈ requires */
    check(!s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⤿ ⦃a:*⦄ ⊆ ⤿ ⦃a:*⦄") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⤿ ⦃a:*⦄ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* have = ⤿ ⦃a:*⦄ */
    check_alloc(have, entity_type(
                YAML
                "!s0!closure\n"
                "branches:\n"
                "  cont:\n"
                "    a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⊶ ⦃a:*⦄ ⊆ ⊶ ⦃a:*⦄") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⊶ ⦃a:*⦄ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* have = ⊶ ⦃a:*⦄ */
    check_alloc(have, entity_type(
                YAML
                "!s0!method\n"
                "inputs:\n"
                "  a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⟪a:*⟫ ⊆ ⟪a:*⟫") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⟪a:*⟫ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* have = ⟪a:*⟫ */
    check_alloc(have, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⟪a:*⟫ ⊈ ⟪a:*,b:*⟫") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⟪a:*⟫ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* have = ⟪a:*⟫ */
    check_alloc(have, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊈ requires */
    check(!s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

TEST_CASE("⟪a:*,b:*⟫ ⊆ ⟪a:*⟫") {
    struct s0_entity_type  *requires;
    struct s0_entity_type  *have;
    /* requires = ⟪a:*⟫ */
    check_alloc(requires, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                ));
    /* have = ⟪a:*,b:*⟫ */
    check_alloc(have, entity_type(
                YAML
                "!s0!object\n"
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_entity_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_entity_type_free(requires);
    s0_entity_type_free(have);
}

/*-----------------------------------------------------------------------------
 * S₀: Environment types
 */

TEST_CASE_GROUP("S₀ environment types");

TEST_CASE("can create empty environment type") {
    struct s0_environment_type  *type;
    check_alloc(type, s0_environment_type_new());
    s0_environment_type_free(type);
}

TEST_CASE("empty environment type has zero elements") {
    struct s0_environment_type  *type;
    check_alloc(type, s0_environment_type_new());
    check(s0_environment_type_size(type) == 0);
    s0_environment_type_free(type);
}

TEST_CASE("empty environment type doesn't contain anything") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_get(type, name) == NULL);
    s0_name_free(name);
    s0_environment_type_free(type);
}

TEST_CASE("can add entries to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    struct s0_entity_type  *etype;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype));
    s0_environment_type_free(type);
}

TEST_CASE("non-empty environment type has accurate size") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    struct s0_entity_type  *etype;
    check_alloc(type, s0_environment_type_new());

    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype));
    check(s0_environment_type_size(type) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype));
    check(s0_environment_type_size(type) == 2);

    s0_environment_type_free(type);
}

TEST_CASE("can check which names belong to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    struct s0_entity_type  *etype1;
    struct s0_entity_type  *etype2;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype1, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(etype2, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_get(type, name) == etype1);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_type_get(type, name) == etype2);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_environment_type_get(type, name) == NULL);
    s0_name_free(name);

    s0_environment_type_free(type);
}

TEST_CASE("can copy ⦃a:*, b:*⦄") {
    struct s0_environment_type  *type;
    struct s0_environment_type  *type_copy;
    /* Construct ⦃a:*, b:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Make a copy of it */
    check_alloc(type_copy, s0_environment_type_new_copy(type));
    /* Verify that they're equal */
    check(s0_environment_type_equiv(type, type_copy));
    /* Free everything */
    s0_environment_type_free(type);
    s0_environment_type_free(type_copy);
}

TEST_CASE("can iterate through names in type") {
    struct s0_environment_type  *type;
    struct s0_environment_type_entry  entry;
    struct s0_name  *name;
    struct s0_entity_type  *etype1;
    struct s0_entity_type  *etype2;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype1, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(etype2, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype2));

    /* This test assumes that the entries are returned in the same order that
     * they were added to the type. */

    entry = s0_environment_type_at(type, 0);
    check_alloc(name, s0_name_new_str("a"));
    check(s0_name_eq(entry.name, name));
    check(entry.type == etype1);
    s0_name_free(name);

    entry = s0_environment_type_at(type, 1);
    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_eq(entry.name, name));
    check(entry.type == etype2);
    s0_name_free(name);

    s0_environment_type_free(type);
}

TEST_CASE("can delete entries from environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    struct s0_entity_type  *etype;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype));
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_delete(type, name) == etype);
    s0_name_free(name);
    s0_entity_type_free(etype);
    s0_environment_type_free(type);
}

TEST_CASE("can extract entries from environment type") {
    struct s0_environment_type  *src;
    struct s0_environment_type  *dest;
    struct s0_name  *name;
    struct s0_entity_type  *etype1;
    struct s0_entity_type  *etype2;
    struct s0_name_set  *set;

    /* Construct ⦃a:*, b:*⦄ */
    check_alloc(src, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype1, s0_any_entity_type_new());
    check0(s0_environment_type_add(src, name, etype1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(etype2, s0_any_entity_type_new());
    check0(s0_environment_type_add(src, name, etype2));

    /* Extract `a` into a separate type */
    check_alloc(dest, s0_environment_type_new());
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check0(s0_environment_type_extract(dest, src, set));
    s0_name_set_free(set);

    /* Verify that dest == ⦃a:*⦄ */
    check(s0_environment_type_size(dest) == 1);
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_get(dest, name) == etype1);
    s0_name_free(name);
    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_type_get(dest, name) == NULL);
    s0_name_free(name);

    /* Verify that src == ⦃b:*⦄ */
    check(s0_environment_type_size(src) == 1);
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_get(src, name) == NULL);
    s0_name_free(name);
    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_type_get(src, name) == etype2);
    s0_name_free(name);

    s0_environment_type_free(src);
    s0_environment_type_free(dest);
}

TEST_CASE("cannot extract duplicate entries from environment type") {
    struct s0_environment_type  *src;
    struct s0_environment_type  *dest;
    struct s0_name  *name;
    struct s0_entity_type  *etype;
    struct s0_name_set  *set;

    /* Construct ⦃a:*⦄ */
    check_alloc(src, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(src, name, etype));

    /* Extract `a` into a separate type that already contains `a` */
    check_alloc(dest, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(dest, name, etype));
    check_alloc(set, s0_name_set_new());
    check_alloc(name, s0_name_new_str("a"));
    check0(s0_name_set_add(set, name));
    check(s0_environment_type_extract(dest, src, set) == -1);
    s0_name_set_free(set);

    s0_environment_type_free(src);
    s0_environment_type_free(dest);
}

TEST_CASE("cannot get deleted entries from environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *name;
    struct s0_entity_type  *etype;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype));
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_delete(type, name) == etype);
    s0_name_free(name);
    s0_entity_type_free(etype);
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_get(type, name) == NULL);
    s0_name_free(name);
    s0_environment_type_free(type);
}

TEST_CASE("cannot iterate deleted entries from environment type") {
    struct s0_environment_type  *type;
    struct s0_environment_type_entry  entry;
    struct s0_name  *name;
    struct s0_entity_type  *etype1;
    struct s0_entity_type  *etype2;
    check_alloc(type, s0_environment_type_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(etype1, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(etype2, s0_any_entity_type_new());
    check0(s0_environment_type_add(type, name, etype2));
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_delete(type, name) == etype1);
    s0_name_free(name);
    s0_entity_type_free(etype1);
    entry = s0_environment_type_at(type, 0);
    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_eq(entry.name, name));
    check(entry.type == etype2);
    s0_name_free(name);
    s0_environment_type_free(type);
}

TEST_CASE("{} ∈ ⦃⦄") {
    struct s0_environment_type  *type;
    struct s0_environment  *env;
    /* type = ⦃⦄ */
    check_alloc(type, s0_environment_type_new());
    /* env = {} */
    check_alloc(env, s0_environment_new());
    /* Verify {} ∈ ⦃⦄ */
    check(s0_environment_type_satisfied_by(type, env));
    /* Free everything */
    s0_environment_type_free(type);
    s0_environment_free(env);
}

TEST_CASE("{a:⋄, b:⋄} ∉ ⦃⦄") {
    struct s0_name  *name;
    struct s0_environment_type  *type;
    struct s0_environment  *env;
    struct s0_entity  *atom;
    /* type = ⦃⦄ */
    check_alloc(type, s0_environment_type_new());
    /* env = {a:⋄, b:⋄} */
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    /* Verify {a:⋄, b:⋄} ∈ ⦃⦄ */
    check(!s0_environment_type_satisfied_by(type, env));
    /* Free everything */
    s0_environment_type_free(type);
    s0_environment_free(env);
}

TEST_CASE("{a:⋄, b:⋄} ∉ ⦃a:*⦄") {
    struct s0_name  *name;
    struct s0_environment_type  *type;
    struct s0_environment  *env;
    struct s0_entity  *atom;
    /* type = ⦃a:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* env = {a:⋄, b:⋄} */
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    /* Verify {a:⋄, b:⋄} ∈ ⦃a:*⦄ */
    check(!s0_environment_type_satisfied_by(type, env));
    /* Free everything */
    s0_environment_type_free(type);
    s0_environment_free(env);
}

TEST_CASE("{a:⋄, b:⋄} ∈ ⦃a:*, b:*⦄") {
    struct s0_name  *name;
    struct s0_environment_type  *type;
    struct s0_environment  *env;
    struct s0_entity  *atom;
    /* type = ⦃a:*, b:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* env = {a:⋄, b:⋄} */
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    /* Verify {a:⋄, b:⋄} ∈ ⦃a:*, b:*⦄ */
    check(s0_environment_type_satisfied_by(type, env));
    /* Free everything */
    s0_environment_type_free(type);
    s0_environment_free(env);
}

TEST_CASE("{a:⋄, b:⋄} ∉ ⦃a:*, b:*, c:*⦄") {
    struct s0_name  *name;
    struct s0_environment_type  *type;
    struct s0_environment  *env;
    struct s0_entity  *atom;
    /* type = ⦃a:*, b:*, c:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                "c: !s0!any {}\n"
                ));
    /* env = {a:⋄, b:⋄} */
    check_alloc(env, s0_environment_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(atom, s0_atom_new());
    check0(s0_environment_add(env, name, atom));
    /* Verify {a:⋄, b:⋄} ∈ ⦃a:*, b:*, c:*⦄ */
    check(!s0_environment_type_satisfied_by(type, env));
    s0_environment_type_free(type);
    s0_environment_free(env);
}

TEST_CASE("can add `create-atom` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    struct s0_name  *name;
    struct s0_entity_type  *actual;
    struct s0_entity_type  *expected;
    /* Create environment from statement */
    check_alloc(type, s0_environment_type_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_atom_new(dest));
    check0(s0_environment_type_add_statement(type, stmt));
    /* expected = * */
    check_alloc(expected, s0_any_entity_type_new());
    /* Verify actual == expected */
    check_alloc(name, s0_name_new_str("a"));
    check_nonnull(actual = s0_environment_type_get(type, name));
    check(s0_entity_type_equiv(actual, expected));
    s0_name_free(name);
    /* Free everything */
    s0_statement_free(stmt);
    s0_environment_type_free(type);
    s0_entity_type_free(expected);
}

TEST_CASE("can add `create-closure` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;
    struct s0_statement  *stmt;
    struct s0_name  *name;
    struct s0_entity_type  *actual;
    struct s0_environment_type_mapping  *branch_types;
    struct s0_entity_type  *expected;
    /* Create environment from statement */
    check_alloc(type, s0_environment_type_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(closed_over, s0_name_set_new());
    check_alloc(branches, s0_named_blocks_new());
    check_alloc(stmt, s0_create_closure_new(dest, closed_over, branches));
    check0(s0_environment_type_add_statement(type, stmt));
    /* expected = ⤿ */
    check_alloc(branch_types, s0_environment_type_mapping_new());
    check_alloc(expected, s0_closure_entity_type_new(branch_types));
    /* Verify actual == expected */
    check_alloc(name, s0_name_new_str("a"));
    check_nonnull(actual = s0_environment_type_get(type, name));
    check(s0_entity_type_equiv(actual, expected));
    s0_name_free(name);
    /* Free everything */
    s0_statement_free(stmt);
    s0_environment_type_free(type);
    s0_entity_type_free(expected);
}

TEST_CASE("can add `create-literal` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *dest;
    struct s0_statement  *stmt;
    struct s0_name  *name;
    struct s0_entity_type  *actual;
    struct s0_entity_type  *expected;
    /* Create environment from statement */
    check_alloc(type, s0_environment_type_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(stmt, s0_create_literal_new(dest, 5, "hello"));
    check0(s0_environment_type_add_statement(type, stmt));
    /* expected = * */
    check_alloc(expected, s0_any_entity_type_new());
    /* Verify actual == expected */
    check_alloc(name, s0_name_new_str("a"));
    check_nonnull(actual = s0_environment_type_get(type, name));
    check(s0_entity_type_equiv(actual, expected));
    s0_name_free(name);
    /* Free everything */
    s0_statement_free(stmt);
    s0_environment_type_free(type);
    s0_entity_type_free(expected);
}

TEST_CASE("can add `create-method` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *dest;
    struct s0_block  *body;
    struct s0_statement  *stmt;
    struct s0_name  *name;
    struct s0_entity_type  *actual;
    struct s0_entity_type  *expected;
    /* Create environment from statement */
    check_alloc(type, s0_environment_type_new());
    check_alloc(dest, s0_name_new_str("a"));
    check_alloc(body, create_empty_block());
    check_alloc(stmt, s0_create_method_new(dest, body));
    check0(s0_environment_type_add_statement(type, stmt));
    /* expected = * */
    check_alloc(expected, s0_any_entity_type_new());
    /* Verify actual == expected */
    check_alloc(name, s0_name_new_str("a"));
    check_nonnull(actual = s0_environment_type_get(type, name));
    check(s0_entity_type_equiv(actual, expected));
    s0_name_free(name);
    /* Free everything */
    s0_statement_free(stmt);
    s0_environment_type_free(type);
    s0_entity_type_free(expected);
}

TEST_CASE("can add `invoke-closure` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    /* initial env = ⦃a:⤿ ⦃b:*⦄, b:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!closure\n"
                "  branches:\n"
                "    cont:\n"
                "      b: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Then add the invocation */
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(branch, s0_name_new_str("cont"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("b"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_closure_new(src, branch, params));
    check0(s0_environment_type_add_invocation(type, invocation));
    /* Free everything */
    s0_invocation_free(invocation);
    s0_environment_type_free(type);
}

TEST_CASE("can add `invoke-method` to environment type") {
    struct s0_environment_type  *type;
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    /* initial env = ⦃a:⟪run:⊶ ⦃self:*⦄⟫⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!object\n"
                "  run: !s0!method\n"
                "    inputs:\n"
                "      self: !s0!any {}\n"
                ));
    /* Then add the invocation */
    check_alloc(src, s0_name_new_str("a"));
    check_alloc(method, s0_name_new_str("run"));
    check_alloc(params, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("self"));
    check0(s0_name_mapping_add(params, from, to));
    check_alloc(invocation, s0_invoke_method_new(src, method, params));
    check0(s0_environment_type_add_invocation(type, invocation));
    /* Free everything */
    s0_invocation_free(invocation);
    s0_environment_type_free(type);
}

TEST_CASE("⦃⦄ ⊆ ⦃⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃⦄ */
    check_alloc(requires, s0_environment_type_new());
    /* have = ⦃⦄ */
    check_alloc(have, s0_environment_type_new());
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃⦄ ⊈ ⦃a:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* have = ⦃⦄ */
    check_alloc(have, s0_environment_type_new());
    /* Verify have ⊈ requires */
    check(!s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*⦄ ⊈ ⦃⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃⦄ */
    check_alloc(requires, s0_environment_type_new());
    /* have = ⦃a:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊈ requires */
    check(!s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*⦄ ⊆ ⦃a:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* have = ⦃a:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*⦄ ⊈ ⦃a:*,b:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*,b:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* have = ⦃a:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊈ requires */
    check(!s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*,b:*⦄ ⊈ ⦃a:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* have = ⦃a:*,b:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Verify have ⊈ requires */
    check(!s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*,b:*⦄ ⊆ ⦃a:*,b:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*,b:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* have = ⦃a:*,b:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*,b:*⦄ ⊆ ⦃b:*,a:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃b:*,a:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "b: !s0!any {}\n"
                "a: !s0!any {}\n"
                ));
    /* have = ⦃a:*,b:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃b:*,a:*⦄ ⊆ ⦃a:*,b:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃a:*,b:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* have = ⦃b:*,a:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "b: !s0!any {}\n"
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃b:*,a:*⦄ ⊆ ⦃b:*,a:*⦄") {
    struct s0_environment_type  *requires;
    struct s0_environment_type  *have;
    /* requires = ⦃b:*,a:*⦄ */
    check_alloc(requires, environment_type(
                YAML
                "b: !s0!any {}\n"
                "a: !s0!any {}\n"
                ));
    /* have = ⦃b:*,a:*⦄ */
    check_alloc(have, environment_type(
                YAML
                "b: !s0!any {}\n"
                "a: !s0!any {}\n"
                ));
    /* Verify have ⊆ requires */
    check(s0_environment_type_satisfied_by_type(requires, have));
    /* Free everything */
    s0_environment_type_free(requires);
    s0_environment_type_free(have);
}

TEST_CASE("⦃a:*⦄[a→b] == ⦃b:*⦄") {
    struct s0_environment_type  *type;
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_environment_type  *expected;
    /* Construct ⦃a:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Construct [a→b] */
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    /* Apply renaming */
    check0(s0_environment_type_rename(type, mapping));
    /* Verify that result == ⦃b:*⦄ */
    check_alloc(expected, environment_type(
                YAML
                "b: !s0!any {}\n"
                ));
    check(s0_environment_type_equiv(type, expected));
    /* Free everything */
    s0_name_mapping_free(mapping);
    s0_environment_type_free(type);
    s0_environment_type_free(expected);
}

TEST_CASE("⦃a:*⦄[a→a] == ⦃a:*⦄") {
    struct s0_environment_type  *type;
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_environment_type  *expected;
    /* Construct ⦃a:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Construct [a→a] */
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(mapping, from, to));
    /* Apply renaming */
    check0(s0_environment_type_rename(type, mapping));
    /* Verify that result == ⦃a:*⦄ */
    check_alloc(expected, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    check(s0_environment_type_equiv(type, expected));
    /* Free everything */
    s0_name_mapping_free(mapping);
    s0_environment_type_free(type);
    s0_environment_type_free(expected);
}

TEST_CASE("⦃a:*,b:*⦄[a→c,b→d] == ⦃c:*,d:*⦄") {
    struct s0_environment_type  *type;
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_environment_type  *expected;
    /* Construct ⦃a:*,b:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Construct [a→c,b→d] */
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("c"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("b"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));
    /* Apply renaming */
    check0(s0_environment_type_rename(type, mapping));
    /* Verify that result == ⦃c:*,d:*⦄ */
    check_alloc(expected, environment_type(
                YAML
                "c: !s0!any {}\n"
                "d: !s0!any {}\n"
                ));
    check(s0_environment_type_equiv(type, expected));
    /* Free everything */
    s0_name_mapping_free(mapping);
    s0_environment_type_free(type);
    s0_environment_type_free(expected);
}

TEST_CASE("⦃a:*,b:*⦄[a→b,b→a] == ⦃a:*,b:*⦄") {
    struct s0_environment_type  *type;
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_environment_type  *expected;
    /* Construct ⦃a:*,b:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    /* Construct [a→b,b→a] */
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("b"));
    check_alloc(to, s0_name_new_str("a"));
    check0(s0_name_mapping_add(mapping, from, to));
    /* Apply renaming */
    check0(s0_environment_type_rename(type, mapping));
    /* Verify that result == ⦃a:*,b:*⦄ */
    check_alloc(expected, environment_type(
                YAML
                "a: !s0!any {}\n"
                "b: !s0!any {}\n"
                ));
    check(s0_environment_type_equiv(type, expected));
    /* Free everything */
    s0_name_mapping_free(mapping);
    s0_environment_type_free(type);
    s0_environment_type_free(expected);
}

TEST_CASE("⦃a:*⦄[a→b,c→d] is invalid") {
    struct s0_environment_type  *type;
    struct s0_name_mapping  *mapping;
    struct s0_name  *from;
    struct s0_name  *to;
    /* Construct ⦃a:*⦄ */
    check_alloc(type, environment_type(
                YAML
                "a: !s0!any {}\n"
                ));
    /* Construct [a→b,c→d] */
    check_alloc(mapping, s0_name_mapping_new());
    check_alloc(from, s0_name_new_str("a"));
    check_alloc(to, s0_name_new_str("b"));
    check0(s0_name_mapping_add(mapping, from, to));
    check_alloc(from, s0_name_new_str("c"));
    check_alloc(to, s0_name_new_str("d"));
    check0(s0_name_mapping_add(mapping, from, to));
    /* Verify that we can't apply renaming */
    check(s0_environment_type_rename(type, mapping) != 0);
    /* Free everything */
    s0_name_mapping_free(mapping);
    s0_environment_type_free(type);
}

/*-----------------------------------------------------------------------------
 * S₀: Environment type mappings
 */

TEST_CASE_GROUP("S₀ environment type mappings");

TEST_CASE("can create empty environment type mapping") {
    struct s0_environment_type_mapping  *mapping;
    check_alloc(mapping, s0_environment_type_mapping_new());
    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("empty environment type mapping has zero elements") {
    struct s0_environment_type_mapping  *mapping;
    check_alloc(mapping, s0_environment_type_mapping_new());
    check(s0_environment_type_mapping_size(mapping) == 0);
    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("empty environment type mapping doesn't contain anything") {
    struct s0_environment_type_mapping  *mapping;
    struct s0_name  *name;
    check_alloc(mapping, s0_environment_type_mapping_new());
    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_mapping_get(mapping, name) == NULL);
    s0_name_free(name);
    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("can add environment types to mapping") {
    struct s0_environment_type_mapping  *mapping;
    struct s0_name  *name;
    struct s0_environment_type  *type;
    check_alloc(mapping, s0_environment_type_mapping_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(type, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type));
    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("non-empty environment type mapping has accurate size") {
    struct s0_environment_type_mapping  *mapping;
    struct s0_name  *name;
    struct s0_environment_type  *type;
    check_alloc(mapping, s0_environment_type_mapping_new());

    check_alloc(name, s0_name_new_str("a"));
    check_alloc(type, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type));
    check(s0_environment_type_mapping_size(mapping) == 1);

    check_alloc(name, s0_name_new_str("b"));
    check_alloc(type, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type));
    check(s0_environment_type_mapping_size(mapping) == 2);

    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("can check which names belong to mapping") {
    struct s0_environment_type_mapping  *mapping;
    struct s0_name  *name;
    struct s0_environment_type  *type1;
    struct s0_environment_type  *type2;
    check_alloc(mapping, s0_environment_type_mapping_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(type1, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(type2, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type2));

    check_alloc(name, s0_name_new_str("a"));
    check(s0_environment_type_mapping_get(mapping, name) == type1);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check(s0_environment_type_mapping_get(mapping, name) == type2);
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_environment_type_mapping_get(mapping, name) == NULL);
    s0_name_free(name);

    s0_environment_type_mapping_free(mapping);
}

TEST_CASE("can copy environment type mapping") {
    struct s0_environment_type_mapping  *mapping;
    struct s0_environment_type_mapping  *mapping_copy;
    struct s0_name  *name;
    struct s0_environment_type  *type;

    /* Create a mapping and then create a copy it it. */
    check_alloc(mapping, s0_environment_type_mapping_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(type, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(type, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type));
    check_alloc(mapping_copy, s0_environment_type_mapping_new_copy(mapping));
    s0_environment_type_mapping_free(mapping);

    check_alloc(name, s0_name_new_str("a"));
    check_nonnull(s0_environment_type_mapping_get(mapping_copy, name));
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("b"));
    check_nonnull(s0_environment_type_mapping_get(mapping_copy, name));
    s0_name_free(name);

    check_alloc(name, s0_name_new_str("c"));
    check(s0_environment_type_mapping_get(mapping_copy, name) == NULL);
    s0_name_free(name);

    s0_environment_type_mapping_free(mapping_copy);
}

TEST_CASE("can iterate through names in mapping") {
    struct s0_environment_type_mapping  *mapping;
    const struct s0_environment_type_mapping_entry  *entry;
    struct s0_name  *name;
    struct s0_environment_type  *type1;
    struct s0_environment_type  *type2;
    check_alloc(mapping, s0_environment_type_mapping_new());
    check_alloc(name, s0_name_new_str("a"));
    check_alloc(type1, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type1));
    check_alloc(name, s0_name_new_str("b"));
    check_alloc(type2, s0_environment_type_new());
    check0(s0_environment_type_mapping_add(mapping, name, type2));

    /* This test assumes that the names are returned in the same order that they
     * were added to the mapping. */

    check_nonnull(entry = s0_environment_type_mapping_at(mapping, 0));
    check_alloc(name, s0_name_new_str("a"));
    check(s0_name_eq(entry->name, name));
    check(entry->type == type1);
    s0_name_free(name);

    check_nonnull(entry = s0_environment_type_mapping_at(mapping, 1));
    check_alloc(name, s0_name_new_str("b"));
    check(s0_name_eq(entry->name, name));
    check(entry->type == type2);
    s0_name_free(name);

    s0_environment_type_mapping_free(mapping);
}

/*-----------------------------------------------------------------------------
 * Harness
 */

int main(void)
{
    run_tests();
    return exit_status();
}
