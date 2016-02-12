/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#ifndef SWANSON_S0_H
#define SWANSON_S0_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>


/*-----------------------------------------------------------------------------
 * S₀: Names
 */

struct s0_name;

/* Makes a copy of content */
struct s0_name *
s0_name_new(size_t size, const void *content);

/* Size is calculated via strlen(3) */
struct s0_name *
s0_name_new_str(const void *content);

void
s0_name_free(struct s0_name *);

const char *
s0_name_content(const struct s0_name *);

size_t
s0_name_size(const struct s0_name *);

bool
s0_name_eq(const struct s0_name *, const struct s0_name *);


/*-----------------------------------------------------------------------------
 * S₀: Name sets
 */

struct s0_name_set;

struct s0_name_set *
s0_name_set_new(void);

void
s0_name_set_free(struct s0_name_set *);

/* Takes ownership of name.  name MUST not already be present in set.  Returns 0
 * if name was added; -1 if we couldn't allocate space for the new entry. */
int
s0_name_set_add(struct s0_name_set *, struct s0_name *name);

size_t
s0_name_set_size(const struct s0_name_set *);

/* Returns entries in order that they were added to the set.  index MUST be <
 * size of name_set.  Set still owns the name; you must not free it. */
struct s0_name *
s0_name_set_at(const struct s0_name_set *, size_t index);


/*-----------------------------------------------------------------------------
 * S₀: Name mappings
 */

struct s0_name_mapping;

struct s0_name_mapping_entry {
    struct s0_name  *from;
    struct s0_name  *to;
};

struct s0_name_mapping *
s0_name_mapping_new(void);

void
s0_name_mapping_free(struct s0_name_mapping *);

/* Takes ownership of from and to.  from MUST not already be present in the
 * mapping's domain, and to MUST not already be present in the mapping's range.
 * Returns 0 if name was added; -1 if we couldn't allocate space for the new
 * entry. */
int
s0_name_mapping_add(struct s0_name_mapping *, struct s0_name *from,
                    struct s0_name *to);

size_t
s0_name_mapping_size(const struct s0_name_mapping *);

/* Returns entries in order that they were added to the set.  index MUST be <
 * size of mapping.  Set still owns the names; you must not free it. */
struct s0_name_mapping_entry
s0_name_mapping_at(const struct s0_name_mapping *, size_t index);

/* Returns NULL if from is not in the mapping's domain. */
struct s0_name *
s0_name_mapping_get(const struct s0_name_mapping *, const struct s0_name *from);


/*-----------------------------------------------------------------------------
 * S₀: Environments
 */

struct s0_entity;
struct s0_environment;

struct s0_environment *
s0_environment_new(void);

void
s0_environment_free(struct s0_environment *);

size_t
s0_environment_size(const struct s0_environment *);

/* Takes ownership of name and entity.  name MUST not already be present in
 * environment.  Returns 0 if entity was added; -1 if we couldn't allocate space
 * for the new entry. */
int
s0_environment_add(struct s0_environment *,
                   struct s0_name *name, struct s0_entity *entity);

/* Returns NULL if name is not in environment. */
struct s0_entity *
s0_environment_get(const struct s0_environment *, const struct s0_name *name);

/* name MUST already be present in environment. */
struct s0_entity *
s0_environment_delete(struct s0_environment *, const struct s0_name *name);


/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

struct s0_block;

struct s0_block *
s0_block_new(void);

void
s0_block_free(struct s0_block *);


/*-----------------------------------------------------------------------------
 * S₀: Named blocks
 */

struct s0_named_blocks;

struct s0_named_blocks *
s0_named_blocks_new(void);

void
s0_named_blocks_free(struct s0_named_blocks *);

size_t
s0_named_blocks_size(const struct s0_named_blocks *);

/* Takes ownership of name and block.  name MUST not already be present in
 * collection.  Returns 0 if block was added; -1 if we couldn't allocate space
 * for the new entry. */
int
s0_named_blocks_add(struct s0_named_blocks *,
                    struct s0_name *name, struct s0_block *block);

/* Returns NULL if name is not in named_blocks. */
struct s0_block *
s0_named_blocks_get(const struct s0_named_blocks *, const struct s0_name *name);


/*-----------------------------------------------------------------------------
 * S₀: Statements
 */

struct s0_statement;

enum s0_statement_type {
    S0_STATEMENT_TYPE_CREATE_ATOM,
    S0_STATEMENT_TYPE_CREATE_CLOSURE,
    S0_STATEMENT_TYPE_CREATE_LITERAL,
    S0_STATEMENT_TYPE_CREATE_METHOD
};

void
s0_statement_free(struct s0_statement *);

enum s0_statement_type
s0_statement_type(const struct s0_statement *);


/* Takes control of dest */
struct s0_statement *
s0_create_atom_new(struct s0_name *dest);

/* Statement MUST be CreateAtom */
struct s0_name *
s0_create_atom_dest(const struct s0_statement *);


/* Takes control of dest, closed_over, and branches */
struct s0_statement *
s0_create_closure_new(struct s0_name *dest, struct s0_name_set *closed_over,
                      struct s0_named_blocks *branches);

/* Statement MUST be CreateClosure */
struct s0_name *
s0_create_closure_dest(const struct s0_statement *);

