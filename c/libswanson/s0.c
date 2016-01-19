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
 * Entities
 */

struct s0_entity {
    enum s0_entity_type  type;
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

void
s0_entity_free(struct s0_entity *entity)
{
    free(entity);
}

enum s0_entity_type
s0_entity_type(const struct s0_entity *entity)
{
    return entity->type;
}

bool
s0_atom_eq(const struct s0_entity *a1, const struct s0_entity *a2)
{
    assert(a1->type == S0_ENTITY_TYPE_ATOM);
    assert(a2->type == S0_ENTITY_TYPE_ATOM);
    return (a1 == a2);
}
