#ifndef GLFS_CLI_H
#define GLFS_CLI_H

#include "glfs-util.h"

#include <glusterfs/api/glfs.h>
#include <stdbool.h>

struct cli_context {
        glfs_t *fs;
        struct gluster_url *url;
        struct options *options;
        char *conn_str;
        int argc;
        bool in_shell;
        char **argv;
};

struct options {
        struct xlator_option *xlator_options;
        bool debug;
};

#endif
