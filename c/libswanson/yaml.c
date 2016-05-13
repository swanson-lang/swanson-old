/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include "swanson.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ccan/likely/likely.h"
#include "ccan/str/str.h"
#include "yaml.h"


#define YAML_ERROR_SIZE  (2 * 1024)

struct s0_yaml_stream {
    yaml_parser_t  parser;
    yaml_document_t  document;
    const char  *filename;
    FILE  *fp;
    bool  should_close_fp;
    bool  document_created;
    char  error[YAML_ERROR_SIZE];
};

#define fill_error(stream, ...) \
    snprintf((stream)->error, YAML_ERROR_SIZE, __VA_ARGS__)

#define fill_memory_error(stream) \
    fill_error((stream), "Error allocating memory")

struct s0_yaml_stream *
s0_yaml_stream_new_from_file(FILE *fp, const char *filename,
                             bool should_close_fp)
{
    struct s0_yaml_stream  *stream = malloc(sizeof(struct s0_yaml_stream));
    if (unlikely(stream == NULL)) {
        return NULL;
    }

    stream->filename = strdup(filename);
    if (unlikely(stream->filename == NULL)) {
        free(stream);
        return NULL;
    }

    if (unlikely(!yaml_parser_initialize(&stream->parser))) {
        free((void *) stream->filename);
        free(stream);
        return NULL;
    }

    yaml_parser_set_input_file(&stream->parser, fp);
    stream->fp = fp;
    stream->document_created = false;
    stream->should_close_fp = should_close_fp;
    stream->error[0] = '\0';
    return stream;
}

struct s0_yaml_stream *
s0_yaml_stream_new_from_filename(const char *filename)
{
    struct s0_yaml_stream  *stream = malloc(sizeof(struct s0_yaml_stream));
    if (unlikely(stream == NULL)) {
        return NULL;
    }

    stream->filename = strdup(filename);
    if (unlikely(stream->filename == NULL)) {
        free(stream);
        return NULL;
    }

    if (unlikely(!yaml_parser_initialize(&stream->parser))) {
        free((void *) stream->filename);
        free(stream);
        return NULL;
    }

    stream->fp = NULL;
    stream->document_created = false;
    stream->should_close_fp = false;
    stream->error[0] = '\0';
    return stream;
}

struct s0_yaml_stream *
s0_yaml_stream_new_from_string(const char *str)
{
    struct s0_yaml_stream  *stream = malloc(sizeof(struct s0_yaml_stream));
    if (unlikely(stream == NULL)) {
        return NULL;
    }

    if (unlikely(!yaml_parser_initialize(&stream->parser))) {
        free(stream);
        return NULL;
    }

    yaml_parser_set_input_string
        (&stream->parser, (unsigned char *) str, strlen(str));
    stream->filename = NULL;
    stream->fp = NULL;
    stream->document_created = false;
    stream->should_close_fp = false;
    stream->error[0] = '\0';
    return stream;
}

void
s0_yaml_stream_free(struct s0_yaml_stream *stream)
{
    if (stream->document_created) {
        yaml_document_delete(&stream->document);
    }
    if (stream->should_close_fp) {
        fclose(stream->fp);
    }
    yaml_parser_delete(&stream->parser);
    if (stream->filename != NULL) {
        free((void *) stream->filename);
    }
    free(stream);
}

const char *
s0_yaml_stream_last_error(const struct s0_yaml_stream *stream)
{
    return stream->error;
}


#define s0_yaml_node_get_node(node_)  ((yaml_node_t *) (node_).node)

#define ensure_scalar(node, why) \
    do { \
        yaml_node_t  *_node = s0_yaml_node_get_node(node); \
        if (unlikely(_node->type != YAML_SCALAR_NODE)) { \
            fill_error(node.stream, \
                       "Expected %s to be a YAML scalar at %zu:%zu", \
                       (why), \
                       _node->start_mark.line, \
                       _node->start_mark.column); \
            return NULL; \
        } \
    } while (0)

#define ensure_sequence(node, why) \
    do { \
        yaml_node_t  *_node = s0_yaml_node_get_node(node); \
        if (unlikely(_node->type != YAML_SEQUENCE_NODE)) { \
            fill_error(node.stream, \
                       "Expected %s to be a YAML sequence at %zu:%zu", \
                       (why), \
                       _node->start_mark.line, \
                       _node->start_mark.column); \
            return NULL; \
        } \
    } while (0)

