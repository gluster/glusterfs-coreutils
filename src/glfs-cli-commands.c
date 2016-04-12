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

#include <config.h>

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glfs-cli-commands.h"
#include "glfs-cli.h"
#include "glfs-util.h"

static struct option const connect_options[] =
{
        {"port", required_argument, NULL, 'p'},
        {NULL, no_argument, NULL, 0}
};

int
cli_connect (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int ret;
        int opt;
        int option_index;
        struct gluster_url *url = NULL;
        glfs_t *fs = NULL;
        uint16_t port = GLUSTER_DEFAULT_PORT;
        struct xlator_option *xlator_options = NULL;
        struct xlator_option *option;

        while ((opt = getopt_long (argc, argv, "o:p:", connect_options, &option_index)) != -1) {
                switch (opt) {
                        case 'o':
                                option = parse_xlator_option (optarg);
                                if (option == NULL) {
                                        error (0, errno, "%s", optarg);
                                        goto err;
                                }

                                if (append_xlator_option (&xlator_options, option) == -1) {
                                        error (0, errno, "append_xlator_option: %s", optarg);
                                        goto err;
                                }

                                break;
                        case 'p':
                                port = strtoport (optarg);
                                if (port == 0) {
                                        goto err;
                                }

                                break;
                        default:
                                goto err;
                }
        }

        ret = gluster_parse_url (argv[argc - 1], &url);
        if (ret == -1) {
                printf ("Usage: %s [OPTION]... URL\n"
                        "Connect to a Gluster volume for this session.\n\n"
                        "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                        "                               connection. Multiple options are supported\n"
                        "                               and take the form xlator.key=value.\n"
                        "  -p, --port=PORT              specify the port on which to connect\n",
                        argv[0]);
                goto err;
        }

        url->port = port;

        ret = gluster_getfs (&fs, url);
        if (ret == -1) {
                error (0, errno, "failed to connect to %s/%s", url->host, url->volume);
                goto err;
        }

        ret = apply_xlator_options (fs, &xlator_options);
        if (ret == -1) {
                error (0, errno, "failed to apply translator options");
                goto err;
        }

        ret = cli_disconnect (ctx);
        if (ret == -1) {
                error (0, 0, "failed to terminate previous connection");
                goto err;
        }

        ctx->fs = fs;
        ctx->url = url;

        // TODO(craigcabrey): Look into using asprintf here.
        // 5 is the length of the string format: (%s/%s)
        size_t length = strlen (ctx->url->host) + strlen (ctx->url->volume) + 5;

        ctx->conn_str = malloc (length);
        if (ctx->conn_str == NULL) {
                error (0, errno, "malloc");
                handle_quit (ctx);
        }

        snprintf (ctx->conn_str,
                        length,
                        "(%s/%s)",
                        ctx->url->host,
                        ctx->url->volume);

        goto out;

err:
        ret = -1;
        gluster_url_free (url);
out:
        return ret;
}

int
cli_disconnect (struct cli_context *ctx)
{
        int ret = 0;
        struct fd_list *cur, *ptr = NULL;

        free_xlator_options (&ctx->options->xlator_options);

        /* Traverse fd_list and cleanup each entry.*/
        ptr = ctx->flist;
        while (ptr) {
                if (ptr->fd) {
                        glfs_close (ptr->fd);
                        ptr->fd = NULL;
                }

                if (ptr->path) {
                        free (ptr->path);
                        ptr->path = NULL;
                }

                cur = ptr;
                ptr = ptr->next;
                free (cur);
        }

        if (ctx->fs) {
                // FIXME: Memory leak occurs here in GFS >= 3.6. Test with 3.7
                // and if fixed, remove the entry (xlator_mem_acct_init) from
                // the valgrind suppression file.
                ret = glfs_fini (ctx->fs);
                ctx->fs = NULL;
        }

        if (ctx->url) {
                gluster_url_free (ctx->url);
                ctx->url = NULL;
        }

        free (ctx->conn_str);
        ctx->conn_str = NULL;

        return ret;
}

int
handle_quit (struct cli_context *ctx)
{
        cli_disconnect (ctx);

        if (ctx->argv) {
                free (ctx->argv);
        }

        free (ctx->options);
        free (ctx);

        exit (EXIT_SUCCESS);
}

int
not_implemented (struct cli_context *ctx)
{
        printf ("%s: not yet implemented\n", ctx->argv[0]);
        return 0;
}
