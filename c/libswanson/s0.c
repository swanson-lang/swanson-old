/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include "swanson.h"

#include <stdlib.h>
#include <string.h>
#include "ccan/likely/likely.h"

struct s0_name {
    size_t  size;
    const void  *content;
};

struct s0_name *
s0_name_new(size_t size, const void *content)
{
    struct s0_name *name = malloc(sizeof(struct s0_name));
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

void
s0_name_free(struct s0_name *name)
{
    free((void *) name->content);
    free(name);
}