#define ensure_mapping(node, why) \
    do { \
        yaml_node_t  *_node = s0_yaml_node_get_node(node); \
        if (unlikely(_node->type != YAML_MAPPING_NODE)) { \
            fill_error(node.stream, \
                       "Expected %s to be a YAML mapping at %zu:%zu", \
                       (why), \
                       _node->start_mark.line, \
                       _node->start_mark.column); \
            return NULL; \
        } \
    } while (0)

const char *
s0_yaml_node_tag(const struct s0_yaml_node node)
{
    return (char *) s0_yaml_node_get_node(node)->tag;
}

bool
s0_yaml_node_has_tag(const struct s0_yaml_node node, const char *tag)
{
    return streq((char *) s0_yaml_node_get_node(node)->tag, tag);
}

bool
s0_yaml_node_is_mapping(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->type == YAML_MAPPING_NODE;
}

bool
s0_yaml_node_is_scalar(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->type == YAML_SCALAR_NODE;
}

bool
s0_yaml_node_is_sequence(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->type == YAML_SEQUENCE_NODE;
}

const char *
s0_yaml_node_scalar_content(const struct s0_yaml_node node)
{
    return (char *) s0_yaml_node_get_node(node)->data.scalar.value;
}

size_t
s0_yaml_node_scalar_size(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->data.scalar.length;
}

size_t
s0_yaml_node_sequence_size(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->data.sequence.items.top -
        s0_yaml_node_get_node(node)->data.sequence.items.start;
}

struct s0_yaml_node
s0_yaml_node_sequence_at(const struct s0_yaml_node node, size_t index)
{
    struct s0_yaml_node  result;
    yaml_node_item_t  *item =
        s0_yaml_node_get_node(node)->data.sequence.items.start + index;
    result.stream = node.stream;
    result.node = yaml_document_get_node(&node.stream->document, *item);
    return result;
}

size_t
s0_yaml_node_mapping_size(const struct s0_yaml_node node)
{
    return s0_yaml_node_get_node(node)->data.mapping.pairs.top -
        s0_yaml_node_get_node(node)->data.mapping.pairs.start;
}

struct s0_yaml_node
s0_yaml_node_mapping_key_at(const struct s0_yaml_node node, size_t index)
{
    struct s0_yaml_node  result;
    yaml_node_pair_t  *pair =
        s0_yaml_node_get_node(node)->data.mapping.pairs.start + index;
    result.stream = node.stream;
    result.node = yaml_document_get_node(&node.stream->document, pair->key);
    return result;
}

struct s0_yaml_node
s0_yaml_node_mapping_value_at(const struct s0_yaml_node node, size_t index)
{
    struct s0_yaml_node  result;
    yaml_node_pair_t  *pair =
        s0_yaml_node_get_node(node)->data.mapping.pairs.start + index;
    result.stream = node.stream;
    result.node = yaml_document_get_node(&node.stream->document, pair->value);
    return result;
}

struct s0_yaml_node
s0_yaml_node_mapping_get(const struct s0_yaml_node node, const char *needle)
{
    struct s0_yaml_node  result;
    size_t  needle_size = strlen(needle);
    yaml_node_pair_t  *curr;
    for (curr = s0_yaml_node_get_node(node)->data.mapping.pairs.start;
         curr < s0_yaml_node_get_node(node)->data.mapping.pairs.top; curr++) {
        yaml_node_t  *key =
            yaml_document_get_node(&node.stream->document, curr->key);
        if (likely(key->type == YAML_SCALAR_NODE)) {
            if (key->data.scalar.length == needle_size &&
                memcmp(needle, key->data.scalar.value, needle_size) == 0) {
                result.stream = node.stream;
                result.node =
                    yaml_document_get_node(&node.stream->document, curr->value);
                return result;
            }
        }
    }
    result.node = NULL;
    return result;
}

static struct s0_name *
s0_load_name(struct s0_yaml_node node)
{
    ensure_scalar(node, "name");
    return s0_name_new
        (s0_yaml_node_get_node(node)->data.scalar.length,
         s0_yaml_node_get_node(node)->data.scalar.value);
}

