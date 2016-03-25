/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#ifndef SWANSON_H
#define SWANSON_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
s0_name_human_readable(const struct s0_name *);

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

bool
s0_name_set_contains(const struct s0_name_set *, struct s0_name *name);

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

/* Returns NULL if from is not in the mapping's range. */
struct s0_name *
s0_name_mapping_get_from(const struct s0_name_mapping *,
                         const struct s0_name *to);


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
 * S₀: Named blocks
 */

struct s0_block;
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

enum s0_statement_kind {
    S0_STATEMENT_KIND_CREATE_ATOM,
    S0_STATEMENT_KIND_CREATE_CLOSURE,
    S0_STATEMENT_KIND_CREATE_LITERAL,
    S0_STATEMENT_KIND_CREATE_METHOD
};

void
s0_statement_free(struct s0_statement *);

enum s0_statement_kind
s0_statement_kind(const struct s0_statement *);


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
 * S₀: Statement lists
 */

struct s0_statement_list;

struct s0_statement_list *
s0_statement_list_new(void);

void
s0_statement_list_free(struct s0_statement_list *);

/* Takes ownership of stmt.  Returns 0 if name was added; -1 if we couldn't
 * allocate space for the new entry. */
int
s0_statement_list_add(struct s0_statement_list *, struct s0_statement *stmt);

size_t
s0_statement_list_size(const struct s0_statement_list *);

/* Returns statements in order that they were added to the set.  index MUST be <
 * size of list.  List still owns the statement; you must not free it. */
struct s0_statement *
s0_statement_list_at(const struct s0_statement_list *, size_t index);


/*-----------------------------------------------------------------------------
 * S₀: Invocations
 */

struct s0_invocation;

enum s0_invocation_kind {
    S0_INVOCATION_KIND_INVOKE_CLOSURE,
    S0_INVOCATION_KIND_INVOKE_METHOD
};

void
s0_invocation_free(struct s0_invocation *);

enum s0_invocation_kind
s0_invocation_kind(const struct s0_invocation *);


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
 * S₀: Blocks
 */

/* Takes control of inputs, statements, and invocation */
struct s0_block *
s0_block_new(struct s0_name_mapping *inputs,
             struct s0_statement_list *statements,
             struct s0_invocation *invocation);

void
s0_block_free(struct s0_block *);

struct s0_name_mapping *
s0_block_inputs(const struct s0_block *);

struct s0_statement_list *
s0_block_statements(const struct s0_block *);

struct s0_invocation *
s0_block_invocation(const struct s0_block *);


/*-----------------------------------------------------------------------------
 * S₀: Entities
 */

enum s0_entity_kind {
    S0_ENTITY_KIND_ATOM,
    S0_ENTITY_KIND_CLOSURE,
    S0_ENTITY_KIND_LITERAL,
    S0_ENTITY_KIND_METHOD,
    S0_ENTITY_KIND_OBJECT
};

void
s0_entity_free(struct s0_entity *);

enum s0_entity_kind
s0_entity_kind(const struct s0_entity *);


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


/*-----------------------------------------------------------------------------
 * S₀: Entity types
 */

struct s0_entity_type;

enum s0_entity_type_kind {
    S0_ENTITY_TYPE_KIND_ANY
};

void
s0_entity_type_free(struct s0_entity_type *);

enum s0_entity_type_kind
s0_entity_type_kind(const struct s0_entity_type *);

bool
s0_entity_type_satisfied_by(const struct s0_entity_type *,
                            const struct s0_entity *);


struct s0_entity_type *
s0_any_entity_type_new(void);


/*-----------------------------------------------------------------------------
 * S₀: Environment types
 */

struct s0_environment_type;

struct s0_environment_type_entry {
    struct s0_name  *name;
    struct s0_entity_type  *type;
};

struct s0_environment_type *
s0_environment_type_new(void);

void
s0_environment_type_free(struct s0_environment_type *);

/* Takes ownership of name and type.  name MUST not already be present in
 * environment type.  Returns 0 if name was added; -1 if we couldn't allocate
 * space for the new entry. */
int
s0_environment_type_add(struct s0_environment_type *,
                        struct s0_name *name, struct s0_entity_type *type);

size_t
s0_environment_type_size(const struct s0_environment_type *);

/* Returns entries in order that they were added to the set.  index MUST be <
 * size of environment_type.  Set still owns the name and entity type; you must
 * not free them. */
struct s0_environment_type_entry
s0_environment_type_at(const struct s0_environment_type *, size_t index);

