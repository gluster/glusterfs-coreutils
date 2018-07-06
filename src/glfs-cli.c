/**
 * Entry point of all utilities except for put. Also acts as an interactive
 * shell when invoked directly.
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

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "glfs-cli.h"
#include "glfs-cat.h"
#include "glfs-clear.h"
#include "glfs-cp.h"
#include "glfs-cli-commands.h"
#include "glfs-flock.h"
#include "glfs-ls.h"
#include "glfs-mkdir.h"
#include "glfs-truncate.h"
#include "glfs-touch.h"
#include "glfs-rm.h"
#include "glfs-stat.h"
#include "glfs-tail.h"
#include "glfs-util.h"
#include "glfs-rmdir.h"
#include "glfs-mv.h"

#define AUTHORS "Written by Craig Cabrey."

extern bool debug;

struct cli_context *ctx;

struct cmd {
        char *alias;
        char *name;
        int (*execute) (struct cli_context *ctx);
};

static int
shell_usage ()
{
        printf ("The following commands are supported:\n"
                "* cat\n"
                "* connect\n"
                "* cp\n"
                "* disconnect\n"
                "* help\n"
                "* ls\n"
                "* mkdir\n"
                "* touch\n"
                "* quit\n"
                "* rm\n"
                "* stat\n"
                "* tail\n"
                "* truncate\n"
                "* clear\n"
                "* flock\n"
                "* mv\n");

        return 0;
}

static struct cmd const cmds[] =
{
        { .name = "connect", .execute = cli_connect },
        { .name = "disconnect", .execute = cli_disconnect },
        { .alias = "gfcat", .name = "cat", .execute = do_cat },
        { .alias = "gfcp", .name = "cp", .execute = do_cp },
        { .name = "help", .execute = shell_usage },
        { .alias = "gfls", .name = "ls", .execute = do_ls },
        { .alias = "gfmkdir", .name = "mkdir", .execute = do_mkdir },
        { .alias = "gftouch", .name = "touch", .execute = do_touch },
        { .name = "quit", .execute = handle_quit },
        { .alias = "gfrm", .name = "rm", .execute = do_rm },
        { .alias = "gfstat", .name = "stat", .execute = do_stat },
        { .alias = "gftail", .name = "tail", .execute = do_tail },
        { .name = "flock", .execute = do_flock },
        { .alias = "gftruncate", .name = "truncate", .execute = do_truncate },
	{.alias = "gfrmdir", .name = "rmdir", .execute = do_rmdir},
        { .name = "clear", .execute = do_clear },
        { .name = "flock", .execute = do_flock },
        { .alias = "gfmv", .name = "mv", .execute = do_mv }
};
#define NUM_CMDS sizeof cmds / sizeof cmds[0]

static const struct cmd*
get_cmd (char *name)
{
        const struct cmd *cmd = NULL;
        for (int j = 0; j < NUM_CMDS; j++) {
                if (strcmp (name, cmds[j].name) == 0 ||
                                (cmds[j].alias != NULL &&
                                strcmp (name, cmds[j].alias) == 0)) {
                        cmd = &(cmds[j]);
                        break;
                }
        }

        return cmd;
}

static int
split_str (char *line, char **argv[])
{
        int argc = 0;
        char *line_start;

        line_start = line;

        while (*line != '\0') {
                if (*line == ' ') {
                        argc++;
                }

                line++;
        }

        argc++;
        line = line_start;

        *argv = malloc (sizeof (char*) * argc);
        if (*argv == NULL) {
                goto out;
        }

        int cur_arg = 0;
        while (cur_arg < argc && *line != '\0') {
                (*argv)[cur_arg] = line;

                while (*line != ' ' && *line != '\n' && *line != '\0') {
                        line++;
                }

                *line = '\0';
                line++;
                cur_arg++;
        }

out:
        return argc;
}

static int
start_shell ()
{
        int ret = 0;
        char *input = NULL;
        char *prompt = NULL;
        size_t size;
        const struct cmd *cmd;

        using_history ();

        while (true) {
                if (ctx->conn_str) {
                        size = sizeof (char) * (9 + strlen (ctx->conn_str));
                        prompt = malloc (size);

                        ret = snprintf (prompt,
                                        size,
                                        "gfcli %s> ",
                                        ctx->conn_str);

                } else {
                        prompt = malloc (sizeof (char) * 8);

                        ret = sprintf(prompt, "gfcli> ");
                }

                if (ret == -1 || prompt == NULL) {
                        error (EXIT_FAILURE, errno, "allocation error");
                }

                input = readline (prompt);

                // readline () encountered EOF, so exit cleanly
                if (input == NULL) {
                        ret = EXIT_SUCCESS;
                        goto out;
                }

                if (*input == '\0') {
                        continue;
                }

                add_history (input);

                ctx->argc = split_str (input, &ctx->argv);
                if (ctx->argc == 0) {
                        goto out;
                }

                cmd = get_cmd (ctx->argv[0]);

                if (cmd) {
                        program_invocation_name = ctx->argv[0];
                        ret = cmd->execute (ctx);
                } else {
                        fprintf (stderr,
                                 "Unknown command '%s'. "
                                 "Type 'help' for more.\n",
                                 ctx->argv[0]);
                }

                free (prompt);
                free (input);
                free (ctx->argv);
                ctx->argv = NULL;
        }

out:
        printf ("\n");
        free (input);
        return ret;
}

static struct option const long_options[] =
{
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage ()
{
        printf ("Usage: %s [OPTION]... [URL]\n"
                "Start a Gluster shell to execute commands on a remote Gluster volume.\n\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify a port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gfcli glfs://localhost/groot\n"
                "        Start a shell with a connection to localhost opened.\n"
                "  gfcli -o *replicate*.data-self-heal=on glfs://localhost/groot\n"
                "        Start a shell with a connection localhost open, with the\n"
                "        translator option data-self-head set to on.\n",
                program_invocation_name);
        exit (EXIT_SUCCESS);
}

static void
parse_options (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int opt = 0;
        int option_index = 0;
        struct xlator_option *option;

        // Ignore unknown options in order to allow them to be passed
        // through to the underlying utility or cli-specific command.
        opterr = 0;

        while (true) {
                opt = getopt_long (argc, argv, "ho:", long_options,
                                &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'd':
                                ctx->options->debug = true;
                                break;
                        case 'o':
                                option = parse_xlator_option (optarg);
                                if (option == NULL) {
                                        error (EXIT_FAILURE, errno, "%s", optarg);
                                }

                                if (append_xlator_option (&ctx->options->xlator_options, option) == -1) {
                                        error (EXIT_FAILURE, errno, "append_xlator_option");
                                }

                                break;
                        case 'h':
                                usage ();
                                exit (EXIT_SUCCESS);
                        case 'v':
                                printf ("%s (%s) %s\n%s\n%s\n%s\n",
                                                program_invocation_name,
                                                PACKAGE_NAME,
                                                PACKAGE_VERSION,
                                                COPYRIGHT,
                                                LICENSE,
                                                AUTHORS);
                                exit (EXIT_SUCCESS);
                        case '?':
                                break;
                        default:
                                error (EXIT_FAILURE, 0, "Try --help for more information.");
                }
        }

        if (argc - option_index >= 2) {
                if (cli_connect (ctx) == -1) {
                        exit (EXIT_FAILURE);
                }

                if (apply_xlator_options (ctx->fs, &ctx->options->xlator_options) == -1) {
                        exit (EXIT_FAILURE);
                }
        }
}

void
cleanup ()
{
        struct fd_list *cur, *ptr = NULL;

        if (ctx->url) {
                gluster_url_free (ctx->url);
                ctx->url = NULL;
        }

        if (ctx->options) {
                free_xlator_options (&ctx->options->xlator_options);
                free (ctx->options);
        }

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
                glfs_fini (ctx->fs);
                ctx->fs = NULL;
        }

        free (ctx);
}

void
sig_handler (int sig)
{
        if (sig == SIGINT) {
                cleanup ();
                exit (EXIT_SUCCESS);
        }
}

int
main (int argc, char *argv[])
{
        int ret = 0;
        const struct cmd *cmd = NULL;

        // We need to catch SIGINT so that we can gracefully close the
        // connection to the Gluster node(s); this prevents potential issues
        // with buffers not being fully flushed.
        signal (SIGINT, sig_handler);
        program_invocation_name = basename (argv[0]);
        atexit (close_stdout);

        ctx = malloc (sizeof (*ctx));
        if (ctx == NULL) {
                error (EXIT_FAILURE, errno, "failed to initialize context");
        }

        ctx->argc = argc;
        ctx->argv = argv;
        ctx->conn_str = NULL;
        ctx->fs = NULL;
        ctx->flist = NULL;
        ctx->options = malloc (sizeof (*(ctx->options)));
        if (ctx->options == NULL) {
                error (EXIT_FAILURE, errno, "failed to initialize options");
        }

        ctx->options->debug = false;
        ctx->options->xlator_options = NULL;
        ctx->url = NULL;

        cmd = get_cmd (program_invocation_name);

        if (cmd) {
                ctx->in_shell = false;
                ret = cmd->execute (ctx);
        } else {
                // Only parse options if we are being invoked as a shell
                ctx->in_shell = true;
                parse_options (ctx);

                // Set ctx->argv to NULL in the case that we enter the shell
                // and immediately receive a SIGINT. Without this, we would be
                // trying to free () the cli's argv, which would result in an
                // invalid free ().
                ctx->argc = 0;
                ctx->argv = NULL;
                ret = start_shell ();

                if (ctx->options->debug) {
                        print_xlator_options (&ctx->options->xlator_options);
                }
        }

        cleanup ();

        if (ret == -1) {
                ret = EXIT_FAILURE;
        }

        return ret;
}
