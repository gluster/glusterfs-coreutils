/**
 * A utility to list files and directories from a remote Gluster volume.
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

#include <errno.h>
#include <error.h>
#include <fnmatch.h>
#include <inttypes.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "glfs-ls.h"
#include "glfs-util.h"
#include "glfs-stat-util.h"
#include "human.h"

#define AUTHORS "Written by Craig Cabrey."

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_url: Struct of the parsed gluster url supplied by the user.
 * url: Raw url string supplied by the user.
 * debug: Whether to log additional debug information.
 * recursive: Whether to enable recursive mode.
 * show_all: Whether to show hidden files (denoated by a '.' prefix in names).
 * long_form: Whether to enable long form listing (similar to GNU ls).
 */
struct state {
        struct gluster_url *gluster_url;
        char *url;
        bool debug;
        bool human_readable;
        bool recursive;
        bool show_all;
        bool show_atime;
        bool show_ctime;
        bool long_form;
};

static struct state *state;

static struct option const long_options[] =
{
        {"all", no_argument, NULL, 'a'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'x'},
        {"human", no_argument, NULL, 'h'},
        {"port", required_argument, NULL, 'p'},
        {"recursive", no_argument, NULL, 'R'},
        {"version", no_argument, NULL, 'v'},
        {NULL, no_argument, NULL, 0}
};

/**
 * Prints usage information.
 */
static void
usage ()
{
        printf ("Usage: %s [OPTION]... URL\n"
                "list directory contents\n\n"
                "  -a, --all              do not ignore entries starting with .\n"
                "  -h, --human-readable   with -l, print sizes in human readable\n"
                "                         format (e.g., 1K 234M 2G)\n"
                "  -l                     use a long listing format\n"
                "  -R, --recursive        list subdirectories recursively\n"
                "  -p, --port=PORT        specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gfls glfs://localhost/groot/directory\n"
                "       List the contents of /directory on the Gluster volume\n"
                "       root on host localhost.\n"
                "  gfls -l glfs://localhost/groot/directory\n"
                "       List the contents of /directory on the Gluster volume root\n"
                "       on host localhost using the long listing format.\n"
                "  gfls -Rl glfs://localhost/groot/directory\n"
                "       Recursively list the contents of /directory on the Gluster\n"
                "       volume groot on host localhost using the long listing format.\n"
                "  gfcli (localhost/groot)> ls /\n"
                "       List the contents of the root of the connected Gluster volume.\n",
                "  gfcli (localhost/groot)> ls\n"
                "       List the contents of the current directory of the connected Gluster volume.\n",
                program_invocation_name);
}

/**
 * Parses command line flags into a global application state.
 */
static int
parse_options (int argc, char *argv[], bool has_connection)
{
        uint16_t port = 0;
        int ret = -1;
        int opt = 0;
        int option_index = 0;

        // Reset getopt since other utilities may have called it already.
        optind = 0;
        while (true) {
                opt = getopt_long (argc, argv, "abcdhlp:R", long_options,
                                &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'a':
                                state->show_all = true;
                                break;
                        case 'b':
                                state->show_atime = true;
                                break;
                        case 'c':
                                state->show_ctime = true;
                                break;
                        case 'd':
                                state->debug = true;
                                break;
                        case 'h':
                                state->human_readable = true;
                                break;
                        case 'l':
                                state->long_form = true;
                                break;
                        case 'p':
                                port = strtoport (optarg);
                                if (port == 0) {
                                        goto out;
                                }

                                break;
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
                        case 'x':
                                usage ();
                                ret = -2;
                                goto out;
                        default:
                                goto err;
                }
        }

        if(has_connection){
                if ((argc-option_index)<2){
                        const char *curPath=".";
                        state->url = strdup (curPath);
                }
                else {
                        state->url = strdup (argv[argc - 1]);
                }
                ret = 0;
                goto out;
        }
        else{
                if ((argc-option_index)<2){
                        error(0, 0, "missing operand");
                        goto err;
                }
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
                goto out;
        }

err:
        error (0, 0, "Try --help for more information.");
out:
        return ret;
}

