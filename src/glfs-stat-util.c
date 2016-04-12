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

#include "glfs-stat-util.h"

char const *
file_type (struct stat const *stat)
{
        /* See POSIX 1003.1-2001 XCU Table 4-8 lines 17093-17107 for some of
        these formats.

        To keep diagnostics grammatical in English, the returned string
        must start with a consonant.  */

        /* Do these three first, as they're the most common.  */

        if (S_ISREG (stat->st_mode))
                return stat->st_size == 0 ? "regular empty file" :
                        "regular file";

        if (S_ISDIR (stat->st_mode))
                return "directory";

        if (S_ISLNK (stat->st_mode))
                return "symbolic link";

        /* Do the S_TYPEIS* macros next, as they may be implemented in terms
        of S_ISNAM, and we want the more-specialized interpretation.  */

        if (S_TYPEISMQ (stat))
                return "message queue";

        if (S_TYPEISSEM (stat))
                return "semaphore";

        if (S_TYPEISSHM (stat))
                return "shared memory object";

        /* The remaining are in alphabetical order.  */

        if (S_ISBLK (stat->st_mode))
                return "block special file";

        if (S_ISCHR (stat->st_mode))
                return "character special file";

        if (S_ISFIFO (stat->st_mode))
                return "fifo";

        if (S_ISSOCK (stat->st_mode))
                return "socket";

        return "weird file";
}

char
ftypelet (mode_t bits)
{
        // These are the most common, so test for them first.
        if (S_ISREG (bits))
                return '-';
        if (S_ISDIR (bits))
                return 'd';

        // Other letters standardized by POSIX 1003.1-2004.
        if (S_ISBLK (bits))
                return 'b';
        if (S_ISCHR (bits))
                return 'c';
        if (S_ISLNK (bits))
                return 'l';
        if (S_ISFIFO (bits))
                return 'p';

        // Other file types (though not letters) standardized by POSIX.
        if (S_ISSOCK (bits))
                return 's';

        return '?';
}

void
strmode (mode_t mode, char *str)
{
        str[0] = ftypelet (mode);
        str[1] = mode & S_IRUSR ? 'r' : '-';
        str[2] = mode & S_IWUSR ? 'w' : '-';
        str[3] = (mode & S_ISUID
                ? (mode & S_IXUSR ? 's' : 'S')
                : (mode & S_IXUSR ? 'x' : '-'));
        str[4] = mode & S_IRGRP ? 'r' : '-';
        str[5] = mode & S_IWGRP ? 'w' : '-';
        str[6] = (mode & S_ISGID
                ? (mode & S_IXGRP ? 's' : 'S')
                : (mode & S_IXGRP ? 'x' : '-'));
        str[7] = mode & S_IROTH ? 'r' : '-';
        str[8] = mode & S_IWOTH ? 'w' : '-';
        str[9] = (mode & S_ISVTX
                ? (mode & S_IXOTH ? 't' : 'T')
                : (mode & S_IXOTH ? 'x' : '-'));
        str[10] = ' ';
        str[11] = '\0';
}

void
filemodestring (struct stat const *statp, char *str)
{
        strmode (statp->st_mode, str);

        if (S_TYPEISSEM (statp))
                str[0] = 'F';
        else if (S_TYPEISMQ (statp))
                str[0] = 'Q';
        else if (S_TYPEISSHM (statp))
                str[0] = 'S';
}

char*
human_access (struct stat const *stat)
{
        static char modebuf[12];
        filemodestring (stat, modebuf);
        modebuf[10] = 0;
        return modebuf;
}

struct timespec
get_stat_atime (struct stat const *stat)
{
        struct timespec t;

        t.tv_sec = stat->st_atime;
        t.tv_nsec = stat->st_atim.tv_nsec;

        return t;
}

struct timespec
get_stat_mtime (struct stat const *stat)
{
        struct timespec t;

        t.tv_sec = stat->st_mtime;
        t.tv_nsec = stat->st_mtim.tv_nsec;

        return t;
}

struct timespec
get_stat_ctime (struct stat const *stat)
{
        struct timespec t;

        t.tv_sec = stat->st_ctime;
        t.tv_nsec = stat->st_ctim.tv_nsec;

        return t;
}

char*
human_time (struct timespec const t)
{
        static char fmt[64], buf[64];
        struct tm const *tm = localtime (&t.tv_sec);
        strftime (fmt, sizeof fmt, "%F %T.%%06u %z", tm);
        snprintf (buf, sizeof buf, fmt, t.tv_nsec);
        return buf;
}