static struct s0_name_set *
s0_load_name_set(struct s0_yaml_node node)
{
    struct s0_name_set  *set;
    size_t  i;
    size_t  count;

    ensure_sequence(node, "name set");
    count = s0_yaml_node_sequence_size(node);
    set = s0_name_set_new();
    for (i = 0; i < count; i++) {
        struct s0_yaml_node  item = s0_yaml_node_sequence_at(node, i);
        struct s0_name  *name = s0_load_name(item);
        if (unlikely(name == NULL)) {
            s0_name_set_free(set);
            return NULL;
        }

        if (unlikely(s0_name_set_contains(set, name))) {
            fill_error
                (node.stream,
                 "Closure set already contains `%s`.",
                 s0_name_human_readable(name));
            s0_name_free(name);
            s0_name_set_free(set);
            return NULL;
        }

        if (unlikely(s0_name_set_add(set, name))) {
            s0_name_set_free(set);
            return NULL;
        }
    }

    return set;
}

static struct s0_environment_type *
s0_load_environment_type(struct s0_yaml_node node);

static struct s0_environment_type_mapping *
s0_load_environment_type_mapping(struct s0_yaml_node node)
{
    struct s0_environment_type_mapping  *mapping;
    size_t  i;
    size_t  count;

    ensure_mapping(node, "environment type mapping");
    count = s0_yaml_node_mapping_size(node);
    mapping = s0_environment_type_mapping_new();
    for (i = 0; i < count; i++) {
        struct s0_yaml_node  item;
        struct s0_name  *name;
        struct s0_environment_type  *type;

        item = s0_yaml_node_mapping_key_at(node, i);
        name = s0_load_name(item);
        if (unlikely(name == NULL)) {
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }

        if (unlikely(s0_environment_type_mapping_get(mapping, name) != NULL)) {
            fill_error
                (node.stream,
                 "There is already a branch type named `%s`.",
                 s0_name_human_readable(name));
            s0_name_free(name);
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }

        item = s0_yaml_node_mapping_value_at(node, i);
        type = s0_load_environment_type(item);
        if (unlikely(type == NULL)) {
            s0_name_free(name);
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }

        if (unlikely(s0_environment_type_mapping_add(mapping, name, type))) {
            s0_environment_type_mapping_free(mapping);
            return NULL;
        }
    }

    return mapping;
}

static struct s0_entity_type *
s0_load_any_entity_type(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!any mapping node. */
    return s0_any_entity_type_new();
}

static struct s0_entity_type *
s0_load_closure_entity_type(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!closure mapping node. */
    struct s0_yaml_node  item;
    struct s0_environment_type_mapping  *mapping;

    /* branches */

    item = s0_yaml_node_mapping_get(node, "branches");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream,
                   "closure entity type requires a branches at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    mapping = s0_load_environment_type_mapping(item);
    if (unlikely(mapping == NULL)) {
        return NULL;
    }

    return s0_closure_entity_type_new(mapping);
}

static struct s0_entity_type *
s0_load_method_entity_type(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!method mapping node. */
    struct s0_yaml_node  item;
    struct s0_environment_type  *inputs;

    /* inputs */

    item = s0_yaml_node_mapping_get(node, "inputs");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream,
                   "method entity type requires a inputs at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    inputs = s0_load_environment_type(item);
    if (unlikely(inputs == NULL)) {
        return NULL;
    }

    return s0_method_entity_type_new(inputs);
}

static struct s0_entity_type *
s0_load_object_entity_type(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!object mapping node. */
    struct s0_environment_type  *elements;
    elements = s0_load_environment_type(node);
    if (unlikely(elements == NULL)) {
        return NULL;
    }
    return s0_object_entity_type_new(elements);
}