/* Returns NULL if name is not in environment type. */
struct s0_entity_type *
s0_environment_type_get(const struct s0_environment_type *,
                        const struct s0_name *name);

bool
s0_environment_type_satisfied_by(const struct s0_environment_type *,
                                 const struct s0_environment *);


/*-----------------------------------------------------------------------------
 * S₀: YAML
 */


#define SWANSON_TAG_PREFIX  "tag:swanson-lang.org,2016:"
#define S0_CREATE_ATOM_TAG     SWANSON_TAG_PREFIX "create-atom"
#define S0_CREATE_CLOSURE_TAG  SWANSON_TAG_PREFIX "create-closure"
#define S0_CREATE_LITERAL_TAG  SWANSON_TAG_PREFIX "create-literal"
#define S0_CREATE_METHOD_TAG   SWANSON_TAG_PREFIX "create-method"
#define S0_INVOKE_CLOSURE_TAG  SWANSON_TAG_PREFIX "invoke-closure"
#define S0_INVOKE_METHOD_TAG   SWANSON_TAG_PREFIX "invoke-method"


struct s0_yaml_stream;

/* You should allocate these directly on the stack.  All of the fields are
 * opaque; you must only use the functions below to interact with this type. */
struct s0_yaml_node {
    struct s0_yaml_stream  *stream;
    void  *node;
};

#define S0_YAML_NODE_ERROR  ((void *) (uintptr_t) -1)
#define s0_yaml_node_is_error(node_)    ((node_).node == S0_YAML_NODE_ERROR)
#define s0_yaml_node_is_missing(node_)  ((node_).node == NULL)
#define s0_yaml_node_is_valid(node_) \
    (!s0_yaml_node_is_error(node_) && !s0_yaml_node_is_missing(node_))

const char *
s0_yaml_node_tag(const struct s0_yaml_node);

bool
s0_yaml_node_has_tag(const struct s0_yaml_node, const char *tag);

bool
s0_yaml_node_is_mapping(const struct s0_yaml_node);

bool
s0_yaml_node_is_scalar(const struct s0_yaml_node);

bool
s0_yaml_node_is_sequence(const struct s0_yaml_node);

const void *
s0_yaml_node_scalar_content(const struct s0_yaml_node);

size_t
s0_yaml_node_scalar_size(const struct s0_yaml_node);

size_t
s0_yaml_node_sequence_size(const struct s0_yaml_node);

struct s0_yaml_node
s0_yaml_node_sequence_at(const struct s0_yaml_node, size_t index);

size_t
s0_yaml_node_mapping_size(const struct s0_yaml_node);

struct s0_yaml_node
s0_yaml_node_mapping_key_at(const struct s0_yaml_node, size_t index);

struct s0_yaml_node
s0_yaml_node_mapping_value_at(const struct s0_yaml_node, size_t index);

struct s0_yaml_node
s0_yaml_node_mapping_get(const struct s0_yaml_node, const char *key);


/* Makes a copy of `filename`.  `filename` is only descriptive, since you'll
 * already have opened the file. */
struct s0_yaml_stream *
s0_yaml_stream_new_from_file(FILE *fp, const char *filename,
                             bool should_close_fp);

/* Makes a copy of `filename`.  Doesn't actually try to open `filename` until
 * the first time you call s0_yaml_stream_parse_document.  (So the only way this
 * can fail is if we can't allocate memory for the new stream object.) */
struct s0_yaml_stream *
s0_yaml_stream_new_from_filename(const char *filename);

void
s0_yaml_stream_free(struct s0_yaml_stream *);

/* If there was an error during the most recent function call for this stream,
 * returns a string describing the error.  If the most recent function call
 * completed successfully, returns NULL. */
const char *
s0_yaml_stream_last_error(const struct s0_yaml_stream *);

/* Reads the next YAML document from the stream.  If there aren't any more
 * documents in the stream, s0_yaml_node_is_missing will be true of the result.
 * If there's an error reading from the stream, s0_yaml_node_is_error will be
 * true of the result.  The resulting node, and any other nodes you extract from
 * it, are only valid until the next time you call this function. */
struct s0_yaml_node
s0_yaml_stream_parse_document(struct s0_yaml_stream *);

/* Loads an S₀ module from a YAML node.  If the YAML node doesn't conform to the
 * S₀ YAML module schema, then we return NULL, and fill in an error on the
 * stream that the node came from.  You take ownership of the return value and
 * are responsible for freeing it. */
struct s0_entity *
s0_yaml_document_parse_module(struct s0_yaml_node node);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SWANSON_H */