/* Statement MUST be CreateClosure */
struct s0_name_set *
s0_create_closure_closed_over(const struct s0_statement *);

/* Statement MUST be CreateClosure */
struct s0_named_blocks *
s0_create_closure_branches(const struct s0_statement *);


/* Takes control of dest; makes a copy of content */
struct s0_statement *
s0_create_literal_new(struct s0_name *dest, size_t size, const void *content);

/* Statement MUST be CreateLiteral */
struct s0_name *
s0_create_literal_dest(const struct s0_statement *);

/* Statement MUST be CreateLiteral */
const void *
s0_create_literal_content(const struct s0_statement *);

/* Statement MUST be CreateLiteral */
size_t
s0_create_literal_size(const struct s0_statement *);


/* Takes control of dest, self_input, and body */
struct s0_statement *
s0_create_method_new(struct s0_name *dest, struct s0_name *self_input,
                     struct s0_block *body);

/* Statement MUST be CreateMethod */
struct s0_name *
s0_create_method_dest(const struct s0_statement *);

/* Statement MUST be CreateMethod */
struct s0_name *
s0_create_method_self_input(const struct s0_statement *);

/* Statement MUST be CreateMethod */
struct s0_block *
s0_create_method_body(const struct s0_statement *);


/*-----------------------------------------------------------------------------
 * S₀: Invocations
 */

struct s0_invocation;

enum s0_invocation_type {
    S0_INVOCATION_TYPE_INVOKE_CLOSURE,
    S0_INVOCATION_TYPE_INVOKE_METHOD
};

void
s0_invocation_free(struct s0_invocation *);

enum s0_invocation_type
s0_invocation_type(const struct s0_invocation *);


/* Takes control of src and branch */
struct s0_invocation *
s0_invoke_closure_new(struct s0_name *src, struct s0_name *branch);

/* Invocation MUST be InvokeClosure */
struct s0_name *
s0_invoke_closure_src(const struct s0_invocation *);

/* Invocation MUST be InvokeClosure */
struct s0_name *
s0_invoke_closure_branch(const struct s0_invocation *);


/* Takes control of src and method */
struct s0_invocation *
s0_invoke_method_new(struct s0_name *src, struct s0_name *method);

/* Invocation MUST be InvokeMethod */
struct s0_name *
s0_invoke_method_src(const struct s0_invocation *);

/* Invocation MUST be InvokeMethod */
struct s0_name *
s0_invoke_method_method(const struct s0_invocation *);


/*-----------------------------------------------------------------------------
 * S₀: Entities
 */

enum s0_entity_type {
    S0_ENTITY_TYPE_ATOM,
    S0_ENTITY_TYPE_CLOSURE,
    S0_ENTITY_TYPE_LITERAL,
    S0_ENTITY_TYPE_METHOD,
    S0_ENTITY_TYPE_OBJECT
};

void
s0_entity_free(struct s0_entity *);

enum s0_entity_type
s0_entity_type(const struct s0_entity *);


struct s0_entity *
s0_atom_new(void);

/* Both entities MUST be atoms */
bool
s0_atom_eq(const struct s0_entity *, const struct s0_entity *);


/* Takes control of env and blocks */
struct s0_entity *
s0_closure_new(struct s0_environment *env, struct s0_named_blocks *blocks);

/* Entity MUST be a closure.  Closure retains ownership of environment. */
struct s0_environment *
s0_closure_environment(const struct s0_entity *);

/* Entity MUST be a closure.  Closure retains ownership of blocks. */
struct s0_named_blocks *
s0_closure_named_blocks(const struct s0_entity *);


/* Makes a copy of content */
struct s0_entity *
s0_literal_new(size_t size, const void *content);

/* Size is calculated via strlen(3) */
struct s0_entity *
s0_literal_new_str(const void *content);

/* Entity MUST be a literal */
const char *
s0_literal_content(const struct s0_entity *);

/* Entity MUST be a literal */
size_t
s0_literal_size(const struct s0_entity *);


/* Takes control of self_name and block */
struct s0_entity *
s0_method_new(struct s0_name *self_name, struct s0_block *block);

/* Entity MUST be a method.  method retains ownership of self_name. */
struct s0_name *
s0_method_self_name(const struct s0_entity *);

/* Entity MUST be a method.  method retains ownership of block. */
struct s0_block *
s0_method_block(const struct s0_entity *);


struct s0_object_entry {
    struct s0_name  *name;
    struct s0_entity  *entity;
};

struct s0_entity *
s0_object_new(void);

/* Takes ownership of name and entity.  name MUST not already be present in
 * object.  Returns 0 if entity was added; -1 if we couldn't allocate space
 * for the new entry.  Entity MUST be an object. */
int
s0_object_add(struct s0_entity *,
              struct s0_name *name, struct s0_entity *entity);

/* Entity MUST be an object */
size_t
s0_object_size(const struct s0_entity *);

/* Returns entries in order that they were added to object.  Entity MUST be an
 * object; index MUST be < size of object. */
struct s0_object_entry
s0_object_at(const struct s0_entity *, size_t index);

/* Entity MUST be an object.  Returns NULL if name is not in object. */
struct s0_entity *
s0_object_get(const struct s0_entity *, const struct s0_name *name);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SWANSON_S0_H */
