/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include "swanson.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ccan/likely/likely.h"


/*-----------------------------------------------------------------------------
 * Names
 */

struct s0_name {
    size_t  size;
    const void  *content;
};

struct s0_name *
s0_name_new(size_t size, const void *content)
{
    struct s0_name  *name = malloc(sizeof(struct s0_name));
    if (unlikely(name == NULL)) {
        return NULL;
    }
    name->size = size;
    name->content = malloc(size);
    if (unlikely(name->content == NULL)) {
        free(name);
        return NULL;
    }
    memcpy((void *) name->content, content, size);
    return name;
}

struct s0_name *
s0_name_new_str(const void *content)
{
    return s0_name_new(strlen(content), content);
}

void
s0_name_free(struct s0_name *name)
{
    free((void *) name->content);
    free(name);
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

struct s0_name_set {
    size_t  size;
    size_t  allocated_size;
    struct s0_name  **names;
};

#define DEFAULT_INITIAL_NAME_SET_SIZE  4

struct s0_name_set *
s0_name_set_new(void)
{
    struct s0_name_set  *set = malloc(sizeof(struct s0_name_set));
    if (unlikely(set == NULL)) {
        return NULL;
    }
    set->size = 0;
    set->allocated_size = DEFAULT_INITIAL_NAME_SET_SIZE;
    set->names =
        malloc(DEFAULT_INITIAL_NAME_SET_SIZE * sizeof(struct s0_name *));
    if (unlikely(set->names == NULL)) {
        free(set);
        return NULL;
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
            return -1;
        }
        set->names = new_names;
        set->allocated_size = new_size;
    }

    set->names[set->size++] = name;
    return 0;
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


/*-----------------------------------------------------------------------------
 * Environments
 */

struct s0_environment_entry {
    struct s0_environment_entry  *next;
    struct s0_name  *name;
    struct s0_entity  *entity;
};

struct s0_environment {
    struct s0_environment_entry  *head;
};

struct s0_environment *
s0_environment_new(void)
{
    struct s0_environment  *env = malloc(sizeof(struct s0_environment));
    if (unlikely(env == NULL)) {
        return NULL;
    }
    env->head = NULL;
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
    size_t  size = 0;
    struct s0_environment_entry  *curr;
    for (curr = env->head; curr != NULL; curr = curr->next) {
        size++;
    }
    return size;
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
        return -1;
    }

    entry->name = name;
    entry->entity = entity;
    entry->next = env->head;
    env->head = entry;
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
            return entity;
        }
    }

    /* Precondition says this isn't allowed. */
    assert(false);
    return NULL;
}


/*-----------------------------------------------------------------------------
 * S₀: Blocks
 */

struct s0_block {
};

struct s0_block *
s0_block_new(void)
{
    struct s0_block  *block = malloc(sizeof(struct s0_block));
    if (unlikely(block == NULL)) {
        return NULL;
    }
    return block;
}

void
s0_block_free(struct s0_block *block)
{
    free(block);
}


/*-----------------------------------------------------------------------------
 * Named blocks
 */

struct s0_named_blocks_entry {
    struct s0_named_blocks_entry  *next;
    struct s0_name  *name;
    struct s0_block  *block;
};

struct s0_named_blocks {
    struct s0_named_blocks_entry  *head;
};

struct s0_named_blocks *
s0_named_blocks_new(void)
{
    struct s0_named_blocks  *blocks = malloc(sizeof(struct s0_named_blocks));
    if (unlikely(blocks == NULL)) {
        return NULL;
    }
    blocks->head = NULL;
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
        return -1;
    }

    entry->name = name;
    entry->block = block;
    entry->next = blocks->head;
    blocks->head = entry;
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


/*-----------------------------------------------------------------------------
 * Entities
 */

struct s0_entity {
    enum s0_entity_type  type;
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
            struct s0_name  *self_name;
            struct s0_block  *block;
        } method;
        struct {
            size_t  size;
            size_t  allocated_size;
            struct s0_object_entry  *entries;
        } obj;
    } _;
};


struct s0_entity *
s0_atom_new(void)
{
    struct s0_entity  *atom = malloc(sizeof(struct s0_entity));
    if (unlikely(atom == NULL)) {
        return NULL;
    }
    atom->type = S0_ENTITY_TYPE_ATOM;
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
    assert(a1->type == S0_ENTITY_TYPE_ATOM);
    assert(a2->type == S0_ENTITY_TYPE_ATOM);
    return (a1 == a2);
}


struct s0_entity *
s0_closure_new(struct s0_environment *env, struct s0_named_blocks *blocks)
{
    struct s0_entity  *closure = malloc(sizeof(struct s0_entity));
    if (unlikely(closure == NULL)) {
        s0_environment_free(env);
        s0_named_blocks_free(blocks);
        return NULL;
    }
    closure->type = S0_ENTITY_TYPE_CLOSURE;
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
    assert(closure->type == S0_ENTITY_TYPE_CLOSURE);
    return closure->_.closure.env;
}

