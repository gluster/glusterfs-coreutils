/**
 * Helper functions for use in the gluster coreutils project.
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

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <glusterfs/api/glfs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GLFS_MIN_URL_LENGTH 11
#define LOG_EVERY_SECS 30

int
append_xlator_option (struct xlator_option **options, struct xlator_option *option)
{
        int ret = 0;
        struct xlator_option *next = *options;

        if (next == NULL) {
                *options = option;
                goto out;
        }

        while (next->next != NULL) {
                next = next->next;
        }

        next->next = option;

out:
        return ret;
}

char *
append_path (const char *base_path, const char *hanging_path)
{
        size_t new_length;
        size_t base_path_length = strlen (base_path);
        size_t hanging_path_length = strlen (hanging_path);
        const char *fmt;
        char *new_path;

        if (base_path_length > 0 && base_path[base_path_length - 1] == '/') {
                new_length = base_path_length + hanging_path_length + 1;
                fmt = "%s%s";
        } else {
                new_length = base_path_length + hanging_path_length + 2;
                fmt = "%s/%s";
        }

        new_path = malloc (new_length);
        if (new_path == NULL) {
                goto out;
        }

        snprintf (new_path, new_length, fmt, base_path, hanging_path);

out:
        return new_path;
}

int
apply_xlator_options (glfs_t *fs, struct xlator_option **options)
{
        int ret = 0;
        struct xlator_option *next = *options;

        while (next != NULL) {
                ret = glfs_set_xlator_option (
                                fs,
                                next->xlator,
                                next->key,
                                next->value);

                if (ret == -1) {
                        error (0, errno, "failed to set %s xlator option: %s",
                                        next->xlator, next->key);
                        goto out;
                }

                next = next->next;
        }

out:
        return ret;
}

static int
close_stream (FILE *stream)
{
        const bool prev_fail = (ferror (stream) != 0);
        const bool fclose_fail = (fclose (stream) != 0);

        if (prev_fail || fclose_fail) {
                if (!fclose_fail) {
                        errno = 0;
                }

                return EOF;
        }

        return 0;
}

void
close_stdout (void)
{
        if (close_stream (stdout) != 0) {
                error (EXIT_FAILURE, errno, "write error");
        }

        if (close_stream (stderr) != 0) {
                exit (EXIT_FAILURE);
        }
}

void
free_xlator_options (struct xlator_option **options)
{
        // No options were set, so nothing to do here.
        if (options == NULL || *options == NULL) {
                return;
        }

        struct xlator_option *next = *options;
        struct xlator_option *prev = next;

        while (next) {
                prev = next;
                next = next->next;

                if (prev->opt_string) {
                        free (prev->opt_string);
                }

                free (prev);
        }
}

mode_t
get_default_dir_mode_perm ()
{
        return (S_IRWXU | S_IRWXG | S_IRWXO) & ~umask (0);
}

mode_t
get_default_file_mode_perm ()
{
        return (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
                & ~umask (0);
}

struct gluster_url*
gluster_url_init ()
{
        struct gluster_url *gluster_url = malloc (sizeof (*gluster_url));

        if (gluster_url == NULL) {
                goto out;
        }

        gluster_url->host = NULL;
        gluster_url->path = NULL;
        gluster_url->port = GLUSTER_DEFAULT_PORT;
        gluster_url->volume = NULL;

out:
        return gluster_url;
}

void
gluster_url_free (struct gluster_url *gluster_url)
{
        if (gluster_url) {
                free (gluster_url->path);
        }

        free (gluster_url);
}

int
gluster_parse_url (char *url, struct gluster_url **gluster_url)
{
        char *host;
        char *path;
        char *pos = NULL;
        char *volume;
        int ret = 0;
        struct gluster_url *parsed_gluster_url;

        parsed_gluster_url = gluster_url_init ();
        if (parsed_gluster_url == NULL) {
                goto err;
        }

        if (strncmp (url, "glfs://", 7)) {
                goto err;
        }

        // URL Format: glfs://<host>/<volume>/<path>
        if (strlen (url) < GLFS_MIN_URL_LENGTH) {
                goto err;
        }

        strtok_r (url, "/", &pos);
        host = strtok_r (NULL, "/", &pos);
        if (host == NULL) {
                goto err;
        }

        volume = strtok_r (NULL, "/", &pos);
        if (volume == NULL) {
                goto err;
        }

        path = pos && *pos ? strdup (pos) : strdup ("/");
        if (path == NULL) {
                error (0, errno, "gluster_parse_url: strdup");
                goto err;
        }

        // Prefix path with a / if it does not start with one.
        if (*path != '/') {
                // In some cases, the path needs to have a trailing slash
                // appended, so we'll allocate an extra byte now while we can.
                int length = strlen (path) + 3;
                char *new_path = malloc (length);
                if (new_path == NULL) {
                        goto err;
                }

                snprintf (new_path, length, "/%s", path);
                free (path);
                path = new_path;
        }

        parsed_gluster_url->host = host;
        parsed_gluster_url->path = path;
        parsed_gluster_url->volume = volume;

        *gluster_url = parsed_gluster_url;

        goto out;

err:
        ret = -1;
        free (parsed_gluster_url);
out:
        return ret;
}

int
gluster_create_path (glfs_t *fs, char *path, mode_t omode)
{
        struct stat sb;
        int is_last = 0;
        int ret = EXIT_SUCCESS;
        char *dir_end = NULL;
        char *next = path;

        if (*next == '/') {
                next++;
        }

        dir_end = strrchr (next, '/');
        if (dir_end == NULL) {
                goto out;
        }

        do {
                next = strchr (next, '/');
                assert (next != NULL);
                is_last = next == dir_end;
                *next = '\0';

                ret = glfs_mkdir (fs, path, omode);

                if (ret != EXIT_SUCCESS) {
                        if (errno != EEXIST && errno != EISDIR) {
                                goto out;
                        }

                        ret = glfs_stat (fs, path, &sb);
                        if (ret != EXIT_SUCCESS) {
                                goto out;
                        }

                        if (!S_ISDIR (sb.st_mode)) {
                                errno = is_last ? EEXIST : ENOTDIR;
                                goto out;
                        }
                }

                if (!is_last) {
                        *next = '/';
                        ++next;
                }
        } while (!is_last);

out:
        return ret;
}

// type is either F_RDLCK or F_WRLCK.
int
gluster_lock (glfs_fd_t *fd, short type, bool block) {
    struct flock flck = {
        .l_type = type,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,  // lock entire file
    };

    return glfs_posix_lock (fd, block ? F_SETLKW : F_SETLK, &flck);
}

int
gluster_write (int src, glfs_fd_t *fd) {
        char buffer[BUFSIZE];
        size_t num_read = 0;
        size_t num_written = 0;
        size_t ret = 0;
        size_t total_written = 0;
        time_t time_start = time (NULL);
        time_t time_last = time_start;
        time_t time_cur = time_start;

        while (true) {
                num_read = read (src, buffer, BUFSIZE);
                if (num_read == -1) {
                        ret = -1;
                        goto out;
                }

                if (num_read == 0) {
                        goto out;
                }

                for (num_written = 0; num_written < num_read;) {
                        ret = glfs_write (fd,
                                          &buffer[num_written],
                                          num_read - num_written, 0);
                        if (ret == -1) {
                                goto out;
                        }

                        num_written += ret;
                        total_written += ret;

                        time_cur = time (NULL);
                        if (time_cur - time_last > LOG_EVERY_SECS) {
                                time_last = time_cur;
                                fprintf (stderr,
                                         "Wrote: %zu. Time: %zu\n",
                                         total_written,
                                         time_cur - time_start);
                        }
                }
        }

out:
        return ret;
}

int
gluster_read (glfs_fd_t *fd, int dst) {
        char buffer[BUFSIZE];
        size_t num_read = 0;
        size_t num_written = 0;
        size_t ret = 0;
        size_t total_written = 0;
        time_t time_start = time (NULL);
        time_t time_last = time_start;
        time_t time_cur = time_start;

        while (true) {
                num_read = glfs_read (fd, buffer, BUFSIZE, 0);
                if (num_read == -1) {
                        goto out;
                }

                if (num_read == 0) {
                        goto out;
                }

                for (num_written = 0; num_written < num_read;) {
                        ret = write (dst,
                                     &buffer[num_written],
                                     num_read - num_written);
                        if (ret != (num_read - num_written)) {
                                goto err;
                        }

                        num_written += ret;
                        total_written += ret;

                        time_cur = time (NULL);
                        if (time_cur - time_last > LOG_EVERY_SECS) {
                                time_last = time_cur;
                                fprintf (stderr,
                                         "Read: %zu. Time: %zu\n",
                                         total_written,
                                         time_cur - time_start);
                        }
                }
        }

err:
        ret = -1;
out:
        return ret;
}

int
gluster_getfs (glfs_t **fs, const struct gluster_url *gluster_url) {
        int ret = 0;

        *fs = glfs_new (gluster_url->volume);
        if (fs == NULL) {
                goto out;
        }

        ret = glfs_set_volfile_server (*fs,
                        "tcp",
                        gluster_url->host,
                        gluster_url->port);
        if (ret == -1) {
                goto out;
        }

        ret = glfs_init (*fs);

out:
        return ret;
}

struct xlator_option *
parse_xlator_option (const char *optarg)
{
        char *pos;
        char *key;
        char *value;
        char *xlator;
        struct xlator_option *option = NULL;

        char *opt_string = strdup (optarg);
        if (opt_string == NULL) {
                goto err;
        }

        if ((xlator = strtok_r (opt_string, ".", &pos)) == NULL) {
                errno = EINVAL;
                goto err;
        }

        if ((key = strtok_r (NULL, "=", &pos)) == NULL) {
                errno = EINVAL;
                goto err;
        }

        value = pos;
        if (value == NULL || *value == '\0') {
                errno = EINVAL;
                goto err;
        }

        option = malloc (sizeof (*option));
        if (option == NULL) {
                goto err;
        }

        option->xlator = xlator;
        option->key = key;
        option->opt_string = opt_string;
        option->value = value;

        option->next = NULL;

        goto out;

err:
        free (opt_string);
        free (option);
out:
        return option;
}

void
print_xlator_options (struct xlator_option **options)
{
        struct xlator_option *next = *options;

        while (next) {
                printf ("%s: %s => %s\n", next->xlator, next->key, next->value);

                next = next->next;
        }
}

uint16_t
strtoport (const char *str)
{
        long raw_port;
        uint16_t port = 0;
        char *end;

        raw_port = strtol (str, &end, 10);

        if (str == end) {
                goto err;
        }

        // Port ranges from 1 - 65535
        if (raw_port < 1 || raw_port > UINT16_MAX) {
                goto err;
        }

        port = (uint16_t) raw_port;

        goto out;

err:
        error (0, 0, "invalid port number: \"%s\"", str);
out:
        return port;
}
