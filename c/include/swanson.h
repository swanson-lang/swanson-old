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
 * S₀: Entities
 */

enum s0_entity_type {
    S0_ENTITY_TYPE_ATOM
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


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SWANSON_S0_H */
