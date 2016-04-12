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

#ifndef GLFS_CLI_H
#define GLFS_CLI_H

#include "glfs-util.h"

#include <glusterfs/api/glfs.h>
#include <stdbool.h>

struct fd_list {
        struct fd_list *next;
        glfs_fd_t *fd;
        char *path;
};

struct cli_context {
        glfs_t *fs;
        struct fd_list *flist;
        struct gluster_url *url;
        struct options *options;
        char *conn_str;
        int argc;
        bool in_shell;
        char **argv;
};

struct options {
        struct xlator_option *xlator_options;
        bool debug;
};

#endif
