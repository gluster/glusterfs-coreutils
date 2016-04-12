/**
 * Copyright (C) 2015 Facebook Inc.
 *
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GLFS_UTIL_H
#define GLFS_UTIL_H

// FIXME: Logging levels should be part of the API
#define GF_LOG_DEBUG 8

#define BUFSIZE 256*1024
#define GLUSTER_DEFAULT_PORT 24007

#include <glusterfs/api/glfs.h>
#include <stdint.h>
#include <stdbool.h>

struct gluster_url {
        char *host;
        char *path;
        char *volume;
        uint16_t port;
};

struct xlator_option {
        struct xlator_option *next;
        char *key;
        char *opt_string;
        char *value;
        char *xlator;
};

int
append_xlator_option (struct xlator_option **options, struct xlator_option *option);

char *
append_path (const char *base_path, const char *hanging_path);

int
apply_xlator_options (glfs_t *fs, struct xlator_option **options);

void
close_stdout ();

void
free_xlator_options (struct xlator_option **options);

void
gluster_url_free (struct gluster_url *gluster_url);

mode_t
get_default_dir_mode_perm ();

mode_t
get_default_file_mode_perm ();

int
gluster_create_path (glfs_t *fs, char *path, mode_t omode);

int
gluster_lock (glfs_fd_t *fd, short type, bool block);

int
gluster_write (int src, glfs_fd_t *fd);

int
gluster_read (glfs_fd_t *fd, int dst);

int
gluster_getfs (glfs_t **fs, const struct gluster_url *gluster_url);

int
gluster_parse_url (char *url, struct gluster_url **gluster_url);

struct gluster_url*
gluster_url_init ();

struct xlator_option *
parse_xlator_option (const char *optarg);

void
print_xlator_options (struct xlator_option **options);

uint16_t
strtoport (const char *str);

#endif
