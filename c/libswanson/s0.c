/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include "swanson.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "ccan/compiler/compiler.h"
#include "ccan/likely/likely.h"


/*-----------------------------------------------------------------------------
 * Structs
 */

struct s0_name {
    size_t  size;
    const void  *content;
};

struct s0_name_set {
    size_t  size;
    size_t  allocated_size;
    struct s0_name  **names;
};

struct s0_name_mapping {
    size_t  size;
    size_t  allocated_size;
    struct s0_name_mapping_entry  *entries;
};

struct s0_environment_entry {
    struct s0_environment_entry  *next;
    struct s0_name  *name;
    struct s0_entity  *entity;
};

struct s0_environment {
    struct s0_environment_entry  *head;
    size_t  size;
};

struct s0_block {
    struct s0_environment_type  *inputs;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;
};

struct s0_named_blocks_entry {
    struct s0_named_blocks_entry  *next;
    struct s0_name  *name;
    struct s0_block  *block;
};

struct s0_named_blocks {
    struct s0_named_blocks_entry  *head;
    size_t  size;
};

struct s0_statement {
    enum s0_statement_kind  kind;
    union {
        struct {
            struct s0_name  *dest;
        } create_atom;
        struct {
            struct s0_name  *dest;
            struct s0_name_set  *closed_over;
            struct s0_named_blocks  *branches;
        } create_closure;
        struct {
            struct s0_name  *dest;
            size_t  size;
            const void  *content;
        } create_literal;
        struct {
            struct s0_name  *dest;
            struct s0_block  *body;
        } create_method;
    } _;
};

struct s0_statement_list {
    size_t  size;
    size_t  allocated_size;
    struct s0_statement  **statements;
};

struct s0_invocation {
    enum s0_invocation_kind  kind;
    union {
        struct {
            struct s0_name  *src;
            struct s0_name  *branch;
            struct s0_name_mapping  *params;
        } invoke_closure;
        struct {
            struct s0_name  *src;
            struct s0_name  *method;
            struct s0_name_mapping  *params;
        } invoke_method;
    } _;
};

struct s0_entity {
    enum s0_entity_kind  kind;
    union {
        struct {
            struct s0_environment  *env;
            struct s0_named_blocks  *blocks;
        } closure;
        struct {
            size_t  size;
            const void  *content;
        } literal;
        struct {
            struct s0_block  *body;
        } method;
        struct {
            size_t  size;
            size_t  allocated_size;
            struct s0_object_entry  *entries;
        } obj;
        struct {
            struct s0_environment_type  *inputs;
            struct s0_continuation  cont;
            s0_primitive_method_free_f  *free_ud;
        } primitive_method;
    } _;
};

struct s0_entity_type {
    enum s0_entity_type_kind  kind;
    union {
        struct {
            struct s0_environment_type_mapping  *branches;
        } closure;
        struct {
            struct s0_environment_type  *body;
        } method;
        struct {
            struct s0_environment_type  *elements;
        } object;
    } _;
};

struct s0_environment_type {
    size_t  size;
    size_t  allocated_size;
    struct s0_environment_type_entry  *entries;
};

struct s0_environment_type_mapping {
    size_t  size;
    size_t  allocated_size;
    struct s0_environment_type_mapping_entry  *entries;
};


/*-----------------------------------------------------------------------------
 * S₀: Errors
 */

#define MAX_ERROR_DESCRIPTION_LENGTH  4096

static struct {
    enum s0_error_code  code;
    char  description1[MAX_ERROR_DESCRIPTION_LENGTH];
    char  description2[MAX_ERROR_DESCRIPTION_LENGTH];
    char  *current_description;
    char  *other_description;
} last_error = {
    S0_ERROR_NONE,
    "",
    "",
    last_error.description1,
    last_error.description2
};

static PRINTF_FMT(2,3) void
s0_set_error(enum s0_error_code code, const char *fmt, ...)
{
    va_list  args;
    last_error.code = code;
    va_start(args, fmt);
    vsnprintf(last_error.current_description, MAX_ERROR_DESCRIPTION_LENGTH,
              fmt, args);
    last_error.current_description[MAX_ERROR_DESCRIPTION_LENGTH - 1] = '\0';
    va_end(args);
}

static PRINTF_FMT(1,2) void
s0_prefix_error(const char *fmt, ...)
{
    va_list  args;
    int  prefix_size;
    char  *swap;

    /* Write the prefix into the unused buffer. */
    va_start(args, fmt);
    prefix_size = vsnprintf
        (last_error.other_description, MAX_ERROR_DESCRIPTION_LENGTH, fmt, args);
    va_end(args);

    /* The copy the existing buffer to the end of the unused buffer. */
    if (prefix_size < MAX_ERROR_DESCRIPTION_LENGTH) {
        strncpy(last_error.other_description + prefix_size,
                last_error.current_description,
                MAX_ERROR_DESCRIPTION_LENGTH - prefix_size);
    }
    last_error.other_description[MAX_ERROR_DESCRIPTION_LENGTH - 1] = '\0';

    /* Then swap the two buffers so that the new error message is "current". */
    swap = last_error.other_description;
    last_error.other_description = last_error.current_description;
    last_error.current_description = swap;
}

#define s0_set_memory_error_(func) \
    s0_set_error(S0_ERROR_MEMORY_ERROR, "Error allocating memory in %s", func)
#define s0_set_memory_error() s0_set_memory_error_(__func__)

enum s0_error_code
s0_error_get_last_code(void)
{
    return last_error.code;
}

const char *
s0_error_get_last_description(void)
{
    return last_error.current_description;
}


/*-----------------------------------------------------------------------------
 * Names
 */

