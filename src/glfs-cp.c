/**
 * A utility to copy a file to or from a remote Gluster volume locally or to or
 * from another remote Gluster volume.
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

#include "glfs-cp.h"
#include "glfs-util.h"

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define AUTHORS "Written by Craig Cabrey."
#define BUFFER_SIZE 1024*1024

/**
 * Represents the various transfer modes supported by gfcp.
 * In this context, ESTABLISHED refers to the case where a
 * connection has already been established in the CLI and we
 * are getting a connection passed to us via cli_context.
 */
enum transfer_mode {
        ESTABLISHED_TO_ESTABLISHED,
        ESTABLISHED_TO_LOCAL,
        ESTABLISHED_TO_REMOTE,
        LOCAL_TO_ESTABLISHED,
        LOCAL_TO_REMOTE,
        REMOTE_TO_ESTABLISHED,
        REMOTE_TO_LOCAL,
        REMOTE_TO_REMOTE
};

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_dest: Struct of the parsed destination url supplied by the user.
 * gluster_source: Struct of the parsed source url supplied by the user.
 * dest: Raw destination string supplied by the user.
 * source: Raw source string supplied by the user.
 * debug: Whether to log additional debug information.
 * mode: The detected transfer mode (deduced from the supplied source and dest).
 */
struct state {
        struct gluster_url *gluster_dest;
        struct gluster_url *gluster_source;
        struct xlator_option *xlator_options;
        char *dest;
        char *source;
        bool debug;
        enum transfer_mode mode;
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

/**
 * Parses a file:// url into a string with just the path.
 */
static char*
parse_file_url (char *file_url)
{
        char *file_path = NULL;

        // file_url should be minimum of 8 characters: file:///
        if (strlen (file_url) <= 7) {
                goto out;
        }

        // length of file:// is 7 characters
        if (strncmp (file_url, "file://", 7) == 0) {
                file_path = file_url + 7;
        }

out:
        return file_path;
}

/**
 * Prints usage information.
 */
static void
usage ()
{
        printf ("Usage: %s [OPTION]... SOURCE DEST\n"
                "Copy SOURCE to DEST; one of local to remote, remote to local, or remote to remote.\n\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "                               In the case of both the source and the\n"
                "                               destination being Gluster URLs, the options\n"
                "                               will be applied to both connections.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gfcp ./file glfs://localhost/groot/remote_file\n"
                "       Copies the local file 'file' to the destination file 'remote_file'\n"
                "       on the remote Gluster volume of groot on the host localhost.\n"
                "  gfcp glfs://localhost/groot/remote_file ./file\n"
                "       Copies the file 'remote_file' on the remote Gluster gluster\n"
                "       volume of groot on the host localhost to the local file 'file'.\n"
                "  gfcp glfs://localhost/groot/remote_file glfs://remote_host/groot/file\n"
                "       Copies the file 'remote_file' on the remote Gluster gluster\n"
                "       volume of groot on the host localhost to a second remote Gluster\n"
                "       volume of groot on the host remote_host to the file 'file'.\n"
                "  gfcli (localhost/groot)> cp /example file://example\n"
                "       Copy the file example relative to the root of the connected\n"
                "       Gluster volume to a local file called example.\n"
                "  gfcli (localhost/groot)> cp file://example glfs://host/volume/example\n"
                "       Copy the local file example to a remote Gluster volume on the\n"
                "       host 'host'.\n",
                program_invocation_name);
}

/**
 * Parses command line flags into a global application state.
 */
static int
parse_options (int argc, char *argv[], bool has_connection)
{
        uint16_t port = GLUSTER_DEFAULT_PORT;
        int ret = -1;
        int opt = 0;
        int option_index = 0;
        struct xlator_option *option;

        // Reset getopt as other utilities may have called it already.
        optind = 0;
        while (true) {
                opt = getopt_long (argc, argv, "o:p:", long_options,
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
                        default:
                                goto err;
                }
        }

        if ((argc - option_index) < 3) {
                error (0, 0, "missing operand");
                goto err;
        } else {
                /*
                 * The following block of code parses the source and destination
                 * arguments. Depending on what the user has indicated, the
                 * transfer mode of the application will differ. There are a few
                 * possibilities:
                 *
                 *   -> Source and destination are identical: show an invalid error
                 *   -> Local source and remote destination
                 *   -> Remote source and local destination
                 *   -> Remote source and remote destination
                 *   -> Local source and local destination: show an invalid error
                 *
                 * The transfer mode is decided based upon parsing outcomes. For
                 * example, if gluster_parse_url () fails on the second to last
                 * argument, we assume that we will be doing a LOCAL_TO_REMOTE
                 * transfer. Then, if gluster_parse_url () succeeds on the last
                 * argument, then we are definitely in that mode. If it fails,
                 * that must mean the user is attempting to execute a local to
                 * local transfer, which this application does not support.
                 */

                if (strcmp (argv[argc - 1], argv[argc - 2]) == 0) {
                        error (0, EINVAL, "source and destination are the same");
                        goto err;
                }

                /*
                 * If we have a connection passed to us from the shell, then
                 * there are a few differences. One is that local paths must be
                 * prefixed with file://. Otherwise, there is no way to
                 * distinguish between a local path and a path relative to the
                 * volume of the open connection. However, there is still the
                 * chance that there will be a remote source or destination, so
                 * we will still attempt to parse either parameter into a
                 * gluster_url.
                 *
                 *   -> Local (file://) source and remote (current conn) destination
                 *   -> Local (file://) source and remote (differknt conn) destination
                 *   -> Remote (current conn) source and local (file://) destination
                 *   -> Remote (different conn) source and local (file://) destination
                 *   -> Remote (current conn) source and remote (different conn) destination
                 *   -> Remote (different conn) source and remote (current conn) destination
                 *   -> Remote (current conn) source and remote (different conn) destination
                 *   -> Remote (different conn) source and remote (different conn) destination
                 */

                if (has_connection) {
                        char *file_path;

                        // Part 1: Parse the source string and guess our destination.
                        // We'll assume that we'll be interacting with the connected
                        // volume, so start with that.
                        ret = gluster_parse_url (argv[argc - 2], &(state->gluster_source));
                        if (ret == -1) {
                                file_path = parse_file_url (argv[argc - 2]);
                                if (file_path == NULL) {
                                        state->source = strdup (argv[argc - 2]);
                                        if (state->source == NULL) {
                                                error (0, errno, "strdup");
                                                goto out;
                                        }

                                        // Parsed source is neither a valid glfs://
                                        // url or a valid file:// url, so assuming
                                        // a path relative to the connected volume.
                                        state->mode = ESTABLISHED_TO_REMOTE;
                                } else {
                                        state->source = strdup (file_path);
                                        if (state->source == NULL) {
                                                error (0, errno, "strdup");
                                                goto out;
                                        }

                                        // Parsed source is a valid file:// url.
                                        state->mode = LOCAL_TO_ESTABLISHED;
                                }
                        } else {
                                state->source = strdup (argv[argc - 2]);
                                if (state->source == NULL) {
                                        error (0, errno, "strdup");
                                        goto out;
                                }

                                // Parsed source is a valid glfs:// url.
                                state->mode = REMOTE_TO_ESTABLISHED;
                                state->gluster_source->port = port;
                        }

                        // Part 2: Fix our destination based on our source and
                        // the result of parsing the destination string.
                        ret = gluster_parse_url (argv[argc - 1], &(state->gluster_dest));
                        if (ret == -1) {
                                file_path = parse_file_url (argv[argc - 1]);
                                if (file_path == NULL) {
                                        state->dest = strdup (argv[argc - 1]);
                                        if (state->dest == NULL) {
                                                error (0, errno, "strdup");
                                                goto out;
                                        }

                                        // If the source argument is a path relative
                                        // to the root of the connected volume, and the
                                        // destination argument is neither a glfs:// url
                                        // or a file:// url, then we must be cp'ing
                                        // to and from the connected volume.
                                        if (state->mode == ESTABLISHED_TO_REMOTE) {
                                                state->mode = ESTABLISHED_TO_ESTABLISHED;
                                        }

                                        ret = 0;
                                        goto out;
                                } else {
                                        state->dest = strdup (file_path);
                                        if (state->dest == NULL) {
                                                error (0, errno, "strdup");
                                                goto out;
                                        }

                                        switch (state->mode) {
                                                // If the source argument is glfs:// url
                                                // and the destination argument is a valid
                                                // file:// url, then we must be copying from
                                                // a different remote volume to the local
                                                // system.
                                                case REMOTE_TO_ESTABLISHED:
                                                        state->mode = REMOTE_TO_LOCAL;
                                                        ret = 0;
                                                        goto out;
                                                // If the source argument is a path relative
                                                // to the connected volume, and the destination
                                                // argument is a valid file:// url, then we must
                                                // be copying from the connected volume to the
                                                // local system.
                                                case ESTABLISHED_TO_REMOTE:
                                                        state->mode = ESTABLISHED_TO_LOCAL;
                                                        ret = 0;
                                                        goto out;
                                                default:
                                                        error (0, 0, "unknown transfer mode");
                                                        goto out;
                                        }
                                }
                        } else {
                                state->dest = strdup (argv[argc - 1]);
                                if (state->source == NULL) {
                                        error (0, errno, "strdup");
                                        goto out;
                                }

                                state->gluster_dest->port = port;

                                switch (state->mode) {
                                        // Source arugment is a path relative to
                                        // connected the volume, and the destination
                                        // argument is a valid glfs:// url.
                                        case ESTABLISHED_TO_REMOTE:
                                                ret = 0;
                                                goto out;
                                        // If the source argument is a valid glfs:// url and the
                                        // destination argument is a valid glfs:// url, then the
                                        // copy action is from the remote volume to another remote
                                        // volume.
                                        case REMOTE_TO_ESTABLISHED:
                                                state->mode = REMOTE_TO_REMOTE;
                                                ret = 0;
                                                goto out;
                                        // If the source argument is a valid file:// url and the
                                        // destination argument is a valid glfs:// url, then the
                                        // copy action is from the local system to a remote
                                        // volume.
                                        case LOCAL_TO_ESTABLISHED:
                                                state->mode = LOCAL_TO_REMOTE;
                                                ret = 0;
                                                goto out;
                                        default:
                                                error (0, 0, "unknown transfer mode");
                                                goto out;
                                }
                        }
                }

                free (state->dest);
                free (state->source);

                state->source = strdup (argv[argc - 2]);
                if (state->source == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

                state->dest = strdup (argv[argc - 1]);
                if (state->dest == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

                ret = gluster_parse_url (argv[argc - 2], &(state->gluster_source));
                if (ret == -1) {
                        state->mode = LOCAL_TO_REMOTE;
                } else {
                        state->mode = REMOTE_TO_LOCAL;
                        state->gluster_source->port = port;
                }

                ret = gluster_parse_url (argv[argc - 1], &(state->gluster_dest));
                if (ret == -1) {
                        if (state->mode == LOCAL_TO_REMOTE) {
                                error (0, EINVAL, "local source and destination");
                                goto err;
                        }

                        state->mode = REMOTE_TO_LOCAL;
                } else {
                        if (state->mode == REMOTE_TO_LOCAL) {
                                state->mode = REMOTE_TO_REMOTE;
                        }

                        state->gluster_dest->port = port;
                }
        }

        ret = 0;
        goto out;

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
        state->dest = NULL;
        state->gluster_dest = NULL;
        state->gluster_source = NULL;
        state->source = NULL;
        state->xlator_options = NULL;

out:
        return state;
}

/**
 * Helper function that will complete the destination path, given the full local
 * path, the user supplied destination path, and an initialized stat struct for
 * the supplied destination path.
 *
 * For example, presume the following is executed:
 *
 *   $ glfs-cp /tmp/some_data glfs://some_host/some_volume
 *
 * In this case, the source path is '/tmp/some_data' and the destination path is
 * '/' (the default case when the user has not supplied an explicit path in a
 * Gluster URL). We execute a stat () on '/' in the context of the Gluster
 * connection, which indicates that the path is a directory. Since a file
 * cannot overwrite a directory, we complete the path of the destination:
 * '/some_data'. This behavior is much in the same vain as the GNU coreutils cp
 * utility.
 *
 * It is the responsibility of the caller to free the return value.
 */
static char *
complete_path (const char *source_path, const char *dest_path, const struct stat *statbuf)
{
        char *full_path;

        if (statbuf && S_ISDIR (statbuf->st_mode)) {
                char *fmt = "%s/%s";
                size_t fmt_length = 2;
                size_t dest_path_length = strlen (dest_path);

                if (dest_path[dest_path_length - 1] == '/') {
                        fmt_length = 1;
                        fmt = "%s%s";
                }

                char *base_file = basename (source_path);
                size_t length = dest_path_length + strlen (base_file) + fmt_length;

                full_path = malloc (length);
                if (full_path == NULL) {
                        error (0, errno, "complete_path");
                        goto out;
                }

                snprintf (full_path, length, fmt, dest_path, base_file);
        } else {
                full_path = strdup (dest_path);
        }

out:
        return full_path;
}

/**
 * Perform a LOCAL_TO_REMOTE transfer, given the local source and remote
 * destination, and an active connection to the remote destination.
 */
static int
local_to_remote (const char *local_path, const char *remote_path, glfs_t *fs)
{
        int ret = -1;
        int fd;
        glfs_fd_t *remote_fd = NULL;
        struct stat statbuf;
        char *full_path = NULL;

        fd = open (local_path, O_RDONLY);
        if (fd == -1) {
                error (0, errno, "%s", local_path);
                goto out;
        }

        ret = glfs_lstat (fs, remote_path, &statbuf);

        if (ret == -1) {
                full_path = complete_path (local_path, remote_path, NULL);
        } else {
                full_path = complete_path (local_path, remote_path, &statbuf);
        }

        if (full_path == NULL) {
                ret = -1;
                goto out;
        }

        remote_fd = glfs_creat (fs, full_path, O_RDWR, get_default_file_mode_perm ());
        if (remote_fd == NULL) {
                error (0, errno, "failed to create %s", full_path);
                ret = -1;
                goto out;
        }

        ret = gluster_lock (remote_fd, F_WRLCK, false);
        if (ret == -1) {
                error (0, errno, "failed to lock %s", full_path);
                goto out;
        }
#ifdef HAVE_GLFS_7_6
        ret = glfs_ftruncate (remote_fd, 0, NULL, NULL);
#else
        ret = glfs_ftruncate (remote_fd, 0);
#endif
        if (ret == -1) {
                error (0, errno, "failed to truncate %s", full_path);
                goto out;
        }

        if (gluster_write (fd, remote_fd) == 0) {
                ret = -1;
                error (0, errno, "failed to transfer %s", local_path);
        }

out:
        free (full_path);

        if (fd != -1) {
                close (fd);
        }

        if (remote_fd) {
                glfs_close (remote_fd);
        }

        return ret;
}

/**
 * Perform a REMOTE_TO_LOCAL transfer, given the remote path and local
 * destination, and an active connection to the remote source.
 */
static int
remote_to_local (const char *remote_path, const char *local_path, glfs_t *fs)
{
        int ret = -1;
        int local_fd = -1;
        glfs_fd_t *remote_fd = NULL;
        struct stat statbuf;
        char *full_path;

        ret = stat (local_path, &statbuf);
        if (ret == -1) {
                full_path = complete_path (remote_path, local_path, NULL);
        } else {
                full_path = complete_path (remote_path, local_path, &statbuf);
        }

        if (full_path == NULL) {
                ret = -1;
                goto out;
        }

        remote_fd = glfs_open (fs, remote_path, O_RDONLY);
        if (remote_fd == NULL) {
                error (0, errno, "%s", remote_path);
                goto out;
        }

        local_fd = open (full_path, O_CREAT | O_WRONLY, get_default_file_mode_perm ());
        if (local_fd == -1) {
                error (0, errno, "%s", full_path);
                goto out;
        }

        ret = gluster_lock (remote_fd, F_WRLCK, false);
        if (ret == -1) {
                error (0, errno, "failed to lock %s", remote_path);
                goto out;
        }

        if ((ret = gluster_read (remote_fd, local_fd)) == -1) {
                error (0, errno, "write error");
        }

out:
        free (full_path);

        if (local_fd != -1) {
                close (local_fd);
        }

        if (remote_fd) {
                glfs_close (remote_fd);
        }

        return ret;
}

/**
 * Perform a REMOTE_TO_REMOTE transfer, given both source and destination remote
 * paths and active connections to both the source and destination.
 */
static int
remote_to_remote (const char *source_path, const char *dest_path, glfs_t *source_fs, glfs_t *dest_fs)
{
        int ret = -1;
        size_t num_read = 0;
        size_t num_written = 0;
        glfs_fd_t *source_fd = NULL;
        glfs_fd_t *dest_fd = NULL;
        struct stat statbuf;
        char buf[BUFFER_SIZE];
        char *full_path;

        ret = glfs_lstat (dest_fs, dest_path, &statbuf);

        if (ret == -1) {
                full_path = complete_path (source_path, dest_path, NULL);
        } else {
                full_path = complete_path (source_path, dest_path, &statbuf);
        }

        if (full_path == NULL) {
                ret = -1;
                goto out;
        }

        source_fd = glfs_open (source_fs, source_path, O_RDONLY);
        if (source_fd == NULL) {
                error (0, errno, "%s", source_path);
                goto out;
        }

        dest_fd = glfs_creat (dest_fs, full_path, O_CREAT | O_WRONLY, get_default_file_mode_perm ());
        if (dest_fd == NULL) {
                error (0, errno, "%s", full_path);
                goto out;
        }

        while (true) {
                num_read = glfs_read (source_fd, buf, BUFFER_SIZE, 0);
                if (num_read == -1) {
                        ret = -1;
                        goto out;
                }

                if (num_read == 0) {
                        goto out;
                }

                for (num_written = 0; num_written < num_read;) {
                        ret = glfs_write (dest_fd,
                                        &buf[num_written],
                                        num_read - num_written, 0);
                        if (ret == -1) {
                                error (0, errno, "write error");
                                goto out;
                        }

                        num_written += ret;
                }
        }

out:
        free (full_path);

        if (source_fd) {
                glfs_close (source_fd);
        }

        if (dest_fd) {
                glfs_close (dest_fd);
        }

        return ret;
}

static int
cp_without_context ()
{
        glfs_t *dest_fs = NULL;
        glfs_t *source_fs = NULL;
        int ret = -1;

        switch (state->mode) {
                case LOCAL_TO_REMOTE:
                        ret = gluster_getfs (&dest_fs, state->gluster_dest);
                        if (ret == -1) {
                                error (0, errno, "%s", state->dest);
                                goto out;
                        }

                        ret = apply_xlator_options (dest_fs, &state->xlator_options);
                        if (ret == -1) {
                                error (0, errno, "failed to apply translator options");
                                goto out;
                        }

                        ret = local_to_remote (state->source,
                                                state->gluster_dest->path,
                                                dest_fs);
                        if (ret == -1) {
                                goto out;
                        }

                        break;
                case REMOTE_TO_LOCAL:
                        ret = gluster_getfs (&source_fs, state->gluster_source);
                        if (ret == -1) {
                                error (0, errno, "%s", state->source);
                                goto out;
                        }

                        ret = apply_xlator_options (source_fs, &state->xlator_options);
                        if (ret == -1) {
                                error (0, errno, "failed to apply translator options");
                                goto out;
                        }

                        ret = remote_to_local (state->gluster_source->path,
                                                state->dest,
                                                source_fs);
                        if (ret == -1) {
                                goto out;
                        }

                        break;
                case REMOTE_TO_REMOTE:
                        ret = gluster_getfs (&dest_fs, state->gluster_dest);
                        if (ret == -1) {
                                error (0, errno, "%s", state->dest);
                                goto out;
                        }

                        ret = apply_xlator_options (dest_fs, &state->xlator_options);
                        if (ret == -1) {
                                error (0, errno, "failed to apply translator options");
                                goto out;
                        }

                        /**
                         * If the host and volume of the source and destination
                         * are the same, then simply use the same connection to
                         * make the transfer.
                         */
                        if (strcmp (state->gluster_source->host, state->gluster_dest->host) == 0
                               && strcmp (state->gluster_source->volume, state->gluster_dest->volume) == 0) {
                                source_fs = dest_fs;
                        } else {
                                ret = gluster_getfs (&source_fs, state->gluster_source);
                                if (ret == -1) {
                                        error (0, errno, "%s", state->source);
                                        goto out;
                                }

                                ret = apply_xlator_options (source_fs, &state->xlator_options);
                                if (ret == -1) {
                                        error (0, errno, "failed to apply translator options");
                                        goto out;
                                }
                        }

                        ret = remote_to_remote (state->gluster_source->path,
                                                state->gluster_dest->path,
                                                source_fs,
                                                dest_fs);
                        if (ret == -1) {
                                goto out;
                        }

                        if (dest_fs == source_fs) {
                                glfs_fini (dest_fs);
                                dest_fs = source_fs = NULL;
                        }

                        break;
                default:
                        error (0, errno, "unknown error");
        }

out:
        if (dest_fs) {
                glfs_fini (dest_fs);
        }

        if (source_fs) {
                glfs_fini (source_fs);
        }

        return ret;
}

static int
cp_with_context (glfs_t *fs)
{
        glfs_t *dest_fs = NULL;
        glfs_t *source_fs = NULL;
        int ret = -1;

        switch (state->mode) {
                case ESTABLISHED_TO_ESTABLISHED:
                        ret = remote_to_remote (state->source,
                                                state->dest,
                                                fs,
                                                fs);
                        break;
                case ESTABLISHED_TO_LOCAL:
                        ret = remote_to_local (state->source, state->dest, fs);
                        break;
                case ESTABLISHED_TO_REMOTE:
                        ret = gluster_getfs (&dest_fs, state->gluster_dest);
                        if (ret == -1) {
                                error (0, errno, "%s", state->dest);
                                goto out;
                        }

                        ret = apply_xlator_options (dest_fs, &state->xlator_options);
                        if (ret == -1) {
                                error (0, errno, "failed to apply translator options");
                                goto out;
                        }

                        ret = remote_to_remote (state->source, state->gluster_dest->path, fs, dest_fs);

                        break;
                case LOCAL_TO_ESTABLISHED:
                        ret = local_to_remote (state->source, state->dest, fs);

                        break;
                case REMOTE_TO_ESTABLISHED:
                        ret = gluster_getfs (&source_fs, state->gluster_source);
                        if (ret == -1) {
                                error (0, errno, "%s", state->source);
                                goto out;
                        }

                        ret = apply_xlator_options (source_fs, &state->xlator_options);
                        if (ret == -1) {
                                error (0, errno, "failed to apply translator options");
                                goto out;
                        }

                        ret = remote_to_remote (state->gluster_source->path,
                                                state->dest,
                                                source_fs,
                                                fs);

                        break;
                // Fall through to cp_without_context () for the normal
                // transfer functions.
                case LOCAL_TO_REMOTE:
                case REMOTE_TO_LOCAL:
                case REMOTE_TO_REMOTE:
                        ret = cp_without_context ();
                        break;
                default:
                        error (0, 0, "unknown error");
        }

out:
        if (dest_fs) {
                glfs_fini (dest_fs);
        }

        if (source_fs) {
                glfs_fini (source_fs);
        }

        return ret;
}

/**
 * Main entry point into application (called from glfs-cli.c)
 */
int
do_cp (struct cli_context *ctx)
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

                ret = cp_with_context (ctx->fs);
        } else {
                ret = parse_options (argc, argv, false);
                switch (ret) {
                        case -2:
                                // Fall through
                                ret = 0;
                        case -1:
                                goto out;
                }

                ret = cp_without_context ();
        }

out:
        if (state) {
                if (state->gluster_dest) {
                        gluster_url_free (state->gluster_dest);
                }

                if (state->gluster_source) {
                        gluster_url_free (state->gluster_source);
                }

                free (state->dest);
                free (state->source);
        }

        free (state);

        return ret;
}
