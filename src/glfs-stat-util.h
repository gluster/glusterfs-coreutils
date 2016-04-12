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

#ifndef GLFS_STAT_UTIL_H
#define GLFS_STAT_UTIL_H

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define CHMOD_MODE_BITS (S_ISUID | S_ISGID | S_ISVTX | \
                S_IRWXU | S_IRWXG | S_IRWXO)

void
filemodestring (struct stat const *statp, char *str);

char const *
file_type (struct stat const *stat);

char
ftypelet (mode_t bits);

struct timespec
get_stat_atime (struct stat const *stat);

struct timespec
get_stat_ctime (struct stat const *stat);

struct timespec
get_stat_mtime (struct stat const *stat);

char*
human_access (struct stat const *stat);

char*
human_time (struct timespec const t);

void
strmode (mode_t mode, char *str);

#endif
