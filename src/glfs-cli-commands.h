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