static struct s0_entity_type *
s0_load_entity_type(struct s0_yaml_node node)
{
    ensure_mapping(node, "entity type");
    if (s0_yaml_node_has_tag(node, S0_ANY_TAG)) {
        return s0_load_any_entity_type(node);
    } else if (s0_yaml_node_has_tag(node, S0_CLOSURE_TAG)) {
        return s0_load_closure_entity_type(node);
    } else if (s0_yaml_node_has_tag(node, S0_METHOD_TAG)) {
        return s0_load_method_entity_type(node);
    } else if (s0_yaml_node_has_tag(node, S0_OBJECT_TAG)) {
        return s0_load_object_entity_type(node);
    } else {
        fill_error(node.stream, "Unknown entity type at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }
}

static struct s0_environment_type *
s0_load_environment_type(struct s0_yaml_node node)
{
    struct s0_environment_type  *type;
    size_t  i;
    size_t  count;

    ensure_mapping(node, "environment type");
    count = s0_yaml_node_mapping_size(node);
    type = s0_environment_type_new();
    for (i = 0; i < count; i++) {
        struct s0_yaml_node  item;
        struct s0_name  *name;
        struct s0_entity_type  *etype;

        item = s0_yaml_node_mapping_key_at(node, i);
        name = s0_load_name(item);
        if (unlikely(name == NULL)) {
            s0_environment_type_free(type);
            return NULL;
        }

        if (unlikely(s0_environment_type_get(type, name) != NULL)) {
            fill_error
                (node.stream,
                 "There is already an environment type entry named `%s`.",
                 s0_name_human_readable(name));
            s0_name_free(name);
            s0_environment_type_free(type);
            return NULL;
        }

        item = s0_yaml_node_mapping_value_at(node, i);
        etype = s0_load_entity_type(item);
        if (unlikely(etype == NULL)) {
            s0_name_free(name);
            s0_environment_type_free(type);
            return NULL;
        }

        if (unlikely(s0_environment_type_add(type, name, etype))) {
            s0_environment_type_free(type);
            return NULL;
        }
    }

    return type;
}

static struct s0_name_mapping *
s0_load_name_mapping(struct s0_yaml_node node)
{
    struct s0_name_mapping  *mapping;
    size_t  i;
    size_t  count;

    ensure_mapping(node, "name mapping");
    count = s0_yaml_node_mapping_size(node);
    mapping = s0_name_mapping_new();
    for (i = 0; i < count; i++) {
        struct s0_yaml_node  item;
        struct s0_name  *from;
        struct s0_name  *to;

        item = s0_yaml_node_mapping_key_at(node, i);
        from = s0_load_name(item);
        if (unlikely(from == NULL)) {
            s0_name_mapping_free(mapping);
            return NULL;
        }

        item = s0_yaml_node_mapping_value_at(node, i);
        to = s0_load_name(item);
        if (unlikely(to == NULL)) {
            s0_name_free(from);
            s0_name_mapping_free(mapping);
            return NULL;
        }

        if (unlikely(s0_name_mapping_get(mapping, from) != NULL)) {
            fill_error
                (node.stream,
                 "There is already an input named `%s`.",
                 s0_name_human_readable(from));
            s0_name_free(from);
            s0_name_free(to);
            s0_name_mapping_free(mapping);
            return NULL;
        }

        if (unlikely(s0_name_mapping_get_from(mapping, to) != NULL)) {
            fill_error
                (node.stream,
                 "There is already an input that is renamed to `%s`.",
                 s0_name_human_readable(to));
            s0_name_free(from);
            s0_name_free(to);
            s0_name_mapping_free(mapping);
            return NULL;
        }

        if (unlikely(s0_name_mapping_add(mapping, from, to))) {
            fill_memory_error(node.stream);
            s0_name_mapping_free(mapping);
            return NULL;
        }
    }

    return mapping;
}

static struct s0_block *
s0_load_block(struct s0_yaml_node node);

static struct s0_named_blocks *
s0_load_named_blocks(struct s0_yaml_node node)
{
    struct s0_named_blocks  *blocks;
    size_t  i;
    size_t  count;

    ensure_mapping(node, "named blocks");
    count = s0_yaml_node_mapping_size(node);
    blocks = s0_named_blocks_new();
    for (i = 0; i < count; i++) {
        struct s0_yaml_node  item;
        struct s0_name  *name;
        struct s0_block  *block;

        item = s0_yaml_node_mapping_key_at(node, i);
        name = s0_load_name(item);
        if (unlikely(name == NULL)) {
            s0_named_blocks_free(blocks);
            return NULL;
        }

        if (unlikely(s0_named_blocks_get(blocks, name) != NULL)) {
            fill_error
                (node.stream,
                 "There is already a branch named `%s`.",
                 s0_name_human_readable(name));
            s0_name_free(name);
            s0_named_blocks_free(blocks);
            return NULL;
        }

        item = s0_yaml_node_mapping_value_at(node, i);
        block = s0_load_block(item);
        if (unlikely(block == NULL)) {
            s0_name_free(name);
            s0_named_blocks_free(blocks);
            return NULL;
        }

        if (unlikely(s0_named_blocks_add(blocks, name, block))) {
            s0_named_blocks_free(blocks);
            return NULL;
        }
    }

    return blocks;
}

static struct s0_statement *
s0_load_create_atom(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!create-atom mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *dest;

    /* dest */

    item = s0_yaml_node_mapping_get(node, "dest");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-atom requires a dest at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    dest = s0_load_name(item);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    return s0_create_atom_new(dest);
}

static struct s0_statement *
s0_load_create_closure(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!create-closure mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *dest;
    struct s0_name_set  *closed_over;
    struct s0_named_blocks  *branches;

    /* dest */

    item = s0_yaml_node_mapping_get(node, "dest");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-closure requires a dest at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    dest = s0_load_name(item);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    /* closed_over */

    item = s0_yaml_node_mapping_get(node, "closed-over");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream,
                   "create-closure requires a closed-over at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(dest);
        return NULL;
    }

    closed_over = s0_load_name_set(item);
    if (unlikely(closed_over == NULL)) {
        s0_name_free(dest);
        return NULL;
    }

    /* branches */

    item = s0_yaml_node_mapping_get(node, "branches");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-closure requires a branches at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(dest);
        s0_name_set_free(closed_over);
        return NULL;
    }

    branches = s0_load_named_blocks(item);
    if (unlikely(branches == NULL)) {
        s0_name_free(dest);
        s0_name_set_free(closed_over);
        return NULL;
    }
    if (unlikely(s0_named_blocks_size(branches) == 0)) {
        fill_error(node.stream,
                   "create-closure needs at least one branch at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(dest);
        s0_named_blocks_free(branches);
        s0_name_set_free(closed_over);
        return NULL;
    }

    return s0_create_closure_new(dest, closed_over, branches);
}

static struct s0_statement *
s0_load_create_literal(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!create-literal mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *dest;

    /* dest */

    item = s0_yaml_node_mapping_get(node, "dest");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-literal requires a dest at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    dest = s0_load_name(item);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    /* content */

    item = s0_yaml_node_mapping_get(node, "content");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-literal requires a content at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(dest);
        return NULL;
    }

    if (unlikely(!s0_yaml_node_is_scalar(item))) {
        fill_error(node.stream,
                   "create-literal content must be a scalar at %zu:%zu",
                   s0_yaml_node_get_node(item)->start_mark.line,
                   s0_yaml_node_get_node(item)->start_mark.column);
        s0_name_free(dest);
        return NULL;
    }

    return s0_create_literal_new
        (dest, s0_yaml_node_scalar_size(item),
         s0_yaml_node_scalar_content(item));
}

