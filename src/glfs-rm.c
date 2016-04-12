/**
 * A utility to remove a file or directory from a remote Gluster volume.
 *
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

#include <config.h>

#include "glfs-rm.h"
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

#define AUTHORS "Written by Craig Cabrey."

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_url: Struct of the parsed url supplied by the user.
 * url: Full url used to find the remote file or directory (supplied by user).
 * debug: Whether to log additional debug information.
 * directory: Whether to allow removal of a directory.
 * force: Whether to ignore non-existent files or directories.
 */
struct state {
        struct gluster_url *gluster_url;
        struct xlator_option *xlator_options;
        char *url;
        bool debug;
        bool directory;
        bool force;
};

static struct state *state;

static struct option const long_options[] =
{
        {"debug", no_argument, NULL, 'd'},
        {"force", no_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'x'},
        {"port", required_argument, NULL, 'p'},
        {"recursive", no_argument, NULL, 'r'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage ()
{
        printf ("Usage: %s [OPTION]... URL\n"
                "Remove (unlink) the files (or directories) from a remote Gluster volume.\n\n"
                "  -f, --force                  ignore nonexistent files, never prompt\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "  -r, --recursive              remove directories and their contents recursively\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gfrm glfs://localhost/groot/path/to/file\n"
                "       Remove the file /path/to/file on the Gluster\n"
                "       volume of groot on host localhost.\n"
                "  gfrm -r glfs://localhost/groot/path/to/directory\n"
                "       Recursively remove the directory /path/to/directory\n"
                "       on the Gluster volume of groot on host localhost.\n"
                "  gfcli (localhost/groot)> rm /file\n"
                "       In the context of a shell with a connection established,\n"
                "       remove the file on the root of the Gluster volume groot\n"
                "       on localhost.\n",
                program_invocation_name);
}

static int
parse_options (int argc, char *argv[], bool has_connection)
{
        uint16_t port = GLUSTER_DEFAULT_PORT;
        int ret = -1;
        int opt = 0;
        int option_index = 0;
        struct xlator_option *option;

        // Reset getopt since other utilities may have called it already.
        optind = 0;
        while (true) {
                opt = getopt_long (argc, argv, "fro:p:", long_options,
                                   &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'd':
                                state->debug = true;
                                break;
                        case 'f':
                                state->force = true;
                                break;
                        case 'o':
                                option = parse_xlator_option (optarg);
                                if (option == NULL) {
                                        error (0, errno, "%s", optarg);
                                        goto err;
                                }

                                if (append_xlator_option (&state->xlator_options, option) == -1) {
                                        error (0, errno, "append_xlator_option: %s", optarg);
                                        goto err;
                                }

                                break;
                        case 'p':
                                port = strtoport (optarg);
                                if (port == 0) {
                                        goto out;
                                }

                                break;
                        case 'r':
                                state->directory = true;
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

        if ((argc - option_index) < 2) {
                error (0, 0, "missing operand");
                goto err;
        } else {
                // state->url is free'd in do_rm()
                state->url = strdup (argv[argc - 1]);
                if (state->url == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

                // state->gluster_url is free'd in do_rm()
                if (has_connection) {
                        state->gluster_url = gluster_url_init ();
                        if (state->gluster_url == NULL) {
                                error (0, errno, "gluster_url_init");
                                goto out;
                        }

                        state->gluster_url->path = strdup (argv[argc - 1]);
                        if (state->gluster_url->path == NULL) {
                                error (0, errno, "strdup");
                                goto out;
                        }

                        ret = 0;
                        goto out;
                }

                ret = gluster_parse_url (argv[argc - 1], &(state->gluster_url));
                if (ret == -1) {
                        error (0, EINVAL, "%s", state->url);
                        goto err;
                }

                state->gluster_url->port = port;
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
        state->directory = false;
        state->force = false;
        state->gluster_url = NULL;
        state->url = NULL;
        state->xlator_options = NULL;

out:
        return state;
}

static int
rm (glfs_t *fs)
{
        int ret = -1;

        if (state->directory) {
                ret = glfs_rmdir (fs, state->gluster_url->path);
        } else {
                ret = glfs_unlink (fs, state->gluster_url->path);
        }

        if (ret == -1) {
                if (state->force && errno == ENOENT) {
                        ret = 0;
                        goto out;
                }

                error (0, errno, "failed to remove `%s'", state->url);
                goto out;
        }

out:
        return ret;
}

static int
rm_without_context ()
{
        glfs_t *fs = NULL;
        int ret = -1;

        ret = gluster_getfs (&fs, state->gluster_url);
        if (ret == -1) {
                error (0, errno, "failed to connect to `%s'", state->url);
                goto out;
        }

        ret = apply_xlator_options (fs, &state->xlator_options);
        if (ret == -1) {
                error (0, errno, "failed to apply translator options");
                goto out;
        }

        if (state->debug) {
                ret = glfs_set_logging (fs, "/dev/stderr", GF_LOG_DEBUG);

                if (ret == -1) {
                        error (0, errno, "failed to set logging level");
                        goto out;
                }
        }

        ret = rm (fs);

out:
        if (fs) {
                glfs_fini (fs);
        }

        return ret;
}

int
do_rm (struct cli_context *ctx)
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
                ret = parse_options (argc, argv, true);
                if (ret != 0) {
                        goto out;
                }

                ret = rm (ctx->fs);
        } else {
                ret = parse_options (argc, argv, false);
                switch (ret) {
                        case -2:
                                // Fall through
                                ret = 0;
                        case -1:
                                goto out;
                }

                ret = rm_without_context ();
        }

out:
        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
        }

        free (state);

        return ret;
}
