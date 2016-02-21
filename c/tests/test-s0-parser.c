/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include <assert.h>

#include "ccan/likely/likely.h"
#include "ccan/tap/tap.h"
#include "directory-walker.h"
#include "swanson.h"
#include "yaml.h"


/* First we walk through the requested directory, looking for YAML files.  We
 * create a "test case file" descriptor for each one. */

struct test_case_file {
    struct test_case_file  *next;
    const char  *full_path;
    const char  *rel_path;
};

static struct test_case_file  *test_case_files = NULL;
static size_t  test_case_file_count = 0;

static void
test_case_file_new(const char *full_path, const char *rel_path)
{
    struct test_case_file  *test_case_file =
        malloc(sizeof(struct test_case_file));
    assert(test_case_file != NULL);
    test_case_file->full_path = strdup(full_path);
    assert(test_case_file->full_path != NULL);
    test_case_file->rel_path = strdup(rel_path);
    assert(test_case_file->rel_path != NULL);
    test_case_file->next = test_case_files;
    test_case_files = test_case_file;
    test_case_file_count++;
}

static void
test_case_file_free(struct test_case_file *test_case_file)
{
    free((void *) test_case_file->full_path);
    free((void *) test_case_file->rel_path);
    free(test_case_file);
}

static void
create_test_case(int child_fd, const char *full_path, const char *rel_path,
                 void *user_data)
{
    test_case_file_new(full_path, rel_path);
}

static void
load_test_cases(const char *directory)
{
    walk_directory(directory, create_test_case, NULL);
}


/* Then we walk through each test case file, counting the number of total test
 * cases that are in them. */

static unsigned int  test_case_count = 0;

static void
count_test_cases(void)
{
    struct test_case_file  *curr;
    for (curr = test_case_files; curr != NULL; curr = curr->next) {
        struct s0_yaml_stream  *stream;
        struct s0_yaml_node  node;

        stream = s0_yaml_stream_new_from_filename(curr->full_path);
        assert(stream != NULL);

        for (node = s0_yaml_stream_parse_document(stream);
             s0_yaml_node_is_valid(node);
             node = s0_yaml_stream_parse_document(stream)) {
            test_case_count++;
        }

        if (unlikely(s0_yaml_node_is_error(node))) {
            diag("Error reading from %s: %s", curr->rel_path,
                 s0_yaml_stream_last_error(stream));
            exit(EXIT_FAILURE);
        }

        s0_yaml_stream_free(stream);
    }
}


/* And then we can finally run them! */

#define SUCCESSFUL_PARSE_TAG  SWANSON_TAG_PREFIX "successful-parse"
#define INVALID_PARSE_TAG     SWANSON_TAG_PREFIX "invalid-parse"

static void
run_test_cases(void)
{
    struct test_case_file  *curr;
    for (curr = test_case_files; curr != NULL; curr = curr->next) {
        struct s0_yaml_stream  *stream;
        struct s0_yaml_node  node;

        diag(curr->rel_path);
        stream = s0_yaml_stream_new_from_filename(curr->full_path);
        assert(stream != NULL);

        for (node = s0_yaml_stream_parse_document(stream);
             s0_yaml_node_is_valid(node);
             node = s0_yaml_stream_parse_document(stream)) {
            struct s0_yaml_node  name;
            struct s0_yaml_node  module;

            if (unlikely(!s0_yaml_node_is_mapping(node))) {
                diag("Expected a YAML mapping");
                exit(EXIT_FAILURE);
            }

            name = s0_yaml_node_mapping_get(node, "name");
            if (unlikely(s0_yaml_node_is_missing(name))) {
                diag("Test case must have a name");
                exit(EXIT_FAILURE);
            }

            if (unlikely(!s0_yaml_node_is_scalar(name))) {
                diag("Test case name must be a scalar");
                exit(EXIT_FAILURE);
            }

            module = s0_yaml_node_mapping_get(node, "module");
            if (unlikely(s0_yaml_node_is_missing(module))) {
                diag("Test case must have a module");
                exit(EXIT_FAILURE);
            }

            if (s0_yaml_node_is_mapping(module)) {
                if (s0_yaml_node_has_tag(node, SUCCESSFUL_PARSE_TAG)) {
                    struct s0_entity  *entity =
                        s0_yaml_document_parse_module(module);
                    if (entity == NULL) {
                        fail("%.*s",
                             (int) s0_yaml_node_scalar_size(name),
                             s0_yaml_node_scalar_content(name));
                        diag("    Unexpected error: %s",
                             s0_yaml_stream_last_error(stream));
                    } else {
                        pass("%.*s",
                             (int) s0_yaml_node_scalar_size(name),
                             s0_yaml_node_scalar_content(name));
                        s0_entity_free(entity);
                    }
                } else if (s0_yaml_node_has_tag(node, INVALID_PARSE_TAG)) {
                    struct s0_entity  *entity =
                        s0_yaml_document_parse_module(module);
                    if (entity == NULL) {
                        pass("%.*s",
                             (int) s0_yaml_node_scalar_size(name),
                             s0_yaml_node_scalar_content(name));
                    } else {
                        fail("%.*s",
                             (int) s0_yaml_node_scalar_size(name),
                             s0_yaml_node_scalar_content(name));
                        diag("    Unexpected successful parse");
                        s0_entity_free(entity);
                    }
                } else {
                    diag("Test case has unknown tag");
                    exit(EXIT_FAILURE);
                }
            } else if (s0_yaml_node_is_scalar(module)) {
                if (s0_yaml_node_has_tag(node, INVALID_PARSE_TAG)) {
                    pass("NIY %.*s",
                         (int) s0_yaml_node_scalar_size(name),
                         s0_yaml_node_scalar_content(name));
                } else {
                    diag("Test case module can only be scalar "
                         "for an invalid parse");
                    exit(EXIT_FAILURE);
                }
            } else {
                diag("Test case module must be a scalar or mapping");
                exit(EXIT_FAILURE);
            }
        }

        if (unlikely(s0_yaml_node_is_error(node))) {
            diag("Error reading from %s: %s", curr->rel_path,
                 s0_yaml_stream_last_error(stream));
            exit(EXIT_FAILURE);
        }

        s0_yaml_stream_free(stream);
    }
}


int
main(int argc, char **argv)
{
    int  i;
    for (i = 1; i < argc; i++) {
        load_test_cases(argv[i]);
    }

    count_test_cases();
    plan_tests(test_case_count);
    run_test_cases();

    struct test_case_file  *curr;
    struct test_case_file  *next;
    for (curr = test_case_files; curr != NULL; curr = next) {
        next = curr->next;
        test_case_file_free(curr);
    }

    return 0;
}