struct s0_named_blocks *
s0_closure_named_blocks(const struct s0_entity *closure)
{
    assert(closure->type == S0_ENTITY_TYPE_CLOSURE);
    return closure->_.closure.blocks;
}


struct s0_entity *
s0_literal_new(size_t size, const void *content)
{
    struct s0_entity  *literal = malloc(sizeof(struct s0_entity));
    if (unlikely(literal == NULL)) {
        return NULL;
    }
    literal->type = S0_ENTITY_TYPE_LITERAL;
    literal->_.literal.size = size;
    literal->_.literal.content = malloc(size);
    if (unlikely(literal->_.literal.content == NULL)) {
        free(literal);
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
    assert(literal->type == S0_ENTITY_TYPE_LITERAL);
    return literal->_.literal.content;
}

size_t
s0_literal_size(const struct s0_entity *literal)
{
    assert(literal->type == S0_ENTITY_TYPE_LITERAL);
    return literal->_.literal.size;
}


struct s0_entity *
s0_method_new(struct s0_name *self_name, struct s0_block *block)
{
    struct s0_entity  *method = malloc(sizeof(struct s0_entity));
    if (unlikely(method == NULL)) {
        s0_name_free(self_name);
        s0_block_free(block);
        return NULL;
    }
    method->type = S0_ENTITY_TYPE_METHOD;
    method->_.method.self_name = self_name;
    method->_.method.block = block;
    return method;
}

static void
s0_method_free(struct s0_entity *method)
{
    s0_name_free(method->_.method.self_name);
    s0_block_free(method->_.method.block);
}

struct s0_name *
s0_method_self_name(const struct s0_entity *method)
{
    assert(method->type == S0_ENTITY_TYPE_METHOD);
    return method->_.method.self_name;
}

struct s0_block *
s0_method_block(const struct s0_entity *method)
{
    assert(method->type == S0_ENTITY_TYPE_METHOD);
    return method->_.method.block;
}


#define DEFAULT_INITIAL_OBJECT_SIZE  4

struct s0_entity *
s0_object_new(void)
{
    struct s0_entity  *obj = malloc(sizeof(struct s0_entity));
    if (unlikely(obj == NULL)) {
        return NULL;
    }
    obj->type = S0_ENTITY_TYPE_OBJECT;
    obj->_.obj.size = 0;
    obj->_.obj.allocated_size = DEFAULT_INITIAL_OBJECT_SIZE;
    obj->_.obj.entries =
        malloc(DEFAULT_INITIAL_OBJECT_SIZE * sizeof(struct s0_object_entry));
    if (unlikely(obj->_.obj.entries == NULL)) {
        free(obj);
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

    if (unlikely(obj->_.obj.size == obj->_.obj.allocated_size)) {
        size_t  new_size = obj->_.obj.allocated_size * 2;
        struct s0_object_entry  *new_entries =
            realloc(obj->_.obj.entries,
                    new_size * sizeof(struct s0_object_entry));
        if (unlikely(new_entries == NULL)) {
            s0_name_free(name);
            s0_entity_free(entity);
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
    assert(obj->type == S0_ENTITY_TYPE_OBJECT);
    return obj->_.obj.size;
}

struct s0_object_entry
s0_object_at(const struct s0_entity *obj, size_t index)
{
    assert(obj->type == S0_ENTITY_TYPE_OBJECT);
    assert(index < obj->_.obj.size);
    return obj->_.obj.entries[index];
}

struct s0_entity *
s0_object_get(const struct s0_entity *obj, const struct s0_name *name)
{
    size_t  i;
    assert(obj->type == S0_ENTITY_TYPE_OBJECT);
    for (i = 0; i < obj->_.obj.size; i++) {
        if (s0_name_eq(obj->_.obj.entries[i].name, name)) {
            return obj->_.obj.entries[i].entity;
        }
    }
    return NULL;
}


void
s0_entity_free(struct s0_entity *entity)
{
    switch (entity->type) {
        case S0_ENTITY_TYPE_ATOM:
            s0_atom_free(entity);
            break;
        case S0_ENTITY_TYPE_CLOSURE:
            s0_closure_free(entity);
            break;
        case S0_ENTITY_TYPE_LITERAL:
            s0_literal_free(entity);
            break;
        case S0_ENTITY_TYPE_METHOD:
            s0_method_free(entity);
            break;
        case S0_ENTITY_TYPE_OBJECT:
            s0_object_free(entity);
            break;
        default:
            assert(false);
            break;
    }
    free(entity);
}

enum s0_entity_type
s0_entity_type(const struct s0_entity *entity)
{
    return entity->type;
}
