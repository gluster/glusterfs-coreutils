#ifndef GLFS_CLI_COMMANDS_H
#define GLFS_CLI_COMMANDS_H

#include "glfs-cli.h"

int
cli_connect (struct cli_context *ctx);

int
cli_disconnect (struct cli_context *ctx);

int
handle_quit (struct cli_context *ctx);

int
not_implemented (struct cli_context *ctx);

#endif /* GLFS_CLI_COMMANDS_H */
