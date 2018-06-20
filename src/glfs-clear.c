/**
 * A utility to remove a file or directory from a remote Gluster volume.
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
#include <glfs-clear.h>
#include <error.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUTHORS "Written by MoonBlade."

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
        {"help", no_argument, NULL, 'x'},
        {"port", required_argument, NULL, 'p'},
        {"version", no_argument, NULL, 'v'},
        {"xlator-option", required_argument, NULL, 'o'},
        {NULL, no_argument, NULL, 0}
};

static void
usage ()
{
        printf ("Usage: %s\n"
                "Clear Terminal screen.\n\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n",
                program_invocation_name);
}

int
do_clear()
{
        int ret = 0;
        printf("\033[2J\033[1;1H");
        return ret;
}