struct s0_name *
s0_name_new(size_t size, const void *content)
{
    struct s0_name  *name = malloc(sizeof(struct s0_name));
    if (unlikely(name == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    name->size = size;
    name->content = malloc(size + 1);
    if (unlikely(name->content == NULL)) {
        free(name);
        s0_set_memory_error();
        return NULL;
    }
    memcpy((void *) name->content, content, size);
    ((char *) name->content)[size] = '\0';
    return name;
}

struct s0_name *
s0_name_new_str(const void *content)
{
    return s0_name_new(strlen(content), content);
}

struct s0_name *
s0_name_new_copy(const struct s0_name *other)
{
    return s0_name_new(other->size, other->content);
}

void
s0_name_free(struct s0_name *name)
{
    free((void *) name->content);
    free(name);
}

const char *
s0_name_human_readable(const struct s0_name *name)
{
    /* TODO: Return something nicer for binary content. */
    return name->content;
}

const char *
s0_name_content(const struct s0_name *name)
{
    return name->content;
}

size_t
s0_name_size(const struct s0_name *name)
{
    return name->size;
}

bool
s0_name_eq(const struct s0_name *n1, const struct s0_name *n2)
{
    return n1->size == n2->size
        && (memcmp(n1->content, n2->content, n1->size) == 0);
}


/*-----------------------------------------------------------------------------
 * Name sets
 */

#define DEFAULT_INITIAL_NAME_SET_SIZE  4

struct s0_name_set *
s0_name_set_new(void)
{
    struct s0_name_set  *set = malloc(sizeof(struct s0_name_set));
    if (unlikely(set == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    set->size = 0;
    set->allocated_size = DEFAULT_INITIAL_NAME_SET_SIZE;
    set->names =
        malloc(DEFAULT_INITIAL_NAME_SET_SIZE * sizeof(struct s0_name *));
    if (unlikely(set->names == NULL)) {
        free(set);
        s0_set_memory_error();
        return NULL;
    }
    return set;
}

struct s0_name_set *
s0_name_set_new_copy(const struct s0_name_set *other)
{
    size_t  i;
    struct s0_name_set  *set = s0_name_set_new();
    if (unlikely(set == NULL)) {
        return NULL;
    }

    for (i = 0; i < other->size; i++) {
        int  rc;
        struct s0_name  *copy;

        copy = s0_name_new_copy(other->names[i]);
        if (unlikely(copy == NULL)) {
            s0_name_set_free(set);
            return NULL;
        }

        rc = s0_name_set_add(set, copy);
        if (unlikely(rc != 0)) {
            s0_name_set_free(set);
            return NULL;
        }
    }

    return set;
}

void
s0_name_set_free(struct s0_name_set *set)
{
    size_t  i;
    for (i = 0; i < set->size; i++) {
        s0_name_free(set->names[i]);
    }
    free(set->names);
    free(set);
}

int
s0_name_set_add(struct s0_name_set *set, struct s0_name *name)
{
#if !defined(NDEBUG)
    {
        size_t  i;
        for (i = 0; i < set->size; i++) {
            assert(!s0_name_eq(name, set->names[i]));
        }
    }
#endif

    if (unlikely(set->size == set->allocated_size)) {
        size_t  new_size = set->allocated_size * 2;
        struct s0_name  **new_names =
            realloc(set->names, new_size * sizeof(struct s0_name *));
        if (unlikely(new_names == NULL)) {
            s0_name_free(name);
            s0_set_memory_error();
            return -1;
        }
        set->names = new_names;
        set->allocated_size = new_size;
    }

    set->names[set->size++] = name;
    return 0;
}

bool
s0_name_set_contains(const struct s0_name_set *set, struct s0_name *name)
{
    size_t  i;
    for (i = 0; i < set->size; i++) {
        if (s0_name_eq(name, set->names[i])) {
            return true;
        }
    }
    return false;
}

size_t
s0_name_set_size(const struct s0_name_set *set)
{
    return set->size;
}

struct s0_name *
s0_name_set_at(const struct s0_name_set *set, size_t index)
{
    assert(index < set->size);
    return set->names[index];
}

bool
s0_name_set_eq(const struct s0_name_set *s1, const struct s0_name_set *s2)
{
    size_t  i;

    if (s1->size != s2->size) {
        return false;
    }

    for (i = 0; i < s1->size; i++) {
        if (!s0_name_set_contains(s2, s1->names[i])) {
            return false;
        }
    }

    return true;
}


/*-----------------------------------------------------------------------------
 * Name mappings
 */

#define DEFAULT_INITIAL_NAME_MAPPING_SIZE  4

struct s0_name_mapping *
s0_name_mapping_new(void)
{
    struct s0_name_mapping  *mapping = malloc(sizeof(struct s0_name_mapping));
    if (unlikely(mapping == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    mapping->size = 0;
    mapping->allocated_size = DEFAULT_INITIAL_NAME_MAPPING_SIZE;
    mapping->entries =
        malloc(DEFAULT_INITIAL_NAME_MAPPING_SIZE *
               sizeof(struct s0_name_mapping_entry));
    if (unlikely(mapping->entries == NULL)) {
        free(mapping);
        s0_set_memory_error();
        return NULL;
    }
    return mapping;
}

struct s0_name_mapping *
s0_name_mapping_new_copy(const struct s0_name_mapping *other)
{
    size_t  i;
    struct s0_name_mapping  *mapping = s0_name_mapping_new();
    if (unlikely(mapping == NULL)) {
        return NULL;
    }

    for (i = 0; i < other->size; i++) {
        int  rc;
        struct s0_name  *from_copy;
        struct s0_name  *to_copy;

        from_copy = s0_name_new_copy(other->entries[i].from);
        if (unlikely(from_copy == NULL)) {
            s0_name_mapping_free(mapping);
            return NULL;
        }

        to_copy = s0_name_new_copy(other->entries[i].to);
        if (unlikely(to_copy == NULL)) {
            s0_name_free(from_copy);
            s0_name_mapping_free(mapping);
            return NULL;
        }

        rc = s0_name_mapping_add(mapping, from_copy, to_copy);
        if (unlikely(rc != 0)) {
            s0_name_mapping_free(mapping);
            return NULL;
        }
    }

    return mapping;
}

void
s0_name_mapping_free(struct s0_name_mapping *mapping)
{
    size_t  i;
    for (i = 0; i < mapping->size; i++) {
        s0_name_free(mapping->entries[i].from);
        s0_name_free(mapping->entries[i].to);
    }
    free(mapping->entries);
    free(mapping);
}

int
s0_name_mapping_add(struct s0_name_mapping *mapping, struct s0_name *from,
                    struct s0_name *to)
{
    struct s0_name_mapping_entry  *new_entry;

#if !defined(NDEBUG)
    {
        size_t  i;
        for (i = 0; i < mapping->size; i++) {
            assert(!s0_name_eq(from, mapping->entries[i].from));
            assert(!s0_name_eq(to, mapping->entries[i].to));
        }
    }
#endif

    if (unlikely(mapping->size == mapping->allocated_size)) {
        size_t  new_size = mapping->allocated_size * 2;
        struct s0_name_mapping_entry  *new_entries =
            realloc(mapping->entries,
                    new_size * sizeof(struct s0_name_mapping_entry));
        if (unlikely(new_entries == NULL)) {
            s0_name_free(from);
            s0_name_free(to);
            s0_set_memory_error();
            return -1;
        }
        mapping->entries = new_entries;
        mapping->allocated_size = new_size;
    }

    new_entry = &mapping->entries[mapping->size++];
    new_entry->from = from;
    new_entry->to = to;
    return 0;
}

size_t
s0_name_mapping_size(const struct s0_name_mapping *mapping)
{
    return mapping->size;
}

const struct s0_name_mapping_entry *
s0_name_mapping_at(const struct s0_name_mapping *mapping, size_t index)
{
    assert(index < mapping->size);
    return &mapping->entries[index];
}

const struct s0_name *
s0_name_mapping_get(const struct s0_name_mapping *mapping,
                    const struct s0_name *from)
{
    size_t  i;
    for (i = 0; i < mapping->size; i++) {
        if (s0_name_eq(mapping->entries[i].from, from)) {
            return mapping->entries[i].to;
        }
    }
    return NULL;
}

const struct s0_name *
s0_name_mapping_get_from(const struct s0_name_mapping *mapping,
                         const struct s0_name *to)
{
    size_t  i;
    for (i = 0; i < mapping->size; i++) {
        if (s0_name_eq(mapping->entries[i].to, to)) {
            return mapping->entries[i].from;
        }
    }
    return NULL;
}

bool
s0_name_mapping_eq(const struct s0_name_mapping *m1,
                   const struct s0_name_mapping *m2)
{
    size_t  i;

    if (m1->size != m2->size) {
        return false;
    }

    for (i = 0; i < m1->size; i++) {
        const struct s0_name  *m2_to =
            s0_name_mapping_get(m2, m1->entries[i].from);
        if (m2_to == NULL || !s0_name_eq(m2_to, m1->entries[i].to)) {
            return false;
        }
    }

    return true;
}


/*-----------------------------------------------------------------------------
 * Environments
 */

struct s0_environment *
s0_environment_new(void)
{
    struct s0_environment  *env = malloc(sizeof(struct s0_environment));
    if (unlikely(env == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    env->head = NULL;
    env->size = 0;
    return env;
}

void
s0_environment_free(struct s0_environment *env)
{
    struct s0_environment_entry  *curr;
    struct s0_environment_entry  *next;
    for (curr = env->head; curr != NULL; curr = next) {
        next = curr->next;
        s0_name_free(curr->name);
        s0_entity_free(curr->entity);
        free(curr);
    }
    free(env);
}

size_t
s0_environment_size(const struct s0_environment *env)
{
    return env->size;
}

int
s0_environment_add(struct s0_environment *env,
                   struct s0_name *name, struct s0_entity *entity)
{
    struct s0_environment_entry  *entry;

#if !defined(NDEBUG)
    {
        struct s0_environment_entry  *curr;
        for (curr = env->head; curr != NULL; curr = curr->next) {
            assert(!s0_name_eq(curr->name, name));
        }
    }
#endif

    entry = malloc(sizeof(struct s0_environment_entry));
    if (unlikely(entry == NULL)) {
        s0_name_free(name);
        s0_entity_free(entity);
        s0_set_memory_error();
        return -1;
    }

    entry->name = name;
    entry->entity = entity;
    entry->next = env->head;
    env->head = entry;
    env->size++;
    return 0;
}

struct s0_entity *
s0_environment_get(const struct s0_environment *env, const struct s0_name *name)
{
    struct s0_environment_entry  *curr;
    for (curr = env->head; curr != NULL; curr = curr->next) {
        if (s0_name_eq(curr->name, name)) {
            return curr->entity;
        }
    }
    return NULL;
}

struct s0_entity *
s0_environment_delete(struct s0_environment *env, const struct s0_name *name)
{
    struct s0_environment_entry  *prev;
    struct s0_environment_entry  *curr;
    for (prev = NULL, curr = env->head; curr != NULL;
         prev = curr, curr = curr->next) {
        if (s0_name_eq(curr->name, name)) {
            struct s0_entity  *entity = curr->entity;
            if (prev == NULL) {
                env->head = curr->next;
            } else {
                prev->next = curr->next;
            }
            s0_name_free(curr->name);
            free(curr);
            env->size--;
            return entity;
        }
    }

    /* Precondition says this isn't allowed. */
    assert(false);
    return NULL;
}

int
s0_environment_extract(struct s0_environment *dest, struct s0_environment *src,
                       const struct s0_name_set *set)
{
    size_t  i;

#if !defined(NDEBUG)
    for (i = 0; i < set->size; i++) {
        const struct s0_name  *name = set->names[i];
        assert(s0_environment_get(src, name) != NULL);
        assert(s0_environment_get(dest, name) == NULL);
    }
#endif

    for (i = 0; i < set->size; i++) {
        const struct s0_name  *name = set->names[i];
        struct s0_entity  *entity;

        entity = s0_environment_delete(src, name);
        if (entity == NULL) {
            return -1;
        } else {
            int  rc;
            struct s0_name  *name_copy = s0_name_new_copy(name);
            if (unlikely(name_copy == NULL)) {
                s0_entity_free(entity);
                return -1;
            }

            rc = s0_environment_add(dest, name_copy, entity);
            if (unlikely(rc != 0)) {
                return -1;
            }
        }
    }
    return 0;
}

int
s0_environment_merge(struct s0_environment *dest, struct s0_environment *src)
{
    struct s0_environment_entry  *curr;

#if !defined(NDEBUG)
    for (curr = src->head; curr != NULL; curr = curr->next) {
        assert(s0_environment_get(dest, curr->name) == NULL);
    }
#endif

    if (dest->head == NULL) {
        dest->head = src->head;
        dest->size += src->size;
        src->head = NULL;
        src->size = 0;
    } else {
        /* Walk through to the end of the list. */
        for (curr = dest->head; curr->next != NULL; curr = curr->next) {
        }
        /* Spackle the src list onto the end of the dest list. */
        curr->next = src->head;
        dest->size += src->size;
        src->head = NULL;
        src->size = 0;
    }
    return 0;
}

int
s0_environment_rename(struct s0_environment *env,
                      const struct s0_name_mapping *mapping)
{
    struct s0_environment_entry  *curr;

    assert(env->size == mapping->size);

    for (curr = env->head; curr != NULL; curr = curr->next){
        const struct s0_name  *from = curr->name;
        const struct s0_name  *to;
        struct s0_name  *to_copy;

        to = s0_name_mapping_get(mapping, from);
        assert(to != NULL);

        to_copy = s0_name_new_copy(to);
        if (unlikely(to_copy == NULL)) {
            return -1;
        }

        s0_name_free(curr->name);
        curr->name = to_copy;
    }

    return 0;
}


/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

struct s0_block *
s0_block_new(struct s0_environment_type *inputs,
             struct s0_statement_list *statements,
             struct s0_invocation *invocation)
{
    struct s0_block  *block = malloc(sizeof(struct s0_block));
    if (unlikely(block == NULL)) {
        s0_environment_type_free(inputs);
        s0_statement_list_free(statements);
        s0_invocation_free(invocation);
        s0_set_memory_error();
        return NULL;
    }
    block->inputs = inputs;
    block->statements = statements;
    block->invocation = invocation;
    return block;
}

struct s0_block *
s0_block_new_copy(const struct s0_block *other)
{
    struct s0_environment_type  *inputs;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;

    inputs = s0_environment_type_new_copy(other->inputs);
    if (unlikely(inputs == NULL)) {
        return NULL;
    }

    statements = s0_statement_list_new_copy(other->statements);
    if (unlikely(statements == NULL)) {
        s0_environment_type_free(inputs);
        return NULL;
    }

    invocation = s0_invocation_new_copy(other->invocation);
    if (unlikely(invocation == NULL)) {
        s0_environment_type_free(inputs);
        s0_statement_list_free(statements);
        return NULL;
    }

    return s0_block_new(inputs, statements, invocation);
}

void
s0_block_free(struct s0_block *block)
{
    s0_environment_type_free(block->inputs);
    s0_statement_list_free(block->statements);
    s0_invocation_free(block->invocation);
    free(block);
}

struct s0_environment_type *
s0_block_inputs(const struct s0_block *block)
{
    return block->inputs;
}

struct s0_statement_list *
s0_block_statements(const struct s0_block *block)
{
    return block->statements;
}

struct s0_invocation *
s0_block_invocation(const struct s0_block *block)
{
    return block->invocation;
}

bool
s0_block_eq(const struct s0_block *b1, const struct s0_block *b2)
{
    return s0_environment_type_equiv(b1->inputs, b2->inputs)
        && s0_statement_list_eq(b1->statements, b2->statements)
        && s0_invocation_eq(b1->invocation, b2->invocation);
}


/*-----------------------------------------------------------------------------
 * Named blocks
 */

struct s0_named_blocks *
s0_named_blocks_new(void)
{
    struct s0_named_blocks  *blocks = malloc(sizeof(struct s0_named_blocks));
    if (unlikely(blocks == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    blocks->head = NULL;
    blocks->size = 0;
    return blocks;
}

struct s0_named_blocks *
s0_named_blocks_new_copy(const struct s0_named_blocks *other)
{
    struct s0_named_blocks  *blocks;
    struct s0_named_blocks_entry  *curr;

    blocks = s0_named_blocks_new();
    if (unlikely(blocks == NULL)) {
        return NULL;
    }

    for (curr = other->head; curr != NULL; curr = curr->next) {
        int  rc;
        struct s0_name  *name_copy;
        struct s0_block  *block_copy;

        name_copy = s0_name_new_copy(curr->name);
        if (unlikely(name_copy == NULL)) {
            s0_named_blocks_free(blocks);
            return NULL;
        }

        block_copy = s0_block_new_copy(curr->block);
        if (unlikely(block_copy == NULL)) {
            s0_name_free(name_copy);
            s0_named_blocks_free(blocks);
            return NULL;
        }

        rc = s0_named_blocks_add(blocks, name_copy, block_copy);
        if (unlikely(rc != 0)) {
            s0_named_blocks_free(blocks);
            return NULL;
        }
    }

    return blocks;
}

void
s0_named_blocks_free(struct s0_named_blocks *blocks)
{
    struct s0_named_blocks_entry  *curr;
    struct s0_named_blocks_entry  *next;
    for (curr = blocks->head; curr != NULL; curr = next) {
        next = curr->next;
        s0_name_free(curr->name);
        s0_block_free(curr->block);
        free(curr);
    }
    free(blocks);
}

size_t
s0_named_blocks_size(const struct s0_named_blocks *blocks)
{
    size_t  size = 0;
    struct s0_named_blocks_entry  *curr;
    for (curr = blocks->head; curr != NULL; curr = curr->next) {
        size++;
    }
    return size;
}

int
s0_named_blocks_add(struct s0_named_blocks *blocks,
                    struct s0_name *name, struct s0_block *block)
{
    struct s0_named_blocks_entry  *entry;

#if !defined(NDEBUG)
    {
        struct s0_named_blocks_entry  *curr;
        for (curr = blocks->head; curr != NULL; curr = curr->next) {
            assert(!s0_name_eq(curr->name, name));
        }
    }
#endif

    entry = malloc(sizeof(struct s0_named_blocks_entry));
    if (unlikely(entry == NULL)) {
        s0_name_free(name);
        s0_block_free(block);
        s0_set_memory_error();
        return -1;
    }

    entry->name = name;
    entry->block = block;
    entry->next = blocks->head;
    blocks->head = entry;
    blocks->size++;
    return 0;
}

struct s0_block *
s0_named_blocks_get(const struct s0_named_blocks *blocks,
                    const struct s0_name *name)
{
    struct s0_named_blocks_entry  *curr;
    for (curr = blocks->head; curr != NULL; curr = curr->next) {
        if (s0_name_eq(curr->name, name)) {
            return curr->block;
        }
    }
    return NULL;
}

struct s0_block *
s0_named_blocks_delete(struct s0_named_blocks *blocks,
                       const struct s0_name *name)
{
    struct s0_named_blocks_entry  *prev;
    struct s0_named_blocks_entry  *curr;
    for (prev = NULL, curr = blocks->head; curr != NULL;
         prev = curr, curr = curr->next) {
        if (s0_name_eq(curr->name, name)) {
            struct s0_block  *block = curr->block;
            if (prev == NULL) {
                blocks->head = curr->next;
            } else {
                prev->next = curr->next;
            }
            s0_name_free(curr->name);
            free(curr);
            return block;
        }
    }
    return NULL;
}

bool
s0_named_blocks_eq(const struct s0_named_blocks *nb1,
                   const struct s0_named_blocks *nb2)
{
    struct s0_named_blocks_entry  *curr;

    if (nb1->size != nb2->size) {
        return false;
    }

    for (curr = nb1->head; curr != NULL; curr = curr->next) {
        struct s0_block  *nb2_block = s0_named_blocks_get(nb2, curr->name);
        if (nb2_block == NULL || !s0_block_eq(nb2_block, curr->block)) {
            return false;
        }
    }

    return true;
}


/*-----------------------------------------------------------------------------
 * Statements
 */

struct s0_statement *
s0_create_atom_new(struct s0_name *dest)
{
    struct s0_statement  *stmt = malloc(sizeof(struct s0_statement));
    if (unlikely(stmt == NULL)) {
        s0_name_free(dest);
        s0_set_memory_error();
        return NULL;
    }
    stmt->kind = S0_STATEMENT_KIND_CREATE_ATOM;
    stmt->_.create_atom.dest = dest;
    return stmt;
}

static struct s0_statement *
s0_create_atom_new_copy(const struct s0_statement *other)
{
    struct s0_name  *dest;

    dest = s0_name_new_copy(other->_.create_atom.dest);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    return s0_create_atom_new(dest);
}

static void
s0_create_atom_free(struct s0_statement *stmt)
{
    s0_name_free(stmt->_.create_atom.dest);
}

struct s0_name *
s0_create_atom_dest(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_ATOM);
    return stmt->_.create_atom.dest;
}

static bool
s0_create_atom_eq(const struct s0_statement *s1, const struct s0_statement *s2)
{
    return s0_name_eq(s1->_.create_atom.dest, s2->_.create_atom.dest);
}


struct s0_statement *
s0_create_closure_new(struct s0_name *dest, struct s0_name_set *closed_over,
                      struct s0_named_blocks *branches)
{
    struct s0_statement  *stmt = malloc(sizeof(struct s0_statement));
    if (unlikely(stmt == NULL)) {
        s0_name_free(dest);
        s0_name_set_free(closed_over);
        s0_named_blocks_free(branches);
        s0_set_memory_error();
        return NULL;
    }
    stmt->kind = S0_STATEMENT_KIND_CREATE_CLOSURE;
    stmt->_.create_closure.dest = dest;
    stmt->_.create_closure.closed_over = closed_over;
    stmt->_.create_closure.branches = branches;
    return stmt;
}

static struct s0_statement *
s0_create_closure_new_copy(const struct s0_statement *other)
{
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;

    dest = s0_name_new_copy(other->_.create_closure.dest);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    closed_over = s0_name_set_new_copy(other->_.create_closure.closed_over);
    if (unlikely(closed_over == NULL)) {
        s0_name_free(dest);
        return NULL;
    }

    branches = s0_named_blocks_new_copy(other->_.create_closure.branches);
    if (unlikely(branches == NULL)) {
        s0_name_free(dest);
        s0_name_set_free(closed_over);
        return NULL;
    }

    return s0_create_closure_new(dest, closed_over, branches);
}

static void
s0_create_closure_free(struct s0_statement *stmt)
{
    s0_name_free(stmt->_.create_closure.dest);
    s0_name_set_free(stmt->_.create_closure.closed_over);
    s0_named_blocks_free(stmt->_.create_closure.branches);
}

struct s0_name *
s0_create_closure_dest(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_CLOSURE);
    return stmt->_.create_closure.dest;
}

struct s0_name_set *
s0_create_closure_closed_over(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_CLOSURE);
    return stmt->_.create_closure.closed_over;
}

struct s0_named_blocks *
s0_create_closure_branches(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_CLOSURE);
    return stmt->_.create_closure.branches;
}

static bool
s0_create_closure_eq(const struct s0_statement *s1,
                     const struct s0_statement *s2)
{
    return s0_name_eq(s1->_.create_closure.dest, s2->_.create_closure.dest)
        && s0_name_set_eq(s1->_.create_closure.closed_over,
                          s2->_.create_closure.closed_over)
        && s0_named_blocks_eq(s1->_.create_closure.branches,
                              s2->_.create_closure.branches);
}


struct s0_statement *
s0_create_literal_new(struct s0_name *dest, size_t size, const void *content)
{
    struct s0_statement  *stmt = malloc(sizeof(struct s0_statement));
    if (unlikely(stmt == NULL)) {
        s0_name_free(dest);
        s0_set_memory_error();
        return NULL;
    }
    stmt->kind = S0_STATEMENT_KIND_CREATE_LITERAL;
    stmt->_.create_literal.dest = dest;
    stmt->_.create_literal.size = size;
    stmt->_.create_literal.content = malloc(size);
    if (unlikely(stmt->_.create_literal.content == NULL)) {
        s0_name_free(dest);
        free(stmt);
        s0_set_memory_error();
        return NULL;
    }
    memcpy((void *) stmt->_.create_literal.content, content, size);
    return stmt;
}

static struct s0_statement *
s0_create_literal_new_copy(const struct s0_statement *other)
{
    struct s0_name  *dest;

    dest = s0_name_new_copy(other->_.create_literal.dest);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    return s0_create_literal_new
        (dest, other->_.create_literal.size, other->_.create_literal.content);
}

static void
s0_create_literal_free(struct s0_statement *stmt)
{
    s0_name_free(stmt->_.create_literal.dest);
    free((void *) stmt->_.create_literal.content);
}

struct s0_name *
s0_create_literal_dest(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_LITERAL);
    return stmt->_.create_literal.dest;
}

const void *
s0_create_literal_content(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_LITERAL);
    return stmt->_.create_literal.content;
}

size_t
s0_create_literal_size(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_LITERAL);
    return stmt->_.create_literal.size;
}

static bool
s0_create_literal_eq(const struct s0_statement *s1,
                     const struct s0_statement *s2)
{
    return s0_name_eq(s1->_.create_literal.dest, s2->_.create_literal.dest)
        && s1->_.create_literal.size == s2->_.create_literal.size
        && memcmp(s1->_.create_literal.content, s2->_.create_literal.content,
                  s1->_.create_literal.size) == 0;
}


struct s0_statement *
s0_create_method_new(struct s0_name *dest, struct s0_block *body)
{
    struct s0_statement  *stmt = malloc(sizeof(struct s0_statement));
    if (unlikely(stmt == NULL)) {
        s0_name_free(dest);
        s0_block_free(body);
        s0_set_memory_error();
        return NULL;
    }
    stmt->kind = S0_STATEMENT_KIND_CREATE_METHOD;
    stmt->_.create_method.dest = dest;
    stmt->_.create_method.body = body;
    return stmt;
}

static struct s0_statement *
s0_create_method_new_copy(const struct s0_statement *other)
{
    struct s0_name  *dest;
    struct s0_block  *body;

    dest = s0_name_new_copy(other->_.create_method.dest);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    body = s0_block_new_copy(other->_.create_method.body);
    if (unlikely(body == NULL)) {
        s0_name_free(dest);
        return NULL;
    }

    return s0_create_method_new(dest, body);
}

static void
s0_create_method_free(struct s0_statement *stmt)
{
    s0_name_free(stmt->_.create_method.dest);
    s0_block_free(stmt->_.create_method.body);
}

struct s0_name *
s0_create_method_dest(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_METHOD);
    return stmt->_.create_method.dest;
}

struct s0_block *
s0_create_method_body(const struct s0_statement *stmt)
{
    assert(stmt->kind == S0_STATEMENT_KIND_CREATE_METHOD);
    return stmt->_.create_method.body;
}

static bool
s0_create_method_eq(const struct s0_statement *s1,
                    const struct s0_statement *s2)
{
    return s0_name_eq(s1->_.create_method.dest, s2->_.create_method.dest)
        && s0_block_eq(s1->_.create_method.body, s2->_.create_method.body);
}


struct s0_statement *
s0_statement_new_copy(const struct s0_statement *other)
{
    switch (other->kind) {
        case S0_STATEMENT_KIND_CREATE_ATOM:
            return s0_create_atom_new_copy(other);
        case S0_STATEMENT_KIND_CREATE_CLOSURE:
            return s0_create_closure_new_copy(other);
        case S0_STATEMENT_KIND_CREATE_LITERAL:
            return s0_create_literal_new_copy(other);
        case S0_STATEMENT_KIND_CREATE_METHOD:
            return s0_create_method_new_copy(other);
        default:
            assert(false);
            break;
    }
}

void
s0_statement_free(struct s0_statement *stmt)
{
    switch (stmt->kind) {
        case S0_STATEMENT_KIND_CREATE_ATOM:
            s0_create_atom_free(stmt);
            break;
        case S0_STATEMENT_KIND_CREATE_CLOSURE:
            s0_create_closure_free(stmt);
            break;
        case S0_STATEMENT_KIND_CREATE_LITERAL:
            s0_create_literal_free(stmt);
            break;
        case S0_STATEMENT_KIND_CREATE_METHOD:
            s0_create_method_free(stmt);
            break;
        default:
            assert(false);
            break;
    }
    free(stmt);
}

enum s0_statement_kind
s0_statement_kind(const struct s0_statement *stmt)
{
    return stmt->kind;
}

bool
s0_statement_eq(const struct s0_statement *s1, const struct s0_statement *s2)
{
    if (s1->kind != s2->kind) {
        return false;
    }
    switch (s1->kind) {
        case S0_STATEMENT_KIND_CREATE_ATOM:
            return s0_create_atom_eq(s1, s2);
        case S0_STATEMENT_KIND_CREATE_CLOSURE:
            return s0_create_closure_eq(s1, s2);
        case S0_STATEMENT_KIND_CREATE_LITERAL:
            return s0_create_literal_eq(s1, s2);
        case S0_STATEMENT_KIND_CREATE_METHOD:
            return s0_create_method_eq(s1, s2);
        default:
            assert(false);
            break;
    }
}


/*-----------------------------------------------------------------------------
 * Statement lists
 */

#define DEFAULT_INITIAL_STATEMENT_LIST_SIZE  4

struct s0_statement_list *
s0_statement_list_new(void)
{
    struct s0_statement_list  *list = malloc(sizeof(struct s0_statement_list));
    if (unlikely(list == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    list->size = 0;
    list->allocated_size = DEFAULT_INITIAL_STATEMENT_LIST_SIZE;
    list->statements =
        malloc(DEFAULT_INITIAL_STATEMENT_LIST_SIZE *
               sizeof(struct s0_statement *));
    if (unlikely(list->statements == NULL)) {
        free(list);
        s0_set_memory_error();
        return NULL;
    }
    return list;
}

struct s0_statement_list *
s0_statement_list_new_copy(const struct s0_statement_list *other)
{
    size_t  i;
    struct s0_statement_list  *list = s0_statement_list_new();
    if (unlikely(list == NULL)) {
        return NULL;
    }

    for (i = 0; i < other->size; i++) {
        int  rc;
        struct s0_statement  *copy;

        copy = s0_statement_new_copy(other->statements[i]);
        if (unlikely(copy == NULL)) {
            s0_statement_list_free(list);
            return NULL;
        }

        rc = s0_statement_list_add(list, copy);
        if (unlikely(rc != 0)) {
            s0_statement_list_free(list);
            return NULL;
        }
    }

    return list;
}

void
s0_statement_list_free(struct s0_statement_list *list)
{
    size_t  i;
    for (i = 0; i < list->size; i++) {
        s0_statement_free(list->statements[i]);
    }
    free(list->statements);
    free(list);
}

int
s0_statement_list_add(struct s0_statement_list *list, struct s0_statement *stmt)
{
    if (unlikely(list->size == list->allocated_size)) {
        size_t  new_size = list->allocated_size * 2;
        struct s0_statement  **new_statements =
            realloc(list->statements, new_size * sizeof(struct s0_statement *));
        if (unlikely(new_statements == NULL)) {
            s0_statement_free(stmt);
            s0_set_memory_error();
            return -1;
        }
        list->statements = new_statements;
        list->allocated_size = new_size;
    }

    list->statements[list->size++] = stmt;
    return 0;
}

size_t
s0_statement_list_size(const struct s0_statement_list *list)
{
    return list->size;
}

struct s0_statement *
s0_statement_list_at(const struct s0_statement_list *list, size_t index)
{
    assert(index < list->size);
    return list->statements[index];
}

bool
s0_statement_list_eq(const struct s0_statement_list *l1,
                     const struct s0_statement_list *l2)
{
    size_t  i;

    if (l1->size != l2->size) {
        return false;
    }

    for (i = 0; i < l1->size; i++) {
        if (!s0_statement_eq(l1->statements[i], l2->statements[i])) {
            return false;
        }
    }

    return true;
}


/*-----------------------------------------------------------------------------
 * Invocations
 */

struct s0_invocation *
s0_invoke_closure_new(struct s0_name *src, struct s0_name *branch,
                      struct s0_name_mapping *params)
{
    struct s0_invocation  *invocation = malloc(sizeof(struct s0_invocation));
    if (unlikely(invocation == NULL)) {
        s0_name_free(src);
        s0_name_free(branch);
        s0_name_mapping_free(params);
        s0_set_memory_error();
        return NULL;
    }
    invocation->kind = S0_INVOCATION_KIND_INVOKE_CLOSURE;
    invocation->_.invoke_closure.src = src;
    invocation->_.invoke_closure.branch = branch;
    invocation->_.invoke_closure.params = params;
    return invocation;
}

static struct s0_invocation *
s0_invoke_closure_new_copy(const struct s0_invocation *other)
{
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *params;

    src = s0_name_new_copy(other->_.invoke_closure.src);
    if (unlikely(src == NULL)) {
        return NULL;
    }

    branch = s0_name_new_copy(other->_.invoke_closure.branch);
    if (unlikely(branch == NULL)) {
        s0_name_free(src);
        return NULL;
    }

    params = s0_name_mapping_new_copy(other->_.invoke_closure.params);
    if (unlikely(params == NULL)) {
        s0_name_free(src);
        s0_name_free(branch);
        return NULL;
    }

    return s0_invoke_closure_new(src, branch, params);
}

static void
s0_invoke_closure_free(struct s0_invocation *invocation)
{
    s0_name_free(invocation->_.invoke_closure.src);
    s0_name_free(invocation->_.invoke_closure.branch);
    s0_name_mapping_free(invocation->_.invoke_closure.params);
}

struct s0_name *
s0_invoke_closure_src(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_CLOSURE);
    return invocation->_.invoke_closure.src;
}

struct s0_name *
s0_invoke_closure_branch(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_CLOSURE);
    return invocation->_.invoke_closure.branch;
}

struct s0_name_mapping *
s0_invoke_closure_params(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_CLOSURE);
    return invocation->_.invoke_closure.params;
}

static bool
s0_invoke_closure_eq(const struct s0_invocation *i1,
                     const struct s0_invocation *i2)
{
    return s0_name_eq(i1->_.invoke_closure.src, i2->_.invoke_closure.src)
        && s0_name_eq(i1->_.invoke_closure.branch, i2->_.invoke_closure.branch)
        && s0_name_mapping_eq(i1->_.invoke_closure.params,
                              i2->_.invoke_closure.params);
}


struct s0_invocation *
s0_invoke_method_new(struct s0_name *src, struct s0_name *method,
                     struct s0_name_mapping *params)
{
    struct s0_invocation  *invocation = malloc(sizeof(struct s0_invocation));
    if (unlikely(invocation == NULL)) {
        s0_name_free(src);
        s0_name_free(method);
        s0_name_mapping_free(params);
        s0_set_memory_error();
        return NULL;
    }
    invocation->kind = S0_INVOCATION_KIND_INVOKE_METHOD;
    invocation->_.invoke_method.src = src;
    invocation->_.invoke_method.method = method;
    invocation->_.invoke_method.params = params;
    return invocation;
}

static struct s0_invocation *
s0_invoke_method_new_copy(const struct s0_invocation *other)
{
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_name_mapping  *params;

    src = s0_name_new_copy(other->_.invoke_method.src);
    if (unlikely(src == NULL)) {
        return NULL;
    }

    method = s0_name_new_copy(other->_.invoke_method.method);
    if (unlikely(method == NULL)) {
        s0_name_free(src);
        return NULL;
    }

    params = s0_name_mapping_new_copy(other->_.invoke_method.params);
    if (unlikely(params == NULL)) {
        s0_name_free(src);
        s0_name_free(method);
        return NULL;
    }

    return s0_invoke_method_new(src, method, params);
}

static void
s0_invoke_method_free(struct s0_invocation *invocation)
{
    s0_name_free(invocation->_.invoke_method.src);
    s0_name_free(invocation->_.invoke_method.method);
    s0_name_mapping_free(invocation->_.invoke_method.params);
}

struct s0_name *
s0_invoke_method_src(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_METHOD);
    return invocation->_.invoke_method.src;
}