static struct s0_statement *
s0_load_create_method(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!create-method mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *dest;
    struct s0_block  *body;

    /* dest */

    item = s0_yaml_node_mapping_get(node, "dest");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-method requires a dest at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    dest = s0_load_name(item);
    if (unlikely(dest == NULL)) {
        return NULL;
    }

    /* body */

    item = s0_yaml_node_mapping_get(node, "body");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "create-method requires a body at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(dest);
        return NULL;
    }

    body = s0_load_block(item);
    if (unlikely(body == NULL)) {
        s0_name_free(dest);
        return NULL;
    }

    return s0_create_method_new(dest, body);
}

static struct s0_statement *
s0_load_statement(struct s0_yaml_node node)
{
    ensure_mapping(node, "statement");
    if (s0_yaml_node_has_tag(node, S0_CREATE_ATOM_TAG)) {
        return s0_load_create_atom(node);
    } else if (s0_yaml_node_has_tag(node, S0_CREATE_CLOSURE_TAG)) {
        return s0_load_create_closure(node);
    } else if (s0_yaml_node_has_tag(node, S0_CREATE_LITERAL_TAG)) {
        return s0_load_create_literal(node);
    } else if (s0_yaml_node_has_tag(node, S0_CREATE_METHOD_TAG)) {
        return s0_load_create_method(node);
    } else {
        fill_error(node.stream, "Unknown statement type at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }
}

static struct s0_statement_list *
s0_load_statement_list(struct s0_yaml_node node,
                       struct s0_environment_type *type)
{
    struct s0_statement_list  *list;
    size_t  i;
    size_t  count;

    ensure_sequence(node, "statement list");
    count = s0_yaml_node_sequence_size(node);
    list = s0_statement_list_new();
    for (i = 0; i < count; i++) {
        int  rc;
        struct s0_yaml_node  item = s0_yaml_node_sequence_at(node, i);
        struct s0_statement  *statement = s0_load_statement(item);
        if (unlikely(statement == NULL)) {
            s0_statement_list_free(list);
            return NULL;
        }

        rc = s0_environment_type_add_statement(type, statement);
        if (unlikely(rc != 0)) {
            fill_error(node.stream, "%s\nat %zu:%zu",
                       s0_error_get_last_description(),
                       s0_yaml_node_get_node(node)->start_mark.line,
                       s0_yaml_node_get_node(node)->start_mark.column);
            s0_statement_free(statement);
            s0_statement_list_free(list);
            return NULL;
        }

        rc = s0_statement_list_add(list, statement);
        if (unlikely(rc != 0)) {
            s0_statement_list_free(list);
            return NULL;
        }
    }

    return list;
}

static struct s0_invocation *
s0_load_invoke_closure(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!invoke-closure mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *src;
    struct s0_name  *branch;
    struct s0_name_mapping  *parameters;

    /* src */

    item = s0_yaml_node_mapping_get(node, "src");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "invoke-closure requires a src at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    src = s0_load_name(item);
    if (unlikely(src == NULL)) {
        return NULL;
    }

    /* branch */

    item = s0_yaml_node_mapping_get(node, "branch");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "invoke-closure requires a branch at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(src);
        return NULL;
    }

    branch = s0_load_name(item);
    if (unlikely(branch == NULL)) {
        s0_name_free(src);
        return NULL;
    }

    /* parameters */

    item = s0_yaml_node_mapping_get(node, "parameters");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream,
                   "invoke-closure requires a parameters at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(src);
        s0_name_free(branch);
        return NULL;
    }

    parameters = s0_load_name_mapping(item);
    if (unlikely(parameters == NULL)) {
        s0_name_free(src);
        s0_name_free(branch);
        return NULL;
    }

    return s0_invoke_closure_new(src, branch, parameters);
}

