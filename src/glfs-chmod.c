/**
 * A utility to read a file from a remote Gluster volume and stream it to
 * stdout.
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

#include "glfs-cat.h"
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
 * url: Full url used to find the remote file (supplied by user).
 * debug: Whether to log additional debug information.
 */
struct state {
    struct gluster_url *gluster_url;
    struct xlator_option *xlator_options;
    char *url;
    bool debug;
    char* mode;
};

static struct state *state;

static struct option const long_options[] =
        {
                {"debug", no_argument, NULL, 'd'},
                {"help", no_argument, NULL, 'x'},
                {"port", required_argument, NULL, 'p'},
                {"version", no_argument, NULL, 'v'},
                {"xlator-option", required_argument, NULL, 'o'},
                {NULL, no_argument, NULL, 0}
        };

static int
gluster_chmod (glfs_t *fs, const char *filename) {
    glfs_fd_t *fd = NULL;
    int ret = -1;
    struct stat statbuf;
    ret = glfs_stat (fs, state->gluster_url->path, &statbuf);
    if (ret == -1) {
        error (0, errno, "cannot open `%s' for reading", state->url);
        goto out;
    }
    mode_t current_mode = statbuf.st_mode;
    char *mode = state->mode;
    int size;
    mode_t numeric_mode = current_mode;
    for(size=0;mode[size]!='\0';++size);
    if(size==3 && mode[0] >='0' && mode[0] <= '9'){
        numeric_mode = strtol(mode,NULL,8);//((mode[0]-'0') * 100) + ((mode[1]-'0') *10) + (mode[2] - '0');
    }
    else{

        if(mode[1]=='+'){
            for(int i=2; i<size;++i){
                if(mode[0]=='a'){

                }
            }
        }
    }


    printf("%d",numeric_mode);
    ret = glfs_chmod(fs,filename,numeric_mode);

    ret = 0;

    out:
    if (fd) {
        if (glfs_close (fd) == -1) {
            ret = -1;
            error (0, errno, "cannot close file %s",
                   state->gluster_url->path);
        }
    }

    return ret;
}

static void
usage ()
{
    printf ("Usage: %s [OPTION]... URL\n"
                    "Read a file on a remote Gluster volume and write it to standard output.\n\n"
                    "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                    "                               connection. Multiple options are supported\n"
                    "                               and take the form xlator.key=value.\n"
                    "  -p, --port=PORT              specify the port on which to connect\n"
                    "      --help     display this help and exit\n"
                    "      --version  output version information and exit\n\n"
                    "Examples:\n"
                    "  gfcat glfs://localhost/groot/path/to/file\n"
                    "        Write the contents of /path/to/file on the Gluster volume\n"
                    "        of groot on host localhost to standard output.\n"
                    "  gfcli (localhost/groot)> cat /file\n"
                    "        In the context of a shell with a connection established,\n"
                    "        cat the file on the root of the Gluster volume groot\n"
                    "        on localhost.\n",
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

    if (argc < 3) {
        error (0, 0, "missing operand");
        goto err;
    } else {
        state->mode = argv[argc-2];
        state->url = strdup (argv[argc - 1]);
        if (state->url == NULL) {
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
    state->xlator_options = NULL;

    out:
    return state;
}

static int
chmod_without_context ()
{
    glfs_t *fs = NULL;
    int ret = -1;

    ret = gluster_getfs (&fs, state->gluster_url);
    if (ret == -1) {
        error (0, errno, "%s", state->url);
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

    ret = gluster_chmod (fs, state->gluster_url->path);
    if (ret == -1) {
        goto out;
    }

    ret = 0;

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
    int ret = EXIT_FAILURE;

    state = init_state ();
    if (state == NULL) {
        error (0, errno, "failed to initialize state");
        goto out;
    }

    if (ctx->fs) {
        ret = parse_options (argc, argv, true);
        if (ret != 0) {
            goto out;
        }

        ret = gluster_chmod (ctx->fs, state->gluster_url->path);
    } else {
        state->debug = ctx->options->debug;
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
    }

    free (state);

    return ret;
}
