#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfrm"
USAGE="Usage: gfrm [-f|--force] [-r|--recursive] [-p|--port PORT] [-h|--help] [-v|--version] glfs://<host>/<volume>/<path>"

setup() {
        TEST_RM_FILE=$(mktemp --tmpdir="$GLUSTER_MOUNT_DIR$ROOT_DIR")
        TEST_RM_FILE=$(basename "$TEST_RM_FILE")

        TEST_RM_DIR=$(mktemp -d --tmpdir="$GLUSTER_MOUNT_DIR$ROOT_DIR")
        TEST_RM_DIR=$(basename "$TEST_RM_DIR")
}

teardown() {
        rm -f "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_RM_FILE"
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_RM_DIR"
}

@test "no arguments" {
        run $CMD

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "short help flag" {
        run $CMD "-h"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "invalid port flag" {
        run $CMD "-p" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host and volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME': Is a directory" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME/': Is a directory" ]
}

@test "uri with host, volume, and empty path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME//"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME//': Is a directory" ]
}

@test "rm a file" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_FILE"

        [ "$status" -eq 0 ]
}

@test "rm a file with recursive flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_FILE"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_FILE': Not a directory" ]
}

@test "rm a directory" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_DIR"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_DIR': Is a directory" ]
}

@test "rm a directory with recursive flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_RM_DIR"

        [ "$status" -eq 0 ]
}

@test "rm a path that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file': No such file or directory" ]
}

@test "rm a path that does not exist with recursive flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file"

        [ "$status" -eq 1 ]
        [ "$output" == "gfrm: cannot remove \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file': No such file or directory" ]
}
