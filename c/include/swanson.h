/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#ifndef SWANSON_S0_H
#define SWANSON_S0_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>


struct s0_name;

/* Makes a copy of content */
struct s0_name *
s0_name_new(size_t size, const void *content);

void
s0_name_free(struct s0_name *);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SWANSON_S0_H */
