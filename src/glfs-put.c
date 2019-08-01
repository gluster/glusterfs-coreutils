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

#define AUTHORS "Written by Craig Cabrey."

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_url: Struct of the parsed url supplied by the user.
 * url: Full url used to find the remote file (supplied by user).
 * append: Whether to append to the file instead of replacing it.
 * debug: Whether to log additional debug information.
 * parents: Whether all parent directories in the path are created.
 */
struct state {
        struct gluster_url *gluster_url;
        struct xlator_option *xlator_options;
        char *url;
        bool append;
        bool debug;
        bool overwrite;
        bool parents;
};

static struct state *state;

static struct option const long_options[] =
{
        {"append", no_argument, NULL, 'a'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'x'},
        {"overwrite", no_argument, NULL, 'f'},
        {"parents", required_argument, NULL, 'r'},
        {"port", required_argument, NULL, 'p'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage (const int status)
{
        printf ("Usage: %s [OPTION]... URL\n"
                "Put data from standard input on a remote Gluster volume.\n\n"
                "  -a, --append                 append data to the end of the file\n"
                "  -f, --overwrite              overwrite the existing file\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "  -r, --parents                no error if existing, make parent\n"
                "                               directories as needed\n"
                "      --help       display this help and exit\n"
                "      --version    output version information and exit\n\n"
                "Examples:\n"
                "  gfput glfs://localhost/groot/file\n"
                "        Write the contents of standard input to /file on the\n"
                "        Gluster volume of groot on host localhost.\n"
                "  gfput -r glfs://localhost/groot/path/to/file\n"
                "        Write the contents of standard input to /file on the\n"
                "        Gluster volume of groot on host localhost, creating\n"
                "        the parent directories as necessary.\n",
                program_invocation_name);
        exit (status);
}

void
parse_options (int argc, char *argv[])
{
        uint16_t port = GLUSTER_DEFAULT_PORT;
        int ret = -1;
        int opt = 0;
        int option_index = 0;
        struct xlator_option *option;

        while (true) {
                opt = getopt_long (argc, argv, "adfo:p:r", long_options,
                                   &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'a':
                                state->append = true;
                                break;
                        case 'd':
                                state->debug = true;
                                break;
                        case 'f':
                                state->overwrite = true;
                                break;
                        case 'o':
                                option = parse_xlator_option (optarg);
                                if (option == NULL) {
                                        error (0, errno, "%s", optarg);
                                        goto err;
                                }

                                if (append_xlator_option (&state->xlator_options, option) == -1) {
                                        error (EXIT_FAILURE, errno, "append_xlator_option: %s", optarg);
                                }

                                break;
                        case 'p':
                                port = strtoport (optarg);
                                if (port == 0) {
                                        exit (EXIT_FAILURE);
                                }

                                break;
                        case 'r':
                                state->parents = true;
                                break;
                        case 'v':
                                printf ("%s (%s) %s\n%s\n%s\n%s\n",
                                        program_invocation_name,
                                        PACKAGE_NAME,
                                        PACKAGE_VERSION,
                                        COPYRIGHT,
                                        LICENSE,
                                        AUTHORS);
                                exit (EXIT_SUCCESS);
                        case 'x':
                                usage (EXIT_SUCCESS);
                        default:
                                goto err;
                }
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

                ret = gluster_parse_url (argv[argc - 1], &(state->gluster_url));
                if (ret == -1) {
                        error (0, EINVAL, "%s", state->url);
                        goto err;
                }

                state->gluster_url->port = port;
        }

        goto out;

err:
        error (EXIT_FAILURE, 0, "Try --help for more information.");
out:
        return;
}

struct state*
init_state ()
{
        struct state *state = malloc (sizeof (*state));

        if (state == NULL) {
                goto out;
        }

        state->append = false;
        state->debug = false;
        state->gluster_url = NULL;
        state->overwrite = false;
        state->parents = false;
        state->url = NULL;

out:
        return state;
}

int
gluster_put (glfs_t *fs, struct state *state)
{
        int ret = -1;
        glfs_fd_t *fd = NULL;
        char *filename = state->gluster_url->path;
        char *dir_path = strdup (state->gluster_url->path);
        struct stat statbuf;

        if (dir_path == NULL) {
                error (EXIT_FAILURE, errno, "strdup");
                goto out;
        }

        ret = glfs_lstat (fs, filename, &statbuf);
        if (ret != -1 && !state->append && !state->overwrite) {
                errno = EEXIST;
                ret = -1;
                goto out;
        }

        if (state->parents) {
                ret = gluster_create_path (fs, dir_path,
                                get_default_dir_mode_perm ());
                if (ret == -1) {
                        goto out;
                }
        }

        fd = glfs_creat (fs, filename, O_RDWR, get_default_file_mode_perm ());
        if (fd == NULL) {
                ret = -1;
                goto out;
        }

        ret = gluster_lock (fd, F_WRLCK, false);
        if (ret == -1) {
                goto out;
        }

        if (state->append) {
                ret = glfs_lseek (fd, 0, SEEK_END);
                if (ret == -1) {
                        error (0, errno, "seek error: %s", filename);
                        goto out;
                }
        } else {
#ifdef HAVE_GLFS_7_6
                ret = glfs_ftruncate (fd, 0, NULL, NULL);
#else
                ret = glfs_ftruncate (fd, 0);
#endif
                if (ret == -1) {
                        error (0, errno, "truncate error: %s", filename);
                        goto out;
                }
        }

        if (gluster_write (STDIN_FILENO, fd) == 0) {
                ret = -1;
        }

out:
        free (dir_path);

        if (fd) {
                glfs_close (fd);
        }

        return ret;
}

int
main (int argc, char *argv[])
{
        glfs_t *fs;
        int ret;

        program_invocation_name = basename (argv[0]);
        atexit (close_stdout);

        state = init_state ();
        if (state == NULL) {
                error (0, errno, "failed to initialize state");
                goto err;
        }

        parse_options (argc, argv);

        ret = gluster_getfs (&fs, state->gluster_url);
        if (ret == -1) {
                error (0, errno, "%s", state->url);
                goto err;
        }

        ret = apply_xlator_options (fs, &state->xlator_options);
        if (ret == -1) {
                error (0, errno, "failed to apply translator options");
                goto err;
        }

        if (state->debug) {
                ret = glfs_set_logging (fs, "/dev/stderr", GF_LOG_DEBUG);

                if (ret == -1) {
                        error (0, errno, "failed to set logging level");
                        goto err;
                }
        }

        ret = gluster_put (fs, state);
        if (ret == -1) {
                error (0, errno, "%s", state->url);
                goto err;
        }

        ret = EXIT_SUCCESS;
        goto out;

err:
        ret = EXIT_FAILURE;
out:
        if (fs) {
                glfs_fini (fs);
        }

        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
        }

        free (state);

        return ret;
}