/**
 * Initializes the global application state.
 */
static struct state*
init_state ()
{
        struct state *state = malloc (sizeof (*state));

        if (state == NULL) {
                goto out;
        }

        state->debug = false;
        state->human_readable = false;
        state->gluster_url = NULL;
        state->long_form = false;
        state->recursive = false;
        state->show_all = false;
        state->show_atime = false;
        state->show_ctime = false;
        state->url = NULL;

out:
        return state;
}

/**
 * Prints the long form of a directory entry.
 */
static void
print_long (const char *ent_name, struct stat *statbuf)
{
        struct timespec time;
        struct tm *tm;
        struct passwd *pw_ent;
        struct group *gr_ent;
        const char *mode_str;
        char buf[LONGEST_HUMAN_READABLE + 1];
        const char *size;
        unsigned int num_links = (unsigned int) statbuf->st_nlink;

        pw_ent = getpwuid (statbuf->st_uid);
        gr_ent = getgrgid (statbuf->st_gid);
        mode_str = human_access (statbuf);

        time = get_stat_ctime (statbuf);
        tm = localtime (&(time.tv_sec));
        char c_time_str[17];
        strftime (c_time_str, sizeof c_time_str, "%b %e %T", tm);

        time = get_stat_mtime (statbuf);
        tm = localtime (&(time.tv_sec));
        char m_time_str[17];
        strftime (m_time_str, sizeof m_time_str, "%b %e %T", tm);

        time = get_stat_atime (statbuf);
        tm = localtime (&(time.tv_sec));
        char a_time_str[17];
        strftime (a_time_str, sizeof a_time_str, "%b %e %T", tm);

        printf ("%s. ", mode_str);
        printf ("%i ", num_links);
        printf ("%-15s ", pw_ent ? pw_ent->pw_name : "UNKNOWN");
        printf ("%-15s ", gr_ent ? gr_ent->gr_name : "UNKNOWN");

        if (state->human_readable) {
                size = human_readable (
                                statbuf->st_size,
                                buf,
                                human_autoscale | human_floor | human_SI,
                                1,
                                1);
                printf ("%-10s", size);
        } else {
                unsigned long size = (unsigned long) statbuf->st_size;
                printf ("%-10lu ", size);
        }

        if (state->show_ctime) {
                printf ("%s ", c_time_str);
        }

        printf ("%s ", m_time_str);

        if (state->show_atime) {
                printf ("%s ", a_time_str);
        }

        printf ("%s\n", ent_name);
}

/**
 * Prints the short form of a directory entry.
 */
static void
print_short (const char *ent_name, struct stat *statbuf)
{
        printf ("%s ", ent_name);
}

/**
 * Perform a list directory with the given gluster connection, path, pattern,
 * and print helper function.
 */
