/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#include "directory-walker.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ccan/likely/likely.h"
#include "ccan/str/str.h"
#include "ccan/tap/tap.h"


/* Process all of the direct child files and subdirectories of dir_fd.
 * walk_path should already contain the full relative path to the directory that
 * dir_fd refers to.  walk_path_end should point to the NUL terminator at the
 * end of that path. */
static void
walk_directory_fd(int dir_fd, char *walk_path, char *walk_rel_path,
                  char *walk_path_end, directory_walker_callback *callback,
                  void *user_data)
{
    DIR  *dir;
    struct dirent  *dirent;
    struct stat  child_stat;

    dir = fdopendir(dir_fd);
    if (unlikely(dir == NULL)) {
        diag("Cannot open directory: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while ((dirent = readdir(dir)) != NULL) {
        int  child_fd;
        char  *child_walk_path_end;

        if (streq(dirent->d_name, ".") || streq(dirent->d_name, "..")) {
            continue;
        }

        /* Append the child's base name to the directory's path, yielding the
         * full relative path to the child. */
        child_walk_path_end = stpcpy(walk_path_end, dirent->d_name);

        /* Open up the child to see whether it's a file or directory. */
        child_fd = openat(dir_fd, dirent->d_name, O_RDONLY);
        if (child_fd == -1) {
            diag("Cannot open file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fstat(child_fd, &child_stat) == -1) {
            diag("Cannot stat file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* If the child is a directory, recurse into it.  Otherwise call the
         * callback. */
        if (S_ISDIR(child_stat.st_mode)) {
            *child_walk_path_end++ = '/';
            walk_directory_fd
                (child_fd, walk_path, walk_rel_path, child_walk_path_end,
                 callback, user_data);
            close(child_fd);
        } else if (strends(dirent->d_name, ".yaml")) {
            callback(child_fd, walk_path, walk_rel_path, user_data);
            close(child_fd);
        }
    }

    closedir(dir);
}

void
walk_directory(const char *root_path, directory_walker_callback *callback,
               void *user_data)
{
    int  dir_fd;
    char  walk_path[PATH_MAX];
    char  *walk_rel_path;
    char  *walk_path_end;

    assert(root_path != NULL && *root_path != '\0');

    dir_fd = open(root_path, O_RDONLY);
    if (dir_fd == -1) {
        diag("Cannot open directory: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* After this block, walk_path will contain a copy of root_path, and is
     * guaranteed to end with a slash.  walk_rel_path and walk_path_end will
     * point at the NUL terminator.  walk_rel_path will always point at that
     * same character, even as the filenames are being filled in, which causes
     * walk_rel_path to always point at the relative path (underneath root_path)
     * of the current file.  walk_path_end will be updated so that it always
     * points at the NUL terminator, so that each time we recurse further down
     * into the subdirectories, we always know exactly where to copy the next
     * filename. */
    walk_path_end = stpcpy(walk_path, root_path);
    if (walk_path_end[-1] != '/') {
        *walk_path_end++ = '/';
        *walk_path_end = '\0';
    }
    walk_rel_path = walk_path_end;

    walk_directory_fd
        (dir_fd, walk_path, walk_rel_path, walk_path_end, callback, user_data);
    close(dir_fd);
}
