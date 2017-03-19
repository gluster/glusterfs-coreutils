/**
 * A utility to read a file from a remote Gluster volume and stream it to
 * stdout.
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

#include "glfs-truncate.h"
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

#define AUTHORS "Written by Moonblade."

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
        off_t size;
        bool debug;
};

static struct state *state;

static struct option const long_options[] =
{
        {"size", required_argument, 0, 's'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'x'},
        {"port", required_argument, NULL, 'p'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage()
{
        printf ("Usage: %s [OPTION]... URL\n"
                "Read a file on a remote Gluster volume and write it to standard output.\n\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "      --size=SIZE              set or adjust the file size by SIZE bytes\n"
                "                               size is integer and optional unit (Eg: 10, 10K, 10KB)\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gftruncate glfs://localhost/groot/path/to/file\n"
                "        Truncate /path/to/file on the Gluster volume\n"
                "  gfcli (localhost/groot)> cat /file\n"
                "        In the context of a shell with a connection established,\n"
                "        Truncate the file on the gluster volume.\n",
                program_invocation_name);
}

off_t
parse_size(char *optarg)
{
        off_t size = 0;
        int argLen = strlen(optarg);
        char *ptr;
        if(argLen<1)
        {
                return -1;
        }
        if(optarg[argLen-1]=='B')
        {
                if(argLen<2)
                        return -1;
                char *number_alone = malloc(sizeof((argLen-2)*sizeof(char)));
                memcpy(number_alone, optarg, argLen-2);
                off_t pureNo = strtol(number_alone, &ptr, 10);
                if (pureNo == EINVAL)
                        return -1;
                off_t multiplier = 1;
                switch(optarg[argLen-2])
                {
                        case 'K':
                                multiplier = 1000;
                                break;
                        case 'M':
                                multiplier = 1000 * 1000;
                                break;
                        case 'G':
                                multiplier = 1000 * 1000 * 1000;
                                break;
                        default:
                                return -1;
                }
                size = pureNo * multiplier;
                return size;
        }
        off_t multiplier = 1;
        switch(optarg[argLen-1])
        {
                case 'K':
                        multiplier = 1024;
                        break;
                case 'M':
                        multiplier = 1024 * 1024;
                        break;
                case 'G':
                        multiplier = 1024 * 1024 * 1024;
                        break;
        }
        char *number_alone;
        switch(optarg[argLen-1])
        {
                case 'K':
                case 'M':
                case 'G':
                        number_alone = malloc(sizeof((argLen-1)*sizeof(char)));
                        memcpy(number_alone, optarg, argLen-1);
                        off_t pureNo = strtol(number_alone, &ptr, 10);
                        if (pureNo == EINVAL)
                                return -1;
                        size = pureNo * multiplier;
                        return size;
                        break;
                default:
                        size = strtol(optarg, &ptr, 10);
                        if (size == EINVAL)
                                return -1;
                        return size;
        }
        return size;
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
        int hasSize = 0;
        while (true) {
                opt = getopt_long (argc, argv, "do:p:", long_options,
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
                        case 's':
                                hasSize = 1;
                                state->size = parse_size(optarg);
                                // printf("size : %ld\n", state->size);
                                if(state->size<0)
                                {
                                        error(0, 0, "Invalid size");
                                        goto err;
                                }
                                break;
                        default:
                                goto err;
                }
        }
        if(!hasSize)
        {
                error(0 , 0, "Please specify --size argument");
                goto err;
        }

        if ((argc - option_index) < 2) {
                error (0, 0, "missing operand");
                goto err;
        } else {
                state->url = strdup (argv[argc - 1]);
                if (state->url == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

                if (has_connection) {
                        state->gluster_url = gluster_url_init();
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
                ret = gluster_parse_url(argv[argc - 1], &(state->gluster_url));
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
        state->size = 0;
        state->xlator_options = NULL;

out:
        return state;
}

gluster_get (glfs_t *fs, const char *filename) {
        glfs_fd_t *fd = NULL;
        int ret = -1;

        fd = glfs_open (fs, filename, O_RDONLY);
        if (fd == NULL) {
                error (0, errno, "%s", state->url);
                goto out;
        }

        ret = gluster_lock (fd, F_WRLCK, false);
        if (ret == -1) {
                error (0, errno, "%s", state->url);
                goto out;
        }

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


truncate_without_context ()
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
        ret = glfs_stat (fs, state->gluster_url->path, NULL);
        if (ret != 0)
        {
                glfs_fd_t *fd = glfs_creat(fs, state->gluster_url->path, 0, 0777);
                if (fd == NULL)
                {
                        error(0, 0, "Error creating new file");
                        goto out;
                }
        }
        ret = gluster_get (fs, state->gluster_url->path);
        if (ret == -1) {
                goto out;
        }

        ret = 0;
        ret = glfs_truncate(fs, state->gluster_url->path, state->size);

out:
        if (fs) {
                glfs_fini (fs);
        }

        return ret;
}


int
do_truncate (struct cli_context *ctx)
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
                ret = glfs_stat (ctx->fs, state->gluster_url->path, NULL);
                if (ret != 0)
                {
                        glfs_fd_t *fd = glfs_creat(ctx->fs, state->gluster_url->path, 0, 0777);
                        if (fd == NULL)
                        {
                                error(0, 0, "Error creating new file");
                                goto out;
                        }
                }
                ret = glfs_truncate(ctx->fs, state->gluster_url->path, state->size);
        }else {
                state->debug = ctx->options->debug;
                ret = parse_options (argc, argv, false);
                switch (ret) {
                        case -2:
                                // Fall through
                                ret = 0;
                        case -1:
                                goto out;
                }

                ret = truncate_without_context ();
        }


out:
        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
        }

        free (state);

        return ret;
}
