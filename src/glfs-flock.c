/**
 * A utility to request for full file advisory lock on a file from remote Gluster volume.
 *
 * Copyright (C) 2016 Red Hat Inc.
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

#include <config.h>

#include "glfs-flock.h"
#include "glfs-util.h"

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUTHORS "Written by Anoop C S."

/**
 * Used to store the state of the program, including user supplied options.
 *
 * path         : Path used to find the remote file or directory (supplied by user).
 * block        : Whether to block the lock request in case of a conflicting lock.
 * debug        : Whether to log additional debug information.
 * l_type       : Stores the requested lock type.
 */
struct state {
        char *path;
        bool block;
        bool debug;
        short l_type;
};

static struct state *state;

static struct option const long_options[] =
{
        {"exclusive", no_argument, NULL, 'e'},
        {"help", no_argument, NULL, 'x'},
        {"nonblock", no_argument, NULL, 'n'},
        {"shared", no_argument, NULL, 's'},
        {"unlock", no_argument, NULL, 'u'},
        {"version", no_argument, NULL, 'v'},
        {NULL, no_argument, NULL, 0}
};

static void
usage ()
{
        printf ("Usage: %s [OPTION]... URL\n"
                "Request a full file advisory lock on files from a remote Gluster volume.\n\n"
                "  -e, --exclusive              Obtain an exclusive lock. This is the default.\n"
                "      --help                   Display this help and exit.\n"
                "  -n, --nonblock               Fail rather than wait if the lock cannot be immediately acquired.\n"
                "  -s, --shared                 Obtain a shared lock.\n"
                "  -u, --unlock                 Drop  a  lock.\n"
                "  -v, --version                Output version information and exit\n\n"
                "Examples:\n"
                "  gfcli (localhost/groot)> flock /file\n"
                "       In the context of a shell with a connection established, request full file\n"
                "       lock on a file present under root of the Gluster volume groot on localhost.\n",
                program_invocation_name);
}

static int
parse_options (int argc, char *argv[])
{
        int ret = -1;
        int opt = 0;
        int option_index = 0;

        optind = 0;
        while (true) {
                opt = getopt_long (argc, argv, "ensuv", long_options,
                                   &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'e':
                                state->l_type = F_WRLCK;
                                break;
                        case 'n':
                                state->block = false;
                                break;
                        case 's':
                                state->l_type = F_RDLCK;
                                break;
                        case 'u':
                                state->l_type = F_UNLCK;
                                break;
                        case 'v':
                                printf ("%s (%s) %s\n%s\n%s\n%s\n",
                                        program_invocation_name,
                                        PACKAGE_NAME,
                                        PACKAGE_VERSION,
                                        COPYRIGHT,
                                        LICENSE,
                                        AUTHORS);
                                ret = -2;
                                goto out;
                        case 'x':
                                usage ();
                                ret = -2;
                                goto out;
                        default:
                                goto err;
                }
        }

        if (argc <= optind) {
                usage ();
                ret = -2;
                goto out;
        } else {
                state->path = strdup (argv[argc - 1]);
                if (state->path == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }
                ret = 0;
        }

        goto out;

err:
        error (0, 0, "Try --help for more information.");
out:
        return ret;
}

static struct state*
init_state ()
{
        struct state *state = malloc (sizeof (*state));

        if (state == NULL) {
                goto out;
        }

        state->debug = false;
        state->path = NULL;
        state->block = true;
        state->l_type = F_WRLCK;

out:
        return state;
}

static char *
format_file_path (char *str)
{
        char *last_char;

        while (*str == '/')
                str++;

        if (*str == '\0')
                return str;

        last_char = str + strlen (str) - 1;

        while (last_char > str && *last_char == '/')
                last_char--;

        *(last_char + 1) = '\0';

        return str;
}

static glfs_fd_t *
query_fd_from_path (struct cli_context *ctx, const char *path)
{
        struct fd_list *ptr = NULL;

        ptr = ctx->flist;
        while (ptr) {
                if (strcmp(ptr->path, path) == 0)
                        return ptr->fd;

                ptr = ptr->next;
        }

        return NULL;
}

static int
flock_with_fs (struct cli_context **ctx)
{
        int ret = -1;
        char *tmp_path = NULL;
        glfs_fd_t *fd = NULL;
        struct fd_list *new, *ptr, *cur = NULL;

        if (state->debug)
                glfs_set_logging ((*ctx)->fs, "/dev/stderr", GF_LOG_DEBUG);

        /* Format the file path in such a way that leading and trailing forward
         * slashes are removed for a generic comaprison. */
        tmp_path = strdup (format_file_path (state->path));

        /* Check whether we already have an open fd corresponding to the file
         * on which lock request is issued.*/
        fd = query_fd_from_path (*ctx, tmp_path);
        if (!fd) {
                /* No previous open fd. Create new one and store it to fd_list
                 * for future reference.*/
                new = (struct fd_list *)malloc (sizeof(struct fd_list));
                new->path = NULL;

                new->fd = fd = glfs_open ((*ctx)->fs, state->path, O_RDWR);
                if (!fd) {
                        error (0, errno, "failed to open %s", state->path);
                        ret = -1;
                        goto out;
                }

                new->path = strdup (tmp_path);
                new->next = NULL;

                if ((*ctx)->flist == NULL) {
                        (*ctx)->flist = new;
                } else {
                        ptr = (*ctx)->flist;
                        while (ptr) {
                                cur = ptr;
                                ptr = ptr->next;
                        }

                        cur->next = new;
                }
        }

        /* Request advisory lock. */
        ret = gluster_lock (fd, state->l_type, state->block);
        if (ret)
                error (0, errno, "failed to lock %s", state->path);
out:
        if (tmp_path)
                free (tmp_path);

        return ret;
}

int
do_flock (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int ret = -1;

        state = init_state ();
        if (state == NULL) {
                error (0, errno, "failed to initialize state");
                ret = -1;
                goto out;
        }

        state->debug = ctx->options->debug;

        if (ctx->fs) {
                ret = parse_options (argc, argv);
                if (ret != 0) {
                        goto out;
                }

                ret = flock_with_fs (&ctx);
        } else {
                /* flock can only be invoked within a Gluster remote shell
                 * connected to Gluster volume. Or else we return error. */
                printf ("Client not connected to remote Gluster volume. Use "
                        "connect command to do so and try flock again.\n");
        }

out:
        if (state) {
                if (state->path) {
                        free (state->path);
                }
                free (state);
        }

        return ret;
}
