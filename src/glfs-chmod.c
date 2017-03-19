/**
 * A utility to create a directory on a remote Gluster volume, with the ability
 * to optionally create nested directories as required.
 *
 * Copyright (C) 2017 RedHat Inc.
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

#include "glfs-chmod.h"
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
#include <sys/stat.h>

#define AUTHORS "Written by Jayadeep KM."

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_url: Struct of the parsed url supplied by the user.
 * url: Full url used to find the remote file (supplied by user).
 * debug: Whether to log additional debug information.
 * recursive: Whether to apply chmod recursively.
 */
struct state {
        struct gluster_url *gluster_url;
        struct xlator_option *xlator_options;
        char *url;
        bool debug;
        bool recursive;
        char *mode;
};

static struct state *state;

static struct option const long_options[] =
{
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"port", required_argument, NULL, 'p'},
        {"recursive", no_argument, NULL, 'R'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage ()
{
        printf ("Usage: %s [OPTION]... MODE URL\n\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the \n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  chmod 777 glfs://localhost/groot/directory\n"
                "          Grant all permissions to all users for directory\n"
                "  chmod 444 glfs://localhost/groot/directory/subdirectory\n"
                "          Grant readonly permission to all users for /directory/subdirectory\n",
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
                opt = getopt_long (argc, argv, "do:p:rwx:Rv", long_options,
                                   &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'd':
                                state->debug = true;
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
                        case 'r': case 'w': case 'x': break;
                        case 'R':
                                state->recursive = true;
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
                        case 'h':
                                usage ();
                                ret = -2;
                                goto out;
                        default:
                                goto err;
                }
        }

        if ((argc - option_index) < 3) {
                error (0, 0, "missing operand");
                goto err;
        } else {
                state->url = strdup (argv[argc - 1]);
                state->mode = strdup (argv[argc - 2]);
                if (state->url == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }
                if (state->mode == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

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

                // gluster_create_path will not create
                // the last directory in a path if that path does not
                // include a trailing slash ('/'), so add one if it does not
                // exist.
                char *path = state->gluster_url->path;
                if (path[strlen (path) - 1] != '/') {
                        strncat (path, "/", strlen (path) + 2);
                }
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
        state->gluster_url = NULL;
        state->url = NULL;
        state->recursive = 0;
        state->mode = NULL;
        state->xlator_options = NULL;

out:
        return state;
}

static int
chmod_with_fs (glfs_t *fs)
{
        int ret;

        mode_t mode = 0;

        size_t octal = 0;
        size_t decimal = 0;
        struct stat *local_stat;

        if(state->mode[0]>='0'&&state->mode[0]<='7'){
                octal = atoi(state->mode);

                while(octal!=0){
                        decimal = decimal*10 + octal%10;
                        octal = octal / 10;
                }
                while(decimal!=0){
                        octal = octal*8 + decimal%10;
                        decimal = decimal / 10;
                }
                mode = octal;
        }else{
                local_stat =(struct stat*) malloc(sizeof(struct stat));
                ret = glfs_stat(fs,state->gluster_url->path,local_stat);
                if(ret == -1)
                        goto err;
                mode = local_stat->st_mode;
                if(state->mode[0]=='+'){
                        switch (state->mode[1]) {
                                case 'r':
                                        mode |= S_IRUSR; break;
                                case 'w':
                                        mode |= S_IWUSR; break;
                                case 'x':
                                        mode |= S_IXUSR; break;
                                default:
                                        ret = -2;
                                        goto err;
                        }
                }else if(state->mode[0]=='-'){
                        switch (state->mode[1]) {
                                case 'r':
                                        mode &= ~S_IRUSR; break;
                                case 'w':
                                        mode &= ~S_IWUSR; break;
                                case 'x':
                                        mode &= ~S_IXUSR; break;
                                default:
                                        ret = -2;
                                        goto err;

                        }
                }else{
                        ret = -2;
                        goto err;
                }
        }


        ret = glfs_chmod(fs,state->gluster_url->path,mode);

err:

        if (ret == -1) {
                error (0, errno, "cannot change permissions `%s'", state->url);
                goto out;
        }

        if (ret == -2) {
                error (0, errno, "Invalid permission mode`%s'", state->url);
                goto out;
        }

out:
        return ret;
}

static int
chmod_without_context ()
{
        glfs_t *fs = NULL;
        int ret = -1;

        ret = gluster_getfs (&fs, state->gluster_url);
        if (ret == -1) {
                error (0, errno, "cannot create directory `%s'", state->url);
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

        ret = chmod_with_fs (fs);

out:
        if (fs) {
                glfs_fini (fs);
        }

        return ret;
}

int
do_chmod (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int ret = -1;

        state = init_state ();
        if (state == NULL) {
                error (0, errno, "failed to initialize state");
                goto out;
        }

        state->debug = ctx->options->debug;

        if (ctx->fs) {
                ret = parse_options (argc, argv, true);
                if (ret != 0) {
                        goto out;
                }

                ret = chmod_with_fs (ctx->fs);
        } else {
                ret = parse_options (argc, argv, false);
                switch (ret) {
                        case -2:
                                // Fall through
                                ret = 0;
                        case -1:
                                goto out;
                }

                ret = chmod_without_context ();
        }

out:
        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
                free (state->mode);
        }

        free (state);

        return ret;
}