int
ls_dir (glfs_t *fs, char *path, char *pattern, void (*print_func)(const char *, struct stat *))
{
        int ret = -1;
        glfs_fd_t *fd = NULL;
        struct stat stat;
        struct stat statbuf;
        struct dirent *dirent;
        char *full_path;

        ret = glfs_lstat (fs, path, &stat);
        if (ret == -1) {
                error (0, errno, "%s", path);
                goto out;
        }

        fd = glfs_opendir (fs, path);
        if (fd == NULL) {
                error (0, errno, "%s", path);
                goto out;
        }

        if (state->recursive) {
                printf ("%s:\n", path);
        }

        if (state->show_all) {
                print_func (".", &stat);

                full_path = append_path (path, "..");
                glfs_lstat (fs, full_path, &statbuf);
                print_func ("..", &statbuf);
                free (full_path);
        }

        while ((dirent = glfs_readdirplus (fd, &stat)) != NULL) {
                if (pattern && fnmatch (pattern, dirent->d_name, 0) != 0) {
                        continue;
                }

                if (strcmp (dirent->d_name, ".") == 0) {
                        continue;
                }

                if (strcmp (dirent->d_name, "..") == 0) {
                        continue;
                }

                full_path = append_path (path, dirent->d_name);
                if (glfs_lstat (fs, full_path, &statbuf) == -1) {
                        error (0, errno, "failed to stat %s", path);
                } else {
                        print_func (dirent->d_name, &statbuf);
                }

                free (full_path);
        }

        if (!state->recursive) {
                goto out;
        }

        /**
         * In order to recurse, we need to loop over the directory entries
         * again. To do so, close and re-open the directory to force a rewind.
         */
        ret = glfs_closedir (fd);
        fd = glfs_opendir (fs, path);
        if (fd == NULL) {
                error (0, errno, "failed to open %s", path);
                goto out;
        }

        while ((dirent = glfs_readdirplus (fd, &stat)) != NULL) {
                if (pattern && fnmatch (pattern, dirent->d_name, 0) != 0) {
                        continue;
                }

                if (strcmp (dirent->d_name, ".") == 0) {
                        continue;
                }

                if (strcmp (dirent->d_name, "..") == 0) {
                        continue;
                }

                if (dirent->d_type == DT_DIR) {
                        full_path = append_path (path, dirent->d_name);
                        if (full_path == NULL) {
                                goto out;
                        }

                        if (state->long_form) {
                                printf ("\n");
                        } else {
                                printf ("\n\n");
                        }

                        ls_dir (fs, full_path, "*", print_func);
                        free (full_path);
                }
        }

out:
        if (fd) {
                ret = glfs_closedir (fd);
        }

        return ret;
}

static int
ls (glfs_t *fs, char *path)
{
        char *pattern = NULL;
        char *real_path = NULL;
        int ret = -1;
        struct stat statbuf;

        /**
         * Determines the pattern matching string.
         *
         * Start by finding the basename of the path. If the result does not
         * contain a wildcard ('*'), stat() the path. If it does contain a
         * wildcard, set the pattern to the basename of the path and the
         * real_path to the dirname() of the path.
         *
         * Example: /first/te*
         *
         * Outcome:
         *   -> real_path: /first
         *   -> pattern: te*
         */
        real_path = strdup (path);
        if (real_path == NULL) {
                error (0, errno, "strdup");
                goto out;
        }

        pattern = basename (path);
        if (pattern && strchr (pattern, '*') == NULL) {
                if (glfs_stat (fs, path, &statbuf)) {
                        error (0, errno, "failed to access %s", state->url);
                        goto out;
                }

                pattern = "*";
        } else {
                pattern = basename (path);
                real_path = dirname (real_path);
        }

        if (state->long_form) {
                ls_dir (fs, real_path, pattern, print_long);
        } else {
                ls_dir (fs, real_path, pattern, print_short);
                printf ("\n");
        }

        ret =  0;

out:
        free (real_path);

        return ret;
}

static int
ls_without_context ()
{
        glfs_t *fs = NULL;
        int ret;

        ret = gluster_getfs (&fs, state->gluster_url);
        if (ret == -1) {
                error (0, errno, "failed to access %s", state->url);
                goto out;
        }

        if (state->debug) {
                ret = glfs_set_logging (fs, "/dev/stderr", GF_LOG_DEBUG);

                if (ret == -1) {
                        error (0, errno, "failed to set logging level");
                        goto out;
                }
        }

        ret = ls (fs, state->gluster_url->path);

out:
        if (fs) {
                glfs_fini (fs);
        }

        return ret;
}

/**
 * Main entry point into application (called from glfs-cli.c)
 */
int
do_ls (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int ret = -1;

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

                ret = ls (ctx->fs, state->url);
        } else {
                ret = parse_options (argc, argv, false);
                switch (ret) {
                        case -2:
                                // Fall through
                                ret = 0;
                        case -1:
                                goto out;
                }

                ret = ls_without_context ();
        }

out:
        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
        }

        free (state);

        return ret;
}