static struct s0_invocation *
s0_load_invoke_method(struct s0_yaml_node node)
{
    /* We've already verified that this is a !s0!invoke-method mapping node. */
    struct s0_yaml_node  item;
    struct s0_name  *src;
    struct s0_name  *method;
    struct s0_name_mapping  *parameters;

    /* src */

    item = s0_yaml_node_mapping_get(node, "src");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "invoke-method requires a src at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    src = s0_load_name(item);
    if (unlikely(src == NULL)) {
        return NULL;
    }

    /* method */

    item = s0_yaml_node_mapping_get(node, "method");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "invoke-method requires a method at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(src);
        return NULL;
    }

    method = s0_load_name(item);
    if (unlikely(method == NULL)) {
        s0_name_free(src);
        return NULL;
    }

    /* parameters */

    item = s0_yaml_node_mapping_get(node, "parameters");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream,
                   "invoke-method requires a parameters at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_name_free(src);
        s0_name_free(method);
        return NULL;
    }

    parameters = s0_load_name_mapping(item);
    if (unlikely(parameters == NULL)) {
        s0_name_free(src);
        s0_name_free(method);
        return NULL;
    }

    return s0_invoke_method_new(src, method, parameters);
}

static struct s0_invocation *
s0_load_invocation(struct s0_yaml_node node, struct s0_environment_type *type)
{
    int  rc;
    struct s0_invocation  *invocation;

    ensure_mapping(node, "invocation");
    if (s0_yaml_node_has_tag(node, S0_INVOKE_CLOSURE_TAG)) {
        invocation = s0_load_invoke_closure(node);
    } else if (s0_yaml_node_has_tag(node, S0_INVOKE_METHOD_TAG)) {
        invocation = s0_load_invoke_method(node);
    } else {
        fill_error(node.stream, "Unknown invocation type at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    if (unlikely(invocation == NULL)) {
        return NULL;
    }

    rc = s0_environment_type_add_invocation(type, invocation);
    if (unlikely(rc != 0)) {
        fill_error(node.stream, "%s\nat %zu:%zu",
                   s0_error_get_last_description(),
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_invocation_free(invocation);
        return NULL;
    }

    return invocation;
}

static struct s0_block *
s0_load_block(struct s0_yaml_node node)
{
    struct s0_yaml_node  item;
    struct s0_environment_type  *inputs;
    struct s0_environment_type  *type;
    struct s0_statement_list  *statements;
    struct s0_invocation  *invocation;

    ensure_mapping(node, "block");

    /* inputs */

    item = s0_yaml_node_mapping_get(node, "inputs");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "Block requires a inputs at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        return NULL;
    }

    inputs = s0_load_environment_type(item);
    if (unlikely(inputs == NULL)) {
        return NULL;
    }

    /* As we parse the statements and invocation, we'll use `type` to ensure
     * that each one is well-typed.  (We can't use `inputs` directly, because
     * we'll modify `type` as we use it to type-check the statements and
     * invocations.) */
    type = s0_environment_type_new_copy(inputs);
    if (unlikely(type == NULL)) {
        s0_environment_type_free(inputs);
        return NULL;
    }

    /* statements */

    item = s0_yaml_node_mapping_get(node, "statements");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "Block requires a statements at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_environment_type_free(inputs);
        s0_environment_type_free(type);
        return NULL;
    }

    statements = s0_load_statement_list(item, type);
    if (unlikely(statements == NULL)) {
        s0_environment_type_free(inputs);
        s0_environment_type_free(type);
        return NULL;
    }

    /* invocation */

    item = s0_yaml_node_mapping_get(node, "invocation");
    if (unlikely(s0_yaml_node_is_missing(item))) {
        fill_error(node.stream, "Block requires a invocation at %zu:%zu",
                   s0_yaml_node_get_node(node)->start_mark.line,
                   s0_yaml_node_get_node(node)->start_mark.column);
        s0_environment_type_free(inputs);
        s0_statement_list_free(statements);
        s0_environment_type_free(type);
        return NULL;
    }

    invocation = s0_load_invocation(item, type);
    if (unlikely(invocation == NULL)) {
        s0_environment_type_free(inputs);
        s0_statement_list_free(statements);
        s0_environment_type_free(type);
        return NULL;
    }

    s0_environment_type_free(type);
    return s0_block_new(inputs, statements, invocation);
}