struct s0_name *
s0_invoke_method_method(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_METHOD);
    return invocation->_.invoke_method.method;
}

struct s0_name_mapping *
s0_invoke_method_params(const struct s0_invocation *invocation)
{
    assert(invocation->kind == S0_INVOCATION_KIND_INVOKE_METHOD);
    return invocation->_.invoke_method.params;
}

static bool
s0_invoke_method_eq(const struct s0_invocation *i1,
                     const struct s0_invocation *i2)
{
    return s0_name_eq(i1->_.invoke_method.src, i2->_.invoke_method.src)
        && s0_name_eq(i1->_.invoke_method.method, i2->_.invoke_method.method)
        && s0_name_mapping_eq(i1->_.invoke_method.params,
                              i2->_.invoke_method.params);
}


struct s0_invocation *
s0_invocation_new_copy(const struct s0_invocation *other)
{
    switch (other->kind) {
        case S0_INVOCATION_KIND_INVOKE_CLOSURE:
            return s0_invoke_closure_new_copy(other);
        case S0_INVOCATION_KIND_INVOKE_METHOD:
            return s0_invoke_method_new_copy(other);
        default:
            assert(false);
            break;
    }
}

void
s0_invocation_free(struct s0_invocation *invocation)
{
    switch (invocation->kind) {
        case S0_INVOCATION_KIND_INVOKE_CLOSURE:
            s0_invoke_closure_free(invocation);
            break;
        case S0_INVOCATION_KIND_INVOKE_METHOD:
            s0_invoke_method_free(invocation);
            break;
        default:
            assert(false);
            break;
    }
    free(invocation);
}

