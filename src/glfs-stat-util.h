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
