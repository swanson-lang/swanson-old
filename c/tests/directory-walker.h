/* -*- coding: utf-8 -*-
 * Copyright © 2016, Swanson Project.
 * Please see the COPYING file in this distribution for license details.
 */

#ifndef DIRECTORY_WALKER_H
#define DIRECTORY_WALKER_H
#ifdef __cplusplus
extern "C" {
#endif

/* Called for each file found during a directory walk.  The file will already be
 * open for reading via fd.  filename is only valid for the duration of this
 * call. */
typedef void
directory_walker_callback(int fd, const char *full_path, const char *rel_path,
                          void *user_data);

/* Assumes that we're running in a TAP test harness; if there are any I/O
 * errors, prints out an error message using diag() and aborts. */
void
walk_directory(const char *root_path, directory_walker_callback *callback,
               void *user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* DIRECTORY_WALKER_H */