enum s0_invocation_kind
s0_invocation_kind(const struct s0_invocation *invocation)
{
    return invocation->kind;
}

bool
s0_invocation_eq(const struct s0_invocation *i1, const struct s0_invocation *i2)
{
    if (i1->kind != i2->kind) {
        return false;
    }
    switch (i1->kind) {
        case S0_INVOCATION_KIND_INVOKE_CLOSURE:
            return s0_invoke_closure_eq(i1, i2);
        case S0_INVOCATION_KIND_INVOKE_METHOD:
            return s0_invoke_method_eq(i1, i2);
        default:
            assert(false);
            break;
    }
}


/*-----------------------------------------------------------------------------
 * Entities
 */

struct s0_entity *
s0_atom_new(void)
{
    struct s0_entity  *atom = malloc(sizeof(struct s0_entity));
    if (unlikely(atom == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    atom->kind = S0_ENTITY_KIND_ATOM;
    return atom;
}

static void
s0_atom_free(struct s0_entity *atom)
{
    /* Nothing to do */
}

bool
s0_atom_eq(const struct s0_entity *a1, const struct s0_entity *a2)
{
    assert(a1->kind == S0_ENTITY_KIND_ATOM);
    assert(a2->kind == S0_ENTITY_KIND_ATOM);
    return (a1 == a2);
}


struct s0_entity *
s0_closure_new(struct s0_environment *env, struct s0_named_blocks *blocks)
{
    struct s0_entity  *closure = malloc(sizeof(struct s0_entity));
    if (unlikely(closure == NULL)) {
        s0_environment_free(env);
        s0_named_blocks_free(blocks);
        s0_set_memory_error();
        return NULL;
    }
    closure->kind = S0_ENTITY_KIND_CLOSURE;
    closure->_.closure.env = env;
    closure->_.closure.blocks = blocks;
    return closure;
}

static void
s0_closure_free(struct s0_entity *closure)
{
    s0_environment_free(closure->_.closure.env);
    s0_named_blocks_free(closure->_.closure.blocks);
}

struct s0_environment *
s0_closure_environment(const struct s0_entity *closure)
{
    assert(closure->kind == S0_ENTITY_KIND_CLOSURE);
    return closure->_.closure.env;
}

struct s0_named_blocks *
s0_closure_named_blocks(const struct s0_entity *closure)
{
    assert(closure->kind == S0_ENTITY_KIND_CLOSURE);
    return closure->_.closure.blocks;
}


struct s0_entity *
s0_literal_new(size_t size, const void *content)
{
    struct s0_entity  *literal = malloc(sizeof(struct s0_entity));
    if (unlikely(literal == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    literal->kind = S0_ENTITY_KIND_LITERAL;
    literal->_.literal.size = size;
    literal->_.literal.content = malloc(size);
    if (unlikely(literal->_.literal.content == NULL)) {
        free(literal);
        s0_set_memory_error();
        return NULL;
    }
    memcpy((void *) literal->_.literal.content, content, size);
    return literal;
}

struct s0_entity *
s0_literal_new_str(const void *content)
{
    return s0_literal_new(strlen(content), content);
}

static void
s0_literal_free(struct s0_entity *literal)
{
    free((void *) literal->_.literal.content);
}

const char *
s0_literal_content(const struct s0_entity *literal)
{
    assert(literal->kind == S0_ENTITY_KIND_LITERAL);
    return literal->_.literal.content;
}

size_t
s0_literal_size(const struct s0_entity *literal)
{
    assert(literal->kind == S0_ENTITY_KIND_LITERAL);
    return literal->_.literal.size;
}


struct s0_entity *
s0_method_new(struct s0_block *body)
{
    struct s0_entity  *method = malloc(sizeof(struct s0_entity));
    if (unlikely(method == NULL)) {
        s0_block_free(body);
        s0_set_memory_error();
        return NULL;
    }
    method->kind = S0_ENTITY_KIND_METHOD;
    method->_.method.body = body;
    return method;
}

static void
s0_method_free(struct s0_entity *method)
{
    s0_block_free(method->_.method.body);
}

struct s0_block *
s0_method_body(const struct s0_entity *method)
{
    assert(method->kind == S0_ENTITY_KIND_METHOD);
    return method->_.method.body;
}


#define DEFAULT_INITIAL_OBJECT_SIZE  4

struct s0_entity *
s0_object_new(void)
{
    struct s0_entity  *obj = malloc(sizeof(struct s0_entity));
    if (unlikely(obj == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    obj->kind = S0_ENTITY_KIND_OBJECT;
    obj->_.obj.size = 0;
    obj->_.obj.allocated_size = DEFAULT_INITIAL_OBJECT_SIZE;
    obj->_.obj.entries =
        malloc(DEFAULT_INITIAL_OBJECT_SIZE * sizeof(struct s0_object_entry));
    if (unlikely(obj->_.obj.entries == NULL)) {
        free(obj);
        s0_set_memory_error();
        return NULL;
    }
    return obj;
}

static void
s0_object_free(struct s0_entity *obj)
{
    size_t  i;
    for (i = 0; i < obj->_.obj.size; i++) {
        s0_name_free(obj->_.obj.entries[i].name);
        s0_entity_free(obj->_.obj.entries[i].entity);
    }
    free(obj->_.obj.entries);
}

int
s0_object_add(struct s0_entity *obj,
              struct s0_name *name, struct s0_entity *entity)
{
    struct s0_object_entry  *new_entry;

#if !defined(NDEBUG)
    {
        size_t  i;
        for (i = 0; i < obj->_.obj.size; i++) {
            assert(!s0_name_eq(name, obj->_.obj.entries[i].name));
        }
    }
#endif

    if (unlikely(obj->_.obj.size == obj->_.obj.allocated_size)) {
        size_t  new_size = obj->_.obj.allocated_size * 2;
        struct s0_object_entry  *new_entries =
            realloc(obj->_.obj.entries,
                    new_size * sizeof(struct s0_object_entry));
        if (unlikely(new_entries == NULL)) {
            s0_name_free(name);
            s0_entity_free(entity);
            s0_set_memory_error();
            return -1;
        }
        obj->_.obj.entries = new_entries;
        obj->_.obj.allocated_size = new_size;
    }

    new_entry = &obj->_.obj.entries[obj->_.obj.size++];
    new_entry->name = name;
    new_entry->entity = entity;
    return 0;
}

size_t
s0_object_size(const struct s0_entity *obj)
{
    assert(obj->kind == S0_ENTITY_KIND_OBJECT);
    return obj->_.obj.size;
}

struct s0_object_entry
s0_object_at(const struct s0_entity *obj, size_t index)
{
    assert(obj->kind == S0_ENTITY_KIND_OBJECT);
    assert(index < obj->_.obj.size);
    return obj->_.obj.entries[index];
}

struct s0_entity *
s0_object_get(const struct s0_entity *obj, const struct s0_name *name)
{
    size_t  i;
    assert(obj->kind == S0_ENTITY_KIND_OBJECT);
    for (i = 0; i < obj->_.obj.size; i++) {
        if (s0_name_eq(obj->_.obj.entries[i].name, name)) {
            return obj->_.obj.entries[i].entity;
        }
    }
    return NULL;
}


struct s0_entity *
s0_primitive_method_new(struct s0_environment_type *inputs,
                        struct s0_continuation cont,
                        s0_primitive_method_free_f *free_ud)
{
    struct s0_entity  *method = malloc(sizeof(struct s0_entity));
    if (unlikely(method == NULL)) {
        s0_environment_type_free(inputs);
        free_ud(cont.ud);
        s0_set_memory_error();
        return NULL;
    }
    method->kind = S0_ENTITY_KIND_PRIMITIVE_METHOD;
    method->_.primitive_method.inputs = inputs;
    method->_.primitive_method.cont = cont;
    method->_.primitive_method.free_ud = free_ud;
    return method;
}

static void
s0_primitive_method_free(struct s0_entity *method)
{
    s0_environment_type_free(method->_.primitive_method.inputs);
    method->_.primitive_method.free_ud(method->_.primitive_method.cont.ud);
}


void
s0_entity_free(struct s0_entity *entity)
{
    switch (entity->kind) {
        case S0_ENTITY_KIND_ATOM:
            s0_atom_free(entity);
            break;
        case S0_ENTITY_KIND_CLOSURE:
            s0_closure_free(entity);
            break;
        case S0_ENTITY_KIND_LITERAL:
            s0_literal_free(entity);
            break;
        case S0_ENTITY_KIND_METHOD:
            s0_method_free(entity);
            break;
        case S0_ENTITY_KIND_OBJECT:
            s0_object_free(entity);
            break;
        case S0_ENTITY_KIND_PRIMITIVE_METHOD:
            s0_primitive_method_free(entity);
            break;
        default:
            assert(false);
            break;
    }
    free(entity);
}

enum s0_entity_kind
s0_entity_kind(const struct s0_entity *entity)
{
    return entity->kind;
}


/*-----------------------------------------------------------------------------
 * S₀: Entity types
 */

struct s0_entity_type *
s0_any_entity_type_new(void)
{
    struct s0_entity_type  *type = malloc(sizeof(struct s0_entity_type));
    if (unlikely(type == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    type->kind = S0_ENTITY_TYPE_KIND_ANY;
    return type;
}

static struct s0_entity_type *
s0_any_entity_type_new_copy(const struct s0_entity_type *other)
{
    return s0_any_entity_type_new();
}

static void
s0_any_entity_type_free(struct s0_entity_type *type)
{
    /* Nothing to do */
}

static bool
s0_any_entity_type_satisfied_by(const struct s0_entity_type *type,
                                const struct s0_entity *entity)
{
    return true;
}

static bool
s0_any_entity_type_satisfied_by_type(const struct s0_entity_type *requires,
                                     const struct s0_entity_type *have)
{
    return true;
}


struct s0_entity_type *
s0_closure_entity_type_new(struct s0_environment_type_mapping *branches)
{
    struct s0_entity_type  *type = malloc(sizeof(struct s0_entity_type));
    if (unlikely(type == NULL)) {
        s0_environment_type_mapping_free(branches);
        s0_set_memory_error();
        return NULL;
    }
    type->kind = S0_ENTITY_TYPE_KIND_CLOSURE;
    type->_.closure.branches = branches;
    return type;
}

static struct s0_entity_type *
s0_closure_entity_type_new_from_named_blocks(
        const struct s0_named_blocks *blocks)
{
    struct s0_environment_type_mapping  *branches;
    struct s0_named_blocks_entry  *curr;

    branches = s0_environment_type_mapping_new();
    if (unlikely(branches == NULL)) {
        return NULL;
    }

    for (curr = blocks->head; curr != NULL; curr = curr->next) {
        int  rc;
        struct s0_environment_type  *inputs = curr->block->inputs;
        struct s0_name  *name_copy;
        struct s0_environment_type  *branch_type;

        name_copy = s0_name_new_copy(curr->name);
        if (unlikely(name_copy == NULL)) {
            s0_environment_type_mapping_free(branches);
            return NULL;
        }

        branch_type = s0_environment_type_new_copy(inputs);
        if (unlikely(branch_type == NULL)) {
            s0_name_free(name_copy);
            s0_environment_type_mapping_free(branches);
            return NULL;
        }

        rc = s0_environment_type_mapping_add(branches, name_copy, branch_type);
        if (unlikely(rc != 0)) {
            s0_environment_type_mapping_free(branches);
            return NULL;
        }
    }

    return s0_closure_entity_type_new(branches);
}

static struct s0_entity_type *
s0_closure_entity_type_new_from_closure(const struct s0_entity *entity)
{
    assert(entity->kind == S0_ENTITY_KIND_CLOSURE);
    return s0_closure_entity_type_new_from_named_blocks
        (entity->_.closure.blocks);
}

static struct s0_entity_type *
s0_closure_entity_type_new_copy(const struct s0_entity_type *other)
{
    struct s0_environment_type_mapping  *branches_copy =
        s0_environment_type_mapping_new_copy(other->_.closure.branches);
    if (unlikely(branches_copy == NULL)) {
        return NULL;
    }
    return s0_closure_entity_type_new(branches_copy);
}

static void
s0_closure_entity_type_free(struct s0_entity_type *type)
{
    s0_environment_type_mapping_free(type->_.closure.branches);
}

const struct s0_environment_type_mapping *
s0_closure_entity_type_branches(const struct s0_entity_type *type)
{
    return type->_.closure.branches;
}

static bool
s0_closure_entity_type_satisfied_by(const struct s0_entity_type *type,
                                    const struct s0_entity *entity)
{
    size_t  i;
    struct s0_environment_type_mapping  *branches;
    struct s0_named_blocks  *blocks;

    if (entity->kind != S0_ENTITY_KIND_CLOSURE) {
        s0_set_error(S0_ERROR_TYPE_MISMATCH, "Entity is not a closure");
        return false;
    }

    branches = type->_.closure.branches;
    blocks = entity->_.closure.blocks;

    if (branches->size != blocks->size) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Closure has the wrong number of branches: has %zu, needs %zu",
             blocks->size, branches->size);
        return false;
    }

    for (i = 0; i < branches->size; i++) {
        struct s0_name  *name = branches->entries[i].name;
        struct s0_environment_type  *type = branches->entries[i].type;
        struct s0_block  *block;

        block = s0_named_blocks_get(blocks, name);
        if (block == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Closure doesn't have a branch named `%s`",
                 s0_name_human_readable(name));
            return false;
        }

        if (!s0_environment_type_satisfied_by_type(type, block->inputs)) {
            s0_prefix_error
                ("In closure branch `%s`:\n",
                 s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}

static bool
s0_closure_entity_type_satisfied_by_type(const struct s0_entity_type *requires,
                                         const struct s0_entity_type *have)
{
    size_t  i;
    struct s0_environment_type_mapping  *requires_branches;
    struct s0_environment_type_mapping  *have_branches;

    if (have->kind != S0_ENTITY_TYPE_KIND_CLOSURE) {
        s0_set_error(S0_ERROR_TYPE_MISMATCH, "Entity wouldn't be a closure");
        return false;
    }

    requires_branches = requires->_.closure.branches;
    have_branches = have->_.closure.branches;

    if (requires_branches->size != have_branches->size) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Closure would have the wrong number of branches: "
             "would have %zu, would need %zu",
             have_branches->size, requires_branches->size);
        return false;
    }

    for (i = 0; i < requires_branches->size; i++) {
        struct s0_name  *name = requires_branches->entries[i].name;
        struct s0_environment_type  *requires_type =
            requires_branches->entries[i].type;
        struct s0_environment_type  *have_type;

        have_type = s0_environment_type_mapping_get(have_branches, name);
        if (have_type == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Closure wouldn't have a branch named `%s`",
                 s0_name_human_readable(name));
            return false;
        }

        if (!s0_environment_type_satisfied_by_type(requires_type, have_type)) {
            s0_prefix_error
                ("In closure branch `%s`:\n", s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}


struct s0_entity_type *
s0_method_entity_type_new(struct s0_environment_type *body)
{
    struct s0_entity_type  *type = malloc(sizeof(struct s0_entity_type));
    if (unlikely(type == NULL)) {
        s0_environment_type_free(body);
        s0_set_memory_error();
        return NULL;
    }
    type->kind = S0_ENTITY_TYPE_KIND_METHOD;
    type->_.method.body = body;
    return type;
}

static struct s0_entity_type *
s0_method_entity_type_new_from_block(const struct s0_block *block)
{
    struct s0_environment_type  *inputs = block->inputs;
    struct s0_environment_type  *body;
    body = s0_environment_type_new_copy(inputs);
    if (unlikely(body == NULL)) {
        return NULL;
    }
    return s0_method_entity_type_new(body);
}

static struct s0_entity_type *
s0_method_entity_type_new_from_method(const struct s0_entity *entity)
{
    assert(entity->kind == S0_ENTITY_KIND_METHOD);
    return s0_method_entity_type_new_from_block(entity->_.method.body);
}

static struct s0_entity_type *
s0_method_entity_type_new_copy(const struct s0_entity_type *other)
{
    struct s0_environment_type  *body_copy =
        s0_environment_type_new_copy(other->_.method.body);
    if (unlikely(body_copy == NULL)) {
        return NULL;
    }
    return s0_method_entity_type_new(body_copy);
}

static void
s0_method_entity_type_free(struct s0_entity_type *type)
{
    s0_environment_type_free(type->_.method.body);
}

const struct s0_environment_type *
s0_method_entity_type_body(const struct s0_entity_type *type)
{
    return type->_.method.body;
}

static bool
s0_method_entity_type_satisfied_by(const struct s0_entity_type *type,
                                   const struct s0_entity *entity)
{
    struct s0_environment_type  *body_type;

    body_type = type->_.method.body;
    if (entity->kind == S0_ENTITY_KIND_METHOD) {
        struct s0_block  *body = entity->_.method.body;
        bool  result =
            s0_environment_type_satisfied_by_type(body_type, body->inputs);
        if (!result) {
            s0_prefix_error("In method body:\n");
        }
        return result;
    } else if (entity->kind == S0_ENTITY_KIND_PRIMITIVE_METHOD) {
        struct s0_environment_type  *inputs = entity->_.primitive_method.inputs;
        bool  result =
            s0_environment_type_satisfied_by_type(body_type, inputs);
        if (!result) {
            s0_prefix_error("In primitive method body:\n");
        }
        return result;
    } else {
        return false;
    }
}

static bool
s0_method_entity_type_satisfied_by_type(const struct s0_entity_type *requires,
                                        const struct s0_entity_type *have)
{
    struct s0_environment_type  *requires_body;
    struct s0_environment_type  *have_body;
    bool  result;
    if (have->kind != S0_ENTITY_TYPE_KIND_METHOD) {
        return false;
    }
    requires_body = requires->_.method.body;
    have_body = have->_.method.body;
    result = s0_environment_type_satisfied_by_type(requires_body, have_body);
    if (!result) {
        s0_prefix_error("In method body:\n");
    }
    return result;
}


struct s0_entity_type *
s0_object_entity_type_new(struct s0_environment_type *elements)
{
    struct s0_entity_type  *type = malloc(sizeof(struct s0_entity_type));
    if (unlikely(type == NULL)) {
        s0_environment_type_free(elements);
        s0_set_memory_error();
        return NULL;
    }
    type->kind = S0_ENTITY_TYPE_KIND_OBJECT;
    type->_.object.elements = elements;
    return type;
}

static struct s0_entity_type *
s0_object_entity_type_new_from_object(const struct s0_entity *entity)
{
    size_t  i;
    struct s0_environment_type  *elements;

    assert(entity->kind == S0_ENTITY_KIND_OBJECT);

    elements = s0_environment_type_new();
    if (unlikely(elements == NULL)) {
        return NULL;
    }

    for (i = 0; i < entity->_.obj.size; i++) {
        int  rc;
        struct s0_name  *name_copy;
        struct s0_entity_type  *element_type;

        name_copy = s0_name_new_copy(entity->_.obj.entries[i].name);
        if (unlikely(name_copy == NULL)) {
            s0_environment_type_free(elements);
            return NULL;
        }

        element_type = s0_entity_type_new_from_entity
            (entity->_.obj.entries[i].entity);
        if (unlikely(element_type == NULL)) {
            s0_name_free(name_copy);
            s0_environment_type_free(elements);
            return NULL;
        }

        rc = s0_environment_type_add(elements, name_copy, element_type);
        if (unlikely(rc != 0)) {
            s0_environment_type_free(elements);
            return NULL;
        }
    }

    return s0_object_entity_type_new(elements);
}

static struct s0_entity_type *
s0_object_entity_type_new_copy(const struct s0_entity_type *other)
{
    struct s0_environment_type  *elements_copy =
        s0_environment_type_new_copy(other->_.object.elements);
    if (unlikely(elements_copy == NULL)) {
        return NULL;
    }
    return s0_object_entity_type_new(elements_copy);
}

static void
s0_object_entity_type_free(struct s0_entity_type *type)
{
    s0_environment_type_free(type->_.object.elements);
}

const struct s0_environment_type *
s0_object_entity_type_elements(const struct s0_entity_type *type)
{
    return type->_.object.elements;
}

static bool
s0_object_entity_type_satisfied_by(const struct s0_entity_type *type,
                                   const struct s0_entity *entity)
{
    /* We can't use s0_environment_type_satisfied_by here, because the object is
     * allowed to have more entries than are required by the object type, which
     * isn't true of environments. */

    size_t  i;
    struct s0_environment_type  *elements;

    if (entity->kind != S0_ENTITY_KIND_OBJECT) {
        s0_set_error(S0_ERROR_TYPE_MISMATCH, "Entity is not an object");
        return false;
    }

    elements = type->_.object.elements;
    for (i = 0; i < elements->size; i++) {
        const struct s0_name  *name = elements->entries[i].name;
        const struct s0_entity_type  *etype = elements->entries[i].type;
        struct s0_entity  *element = s0_object_get(entity, name);
        if (element == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Object doesn't have an element named `%s`",
                 s0_name_human_readable(name));
            return false;
        }
        if (!s0_entity_type_satisfied_by(etype, element)) {
            s0_prefix_error
                ("In object element `%s`:\n", s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}

static bool
s0_object_entity_type_satisfied_by_type(const struct s0_entity_type *requires,
                                        const struct s0_entity_type *have)
{
    /* We can't use s0_environment_type_satisfied_by_type here, because the
     * `have` type is allowed to have more entries than the `required` type,
     * which isn't true of environments. */

    size_t  i;
    struct s0_environment_type  *requires_elements;
    struct s0_environment_type  *have_elements;

    if (have->kind != S0_ENTITY_TYPE_KIND_OBJECT) {
        s0_set_error(S0_ERROR_TYPE_MISMATCH, "Entity wouldn't be an object");
        return false;
    }

    requires_elements = requires->_.object.elements;
    have_elements = have->_.object.elements;
    for (i = 0; i < requires_elements->size; i++) {
        const struct s0_name  *name = requires_elements->entries[i].name;
        const struct s0_entity_type  *requires_type =
            requires_elements->entries[i].type;
        const struct s0_entity_type  *have_type =
            s0_environment_type_get(have_elements, name);
        if (have_type == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Object would't have an element named `%s`",
                 s0_name_human_readable(name));
            return false;
        }
        if (!s0_entity_type_satisfied_by_type(requires_type, have_type)) {
            s0_prefix_error
                ("In object element `%s`:\n", s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}


struct s0_entity_type *
s0_entity_type_new_copy(const struct s0_entity_type *other)
{
    switch (other->kind) {
        case S0_ENTITY_TYPE_KIND_ANY:
            return s0_any_entity_type_new_copy(other);
            break;
        case S0_ENTITY_TYPE_KIND_CLOSURE:
            return s0_closure_entity_type_new_copy(other);
            break;
        case S0_ENTITY_TYPE_KIND_METHOD:
            return s0_method_entity_type_new_copy(other);
            break;
        case S0_ENTITY_TYPE_KIND_OBJECT:
            return s0_object_entity_type_new_copy(other);
            break;
        default:
            assert(false);
            break;
    }
}

struct s0_entity_type *
s0_entity_type_new_from_entity(const struct s0_entity *entity)
{
    switch (entity->kind) {
        case S0_ENTITY_KIND_ATOM:
            return s0_any_entity_type_new();
        case S0_ENTITY_KIND_CLOSURE:
            return s0_closure_entity_type_new_from_closure(entity);
        case S0_ENTITY_KIND_LITERAL:
            return s0_any_entity_type_new();
        case S0_ENTITY_KIND_METHOD:
            return s0_method_entity_type_new_from_method(entity);
        case S0_ENTITY_KIND_OBJECT:
            return s0_object_entity_type_new_from_object(entity);
        default:
            assert(false);
            break;
    }
}

void
s0_entity_type_free(struct s0_entity_type *type)
{
    switch (type->kind) {
        case S0_ENTITY_TYPE_KIND_ANY:
            s0_any_entity_type_free(type);
            break;
        case S0_ENTITY_TYPE_KIND_CLOSURE:
            s0_closure_entity_type_free(type);
            break;
        case S0_ENTITY_TYPE_KIND_METHOD:
            s0_method_entity_type_free(type);
            break;
        case S0_ENTITY_TYPE_KIND_OBJECT:
            s0_object_entity_type_free(type);
            break;
        default:
            assert(false);
            break;
    }
    free(type);
}

enum s0_entity_type_kind
s0_entity_type_kind(const struct s0_entity_type *type)
{
    return type->kind;
}

bool
s0_entity_type_satisfied_by(const struct s0_entity_type *type,
                            const struct s0_entity *entity)
{
    switch (type->kind) {
        case S0_ENTITY_TYPE_KIND_ANY:
            return s0_any_entity_type_satisfied_by(type, entity);
            break;
        case S0_ENTITY_TYPE_KIND_CLOSURE:
            return s0_closure_entity_type_satisfied_by(type, entity);
            break;
        case S0_ENTITY_TYPE_KIND_METHOD:
            return s0_method_entity_type_satisfied_by(type, entity);
            break;
        case S0_ENTITY_TYPE_KIND_OBJECT:
            return s0_object_entity_type_satisfied_by(type, entity);
            break;
        default:
            assert(false);
            break;
    }
}

bool
s0_entity_type_satisfied_by_type(const struct s0_entity_type *requires,
                                 const struct s0_entity_type *have)
{
    switch (requires->kind) {
        case S0_ENTITY_TYPE_KIND_ANY:
            return s0_any_entity_type_satisfied_by_type(requires, have);
            break;
        case S0_ENTITY_TYPE_KIND_CLOSURE:
            return s0_closure_entity_type_satisfied_by_type(requires, have);
            break;
        case S0_ENTITY_TYPE_KIND_METHOD:
            return s0_method_entity_type_satisfied_by_type(requires, have);
            break;
        case S0_ENTITY_TYPE_KIND_OBJECT:
            return s0_object_entity_type_satisfied_by_type(requires, have);
            break;
        default:
            assert(false);
            break;
    }
}

bool
s0_entity_type_equiv(const struct s0_entity_type *type1,
                     const struct s0_entity_type *type2)
{
    return s0_entity_type_satisfied_by_type(type1, type2)
        && s0_entity_type_satisfied_by_type(type2, type1);
}


/*-----------------------------------------------------------------------------
 * S₀: Environment types
 */

#define DEFAULT_INITIAL_ENVIRONMENT_TYPE_SIZE  4

struct s0_environment_type *
s0_environment_type_new(void)
{
    struct s0_environment_type  *type =
        malloc(sizeof(struct s0_environment_type));
    if (unlikely(type == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    type->size = 0;
    type->allocated_size = DEFAULT_INITIAL_ENVIRONMENT_TYPE_SIZE;
    type->entries =
        malloc(DEFAULT_INITIAL_ENVIRONMENT_TYPE_SIZE *
               sizeof(struct s0_environment_type_entry));
    if (unlikely(type->entries == NULL)) {
        free(type);
        s0_set_memory_error();
        return NULL;
    }
    return type;
}

struct s0_environment_type *
s0_environment_type_new_copy(const struct s0_environment_type *other)
{
    size_t  i;
    struct s0_environment_type  *type = s0_environment_type_new();
    if (unlikely(type == NULL)) {
        return NULL;
    }

    for (i = 0; i < other->size; i++) {
        int  rc;
        struct s0_name  *name_copy;
        struct s0_entity_type  *type_copy;

        name_copy = s0_name_new_copy(other->entries[i].name);
        if (unlikely(name_copy == NULL)) {
            s0_environment_type_free(type);
            return NULL;
        }

        type_copy = s0_entity_type_new_copy(other->entries[i].type);
        if (unlikely(type_copy == NULL)) {
            s0_name_free(name_copy);
            s0_environment_type_free(type);
            return NULL;
        }

        rc = s0_environment_type_add(type, name_copy, type_copy);
        if (unlikely(rc != 0)) {
            s0_environment_type_free(type);
            return NULL;
        }
    }
    return type;
}

void
s0_environment_type_free(struct s0_environment_type *type)
{
    size_t  i;
    for (i = 0; i < type->size; i++) {
        s0_name_free(type->entries[i].name);
        s0_entity_type_free(type->entries[i].type);
    }
    free(type->entries);
    free(type);
}

int
s0_environment_type_add(struct s0_environment_type *type,
                        struct s0_name *name, struct s0_entity_type *etype)
{
    struct s0_environment_type_entry  *new_entry;

#if !defined(NDEBUG)
    {
        size_t  i;
        for (i = 0; i < type->size; i++) {
            assert(!s0_name_eq(name, type->entries[i].name));
        }
    }
#endif

    if (unlikely(type->size == type->allocated_size)) {
        size_t  new_size = type->allocated_size * 2;
        struct s0_environment_type_entry  *new_entries =
            realloc(type->entries,
                    new_size * sizeof(struct s0_environment_type_entry));
        if (unlikely(new_entries == NULL)) {
            s0_name_free(name);
            s0_entity_type_free(etype);
            s0_set_memory_error();
            return -1;
        }
        type->entries = new_entries;
        type->allocated_size = new_size;
    }

    new_entry = &type->entries[type->size++];
    new_entry->name = name;
    new_entry->type = etype;
    return 0;
}

size_t
s0_environment_type_size(const struct s0_environment_type *type)
{
    return type->size;
}

struct s0_environment_type_entry
s0_environment_type_at(const struct s0_environment_type *type, size_t index)
{
    assert(index < type->size);
    return type->entries[index];
}

struct s0_entity_type *
s0_environment_type_get(const struct s0_environment_type *type,
                        const struct s0_name *name)
{
    size_t  i;
    for (i = 0; i < type->size; i++) {
        if (s0_name_eq(type->entries[i].name, name)) {
            return type->entries[i].type;
        }
    }
    return NULL;
}

struct s0_entity_type *
s0_environment_type_delete(struct s0_environment_type *type,
                           const struct s0_name *name)
{
    size_t  i;
    for (i = 0; i < type->size; i++) {
        if (s0_name_eq(type->entries[i].name, name)) {
            struct s0_entity_type  *result = type->entries[i].type;
            s0_name_free(type->entries[i].name);
            memmove(&type->entries[i], &type->entries[i + 1],
                    sizeof(type->entries[i]) * (type->size - i - 1));
            type->size--;
            return result;
        }
    }
    return NULL;
}

int
s0_environment_type_extend(struct s0_environment_type *dest,
                           const struct s0_environment_type *src)
{
    size_t  i;

#if !defined(NDEBUG)
    for (i = 0; i < src->size; i++) {
        const struct s0_name  *name = src->entries[i].name;
        assert(s0_environment_type_get(dest, name) == NULL);
    }
#endif

    for (i = 0; i < src->size; i++) {
        int  rc;
        struct s0_name  *name_copy;
        struct s0_entity_type  *etype_copy;

        name_copy = s0_name_new_copy(src->entries[i].name);
        if (unlikely(name_copy == NULL)) {
            return -1;
        }

        etype_copy = s0_entity_type_new_copy(src->entries[i].type);
        if (unlikely(etype_copy == NULL)) {
            s0_name_free(name_copy);
            return -1;
        }

        rc = s0_environment_type_add(dest, name_copy, etype_copy);
        if (unlikely(rc != 0)) {
            return -1;
        }
    }
    return 0;
}

int
s0_environment_type_extract(struct s0_environment_type *dest,
                            struct s0_environment_type *src,
                            const struct s0_name_set *set)
{
    size_t  i;

#if !defined(NDEBUG)
    for (i = 0; i < set->size; i++) {
        const struct s0_name  *name = set->names[i];
        assert(s0_environment_type_get(src, name) != NULL);
        assert(s0_environment_type_get(dest, name) == NULL);
    }
#endif

    for (i = 0; i < set->size; i++) {
        const struct s0_name  *name = set->names[i];
        struct s0_entity_type  *etype;

        etype = s0_environment_type_delete(src, name);
        if (etype == NULL) {
            s0_set_error
                (S0_ERROR_UNDEFINED,
                 "Environment wouldn't have an entry named `%s`",
                 s0_name_human_readable(name));
            return -1;
        } else {
            int  rc;
            struct s0_name  *name_copy = s0_name_new_copy(name);
            if (unlikely(name_copy == NULL)) {
                s0_entity_type_free(etype);
                return -1;
            }

            rc = s0_environment_type_add(dest, name_copy, etype);
            if (unlikely(rc != 0)) {
                return -1;
            }
        }
    }
    return 0;
}

int
s0_environment_type_rename(struct s0_environment_type *type,
                           const struct s0_name_mapping *mapping)
{
    size_t  i;

    if (unlikely(type->size != mapping->size)) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Parameter count (%zu) doesn't match size of environment (%zu)",
             mapping->size, type->size);
        return -1;
    }

    for (i = 0; i < type->size; i++) {
        const struct s0_name  *from = type->entries[i].name;
        const struct s0_name  *to;
        struct s0_name  *to_copy;

        to = s0_name_mapping_get(mapping, from);
        if (unlikely(to == NULL)) {
            s0_set_error
                (S0_ERROR_UNDEFINED,
                 "Don't have a parameter for environment entry `%s`",
                 s0_name_human_readable(from));
            return -1;
        }

        to_copy = s0_name_new_copy(to);
        if (unlikely(to_copy == NULL)) {
            return -1;
        }

        s0_name_free(type->entries[i].name);
        type->entries[i].name = to_copy;
    }

    return 0;
}

static int
s0_environment_type_add_create_atom(struct s0_environment_type *type,
                                    const struct s0_statement *stmt)
{
    struct s0_entity_type  *dest_type;
    struct s0_name  *dest_name;

    dest_type = s0_environment_type_get(type, stmt->_.create_atom.dest);
    if (unlikely(dest_type != NULL)) {
        s0_set_error
            (S0_ERROR_REDEFINED,
             "Environment would already have an entry named `%s`",
             s0_name_human_readable(stmt->_.create_atom.dest));
        return -1;
    }

    dest_name = s0_name_new_copy(stmt->_.create_atom.dest);
    if (unlikely(dest_name == NULL)) {
        return -1;
    }

    dest_type = s0_any_entity_type_new();
    if (unlikely(dest_type == NULL)) {
        s0_name_free(dest_name);
        return -1;
    }

    return s0_environment_type_add(type, dest_name, dest_type);
}

static int
s0_environment_type_add_create_closure(struct s0_environment_type *type,
                                       const struct s0_statement *stmt)
{
    struct s0_entity_type  *dest_type;
    struct s0_name  *dest_name;

    dest_type = s0_environment_type_get(type, stmt->_.create_closure.dest);
    if (unlikely(dest_type != NULL)) {
        s0_set_error
            (S0_ERROR_REDEFINED,
             "Environment would already have an entry named `%s`",
             s0_name_human_readable(stmt->_.create_closure.dest));
        return -1;
    }

    dest_name = s0_name_new_copy(stmt->_.create_closure.dest);
    if (unlikely(dest_name == NULL)) {
        return -1;
    }

    dest_type = s0_closure_entity_type_new_from_named_blocks
        (stmt->_.create_closure.branches);
    if (unlikely(dest_type == NULL)) {
        s0_name_free(dest_name);
        return -1;
    }

    return s0_environment_type_add(type, dest_name, dest_type);
}

static int
s0_environment_type_add_create_literal(struct s0_environment_type *type,
                                       const struct s0_statement *stmt)
{
    struct s0_entity_type  *dest_type;
    struct s0_name  *dest_name;

    dest_type = s0_environment_type_get(type, stmt->_.create_literal.dest);
    if (unlikely(dest_type != NULL)) {
        s0_set_error
            (S0_ERROR_REDEFINED,
             "Environment would already have an entry named `%s`",
             s0_name_human_readable(stmt->_.create_literal.dest));
        return -1;
    }

    dest_name = s0_name_new_copy(stmt->_.create_literal.dest);
    if (unlikely(dest_name == NULL)) {
        return -1;
    }

    dest_type = s0_any_entity_type_new();
    if (unlikely(dest_type == NULL)) {
        s0_name_free(dest_name);
        return -1;
    }

    return s0_environment_type_add(type, dest_name, dest_type);
}

static int
s0_environment_type_add_create_method(struct s0_environment_type *type,
                                      const struct s0_statement *stmt)
{
    struct s0_entity_type  *dest_type;
    struct s0_name  *dest_name;

    dest_type = s0_environment_type_get(type, stmt->_.create_method.dest);
    if (unlikely(dest_type != NULL)) {
        s0_set_error
            (S0_ERROR_REDEFINED,
             "Environment would already have an entry named `%s`",
             s0_name_human_readable(stmt->_.create_method.dest));
        return -1;
    }

    dest_name = s0_name_new_copy(stmt->_.create_method.dest);
    if (unlikely(dest_name == NULL)) {
        return -1;
    }

    dest_type = s0_any_entity_type_new();
    if (unlikely(dest_type == NULL)) {
        s0_name_free(dest_name);
        return -1;
    }

    return s0_environment_type_add(type, dest_name, dest_type);
}

int
s0_environment_type_add_statement(struct s0_environment_type *type,
                                  const struct s0_statement *stmt)
{
    switch (stmt->kind) {
        case S0_STATEMENT_KIND_CREATE_ATOM:
            return s0_environment_type_add_create_atom(type, stmt);
        case S0_STATEMENT_KIND_CREATE_CLOSURE:
            return s0_environment_type_add_create_closure(type, stmt);
        case S0_STATEMENT_KIND_CREATE_LITERAL:
            return s0_environment_type_add_create_literal(type, stmt);
        case S0_STATEMENT_KIND_CREATE_METHOD:
            return s0_environment_type_add_create_method(type, stmt);
        default:
            assert(false);
            break;
    }
}

static int
s0_environment_type_add_invoke_closure(struct s0_environment_type *type,
                                       const struct s0_invocation *invocation)
{
    struct s0_entity_type  *src_type;
    struct s0_environment_type  *renamed_type;
    struct s0_environment_type  *branch_inputs;
    struct s0_name_mapping  *params;

    src_type = s0_environment_type_delete
        (type, invocation->_.invoke_closure.src);
    if (unlikely(src_type == NULL)) {
        s0_set_error
            (S0_ERROR_UNDEFINED,
             "Environment wouldn't have an entry named `%s`",
             s0_name_human_readable(invocation->_.invoke_closure.src));
        return -1;
    }
    if (unlikely(src_type->kind != S0_ENTITY_TYPE_KIND_CLOSURE)) {
        s0_entity_type_free(src_type);
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Entry `%s` wouldn't be a closure",
             s0_name_human_readable(invocation->_.invoke_closure.src));
        return -1;
    }

    branch_inputs = s0_environment_type_mapping_get
        (src_type->_.closure.branches, invocation->_.invoke_closure.branch);
    if (unlikely(branch_inputs == NULL)) {
        s0_entity_type_free(src_type);
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Closure `%s` wouldn't have a branch named `%s`",
             s0_name_human_readable(invocation->_.invoke_closure.src),
             s0_name_human_readable(invocation->_.invoke_closure.branch));
        return -1;
    }

    renamed_type = s0_environment_type_new_copy(type);
    if (unlikely(renamed_type == NULL)) {
        s0_entity_type_free(src_type);
        return -1;
    }

    params = invocation->_.invoke_closure.params;
    if (unlikely(s0_environment_type_rename(renamed_type, params) != 0)) {
        s0_entity_type_free(src_type);
        s0_environment_type_free(renamed_type);
        s0_prefix_error
            ("When invoking closure `%s`:\n",
             s0_name_human_readable(invocation->_.invoke_closure.src));
        return -1;
    }

    if (unlikely(!s0_environment_type_satisfied_by_type(
                    branch_inputs, renamed_type))) {
        s0_entity_type_free(src_type);
        s0_environment_type_free(renamed_type);
        s0_prefix_error
            ("When invoking closure `%s`:\n",
             s0_name_human_readable(invocation->_.invoke_closure.src));
        return -1;
    }

    s0_entity_type_free(src_type);
    s0_environment_type_free(renamed_type);
    return 0;
}

static int
s0_environment_type_add_invoke_method(struct s0_environment_type *type,
                                      const struct s0_invocation *invocation)
{
    struct s0_entity_type  *src_type;
    const struct s0_environment_type  *src_elements;
    struct s0_entity_type  *method_type;
    struct s0_environment_type  *renamed_type;
    struct s0_name_mapping  *params;
    struct s0_environment_type  *method_inputs;

    src_type = s0_environment_type_get(type, invocation->_.invoke_method.src);
    if (unlikely(src_type == NULL)) {
        s0_set_error
            (S0_ERROR_UNDEFINED,
             "Environment wouldn't have an entry named `%s`",
             s0_name_human_readable(invocation->_.invoke_method.src));
        return -1;
    }
    if (unlikely(src_type->kind != S0_ENTITY_TYPE_KIND_OBJECT)) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Entry `%s` wouldn't be an object",
             s0_name_human_readable(invocation->_.invoke_method.src));
        return -1;
    }

    src_elements = s0_object_entity_type_elements(src_type);
    method_type = s0_environment_type_get
        (src_elements, invocation->_.invoke_method.method);
    if (unlikely(method_type == NULL)) {
        s0_set_error
            (S0_ERROR_UNDEFINED,
             "Object `%s` wouldn't have an element named `%s`",
             s0_name_human_readable(invocation->_.invoke_method.src),
             s0_name_human_readable(invocation->_.invoke_method.method));
        return -1;
    }
    if (method_type->kind != S0_ENTITY_TYPE_KIND_METHOD) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Element `%s` in object `%s` wouldn't be an object",
             s0_name_human_readable(invocation->_.invoke_method.method),
             s0_name_human_readable(invocation->_.invoke_method.src));
        return -1;
    }

    renamed_type = s0_environment_type_new_copy(type);
    if (unlikely(renamed_type == NULL)) {
        return -1;
    }

    params = invocation->_.invoke_method.params;
    if (unlikely(s0_environment_type_rename(renamed_type, params) != 0)) {
        s0_environment_type_free(renamed_type);
        s0_prefix_error
            ("When invoking method `%s::%s`:\n",
             s0_name_human_readable(invocation->_.invoke_method.src),
             s0_name_human_readable(invocation->_.invoke_method.method));
        return -1;
    }

    method_inputs = method_type->_.method.body;
    if (unlikely(!s0_environment_type_satisfied_by_type(
                    method_inputs, renamed_type))) {
        s0_environment_type_free(renamed_type);
        s0_prefix_error
            ("When invoking method `%s::%s`:\n",
             s0_name_human_readable(invocation->_.invoke_method.src),
             s0_name_human_readable(invocation->_.invoke_method.method));
        return -1;
    }

    s0_environment_type_free(renamed_type);
    return 0;
}

int
s0_environment_type_add_invocation(struct s0_environment_type *type,
                                   const struct s0_invocation *invocation)
{
    switch (invocation->kind) {
        case S0_INVOCATION_KIND_INVOKE_CLOSURE:
            return s0_environment_type_add_invoke_closure(type, invocation);
        case S0_INVOCATION_KIND_INVOKE_METHOD:
            return s0_environment_type_add_invoke_method(type, invocation);
        default:
            assert(false);
            break;
    }
}

bool
s0_environment_type_satisfied_by(const struct s0_environment_type *type,
                                 const struct s0_environment *env)
{
    size_t  i;

    if (type->size != env->size) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Environment has the wrong number of entries: has %zu, needs %zu",
             env->size, type->size);
        return false;
    }

    for (i = 0; i < type->size; i++) {
        const struct s0_name  *name = type->entries[i].name;
        const struct s0_entity_type  *etype = type->entries[i].type;
        struct s0_entity  *element = s0_environment_get(env, name);
        if (element == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Environment doesn't have an entry named `%s`",
                 s0_name_human_readable(name));
            return false;
        }
        if (!s0_entity_type_satisfied_by(etype, element)) {
            s0_prefix_error
                ("In environment entry `%s`:\n",
                 s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}

bool
s0_environment_type_satisfied_by_type(
        const struct s0_environment_type *requires,
        const struct s0_environment_type *have)
{
    size_t  i;

    if (requires->size != have->size) {
        s0_set_error
            (S0_ERROR_TYPE_MISMATCH,
             "Environment would have the wrong number of entries: "
             "would have %zu, would need %zu",
             have->size, requires->size);
        return false;
    }

    for (i = 0; i < requires->size; i++) {
        const struct s0_name  *name = requires->entries[i].name;
        const struct s0_entity_type  *rtype = requires->entries[i].type;
        const struct s0_entity_type  *htype =
            s0_environment_type_get(have, name);
        if (htype == NULL) {
            s0_set_error
                (S0_ERROR_TYPE_MISMATCH,
                 "Environment wouldn't have an entry named `%s`",
                 s0_name_human_readable(name));
            return false;
        }
        if (!s0_entity_type_satisfied_by_type(rtype, htype)) {
            s0_prefix_error
                ("In environment entry `%s`:\n",
                 s0_name_human_readable(name));
            return false;
        }
    }

    return true;
}

bool
s0_environment_type_equiv(const struct s0_environment_type *type1,
                          const struct s0_environment_type *type2)
{
    return s0_environment_type_satisfied_by_type(type1, type2)
        && s0_environment_type_satisfied_by_type(type2, type1);
}


/*-----------------------------------------------------------------------------
 * Environment type mappings
 */

#define DEFAULT_INITIAL_ENVIRONMENT_TYPE_MAPPING_SIZE  4

struct s0_environment_type_mapping *
s0_environment_type_mapping_new(void)
{
    struct s0_environment_type_mapping  *mapping =
        malloc(sizeof(struct s0_environment_type_mapping));
    if (unlikely(mapping == NULL)) {
        s0_set_memory_error();
        return NULL;
    }
    mapping->size = 0;
    mapping->allocated_size = DEFAULT_INITIAL_ENVIRONMENT_TYPE_MAPPING_SIZE;
    mapping->entries =
        malloc(DEFAULT_INITIAL_ENVIRONMENT_TYPE_MAPPING_SIZE *
               sizeof(struct s0_environment_type_mapping_entry));
    if (unlikely(mapping->entries == NULL)) {
        free(mapping);
        s0_set_memory_error();
        return NULL;
    }
    return mapping;
}

struct s0_environment_type_mapping *
s0_environment_type_mapping_new_copy(
        const struct s0_environment_type_mapping *other)
{
    size_t  i;
    struct s0_environment_type_mapping  *mapping =
        s0_environment_type_mapping_new();
    if (unlikely(mapping == NULL)) {
        return NULL;
    }

    for (i = 0; i < other->size; i++) {
        int  rc;
        struct s0_name  *name_copy;
        struct s0_environment_type  *type_copy;

        name_copy = s0_name_new_copy(other->entries[i].name);
        if (unlikely(name_copy == NULL)) {
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }

        type_copy = s0_environment_type_new_copy(other->entries[i].type);
        if (unlikely(type_copy == NULL)) {
            s0_name_free(name_copy);
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }

        rc = s0_environment_type_mapping_add(mapping, name_copy, type_copy);
        if (unlikely(rc != 0)) {
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }
    }
    return mapping;
}

void
s0_environment_type_mapping_free(struct s0_environment_type_mapping *mapping)
{
    size_t  i;
    for (i = 0; i < mapping->size; i++) {
        s0_name_free(mapping->entries[i].name);
        s0_environment_type_free(mapping->entries[i].type);
    }
    free(mapping->entries);
    free(mapping);
}

int
s0_environment_type_mapping_add(struct s0_environment_type_mapping *mapping,
                                struct s0_name *name,
                                struct s0_environment_type *type)
{
    struct s0_environment_type_mapping_entry  *new_entry;

    if (unlikely(mapping->size == mapping->allocated_size)) {
        size_t  new_size = mapping->allocated_size * 2;
        struct s0_environment_type_mapping_entry  *new_entries = realloc
            (mapping->entries,
             new_size * sizeof(struct s0_environment_type_mapping_entry));
        if (unlikely(new_entries == NULL)) {
            s0_name_free(name);
            s0_environment_type_free(type);
            s0_set_memory_error();
            return -1;
        }
        mapping->entries = new_entries;
        mapping->allocated_size = new_size;
    }

    new_entry = &mapping->entries[mapping->size++];
    new_entry->name = name;
    new_entry->type = type;
    return 0;
}

size_t
s0_environment_type_mapping_size(
        const struct s0_environment_type_mapping *mapping)
{
    return mapping->size;
}

const struct s0_environment_type_mapping_entry *
s0_environment_type_mapping_at(
        const struct s0_environment_type_mapping *mapping, size_t index)
{
    assert(index < mapping->size);
    return &mapping->entries[index];
}

struct s0_environment_type *
s0_environment_type_mapping_get(
        const struct s0_environment_type_mapping *mapping,
        const struct s0_name *name)
{
    size_t  i;
    for (i = 0; i < mapping->size; i++) {
        if (s0_name_eq(mapping->entries[i].name, name)) {
            return mapping->entries[i].type;
        }
    }
    return NULL;
}


/*-----------------------------------------------------------------------------
 * S₀: Execution
 */

/* This will never get called; down below in s0_continuation_execute, we look
 * for this function as a special case to abort the trampoline loop. */
static struct s0_continuation
s0_execute_error_continuation(void *ud, struct s0_environment *env)
{
    assert(false);
}

static const struct s0_continuation  s0_error_continuation_ = {
    NULL,
    s0_execute_error_continuation
};

struct s0_continuation
s0_error_continuation(void)
{
    return s0_error_continuation_;
}

/* This will never get called; down below in s0_continuation_execute, we look
 * for this function as a special case to abort the trampoline loop. */
static struct s0_continuation
s0_execute_finish_continuation(void *ud, struct s0_environment *env)
{
    assert(false);
}

static const struct s0_continuation  s0_finish_continuation_ = {
    NULL,
    s0_execute_finish_continuation
};

struct s0_continuation
s0_finish_continuation(void)
{
    return s0_finish_continuation_;
}


static int
s0_create_atom_execute(struct s0_statement *stmt, struct s0_environment *env)
{
    struct s0_name  *dest;
    struct s0_entity  *atom;

    assert(s0_environment_get(env, stmt->_.create_atom.dest) == NULL);

    dest = s0_name_new_copy(stmt->_.create_atom.dest);
    if (unlikely(dest == NULL)) {
        return -1;
    }

    atom = s0_atom_new();
    if (unlikely(atom == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    return s0_environment_add(env, dest, atom);
}

static int
s0_create_closure_execute(struct s0_statement *stmt, struct s0_environment *env)
{
    int  rc;
    struct s0_name  *dest;
    struct s0_environment  *closure_set;
    struct s0_named_blocks  *blocks;
    struct s0_entity  *closure;

    assert(s0_environment_get(env, stmt->_.create_closure.dest) == NULL);

    dest = s0_name_new_copy(stmt->_.create_atom.dest);
    if (unlikely(dest == NULL)) {
        return -1;
    }

    closure_set = s0_environment_new();
    if (unlikely(closure_set == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    rc = s0_environment_extract
        (closure_set, env, stmt->_.create_closure.closed_over);
    if (unlikely(rc != 0)) {
        s0_name_free(dest);
        s0_environment_free(closure_set);
        return rc;
    }

    blocks = s0_named_blocks_new_copy(stmt->_.create_closure.branches);
    if (unlikely(blocks == NULL)) {
        s0_name_free(dest);
        s0_environment_free(closure_set);
        return -1;
    }

    closure = s0_closure_new(closure_set, blocks);
    if (unlikely(closure == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    return s0_environment_add(env, dest, closure);
}

static int
s0_create_literal_execute(struct s0_statement *stmt, struct s0_environment *env)
{
    struct s0_name  *dest;
    struct s0_entity  *literal;

    assert(s0_environment_get(env, stmt->_.create_literal.dest) == NULL);

    dest = s0_name_new_copy(stmt->_.create_literal.dest);
    if (unlikely(dest == NULL)) {
        return -1;
    }

    literal = s0_literal_new
        (stmt->_.create_literal.size, stmt->_.create_literal.content);
    if (unlikely(literal == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    return s0_environment_add(env, dest, literal);
}

static int
s0_create_method_execute(struct s0_statement *stmt, struct s0_environment *env)
{
    struct s0_name  *dest;
    struct s0_block  *body;
    struct s0_entity  *method;

    assert(s0_environment_get(env, stmt->_.create_method.dest) == NULL);

    dest = s0_name_new_copy(stmt->_.create_atom.dest);
    if (unlikely(dest == NULL)) {
        return -1;
    }

    body = s0_block_new_copy(stmt->_.create_method.body);
    if (unlikely(body == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    method = s0_method_new(body);
    if (unlikely(method == NULL)) {
        s0_name_free(dest);
        return -1;
    }

    return s0_environment_add(env, dest, method);
}

static int
s0_statement_execute(struct s0_statement *stmt, struct s0_environment *env)
{
    switch (stmt->kind) {
        case S0_STATEMENT_KIND_CREATE_ATOM:
            return s0_create_atom_execute(stmt, env);
        case S0_STATEMENT_KIND_CREATE_CLOSURE:
            return s0_create_closure_execute(stmt, env);
        case S0_STATEMENT_KIND_CREATE_LITERAL:
            return s0_create_literal_execute(stmt, env);
        case S0_STATEMENT_KIND_CREATE_METHOD:
            return s0_create_method_execute(stmt, env);
        default:
            assert(false);
            break;
    }
}

static int
s0_statement_list_execute(struct s0_statement_list *list,
                          struct s0_environment *env)
{
    size_t  i;
    for (i = 0; i < list->size; i++) {
        int  rc = s0_statement_execute(list->statements[i], env);
        if (unlikely(rc != 0)) {
            return rc;
        }
    }
    return 0;
}

static struct s0_continuation
s0_execute_and_free_block_continuation(struct s0_block *block);

static struct s0_continuation
s0_invoke_closure_execute(struct s0_invocation *invocation,
                          struct s0_environment *env)
{
    int  rc;
    struct s0_entity  *closure;
    struct s0_block  *branch;

    closure = s0_environment_delete(env, invocation->_.invoke_closure.src);
    assert(closure != NULL);
    assert(closure->kind == S0_ENTITY_KIND_CLOSURE);

    branch = s0_named_blocks_delete
        (closure->_.closure.blocks, invocation->_.invoke_closure.branch);
    assert(branch != NULL);

    rc = s0_environment_rename(env, invocation->_.invoke_closure.params);
    if (unlikely(rc != 0)) {
        s0_entity_free(closure);
        s0_block_free(branch);
        return s0_error_continuation_;
    }

    assert(s0_environment_type_satisfied_by(branch->inputs, env));

    rc = s0_environment_merge(env, closure->_.closure.env);
    if (unlikely(rc != 0)) {
        s0_entity_free(closure);
        s0_block_free(branch);
        return s0_error_continuation_;
    }

    s0_entity_free(closure);
    return s0_execute_and_free_block_continuation(branch);
}

static struct s0_continuation
s0_execute_block_continuation(struct s0_block *block);

static struct s0_continuation
s0_invoke_method_execute(struct s0_invocation *invocation,
                         struct s0_environment *env)
{
    int  rc;
    struct s0_entity  *object;
    struct s0_entity  *method;

    object = s0_environment_get(env, invocation->_.invoke_method.src);
    assert(object != NULL);
    assert(object->kind == S0_ENTITY_KIND_OBJECT);

    method = s0_object_get(object, invocation->_.invoke_method.method);
    assert(method != NULL);

    rc = s0_environment_rename(env, invocation->_.invoke_method.params);
    if (unlikely(rc != 0)) {
        return s0_error_continuation_;
    }

    if (method->kind == S0_ENTITY_KIND_METHOD) {
        struct s0_block  *body = method->_.method.body;
        assert(s0_environment_type_satisfied_by(body->inputs, env));
        return s0_execute_block_continuation(body);
    } else if (method->kind == S0_ENTITY_KIND_PRIMITIVE_METHOD) {
        assert(s0_environment_type_satisfied_by
               (method->_.primitive_method.inputs, env));
        return method->_.primitive_method.cont;
    } else {
        assert(false);
    }
}

struct s0_continuation
s0_invocation_execute(struct s0_invocation *invocation,
                      struct s0_environment *env)
{
    switch (invocation->kind) {
        case S0_INVOCATION_KIND_INVOKE_CLOSURE:
            return s0_invoke_closure_execute(invocation, env);
        case S0_INVOCATION_KIND_INVOKE_METHOD:
            return s0_invoke_method_execute(invocation, env);
        default:
            assert(false);
            break;
    }
}

struct s0_continuation
s0_execute_block(void *ud, struct s0_environment *env)
{
    int  rc;
    struct s0_block  *block = ud;

    rc = s0_statement_list_execute(block->statements, env);
    if (unlikely(rc != 0)) {
        return s0_error_continuation_;
    }

    return s0_invocation_execute(block->invocation, env);
}

static struct s0_continuation
s0_execute_block_continuation(struct s0_block *block)
{
    struct s0_continuation  cont;
    cont.ud = block;
    cont.invoke = s0_execute_block;
    return cont;
}

struct s0_continuation
s0_execute_and_free_block(void *ud, struct s0_environment *env)
{
    int  rc;
    struct s0_block  *block = ud;
    struct s0_continuation  cont;

    rc = s0_statement_list_execute(block->statements, env);
    if (unlikely(rc != 0)) {
        s0_block_free(block);
        return s0_error_continuation_;
    }

    cont = s0_invocation_execute(block->invocation, env);
    s0_block_free(block);
    return cont;
}

static struct s0_continuation
s0_execute_and_free_block_continuation(struct s0_block *block)
{
    struct s0_continuation  cont;
    cont.ud = block;
    cont.invoke = s0_execute_and_free_block;
    return cont;
}

static inline struct s0_continuation
s0_continuation_invoke(struct s0_continuation cont, struct s0_environment *env)
{
    return cont.invoke(cont.ud, env);
}

static int
s0_continuation_execute(struct s0_continuation cont, struct s0_environment *env)
{
    while (true) {
        if (cont.invoke == s0_execute_finish_continuation) {
            return 0;
        } else if (unlikely(cont.invoke == s0_execute_error_continuation)) {
            return -1;
        } else {
            cont = s0_continuation_invoke(cont, env);
        }
    }
}

int
s0_block_execute(struct s0_block *block, struct s0_environment *env)
{
    if (!s0_environment_type_satisfied_by(block->inputs, env)) {
        return -1;
    }
    return s0_continuation_execute(s0_execute_block_continuation(block), env);
}


/*-----------------------------------------------------------------------------
 * S₀: Common primitives
 */

struct s0_extractor {
    struct s0_name  *input_name;
    struct s0_entity  **dest;
};

static struct s0_continuation
s0_extractor_method_execute(void *ud, struct s0_environment *env)
{
    struct s0_extractor  *extractor = ud;
    struct s0_entity  *result;
    result = s0_environment_delete(env, extractor->input_name);
    assert(result != NULL);
    *extractor->dest = result;
    return s0_finish_continuation_;
}

static void
s0_extractor_method_free_ud(void *ud)
{
    struct s0_extractor  *extractor = ud;
    s0_name_free(extractor->input_name);
    free(extractor);
}

static struct s0_entity *
s0_extractor_method_new(struct s0_name *input_name,
                        struct s0_entity_type *result_type,
                        struct s0_entity **dest)
{
    int  rc;
    struct s0_extractor  *extractor;
    struct s0_environment_type  *inputs;
    struct s0_name  *self_name;
    struct s0_environment_type  *self_elements;
    struct s0_entity_type  *self_type;
    struct s0_continuation  cont;

    extractor = malloc(sizeof(struct s0_extractor));
    if (unlikely(extractor == NULL)) {
        s0_set_memory_error();
        return NULL;
    }

    extractor->input_name = s0_name_new_copy(input_name);
    if (unlikely(extractor->input_name == NULL)) {
        free(extractor);
        return NULL;
    }

    extractor->dest = dest;

    inputs = s0_environment_type_new();
    if (unlikely(inputs == NULL)) {
        free(extractor);
        return NULL;
    }

    rc = s0_environment_type_add(inputs, input_name, result_type);
    if (unlikely(rc != 0)) {
        s0_environment_type_free(inputs);
        free(extractor);
        return NULL;
    }

    self_name = s0_name_new_str("self");
    if (unlikely(inputs == NULL)) {
        s0_environment_type_free(inputs);
        free(extractor);
        return NULL;
    }

    self_elements = s0_environment_type_new();
    if (unlikely(self_elements == NULL)) {
        s0_environment_type_free(inputs);
        s0_name_free(self_name);
        free(extractor);
        return NULL;
    }

    self_type = s0_object_entity_type_new(self_elements);
    if (unlikely(self_type == NULL)) {
        s0_environment_type_free(inputs);
        s0_name_free(self_name);
        free(extractor);
        return NULL;
    }

    rc = s0_environment_type_add(inputs, self_name, self_type);
    if (unlikely(rc != 0)) {
        s0_environment_type_free(inputs);
        free(extractor);
        return NULL;
    }

    cont.ud = extractor;
    cont.invoke = s0_extractor_method_execute;
    return s0_primitive_method_new(inputs, cont, s0_extractor_method_free_ud);
}

struct s0_entity *
s0_extractor_new(struct s0_name *input_name, struct s0_entity_type *result_type,
                 struct s0_entity **dest)
{
    int  rc;
    struct s0_environment  *closed_over;
    struct s0_entity  *object;
    struct s0_name  *name;
    struct s0_name  *input_name_copy;
    struct s0_entity_type  *result_type_copy;
    struct s0_entity  *method;
    struct s0_named_blocks  *blocks;
    struct s0_environment_type  *inputs;
    struct s0_statement_list  *statements;
    struct s0_name  *src;
    struct s0_name  *method_name;
    struct s0_name_mapping  *params;
    struct s0_name  *from;
    struct s0_name  *to;
    struct s0_invocation  *invocation;
    struct s0_block  *body;

    object = s0_object_new();
    if (unlikely(object == NULL)) {
        return NULL;
    }

    input_name_copy = s0_name_new_copy(input_name);
    if (unlikely(input_name_copy == NULL)) {
        s0_entity_free(object);
        return NULL;
    }

    result_type_copy = s0_entity_type_new_copy(result_type);
    if (unlikely(result_type_copy == NULL)) {
        s0_entity_free(object);
        s0_name_free(input_name_copy);
        return NULL;
    }

    method = s0_extractor_method_new(input_name_copy, result_type_copy, dest);
    if (unlikely(method == NULL)) {
        s0_entity_free(object);
        return NULL;
    }

    name = s0_name_new_str("extractor");
    if (unlikely(name == NULL)) {
        s0_entity_free(object);
        s0_entity_free(method);
        return NULL;
    }

    rc = s0_object_add(object, name, method);
    if (unlikely(rc != 0)) {
        s0_entity_free(object);
        return NULL;
    }

    closed_over = s0_environment_new();
    if (unlikely(closed_over == NULL)) {
        s0_entity_free(object);
        return NULL;
    }

    name = s0_name_new_str("object");
    if (unlikely(name == NULL)) {
        s0_entity_free(object);
        s0_environment_free(closed_over);
        return NULL;
    }

    rc = s0_environment_add(closed_over, name, object);
    if (unlikely(rc != 0)) {
        s0_environment_free(closed_over);
        return NULL;
    }

    statements = s0_statement_list_new();
    if (unlikely(statements == NULL)) {
        s0_environment_free(closed_over);
        return NULL;
    }

    src = s0_name_new_str("object");
    if (unlikely(src == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        return NULL;
    }

    method_name = s0_name_new_str("extractor");
    if (unlikely(method == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        return NULL;
    }

    params = s0_name_mapping_new();
    if (unlikely(method == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        return NULL;
    }

    from = s0_name_new_copy(input_name);
    if (unlikely(from == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        return NULL;
    }

    to = s0_name_new_copy(input_name);
    if (unlikely(to == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        s0_name_free(from);
        return NULL;
    }

    rc = s0_name_mapping_add(params, from, to);
    if (unlikely(rc != 0)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        return NULL;
    }

    from = s0_name_new_str("object");
    if (unlikely(from == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        return NULL;
    }

    to = s0_name_new_str("self");
    if (unlikely(to == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        s0_name_free(from);
        return NULL;
    }

    rc = s0_name_mapping_add(params, from, to);
    if (unlikely(rc != 0)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_name_free(src);
        s0_name_free(method_name);
        s0_name_mapping_free(params);
        return NULL;
    }

    invocation = s0_invoke_method_new(src, method_name, params);
    if (unlikely(invocation == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        return NULL;
    }

    inputs = s0_environment_type_new();
    if (unlikely(inputs == NULL)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_invocation_free(invocation);
        return NULL;
    }

    rc = s0_environment_type_add(inputs, input_name, result_type);
    if (unlikely(rc != 0)) {
        s0_environment_free(closed_over);
        s0_statement_list_free(statements);
        s0_invocation_free(invocation);
        s0_environment_type_free(inputs);
        return NULL;
    }

    body = s0_block_new(inputs, statements, invocation);
    if (unlikely(body == NULL)) {
        s0_environment_free(closed_over);
        return NULL;
    }

    blocks = s0_named_blocks_new();
    if (unlikely(blocks == NULL)) {
        s0_environment_free(closed_over);
        s0_block_free(body);
        return NULL;
    }

    name = s0_name_new_str("body");
    if (unlikely(name == NULL)) {
        s0_environment_free(closed_over);
        s0_block_free(body);
        s0_named_blocks_free(blocks);
        return NULL;
    }

    rc = s0_named_blocks_add(blocks, name, body);
    if (unlikely(rc != 0)) {
        s0_environment_free(closed_over);
        s0_named_blocks_free(blocks);
        return NULL;
    }

    return s0_closure_new(closed_over, blocks);
}


static struct s0_continuation
s0_finish_method_execute(void *ud, struct s0_environment *env)
{
    struct s0_name  *name;
    struct s0_entity  *self;

    name = s0_name_new_str("self");
    if (unlikely(name == NULL)) {
        return s0_error_continuation_;
    }

    self = s0_environment_delete(env, name);
    assert(self != NULL);
    s0_name_free(name);
    s0_entity_free(self);
    return s0_finish_continuation_;
}

static void
s0_finish_method_free_ud(void *ud)
{
}

static const struct s0_continuation s0_finish_method_continuation = {
    NULL,
    s0_finish_method_execute
};

static struct s0_entity *
s0_finish_method_new(void)
{
    int  rc;
    struct s0_environment_type  *inputs;
    struct s0_name  *name;
    struct s0_environment_type  *elements;
    struct s0_entity_type  *type;

    inputs = s0_environment_type_new();
    if (unlikely(inputs == NULL)) {
        return NULL;
    }

    name = s0_name_new_str("self");
    if (unlikely(name == NULL)) {
        s0_environment_type_free(inputs);
        return NULL;
    }

    elements = s0_environment_type_new();
    if (unlikely(elements == NULL)) {
        s0_environment_type_free(inputs);
        s0_name_free(name);
        return NULL;
    }

    type = s0_object_entity_type_new(elements);
    if (unlikely(type == NULL)) {
        s0_environment_type_free(inputs);
        s0_name_free(name);
        return NULL;
    }

    rc = s0_environment_type_add(inputs, name, type);
    if (unlikely(rc != 0)) {
        s0_environment_type_free(inputs);
        return NULL;
    }

    return s0_primitive_method_new
        (inputs, s0_finish_method_continuation, s0_finish_method_free_ud);
}

struct s0_entity *
s0_finish_new(void)
{
    int  rc;
    struct s0_entity  *object;
    struct s0_name  *name;
    struct s0_entity  *method;

    object = s0_object_new();
    if (unlikely(object == NULL)) {
        return NULL;
    }

    name = s0_name_new_str("finish");
    if (unlikely(name == NULL)) {
        s0_entity_free(object);
        return NULL;
    }

    method = s0_finish_method_new();
    if (unlikely(method == NULL)) {
        s0_entity_free(object);
        s0_name_free(name);
        return NULL;
    }

    rc = s0_object_add(object, name, method);
    if (unlikely(rc != 0)) {
        s0_entity_free(object);
        return NULL;
    }

    return object;
}