static struct s0_entity *
s0_load_module(struct s0_yaml_node root)
{
    struct s0_block  *block;
    struct s0_environment  *env;
    struct s0_named_blocks  *blocks;
    struct s0_name  *name;

    block = s0_load_block(root);
    if (unlikely(block == NULL)) {
        return NULL;
    }

    env = s0_environment_new();
    blocks = s0_named_blocks_new();
    name = s0_name_new_str("module");
    if (unlikely(s0_named_blocks_add(blocks, name, block))) {
        s0_environment_free(env);
        s0_named_blocks_free(blocks);
        return NULL;
    }

    return s0_closure_new(env, blocks);
}

struct s0_yaml_node
s0_yaml_stream_parse_document(struct s0_yaml_stream *stream)
{
    struct s0_yaml_node  result;

    if (stream->document_created) {
        yaml_document_delete(&stream->document);
    }

    if (unlikely(stream->fp == NULL && stream->filename != NULL)) {
        stream->fp = fopen(stream->filename, "r");
        if (stream->fp == NULL) {
            fill_error(stream, "Cannot open %s: %s",
                       stream->filename, strerror(errno));
            stream->document_created = false;
            result.node = S0_YAML_NODE_ERROR;
            return result;
        }
        stream->should_close_fp = true;
        yaml_parser_set_input_file(&stream->parser, stream->fp);
    }

    if (unlikely(!yaml_parser_load(&stream->parser, &stream->document))) {
        fill_error(stream, "YAML error at %zu:%zu: %s",
                   stream->parser.problem_mark.line,
                   stream->parser.problem_mark.column,
                   stream->parser.problem);
        stream->document_created = false;
        result.node = S0_YAML_NODE_ERROR;
        return result;
    }

    result.stream = stream;
    result.node = yaml_document_get_root_node(&stream->document);
    stream->document_created = true;
    return result;
}

struct s0_entity *
s0_yaml_document_parse_module(struct s0_yaml_node node)
{
    return s0_load_module(node);
}

struct s0_entity_type *
s0_yaml_document_parse_entity_type(struct s0_yaml_node node)
{
    return s0_load_entity_type(node);
}

struct s0_environment_type *
s0_yaml_document_parse_environment_type(struct s0_yaml_node node)
{
    return s0_load_environment_type(node);
}
